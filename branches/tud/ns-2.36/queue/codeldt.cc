/*
 * CoDel-DT - Edited version of CoDel to use droptail and queue delay
 * estimation
 *
 * Copyright (c) 2012-2013 Cable Television Laboratories, Inc.
 * Copyright (C) 2011-2012 Kathleen Nichols <nichols@pollere.com>
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
 * Authors
 *   Greg White <g.white@cablelabs.com>
 *   Joey Padden <j.padden@cablelabs.com>
 *   Takashi Hayakawa <t.hayakawa@cablelabs.com>
 *   Kathleen Nichols <nichols@pollere.com>
 */

#include <math.h>
#include "codeldt.h"

static class CoDelDtClass : public TclClass {
  public:
    CoDelDtClass() : TclClass("Queue/CoDelDt") {}
    TclObject* create(int, const char*const*) {
        return (new CoDelDtQueue);
    }
} class_CoDelDt;

CoDelDtQueue::CoDelDtQueue() : tchan_(NULL)
{
    bind("interval_", &interval_);
    bind("target_", &target_);
    bind("dq_threshold_", &dq_threshold_);

    bind("curq_", &curq_);
    bind("d_exp_", &d_exp_);

    q_ = new PacketQueue();
    pq_ = q_;
    target_ -= 0.006;
    link_ = 0;
    reset();
}

void CoDelDtQueue::reset()
{
    maxpacket_ = 256;

    curq_ = 0;
    d_exp_ = 0;
    dropping_ = false;
    drop_time_ = 0;
    last_drop_time_ = 0;
    drop_count_ = 0;

    avg_dq_rate_ = 0;
    dq_count_ = 0;
    dq_start_ = 0;

    Queue::reset();
}

/*
 * Adds a new packet to the queue.  Drops it if the queue is full or
 * expected link delay exceeds the target latency and meets the CoDel
 * drop decision.
 */
void CoDelDtQueue::enque(Packet* pkt)
{
    double now = Scheduler::instance().clock();
    int pkt_size = hdr_cmn::access(pkt)->size();
    curq_ = q_->byteLength();

    if (q_->length() >= qlim_) {
        drop(pkt);
        return;
    }

    if (maxpacket_ < pkt_size) {
        maxpacket_ = pkt_size;
    }

    // Estimate the link delay.
    if (avg_dq_rate_ == 0) {
        d_exp_ = 0;
    } else if (link_) {
        d_exp_ = link_->expectedDelay(curq_ + pkt_size);
    } else {
        d_exp_ = (curq_ + pkt_size) / avg_dq_rate_;
    }

    if (d_exp_ < target_ || curq_ <= maxpacket_) {
        // Delay or queue is small enough
        q_->enque(pkt);
        dropping_ = false;      // Leave the dropping_ state
        drop_time_ = 0;         // Reset the drop timer
    } else if (drop_time_ == 0) {
        // Delay has grown big for the first time
        q_->enque(pkt);
        assert(dropping_ == false);

        // Set the drop timer accordingly
        if (is_prev_drop_stale() || drop_count_ <= 1) {
            // Previous drop was long ago - give a full parole
            drop_time_ = control_law(now, 1);
        } else {
            // Previous drop was recent - give a partial parole
            drop_time_ = control_law(now, drop_count_);
        }
    } else if (now < drop_time_) {
        // Delay has been big but not for long
        q_->enque(pkt);
    } else {
        // Delay has been big and for long - enter/stay the dropping_ state
        drop(pkt);
        if (dropping_) {
            drop_count_++;
            drop_time_ = control_law(drop_time_, drop_count_);
        } else {
            dropping_ = true;
            refresh_drop_count();
            drop_time_ = control_law(now, drop_count_);
        }
        last_drop_time_ = drop_time_;
    }
}

bool CoDelDtQueue::is_prev_drop_stale()
{
    double now = Scheduler::instance().clock();
    if (last_drop_time_ == 0) {
        return true;
    } else {
        return (now - last_drop_time_ >= 8 * interval_);
    }
}

void CoDelDtQueue::refresh_drop_count()
{
    if (drop_count_ <= 2 || is_prev_drop_stale()) {
        drop_count_ = 1;
    } else if (drop_count_ <= 128) {
        drop_count_ -= 2;
    } else {
        drop_count_ *= 0.9844; // ~ 126/128
    }
}

/*
 * Calculates the time of next drop, relative to base_time.
 */
double CoDelDtQueue::control_law(double base_time, int count)
{
    return base_time + interval_ / sqrt(count);
}

Packet* CoDelDtQueue::deque()
{
    Packet *p = q_->deque();
    if (p) {
        update_deque_rate(p);
    }
    return p;
}

void CoDelDtQueue::update_deque_rate(Packet* p)
{
    double now = Scheduler::instance().clock();
    int pkt_size = hdr_cmn::access(p)->size();

    if (q_->byteLength() >= dq_threshold_ && dq_count_ == -1) {
        dq_start_ = now;
        dq_count_ = 0;
    }

    if (dq_count_ != -1) {
        dq_count_ += pkt_size;

        double elapsed = now - dq_start_;
        if (elapsed > 0 && dq_count_ >= dq_threshold_) {
            double recent_dq_rate = dq_count_ / elapsed;
            if (avg_dq_rate_ == 0) {
                avg_dq_rate_ = recent_dq_rate;
            } else {
                avg_dq_rate_ = 0.5 * avg_dq_rate_ + 0.5 * recent_dq_rate;
            }

            if (q_->byteLength() > dq_threshold_) {
                dq_start_ = now;
                dq_count_ = 0;
            } else {
                dq_count_ = -1;
            }
        }
    }
}


int CoDelDtQueue::command(int argc, const char*const* argv)
{
    Tcl& tcl = Tcl::instance();
    if (argc == 2) {
        if (strcmp(argv[1], "reset") == 0) {
            reset();
            return (TCL_OK);
        }
    } else if (argc == 3) {
        // attach a file for variable tracing
        if (strcmp(argv[1], "attach") == 0) {
            int mode;
            const char* id = argv[2];
            tchan_ = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
            if (tchan_ == 0) {
                tcl.resultf("CoDelDt trace: can't attach %s for writing", id);
                return (TCL_ERROR);
            }
            return (TCL_OK);
        }
        // connect CoDel to the underlying queue
        if (!strcmp(argv[1], "packetqueue-attach")) {
            delete q_;
            if (!(q_ = (PacketQueue*) TclObject::lookup(argv[2])))
                return (TCL_ERROR);
            else {
                pq_ = q_;
                return (TCL_OK);
            }
        }

        if (strcmp(argv[1], "link") == 0) {
            TclObject* del = TclObject::lookup(argv[2]);
            if (del == 0) {
                tcl.resultf("CoDelDtQueue: non LinkDelay object %s", argv[2]);
                return TCL_ERROR;
            }

            tcl.evalf("%s info class", argv[2]);
            const char* classname = tcl.result();
            if (strcmp(classname, "DocsisLink") == 0) {
                link_ = (DocsisLink*) del;
            }

            return TCL_OK;
        }
    }
    return (Queue::command(argc, argv));
}

void CoDelDtQueue::trace(TracedVar* v)
{
    const char *p;

    if (((p = strstr(v->name(), "curq")) == NULL) &&
        ((p = strstr(v->name(), "d_exp")) == NULL) ) {
        fprintf(stderr, "CoDelDt: unknown trace var %s\n", v->name());
        return;
    }
    if (tchan_) {
        char wrk[500];
        double t = Scheduler::instance().clock();
        if (*p == 'c') {
            sprintf(wrk, "c %g %d\n", t, int(*((TracedInt*) v)));
        } else if(*p == 'd') {
            sprintf(wrk, "d %g %g\n", t, double(*((TracedDouble*) v)));
        }
        int n = strlen(wrk);
        (void) Tcl_Write(tchan_, wrk, n);
    }
}
