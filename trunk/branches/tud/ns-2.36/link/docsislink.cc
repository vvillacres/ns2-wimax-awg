/*
 * DocsisLink - A delay link that models DOCSIS Cable Modem upstream link
 */

/*
 * Copyright (c) 2012-2013 Cable Television Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * Alternatively, provided that this notice is retained in full, this
 * software may be distributed under the terms of the GNU General
 * Public License ("GPL") version 2, in which case the provisions of the
 * GPL apply INSTEAD OF those given above.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * Authors:
 *   Greg White <g.white@cablelabs.com>
 *   Joey Padden <j.padden@cablelabs.com>
 *   Takashi Hayakawa <t.hayakawa@cablelabs.com>
 */

#include "docsislink.h"
#include "random.h"
#include <assert.h>
#include <iostream>
#include <algorithm>

#define MAP_PROCESSING_TIME 0.000650  // 650us

static class DocsisLinkClass : public TclClass {
public:
    DocsisLinkClass() : TclClass("DocsisLink") {}
    TclObject* create(int /* argc */, const char*const* /* argv */) {
        return (new DocsisLink);
    }
} class_docsislink_link;

DocsisLink::DocsisLink() : mapint_timer_(this, 0), mapmsg_timer_(this, 1), grant_timer_(this, 2)
{
    initialized_ = false;

    feederq_ = NULL;
    qh_ = NULL;

    bind("maxgrant_", &maxgrant_);
    bind("mgvar_", &mgvar_);
    bind_time("mapint_", &mapint_);
    bind_time("mapadv_", &mapadv_);
    bind("rate_",  &rate_);
    bind("peakrate_", &peakrate_);
    bind("bucket_", &bucket_);

    mapint_timer_.resched(mapint_);
}

int DocsisLink::command(int argc, const char*const* argv)
{
    Tcl& tcl = Tcl::instance();

    if (argc == 3) {
        if (strcmp(argv[1], "feederq") == 0) {
            const char *feederq_id = argv[2];
            if (feederq_id[0] == '0') {
                feederq_ = 0;
                return (TCL_OK);
            }
            feederq_ = (Queue*) TclObject::lookup(feederq_id);
            if (feederq_ == 0) {
                tcl.resultf("no such object %s", feederq_id);
                return (TCL_ERROR);
            }

            feederq_->block();
            tcl.evalf("%s set unblock_on_resume_ false", feederq_id);
            return (TCL_OK);
        }
    }

    return LinkDelay::command(argc, argv);
}

void DocsisLink::reset()
{
    initialized_ = false;
    LinkDelay::reset();
}

void DocsisLink_Timer::expire(Event*)
{
    switch (timerType_) {
    case 0:
        doc_->handleMapIntTimer();
        break;
    case 1:
        doc_->handleMapMsgTimer();
        break;
    case 2:
        doc_->handleGrantTimer();
        break;
    default:
        fprintf(stderr, "DocsisLink_Timer::expire: invalid timeType_: %d\n", timerType_);
        exit(1);
    }
}

////////////////////////////////////////////////////////////////

/*
 * Called at the beginning of each MAP interval.
 */
void DocsisLink::handleMapIntTimer()
{
    initializeIfNecessary();

    // Schedule the next MAP interval and MAP message.
    mapint_timer_.resched(mapint_);
    mapmsg_timer_.resched(mapint_ - MAP_PROCESSING_TIME);

    // Load the tokens for this MAP interval.
    tokens_current_ += tokens_next_;
    tokens_next_ = 0;

    if (tokens_current_ == 0) {
        // If no grant in this MAP interval, make a contention request if needed.
        makeRequest();
    } else {
        // If we have a grant in this MAP interval, schedule use of it.
        double grant_time_max = mapint_ - transmissionTime(tokens_current_ - frag_sent_bytes_);
        double grant_time = Random::uniform(std::max(0.0, grant_time_max));
        grant_timer_.resched(grant_time);
    }
}

/*
 * Called at the MAP message delivery from CMTS to CM containing the
 * new grant.
 *
 * The "mgvar_" TCL variable allows to simulate the grant variability
 * from congestion.  For a non-congested situation, it should be set
 * low so that free capacity (nextmax) in the MAP is fairly uniform
 * from one MAP to the next.  For a congested situation, it should be
 * set high which will cause nextmax to vary significantly from one
 * MAP interval to the next.  This simulates the bursty nature of
 * bandwidth access when a CMTS scheduler is juggling many
 * simultaneous requests from modems and will temporarily starve some
 * flows while flooding others.  Actual grant for this MAP interval is
 * the minimum of requested bytes and free capacity.
 */
void DocsisLink::handleMapMsgTimer()
{
    assert(tokens_next_ == 0);

    int w = mgvar_ * maxgrant_ / 100;
    int r = (w == 0) ? 0 : (rand() % (2 * w) - w);
    int nextmax = maxgrant_ + r;
    int grant = std::min(req_[0], nextmax);

    // Move previous request in the request pipeline.
    req_[0] = std::max(0, req_[0] - grant) + req_[1];
    for (int i = 1; i < req_count_ - 1; i++) {
        req_[i] = req_[i + 1];
    }
    req_[req_count_ - 1] = 0;

    // Apply the grant to tokens.
    tokens_next_ = grant;
}

/*
 * Called at the start of each grant.  Transmits a segment containing as many
 * packets as will fit.  The CM will get at most one grant in each MAP interval.
 */
void DocsisLink::handleGrantTimer()
{

    // make a piggybacked request in the segment header
    makeRequest();

    if (frag_pkt_ == NULL) {
        // Fragmentation not in progress - ask for a new packet
        askForNextPacket(qh_, 0);
    } else {
        // Fragmentation in progress
        int frag_pkt_size = HDR_CMN(frag_pkt_)->size_;
        if (tokens_current_ < frag_pkt_size) {
            // Not enough tokens - continue fragmentation
            frag_sent_bytes_ = tokens_current_;
        } else {
            // Enough tokens - finish this packet within this grant
            double pkt_departure = transmissionTime(frag_pkt_size - frag_sent_bytes_);
            assert(pkt_departure > 0);
            Scheduler::instance().schedule(target_, frag_pkt_, delay_ + pkt_departure);
            tokens_current_ -= frag_pkt_size;
            frag_pkt_ = NULL;
            frag_sent_bytes_ = 0;

            askForNextPacket(qh_, pkt_departure);
        }
    }

}

/*
 * Receives a new packet from upstream queue.  Only gets called by the
 * upstream queue in response to the queue being unblocked by
 * askForNextPacket().  If enough tokens_current_ for the packet, send
 * it to target_.  Otherwise, hold the packet for fragmentation.
 */
void DocsisLink::recv(Packet* p, Handler* h)
{
    assert(frag_pkt_ == NULL);
    assert(frag_sent_bytes_ == 0);

    int pkt_size = HDR_CMN(p)->size_;

    if (tokens_current_ >= pkt_size) {
        // Fits in current grant - send it entirely
        double pkt_departure = transmissionTime(pkt_size);
        Scheduler::instance().schedule(target_, p, delay_ + pkt_departure);
        tokens_current_ -= pkt_size;    // Spend the tokens for this packet.
        askForNextPacket(h, pkt_departure);
    } else {
        // Doesn't fit in current grant - start fragmentation
        frag_pkt_ = p;
        frag_sent_bytes_ = tokens_current_;
        qh_ = h;
    }
}

/*
 * Generates request based on current queue length minus already requested
 * bytes, capped by rate shaper tokens.
 */
void DocsisLink::makeRequest(void)
{
    // Refill rate shaper tokens.
    double now = Scheduler::instance().clock();
    double elapsed = now - last_update_time_;
    msrtokens_ += elapsed * rate_ / 8;
    if (msrtokens_ > bucket_)
        msrtokens_ = bucket_;
    peaktokens_ = elapsed * peakrate_ / 8;
    last_update_time_ = now;

    // Determine the new request size.
    int curq = feederq_->byteLength() + (frag_pkt_ ? HDR_CMN(frag_pkt_)->size_ : 0);

    int req_total = 0;
    for (int i = 0; i < req_count_; i++) {
        req_total += req_[i];
    }

    int req_new = std::max(0, curq - req_total - tokens_current_ - tokens_next_);
    req_new = std::min(std::min(req_new, msrtokens_), peaktokens_);

    req_[req_count_ - 1] += req_new;
    msrtokens_ -= req_new;
}

void DocsisLink::askForNextPacket(Handler* h, double howSoon)
{
    assert(frag_pkt_ == NULL);
    assert(frag_sent_bytes_ == 0);

    if (tokens_current_ > 0) {
        if (feederq_->byteLength() > 0) {
            if (howSoon > 0) {
                assert(h != NULL);
                Scheduler::instance().schedule(h, &intr_, howSoon);
            } else {
                feederq_->resume();
            }
        } else {
            // Queue is empty, but we still have tokens.
            // Dump the tokens.  This can happen when feederq_ drops its packets.
            fprintf(stderr, "DocsisLink: Dumping %d tokens.\n", tokens_current_);
            tokens_current_ = 0;
        }
    }
}

void DocsisLink::initializeIfNecessary()
{
    if (!initialized_) {
        initialized_ = true;
        tokens_current_ = 0;
        tokens_next_ = 0;
        msrtokens_ = bucket_;
        peaktokens_ = 0;
        last_update_time_ = Scheduler::instance().clock();

        frag_pkt_ = NULL;
        frag_sent_bytes_ = 0;

        if (mapint_ <= MAP_PROCESSING_TIME) {
            fprintf(stderr, "DocsisLink: mapint_ too small: %f\n", mapint_);
            exit(1);
        }

        if (mapadv_ == 0) {
            mapadv_ = mapint_;
        } else if (mapadv_ < mapint_) {
            fprintf(stderr, "DocsisLink: mapadv_ too small: %f\n", mapadv_);
            exit(1);
        }

        req_count_ = floor(mapadv_ / mapint_) + 1;
        req_ = new int[req_count_];
        for (int i = 0; i < req_count_; i++) {
            req_[i] = 0;
        }
    }
}

////////////////////////////////////////////////////////////////

double DocsisLink::expectedDelay(int queue_size) {
    double latency;

    int frag_pkt_size = (!initialized_ || frag_pkt_ == NULL) ? 0 : HDR_CMN(frag_pkt_)->size_;

    queue_size += frag_pkt_size - tokens_current_ - tokens_next_;
    if (queue_size <= msrtokens_) {
        latency = 8 * queue_size / peakrate_;
    } else {
        latency = 8 * ((queue_size - msrtokens_) / rate_ +  msrtokens_ / peakrate_);
    }

    return latency;
}
