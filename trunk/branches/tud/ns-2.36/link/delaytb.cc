/*
 * DelayLinkTb - A delay link based on DOCSIS token bucket rate shaping
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

#include "delaytb.h"
#include <iostream>

static class LinkDelayTbClass : public TclClass {
public:
    LinkDelayTbClass() : TclClass("DelayLinkTb") {}
    TclObject* create(int /* argc */, const char*const* /* argv */) {
        return (new LinkDelayTb);
    }
} class_delaytb_link;

LinkDelayTb::LinkDelayTb()
{
    initialized_ = false;

    bind("rate_", &rate_);              // Token generation rate (bytes/s)
    bind("bucket_", &bucket_);          // Token bucket maximum size (bytes)
    bind("peakrate_", &peak_rate_);     // Peak rate token generation rate (bytes/s)
    bind("peakbucket_", &peak_bucket_); // Peak rate token bucket maximum size (bytes)
}

int LinkDelayTb::command(int argc, const char*const* argv)
{
    return LinkDelay::command(argc, argv);
}

////////////////////////////////////////////////////////////////

void LinkDelayTb::recv(Packet* p, Handler* feeder)
{
    initializeIfNecessary();

    Scheduler& s = Scheduler::instance();

    double txt;
    if (bandwidth_ > 0) {
       txt = LinkDelay::txtime(p);
    } else {
       txt = 0.0;   // For infinite bandwidth
    }

    // Refill tokens.
    double now = s.clock();
    double elapsed = now - last_update_time_;
    tokens_ += elapsed * rate_ / 8;
    if (tokens_ > bucket_) {
        tokens_ = bucket_;
    }
    peak_tokens_ += elapsed * peak_rate_ / 8;
    if (peak_tokens_ > peak_bucket_) {
        peak_tokens_ = peak_bucket_;
    }
    last_update_time_ = now;

    // Adjust transmission time if insufficient tokens.
    int packet_size = HDR_CMN(p)->size_;
    if (tokens_ < packet_size) {
        txt += (packet_size - tokens_) * 8 / rate_;
    } else if (peak_tokens_ < packet_size) {
        txt += (packet_size - peak_tokens_) * 8 / peak_rate_;
    }

    // Pay the tokens for sending this packet.
    tokens_ -= packet_size;
    peak_tokens_ -= packet_size;

    // Schedule this packet to arrive at downstream and feeder to
    // deliver new packet.
    s.schedule(target_, p, txt + delay_);
    s.schedule(feeder, &intr_, txt);
}

void LinkDelayTb::reset()
{
    initialized_ = false;
    LinkDelay::reset();
}

void LinkDelayTb::initializeIfNecessary()
{
    if (!initialized_) {
       tokens_ = bucket_;
       peak_tokens_ = peak_bucket_;
       last_update_time_ = Scheduler::instance().clock();
       initialized_ = true;
    }
}
