/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/* End to End Monitor connector
   This code has been developed within the EuQoS project of the EU IST 6th Framework Programme. 
   The "end to end monitor" connector sits between a receiving agent and the node the latter is attached to. 

   For every packet sent TO the agent, it checks the following EuQoS extension fields, set by a companion 
   "End to End Tagger" agent (see e2et.{cc.h}), in the common header:
   - e2e_ts_ : end-to-end timestamp, which stores the time at which the packet was transmitted
   - e2e_sn_ : end-to-end sequence number, starting from 0

   The e2em end connector computes delay, e2e_jitter, throughput and packet loss ratio statistics.
   Furthermore, it can log the above measures on separate files.

   Giovanni Stea (g.stea@iet.unipi.it), University of Pisa, Italy, January 2005. 

   ------
   CAVEAT (July 2005)
   The measurement connectors have been tested to work with Token Bucket filter (which is a connector itself). In that case, 
   the RELATIVE order in which the attach-e2et and attach-tbf operations are performed determines whether the queueing delay of the TBF
   is/is not counted in the e2e delay. More specifically, the *last* attach- operation attaches the connector which is closest to the agent.

   ------
   UPDATE (March 2006)
   Modified as follows: rather than one timestamp, an ARRAY of timestamps is provisioned. The e2e tagger sets the FIRST timestamp in the array.
   Other timestamps can be set by the timestamper connector (see timestamper.cc), whch sits at the ingress of a link. This way, the e2e monitor 
   can compute the delay and jitter contribution of sub-paths in an e2e path.

*/

#define DEBUG(x);

#include <e2em.h>
#include <stat.h>


/////////////////////////////////////////////////////////////////////////////
//
// END TO END MONITOR TIMER CLASS
//
/////////////////////////////////////////////////////////////////////////////

void e2em_Timer::init (e2em* e2emconn, int me, double interv)
{
	if (me<0 || me >= NoMeasures)
	{

	}

	e2em_= e2emconn;
	m_=me;
	interval_=interv;

}

void e2em_Timer::settimeout (double d)
{
	if (d<=0.)
	{
		fprintf (stderr, "\n\nERROR: e2em_Timer::settimeout (Interval=%g). \nAveraging interval must be greater than zero. \nAborting at time %g...\n", 
			 d, Scheduler::instance().clock());
		exit (-1);
	}

	interval_=d;
}


double e2em_Timer::gettimeout (void)
{
	return interval_;
}


void e2em_Timer::expire(Event* /*e*/)
{
	e2em_->timeout(m_);
}




/////////////////////////////////////////////////////////////////////////////
//
// END TO END MONITOR CLASS
//
/////////////////////////////////////////////////////////////////////////////


e2em::e2em()
{
	for (int i=0; i<NoMeasures; i++)
	{
		measure[i].timer = new (e2em_Timer);
		measure[i].timer->init(this, i, 5.);
		measure[i].active =0;
		measure[i].interval =5.;
		measure[i].sum=0;
		measure[i].sample_no=0;
		measure[i].tracefile=NULL;
		strcpy(measure[i].filename, "\0");
	}

	measure[OWD].name="OWD";
	measure[IPDV].name="IPDV";
	measure[IPT].name="IPT";
	measure[OWPL].name="OWPL";	

	for (int j=0; j<TSTAMP_NO; j++) last_packet_delay[j]=-1.;
	last_packet_e2e_delay=-1.;
	next_expected=0;

	static int nextIndex = 0;
	index_ = nextIndex++;

	fairnessEstimator_ = 0;

	reset_stats();
}
	


void e2em::recv(Packet *p, Handler *)
{
	double now= Scheduler::instance().clock();
	hdr_cmn* cmn = hdr_cmn::access(p);	

	int out_of_seq, ts_no;
	double del[TSTAMP_NO], e2e_del, e2e_jit;


	// check sequence number and compute out-of-sequence	
	// :TODO: manage wrap-around of the sequence numbers
	out_of_seq=cmn->e2e_sn()-next_expected;
	next_expected=cmn->e2e_sn()+1;	

	DEBUG(
		if (out_of_seq<0)
			fprintf (stderr, "\nInternal error at time %g: expected packet %d, arrived %d.\n", now, next_expected, cmn->e2e_sn());
	);


	// compute e2e delay and jitter
	e2e_del = now-cmn->e2e_timestamp(0);
	e2e_jit = last_packet_e2e_delay>=0? last_packet_e2e_delay-e2e_del : 0;
	last_packet_e2e_delay=e2e_del;

	// add a delay sample
	Stat::put ("e2e_owd_a", index_, e2e_del);
	Stat::put ("e2e_owd_d", index_, e2e_del);

	// add a jitter sample if the last packet was received in order
	if ( out_of_seq == 0 ) Stat::put ("e2e_ipdv_d", index_, e2e_jit);

	
	DEBUG(
		if (e2e_del<0.)
			fprintf (stderr, "\nInternal error at time %g: packet left at %g and arrived at %g.\n", now, cmn->e2e_timestamp(0), now);
	);
		

	if (measure[OWD].active || measure[IPDV].active)
	{
		/* Stea, March 2006
	 	the e2e tagger and monitor can also compute per-subpath delays and jitter. Every link can timestamp the packet on arrival, 
		(provided a timestamp connector is attached to it), and the e2e monitor parses all the timestamps and writes the 
		delays and jitter to a file. The first timestamp is put by the e2e tagger. The delays and jitter are written as follows:
		<arrival time> <e2e del/jit> <del/jit from e2et to 1st timestamper> <del/jit from 1st to 2nd timestamper> [...] <del/jit from last timestamper to e2em>
		*/

		ts_no=cmn->e2e_timestamp_no();	

		for (int ts=0; ts<ts_no-1; ts++)
		{
			// per subpath delays
			del[ts] = cmn->e2e_timestamp(ts+1)-cmn->e2e_timestamp(ts);
	
			DEBUG(
				if (del[ts]<0)
					fprintf (stderr, "\nInternal error at time %g (computing partial delays): packet left at %g and arrived at %g.\n", now, cmn->e2e_timestamp(), cmn->e2e_timestamp(ts+1));
				);
		}
		if (ts_no>1)
			del[ts_no-1]=now-cmn->e2e_timestamp(ts_no-1);
	
		// write down packet-wise measures
		/*
		if (measure[OWD].active)
		{
			fprintf (measure[OWD].tracefile, "%g\t%g", now, e2e_del);
			if (ts_no>1)
				for (int no=0; no<ts_no; no++)
					fprintf (measure[OWD].tracefile, "\t%g", del[no]);
			fprintf (measure[OWD].tracefile, "\n");
		}
	
		if (measure[IPDV].active)
		{
			fprintf (measure[IPDV].tracefile, "%g\t%g", now, e2e_jit);
			if (ts_no>1)
				for (int no=0; no<ts_no; no++)
					fprintf (measure[IPDV].tracefile, "\t%g", (last_packet_delay[no]>=0.)?last_packet_delay[no]-del[no]:0.);
			fprintf (measure[IPDV].tracefile, "\n");
		}
		*/

		for (int j=0; j<ts_no; j++) last_packet_delay[j]=del[j];
	}


	// update time-averaged measures
	if (measure[IPT].active)
	{
		measure[IPT].sum+=cmn->size();
		measure[IPT].sample_no++;
		Stat::put ("e2e_tpt", index_, cmn->size());
		if ( fairnessEstimator_ ) fairnessEstimator_->put (cmn->size(), index_);
	}

	if (measure[OWPL].active)
	{
		measure[OWPL].sum+= out_of_seq;
		measure[OWPL].sample_no+=out_of_seq+1;
		Stat::put ("e2e_owpl", index_, out_of_seq);
	}
	
	// update statistics
	statistics.arrived++;
	statistics.bytesum+=cmn->size();
	statistics.lostp+=out_of_seq; 

	statistics.stat[OWD].min = e2e_del<statistics.stat[OWD].min ? e2e_del : statistics.stat[OWD].min;
	statistics.stat[OWD].max = e2e_del>statistics.stat[OWD].max ? e2e_del : statistics.stat[OWD].max;
	statistics.stat[OWD].sum += e2e_del;
	statistics.stat[OWD].sqsum += e2e_del*e2e_del;

	statistics.stat[IPDV].min = e2e_jit<statistics.stat[IPDV].min ? e2e_jit : statistics.stat[IPDV].min;
	statistics.stat[IPDV].max = e2e_jit>statistics.stat[IPDV].max ? e2e_jit : statistics.stat[IPDV].max;
	statistics.stat[IPDV].sum += e2e_jit;
	statistics.stat[IPDV].sqsum += e2e_jit*e2e_jit;

	// handle the packet to the receiving agent
	target_->recv(p);

}


void e2em::timeout(int i)
{
	if (i>=0 && i<IPT) return;	// shouldn't ever occur...

	double now= Scheduler::instance().clock();

	if (i>=NoMeasures || i<0)
	{
		fprintf(stderr, "\n\nERROR: e2em::timeout (%d). Unknown measure id. Should be between 0 and %d:\n", i, NoMeasures-1);
		fprintf (stderr, "\nAborting at time %g\n", now);
		exit (-1);
	}

	if (measure[i].active)		// "active" could have been reset while the timer was running. In that case, the file would not be open.
	{
		double deltat=measure[i].timer->gettimeout();
		// write down on the tracefile the relevant time-averaged measure
		/*
		if (i==IPT)
			fprintf (measure[i].tracefile, "%g\t%g\t%g\n", now, 
				measure[i].sum/deltat, measure[i].sample_no/deltat);
		else if (i==OWPL)
			fprintf (measure[i].tracefile, "%g\t%g\n", now, 
				 measure[i].sample_no>0 ? measure[i].sum/measure[i].sample_no : 0.);
				 */

		// reset the counters
		measure[i].sum=0.;
		measure[i].sample_no=0;

		// restart the timer
		measure[i].timer->settimeout(measure[i].interval);	// the latter might have been changed while the timer was running
		measure[i].timer->resched(measure[i].interval);
	}
}


int e2em::string2measure (const char* const meas)
{
	int i;
	for (i=0; i<NoMeasures; i++)
	if (!strcmp (meas, measure[i].name)) return i;

	fprintf(stderr, "\n\nERROR: e2em::string2measure (%s). Unknown measure. Those available are:\n", meas);
	for (i=0; i<NoMeasures; i++)
		fprintf(stderr, "%s\t",measure[i].name);
	fprintf (stderr, "\nAborting at time %g\n", Scheduler::instance().clock());
	exit (-1);
}


void e2em::write_stats(void)
{
	double deltat = Scheduler::instance().clock()-statistics.left_edge;
	fprintf (stdout, "\nFrom time %g to time %g", statistics.left_edge, Scheduler::instance().clock());
	fprintf (stdout, "\nArrived %d packets (%g bytes)", statistics.arrived, statistics.bytesum);
	fprintf (stdout, "\nPacket Loss %g %%; throughput %g bits per second, (%g packets per second)", 
				(statistics.arrived>0 ? statistics.lostp/((double)statistics.arrived+statistics.lostp) : 0), 
				(deltat>0? statistics.bytesum/deltat : 0),
				(deltat>0? statistics.arrived/deltat : 0));

	for (int i=0; i<2; i++)
		fprintf (stdout, "\n%s - Min %g; Avg %g; Max %g; SqAvg %g", measure[i].name,
			(statistics.arrived>0 ? statistics.stat[i].min : 0),  
			(statistics.arrived>0 ? statistics.stat[i].sum/(double)statistics.arrived : 0),
			(statistics.arrived>0 ? statistics.stat[i].max : 0),
			(statistics.arrived>0 ? (statistics.stat[i].sqsum)/(double)statistics.arrived : 0));

	fprintf (stdout, "\n");
	fflush (stdout);
}


void e2em::reset_stats(void)
{
	
	statistics.left_edge=Scheduler::instance().clock();
	statistics.arrived=0;
	statistics.bytesum=0.;
	statistics.lostp=0;

	for (int j=0; j<IPT; j++)
	{
		statistics.stat[j].min=1e10;
		statistics.stat[j].max=-1e10;
		statistics.stat[j].sum=0;
		statistics.stat[j].sqsum=0;
	}
}


void e2em::start_log (const char* const meas)
{
	int i= string2measure(meas);
	
	DEBUG (		
		fprintf (stdout, "\n\nStart log measure %d, which is now active=%d\n\n", i, measure[i].active);
		);

	if (!measure[i].active)	// otherwise do nothing
	{
		// measure[i].tracefile = fopen (measure[i].filename, "a+");
		measure[i].active =1;
		measure[i].sum=0;
		measure[i].sample_no=0;

		for (int j=0; j<TSTAMP_NO; j++) last_packet_delay[j]=-1.;
		last_packet_e2e_delay=-1.;
	
		if (i>=IPT)	// timer is only to be activated for time-averaged measures
		{
			measure[i].timer->settimeout(measure[i].interval);
			measure[i].timer->resched(measure[i].interval);
		}
	}
}


void e2em::stop_log (const char* const meas)
{
	int i= string2measure(meas);

	DEBUG (		
		fprintf (stdout, "\n\nStop log measure %d, which is now active=%d\n\n", i, measure[i].active);
		);

	if (measure[i].active)	// otherwise do nothing
	{
		// fclose(measure[i].tracefile);
		measure[i].active =0;

		// timer NOT to be stopped here (rather, check the active flag at next timeout)
	}
}


void e2em::attach_file (const char* const meas, const char* const fname)
{
	int i= string2measure(meas);
	strcpy (measure[i].filename, fname);
	// measure[i].tracefile = fopen (measure[i].filename, "w");
	// fclose(measure[i].tracefile);
}


void e2em::set_interval (const char* const meas, double interval)
{
	int i= string2measure(meas);

	if (interval <=0.)
	{
		fprintf (stderr, "\n\nERROR: e2em::set_interval (string, Interval=%g). \nAveraging interval must be greater than zero. \nAborting at time %g...\n", 
			 interval, Scheduler::instance().clock());
		exit (-1);
	}

	measure[i].interval=interval;
}


int e2em::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "get-stats") == 0) {
			write_stats();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "reset-stats") == 0) {
			reset_stats();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "start-log") == 0) {
			start_log ("IPT");
			start_log ("OWD");
			start_log ("OWPL");
			start_log ("IPDV");
			return (TCL_OK);
		}
	}
	else if (argc==3)
	{
		if (strcmp(argv[1], "fairness-estimator") == 0) {
			fairnessEstimator_ = (FairnessEstimator*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "index") == 0) {
			index_ = atoi (argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "start-log") == 0) {
			start_log (argv[2]); 
			return (TCL_OK);
		}
		if (strcmp(argv[1], "stop-log") == 0) {
			stop_log (argv[2]); 
			return (TCL_OK);
		}
	}

	else if (argc==4)
	{
		if (strcmp(argv[1], "attach-file") == 0) {
			attach_file (argv[2], argv[3]); 
			return (TCL_OK);
		}
		if (strcmp(argv[1], "set-interval") == 0) {
			set_interval (argv[2], atof(argv[3])); 
			return (TCL_OK);
		}
	}
	return (Connector::command(argc, argv));
}



/////////////////////////////////////////////////////////////////////////////
//
// END TO END MONITOR - OTCL LINKAGE
//
/////////////////////////////////////////////////////////////////////////////


static class e2emClass : public TclClass {
public:
	e2emClass() : TclClass ("e2em") {}
	TclObject* create(int,const char*const*) {
		return (new e2em());
	}
}class_e2em;
