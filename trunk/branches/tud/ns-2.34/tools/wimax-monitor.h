/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1994-1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef ns_wimax_monitor_h
#define ns_wimax_monitor_h

#include <tclcl.h>

#include "agent.h"
#include "config.h"
#include "packet.h"
#include "ip.h"
#include "rtp.h"
#include "mobilenode.h"
#include <vector>


class WimaxMonitor : public Agent {
public:
	WimaxMonitor();
	virtual int command(int argc, const char*const* argv);
	virtual void recv(Packet* pkt, Handler*);
protected:
	int nlost_;
	int npkts_;
	int total_npkts_;
	int expected_;
	int bytes_;
	int total_bytes_;
	int source_;
	int destination_;
	double distance_;
	double delay_sum_;
	double delay_mean_;
	double delay_max_;
	double jitter_sum_;
	double jitter_mean_;
	double jitter_max_;
	double previous_delay_;
	double rxpr_;
	double rssi_;
	// saves  frequency of occurrence for delay
	vector<int> * delayBinVector_;
	bool delayFrequency_;
	double delayLowBound_;
	double delayHighBound_;
	int delayNbBins_;
	double delayBinSize_;
	// saves frequency of occurrence for jitter
	vector<int> * jitterBinVector_;
	bool jitterFrequency_;
	double jitterLowBound_;
	double jitterHighBound_;
	int jitterNbBins_;
	double jitterBinSize_;
};

#endif // ns_wimax_monitor_h
