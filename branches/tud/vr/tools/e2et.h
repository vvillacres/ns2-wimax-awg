/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/* End to End Tagger connector
   This code has been developed within the EuQoS project of the EU IST 6th Framework Programme. 
   The "end to end tagger" connector sits between a sending agent and the node the latter is attached to. 

   For every packet sent BY the agent, it sets the following extension fields in the common header:
   - e2e_ts_ : end-to-end timestamp, set to the current clock
   - e2e_sn_ : end-to-end sequence number, starting from 0

   This way, a companion "end to end monitor" connector (see e2em.{cc.h}), sitting between the receiving node
   and the receiving agent, can use the above information to compute delay, jitter, throughput and packet loss
   ratio statistics.

   Giovanni Stea (g.stea@iet.unipi.it), University of Pisa, Italy, January 2005. 

   ------
   UPDATE (July 2005)
   Modified as follows: rather than one timestamp, an ARRAY of timestamps is provisioned. The e2e tagger sets the FIRST timestamp in the array.
   Other timestamps can be set by the timestamper connector (see timestamper.cc), whch sits at the ingress of a link. This way, the e2e monitor 
   can compute the delay contribution of sub-paths in an e2e path.

   ------
   CAVEAT (July 2005)
   The measurement connectors have been tested to work with Token Bucket filter (which is a connector itself). In that case, 
   the RELATIVE order in which the attach-e2et and attach-tbf operations are performed determines whether the queueing delay of the TBF
   is/is not counted in the e2e delay. More specifically, the *last* attach- operation attaches the connector which is closest to the agent.
*/


#ifndef ns_e2et_h
#define ns_e2et_h

#include <connector.h>
#include <ranvar.h>
#include <t_timers.h>

class e2et : public Connector {
public:
	//! Create a defaul end-to-end tagger, with no additional delay.
	e2et();
	
	//! Receive a packet from the upper layer entity.
	void recv(Packet *, Handler *);

	//! Tcl interface.
	virtual int command (int argc, const char*const* argv);

	//! Send the packet to the down target.
	void handle (Packet* p);
	
protected:

	//! Current sequence number.
	int sn_;

	//! Random variable to add delay.
	RandomVariable* ranvar_;

	//! Random variable to get packets lost.
	UniformRandomVariable loss_;

	//! Packet error rate. Set via Tcl.
	double per_;

	//! Timer to delay packets randomly.
	TMultiTimer<e2et, Packet*> timer_;
};

#endif

