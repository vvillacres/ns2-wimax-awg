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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/tools/loss-monitor.cc,v 1.18 2000/09/01 03:04:06 haoboy Exp $ (LBL)";
#endif

#include <tclcl.h>

#include "agent.h"
#include "config.h"
#include "packet.h"
#include "ip.h"
#include "rtp.h"
#include "wimax-monitor.h"
#include "mobilenode.h"
#include "wimax/connection.h"
#include <fstream>

static class WimaxMonitorClass : public TclClass {
public:
	WimaxMonitorClass() : TclClass("Agent/WimaxMonitor") {}
	TclObject* create(int, const char*const*) {
		return (new WimaxMonitor());
	}
} class_wimax_mon;

WimaxMonitor::WimaxMonitor() : Agent(PT_NTYPE)
{
	expected_ = -1;

	bytes_ = 0;
	nlost_ = 0;
	npkts_ = 0;
	previous_delay_ = -1;
	delay_sum_ = 0;
	jitter_sum_ = 0;
	bind("bytes_", &bytes_);
	bind("npkts_", &npkts_);
	bind("nlost_", &nlost_);
	bind("total_bytes_", &total_bytes_);
	bind("total_npkts_", &total_npkts_);
	bind("distance_", &distance_);
	bind("source_", &source_);
	bind("destination_", &destination_);
	bind("delay_mean_", &delay_mean_);
	bind("delay_max_", &delay_max_);
	bind("jitter_mean_", &jitter_mean_);
	bind("jitter_max_", &jitter_max_);
	bind("rxpr_", &rxpr_);
	bind("rssi_", &rssi_);
	// delay frequency
	delayFrequency_ = false;
	delayLowBound_ = 0.0;
	delayHighBound_ = 0.0;
	delayNbBins_ = 0;
	delayBinSize_ = 0.0;
	delayBinVector_ = NULL;
	// jitter frequency
	jitterFrequency_ = false;
	jitterLowBound_ = 0.0;
	jitterHighBound_ = 0.0;
	jitterNbBins_ = 0;
	jitterBinSize_ = 0.0;
	jitterBinVector_ = NULL;
}

void WimaxMonitor::recv(Packet* pkt, Handler*)
{
	double sX, sY, sZ; //coordinates of transmitting node (source)
	double dX, dY, dZ; //coordinates of receiving node (destination)
	double bX, bY, bZ; //coordinates of base station node
	double mX, mY, mZ; //coordinates of base station node

	struct hdr_cmn *ch = HDR_CMN(pkt);

	try {

		int bstation = ((MobileNode *)pkt->txinfo_.getNode())->base_stn();
		int mobilenode = ((MobileNode *)pkt->txinfo_.getNode())->address();
		int next = ch->next_hop_;

		if (mobilenode == next){
			// destination is mobilenode
			((MobileNode *)Node::get_node_by_address(mobilenode))->getLoc(&mX, &mY, &mZ);
			((MobileNode *)Node::get_node_by_address(mobilenode))->getLoc(&dX, &dY, &dZ);
			destination_ = mobilenode;

			// source is base station
			((MobileNode *)Node::get_node_by_address(bstation))->getLoc(&bX, &bY, &bZ);
			((MobileNode *)Node::get_node_by_address(bstation))->getLoc(&sX, &sY, &sZ);
			source_ = bstation;
		}

		if (bstation == next){
			// destination is bstation
			((MobileNode *)Node::get_node_by_address(bstation))->getLoc(&bX, &bY, &bZ);
			((MobileNode *)Node::get_node_by_address(bstation))->getLoc(&dX, &dY, &dZ);
			destination_ = bstation;

			// source is mobilenode
			((MobileNode *)Node::get_node_by_address(mobilenode))->getLoc(&mX, &mY, &mZ);
			((MobileNode *)Node::get_node_by_address(mobilenode))->getLoc(&sX, &sY, &sZ);
			source_ = mobilenode;
		}

		if (mobilenode == bstation){
				// destination is mobilenode
				((MobileNode *)Node::get_node_by_address(next))->getLoc(&mX, &mY, &mZ);
				((MobileNode *)Node::get_node_by_address(next))->getLoc(&dX, &dY, &dZ);
				destination_ = next;

				// source is base station
				((MobileNode *)Node::get_node_by_address(bstation))->getLoc(&bX, &bY, &bZ);
				((MobileNode *)Node::get_node_by_address(bstation))->getLoc(&sX, &sY, &sZ);
				source_ = bstation;
		}

		// find the distance between the two nodes
		double dist = sqrt((dX - sX) * (dX - sX)
		   + (dY - sY) * (dY - sY)
		   + (dZ - sZ) * (dZ - sZ));

		// decide if mobile node is right or left of base station (postitiv, negative distance)
		if (mX < bX){
			distance_ = dist*-1;
		} else {
			distance_ = dist;
		}


		hdr_rtp* p = hdr_rtp::access(pkt);
		int seqno = p->seqno();
		bytes_ += hdr_cmn::access(pkt)->size();
		++npkts_;
		total_bytes_ += hdr_cmn::access(pkt)->size();
		++total_npkts_;

		if (expected_ >= 0) {
			int loss = seqno - expected_;
			if (loss > 0) {
				nlost_ += loss;
			}
		}
		expected_ = seqno + 1;

		double current_delay = NOW - ch->ts_;

		// mean delay
		delay_sum_ += current_delay;
		delay_mean_ = delay_sum_ / npkts_;

		// delay frequency of occurrence
		if ( delayFrequency_) {
			int binIndex = int(floor((current_delay - delayLowBound_) / delayBinSize_));
			if (binIndex < 0) {
				binIndex = 0;
			}
			if (binIndex >= delayNbBins_) {
				binIndex = delayNbBins_ - 1;
			}
			delayBinVector_->at(binIndex)++;
		}


		// max delay
		if ( current_delay > delay_max_) {
			delay_max_ = current_delay;
		}

		// mean jitter (ipdv)
		if ( (previous_delay_  > 0) && (npkts_ > 1) ) {
			double current_jitter = fabs(current_delay - previous_delay_);

			// mean jitter
			jitter_sum_ += current_jitter;
			jitter_mean_ = jitter_sum_/(npkts_ - 1);

			// max jitter
			if ( current_jitter > jitter_max_) {
				jitter_max_ = current_jitter;
			}

			if (jitterFrequency_) {
				int binIndex = int(floor((current_jitter - jitterLowBound_) / jitterBinSize_));
				if (binIndex < 0) {
					binIndex = 0;
				}
				if (binIndex >= delayNbBins_) {
					binIndex = delayNbBins_ - 1;
				}
				jitterBinVector_->at(binIndex)++;

			}

		}

		previous_delay_ = current_delay;

		rxpr_ = pkt->txinfo_.RxPr;
		rssi_ = 10*log10(rxpr_*1000); // dBm
	}
	catch (...) {
		fprintf(stderr,"ERROR: Exception in WiMAX Monitor::recv \n");
		fprintf(stderr,"ERROR: Seems not to be a WiMAX connection  \n");
	}

	Packet::free(pkt);
}

/*
 * $proc interval $interval
 * $proc size $size
 */
int WimaxMonitor::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "reset") == 0) {
			printf("Reset WiMAX Monitor \n");
			expected_ = -1;
			bytes_ = 0;
			nlost_ = 0;
			npkts_ = 0;
			previous_delay_ = -1;
			total_bytes_ = 0;
			total_npkts_ = 0;
			delay_sum_ = 0;
			delay_max_ = 0;
			jitter_sum_ = 0;
			jitter_max_ = 0;

			// delay frequency
			delayFrequency_ = false;
			delayLowBound_ = 0.0;
			delayHighBound_ = 0.0;
			delayNbBins_ = 0;
			delayBinSize_ = 0.0;
			if ( delayBinVector_ != NULL) {
				delete (delayBinVector_);
				delayBinVector_ = NULL;
			}
			// jitter frequency
			jitterLowBound_ = 0.0;
			jitterHighBound_ = 0.0;
			jitterNbBins_ = 0;
			jitterBinSize_ = 0.0;
			jitterFrequency_ = false;
			if ( jitterBinVector_ != NULL) {
				delete (jitterBinVector_);
				jitterBinVector_ = NULL;
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "clear") == 0) {
			expected_ = -1;
			bytes_ = 0;
			nlost_ = 0;
			npkts_ = 0;
			previous_delay_ = -1;
			total_bytes_ = 0;
			total_npkts_ = 0;
			delay_sum_ = 0;
			delay_max_ = 0;
			jitter_sum_ = 0;
			jitter_max_ = 0;
			// delay frequency
			if ( delayBinVector_ != NULL) {
				  vector<int>::iterator delayBinIterator;
				  for ( delayBinIterator=delayBinVector_->begin() ; delayBinIterator < delayBinVector_->end(); delayBinIterator++ ) {
					  *delayBinIterator = 0;
				  }
			}
			// jitter frequency
			if ( jitterBinVector_ != NULL) {
				vector<int>::iterator jitterBinIterator;
				for ( jitterBinIterator=jitterBinVector_->begin() ; jitterBinIterator < jitterBinVector_->end(); jitterBinIterator++ ) {
					*jitterBinIterator = 0;
				}
			}

			return (TCL_OK);
		}
	} else if (argc == 5) {
		if (strcmp(argv[1], "collectdelay") == 0 ) { // Switch on collection for delay
			printf("Collect frequency of occurrence for delay \n");
			delayFrequency_ = true;
			delayLowBound_ = atof(argv[2]);
			delayHighBound_ = atof(argv[3]);
			delayNbBins_ = int(atof(argv[4]));
			delayBinSize_ = (delayHighBound_ - delayLowBound_) / delayNbBins_;

			if ( delayBinVector_ != NULL) {
				printf("ERROR: delayBinVector already allocated \n");
			} else {
				delayBinVector_ = new vector<int> (delayNbBins_,0);
			}
			return (TCL_OK);

		} else if (strcmp(argv[1], "collectjitter") == 0 ) { // Switch on collection for jitter
			printf("Collect frequency of occurrence for jitter \n");
			jitterFrequency_ = true;
			jitterLowBound_ = atof(argv[2]);
			jitterHighBound_ = atof(argv[3]);
			jitterNbBins_ = int(atof(argv[4]));
			jitterBinSize_ = (jitterHighBound_ - jitterLowBound_) / jitterNbBins_;

			if ( jitterBinVector_ != NULL) {
				printf("ERROR: JitterBinVector already allocated \n");
			} else {
				jitterBinVector_ = new vector<int> (jitterNbBins_,0);
			}
			return (TCL_OK);

		} else if (strcmp(argv[1], "writedelay") == 0) {

			if ( delayFrequency_ ) {
				const char * run = argv[2];
				const char * station = argv[3];
				const char * fileName = argv[4];
				printf("Write delay frequency of occurence to file %s Run %s Station %s \n", fileName, run, station);

				fstream outputFile;
				outputFile.open(fileName, fstream::out | fstream::app );
				if ( outputFile.is_open()) {
					outputFile << run << " " << station << " " << delayLowBound_ << " " << delayHighBound_ << " " << delayNbBins_ << " ";
					for ( uint i = 0; i < delayBinVector_->size(); i++) {
						outputFile << delayBinVector_->at(i) << " ";
					}
					outputFile << endl;
					outputFile.close();
				} else {
					fprintf(stderr,"Failed to open the file for delay frequency of occurence ! \n");
				}
				delayFrequency_ = false;
				delete (delayBinVector_);
				delayBinVector_ = NULL;
			} else {
				fprintf(stderr,"No data has been collected for delay frequency of occurrence ! Use \"collectdelay\" \n");
			}

			return (TCL_OK);
		} else if (strcmp(argv[1], "writejitter") == 0) {

			if ( jitterFrequency_ ) {
				const char * run = argv[2];
				const char * station = argv[3];
				const char * fileName = argv[4];
				printf("Write jitter frequency of occurence to file %s Run %s Station %s \n", fileName, run, station);

				fstream outputFile;
				outputFile.open(fileName, fstream::out | fstream::app );
				if ( outputFile.is_open()) {
					outputFile << run << " " << station << " " << jitterLowBound_ << " " << jitterHighBound_ << " " << jitterNbBins_ << " ";
					for ( uint i = 0; i < jitterBinVector_->size(); i++) {
						outputFile << jitterBinVector_->at(i) << " ";
					}
					outputFile << endl;
					outputFile.close();
				} else {
					fprintf(stderr,"Failed to open the file for delay frequency of occurence ! \n");
				}
				jitterFrequency_ = false;
				delete (jitterBinVector_);
				jitterBinVector_ = NULL;
			} else {
				fprintf(stderr,"No data has been collected for jitter frequency of occurrence ! Use \"collectjitter\" \n");
			}

			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}
