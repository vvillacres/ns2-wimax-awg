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

#ifndef ns_codeldt_h
#define ns_codeldt_h

#include "queue.h"
#include "docsislink.h"

class CoDelDtQueue : public Queue {
public:
    CoDelDtQueue();

protected:
    void enque(Packet* pkt);
    Packet* deque();
    int command(int argc, const char*const* argv);
    void reset();
    void trace(TracedVar*); // routine to write trace records

    // Config values
    double target_;         // target queue size (in time)
    double interval_;       // width of moving time window over which to compute min


    // Module variables
    double drop_time_;      // Recent drop time scheduled
    double last_drop_time_; // Recent drop time scheduled when dropping a packet
    int drop_count_;        // How many drops since the last time in dropping state
    bool dropping_;         // In dropping_ state
    int maxpacket_;         // Largest packet size observed

    double avg_dq_rate_;    // Average deque rate
    int dq_threshold_;      // - For measuring avg_dq_rate_
    int dq_count_;          // - For measuring avg_dq_rate_
    double dq_start_;       // - For measuring avg_dq_rate_

    PacketQueue *q_;        // underlying FIFO queue_

    // Variable tracer
    Tcl_Channel tchan_;
    TracedInt curq_;        // Current qlen at packet arrivals
    TracedDouble d_exp_;    // Expected delay at packet arrivals

private:
    double control_law(double base_time, int count);
    bool is_prev_drop_stale();
    void refresh_drop_count();
    void update_deque_rate(Packet* p);

    DocsisLink* link_;
};

#endif
