/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/* End to End Monitor connector
   This code has been developed within the EuQoS project of the EU IST 6th Framework Programme. 
   The "end to end monitor" connector sits between a receiving agent and the node the latter is attached to. 

   For every packet sent TO the agent, it checks the following EuQoS extension fields, set by a companion 
   "End to End Tagger" agent (see e2et.{cc.h}), in the common header:
   - e2e_ts_ : end-to-end timestamp, which stores the time at which the packet was transmitted
   - e2e_sn_ : end-to-end sequence number, starting from 0

   The e2em end connector computes delay, jitter, throughput and packet loss ratio statistics.
   Furthermore, it can log the above measures on separate files.

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



#ifndef ns_e2em_h
#define ns_e2em_h

#include <fairness.h>
#include <connector.h>
#include <config.h>
#include <packet.h>
#include <timer-handler.h>

class e2em;

enum MeasureID {OWD=0, IPDV=1, IPT=2, OWPL=3, NoMeasures};
// OWD= One-Way Delay
// IDPV = Inter-Packet Delay Variation
// IPT = IP Throughput
// OWPL = One-Way Packet Loss
// NB: if needed, add extra measures BEFORE NoMeasures. Leave NoMeasures as the last item.
// NB2: in case MORE measures are added, leave IPT as the *first* time-averaged measures



/////////////////////////////////////////////////////////////////////////////
//
// END TO END MONITOR TIMER CLASS
//
/////////////////////////////////////////////////////////////////////////////


class e2em_Timer : public TimerHandler {
public:
	e2em_Timer() : TimerHandler() {};
	void init (e2em*, int, double);
	void settimeout (double);
	double gettimeout (void);


protected:
	virtual void expire(Event *e);
	e2em* e2em_;	
	int m_;		// # of the measure to which the timer is associated
	double interval_;	// # timer interval
};



/////////////////////////////////////////////////////////////////////////////
//
// END TO END MONITOR CLASS
//
/////////////////////////////////////////////////////////////////////////////

class e2em : public Connector {
public:
	e2em();
	void timeout(int);

protected:
	virtual int command(int argc, const char*const* argv);
	virtual void recv(Packet *, Handler *);
	e2em_Timer e2em_timer_;

private:
	void write_stats(void);
	void reset_stats(void);
	void start_log (const char* const);
	void stop_log (const char* const);
	void attach_file (const char* const, const char* const);
	void set_interval (const char* const, double);
	int string2measure (const char* const);

struct Measure  {char* name;
		 e2em_Timer* timer;
		 FILE* tracefile;
		 char filename[100];
		 int active;
		 double interval;	// meaningful only for time-averaged measures
		 double sum;		// meaningful only for time-averaged measures
		 int sample_no;		// meaningful only for time-averaged measures
		} measure[NoMeasures];


struct StatDesc	{ double min;
		  double max;
		  double sum;
		  double sqsum;
		};

struct Statistics {				// those appearing as output of "get-stat"
		 
		  double left_edge;		// left edge of the time window
		  int arrived;
		  double bytesum;
		  int lostp;
		  struct StatDesc stat[IPT];
		} statistics;

double 	last_packet_delay[TSTAMP_NO],	// used for computing jitter on a packet arrival.
	last_packet_e2e_delay;			
int next_expected;			// used for checking losses
	int index_;             // used to set the ID in the Stat measures
	                        // can be changed via Tcl command
	FairnessEstimator* fairnessEstimator_;  // fairness estimator object

};

#endif
