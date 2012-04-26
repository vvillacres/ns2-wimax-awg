/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/* End to End Tagger connector
   This code has been developed within the EuQoS project of the EU IST 6th Framework Programme. 
   The "end to end tagger" connector sits between a sending agent and the node the latter is attached to. 

   For every packet sent BY the agent, it sets the following extension fields in the common header:
   - e2e_ts_ : e2e timestamp, set to the current clock
   - e2e_sn_ : e2e sequence number, starting from 0

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

#include <connector.h>
#include <packet.h>
#include "e2et.h"

/* Stea, June 2005
 the e2e tagger and monitor can compute per-path subdelays. Every node can timestamp the packet on arrival, and the
e2e monitor collects them all and puts them into a file. The first timestamp is put by the e2e tagger.
*/

static class e2etClass : public TclClass {
public:
	e2etClass() : TclClass ("e2et") {}
	TclObject* create(int,const char*const*) {
		return (new e2et());
	}
} class_e2et;

e2et::e2et () : loss_(0.0, 1.0), timer_(this)
{
	sn_     = 0;
	ranvar_ = 0;
	per_    = 0;
}

int
e2et::command (int argc, const char*const* argv)
{
	if ( argc == 3 && strcmp (argv[1], "ranvar") == 0 ) {
		ranvar_ = (RandomVariable*) TclObject::lookup (argv[2]);
		return TCL_OK;
	} else if ( argc == 3 && strcmp (argv[1], "per") == 0 ) {
		per_ = atof (argv[2]);
		if ( per_ < 0 ) {
			fprintf (stderr, "Invalid packet error rate '%f'\n", per_);
			return TCL_ERROR;
		}
		return TCL_OK;
	}
	return Connector::command (argc, argv);
}

void
e2et::recv(Packet *p, Handler *)
{
	hdr_cmn *ch = hdr_cmn::access(p);
	ch->e2e_timestamp_no() = 0;
	ch->e2e_timestamp(0) = Scheduler::instance().clock();
	ch->e2e_sn() = sn_++;
	ch->e2e_timestamp_no()++;
	if ( per_ == 0 || loss_.value() > per_ ) {
		if ( ! ranvar_ ) handle (p);
		else {
			double jitter = ranvar_->value();
			timer_.add ( ( jitter > 0 ) ? jitter : 0, p);
		}
	} else
		Packet::free (p);
}

void
e2et::handle (Packet* p)
{
	target_->recv(p);
}
