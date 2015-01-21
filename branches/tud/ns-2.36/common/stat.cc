#include <stat.h>

#include "object.h"
#include "simulator.h"

//! \todo just for test
#include <iostream>

/* ======================================================================
   SamplesAverage Functions
   ====================================================================== */

SamplesAverage::SamplesAverage ()
{
	nsamples = 0;
	sumS = 0;
	minS = FP_NAN;
	maxS = FP_NAN;
}

void
SamplesAverage::add (double x)
{
	// update the min/max sample
	minS = ( nsamples == 0 || x < minS ) ? x : minS;
	maxS = ( nsamples == 0 || x > maxS ) ? x : maxS;

	// update the number of samples
	++nsamples;

	// update the sum of samples
	sumS += x;
}

/* ======================================================================
   SamplesDist Functions
   ====================================================================== */

SamplesDist::SamplesDist ( unsigned int n, double l, double u )
{
	bucket = new double[n];
	nbins = n;

	lower = l;
	upper = u;
	size = ( upper - lower ) / double(nbins);

	reset ();
}

void
SamplesDist::reset ()
{
	n_underflow = 0;
	n_overflow = 0;
	nsamples = 0;
	minS = FP_NAN;
	maxS = FP_NAN;
	for ( unsigned int i = 0 ; i < nbins ; i++ )
		bucket[i] = 0;
}

void
SamplesDist::add (double x, double y)
{
	// get the bucket index
	int i = int ( ( x - lower ) / size );

	if ( i >= 0 && i < (int) nbins )
		bucket[i] += y;
	else if ( i < 0 )
		n_underflow += y;
	else /* if ( i >= nbins ) */
		n_overflow += y;

	// update the minimum and maximum samples
	minS = ( nsamples == 0 || x < minS ) ? x : minS;
	maxS = ( nsamples == 0 || x > maxS ) ? x : maxS;

	// update the number of samples
	nsamples += y;
}

double
SamplesDist::getValue (int i) const
{
	return lower + ( 1.0 + i ) * size;
}

double
SamplesDist::mean () const
{
	double E = 0;
	for ( unsigned int i = 0 ; i < nbins ; i++ )
		E += getValue(i) * bucket[i];

	return ( nsamples == 0 ) ? FP_NAN : ( E / nsamples );
}

/* ======================================================================
   Stat Functions
   ====================================================================== */

Stat::Stat ()
{
	collect = false;
	fileout = 0;
}

int Stat::command(int argc, const char*const* argv) {

	// turn on statistics collection
   if ( argc == 1 && strcmp (argv[0], "on") == 0 ) {
		if ( collect == false ) {
			collect = true;
			enabled = Scheduler::instance().clock();
		}
		return 0;  // TCL_OK

	// turn off statistics collection
	} else if ( argc == 1 && strcmp (argv[0], "off") == 0 ) {
		// ignore if statistics collection is already disabled
		if ( collect == true ) {
			collect  = false;
			// update the run duration
			duration += Scheduler::instance().clock() - enabled;
		}
		return 0; // TCL_OK

	// print out collected statistics, if collection is enabled
	} else if ( argc == 1 && strcmp (argv[0], "print") == 0 ) {
		if ( initialized () == false )
			return 1; // TCL_ERROR
		if ( collect == true )  {
			// update the run duration
			duration += Scheduler::instance().clock() - enabled;
			// print out data samples
			print ();
		}
		return 0; // TCL_OK

	// set the output file for statistics collection
	// does *not* turn statistics collection on
   } else if ( argc == 2 && strcmp (argv[0], "file") == 0 ) {
		bool ret = initialize (argv[1]);
		if ( ret == false ) return 1; // TCL_ERROR
		return 0; // TCL_OK

	// add a scalar metric
	} else if ( argc == 4 && strcmp (argv[0], "add") == 0
			&& strcmp (argv[2], "avg") == 0 ) {

		stat_desc& desc = metrics[std::string(argv[1])];

		desc.type = SCALAR;
		desc.subtype =
			( strcmp (argv[3], "rate") == 0 ) ? RATE :
			( strcmp (argv[3], "continuous") == 0 ) ? CONTINUOUS :
			( strcmp (argv[3], "discrete") == 0 ) ? DISCRETE :
			( strcmp (argv[3], "counter") == 0 ) ? COUNTER : UNKNOWN;

		if ( desc.subtype == UNKNOWN ) {
			fprintf (stderr, "Unknown metric type '%s'\n", argv[3]);
			return 1; // TCL_ERROR
		}

		return 0; // TCL_OK

	// add a distribution metric
	} else if ( argc == 7 && strcmp (argv[0], "add") == 0
			&& strcmp (argv[2], "dst") == 0 ) {

		stat_desc& desc = metrics[std::string(argv[1])];

		desc.type = DISTRIBUTION;
		desc.subtype =
			( strcmp (argv[3], "rate") == 0 ) ? RATE :
			( strcmp (argv[3], "continuous") == 0 ) ? CONTINUOUS :
			( strcmp (argv[3], "discrete") == 0 ) ? DISCRETE : 
			( strcmp (argv[3], "debug") == 0 ) ? COUNTER : UNKNOWN;
		desc.lower = atof (argv[4]);
		desc.upper = atof (argv[5]);
		desc.nbins = atoi (argv[6]);

		if ( desc.nbins <= 0 ) {
			fprintf (stderr, "Metric '%s' has a negative number of bins %d\n",
					argv[1], desc.nbins);
			return 1; // TCL_ERROR
		}

		if ( desc.upper <= desc.lower ) {
			fprintf (stderr,
					"Metric '%s' has lower bound (%f) >= upper bound (%f)\n",
					argv[1], desc.lower, desc.upper);
			return 1; // TCL_ERROR
		}

		if ( desc.subtype == UNKNOWN || desc.subtype == COUNTER ) {
			fprintf (stderr, "Unknown or invalid metric type '%s'\n", argv[3]);
			return 1; // TCL_ERROR
		}

		return 0; // TCL_OK

	// trace the samples of a flow
	} else if ( argc == 4 && strcmp (argv[0], "trace") == 0 ) {

		stat_desc& desc = metrics[std::string(argv[1])];

		desc.trace[atoi(argv[2])] = fopen (argv[3], "w");
		
		if ( ! desc.trace[atoi(argv[2])] ) {
			fprintf (stderr, "Could not open '%s' for writing\n", argv[3]);
			return 1; // TCL_ERROR
		}

		return 0; // TCL_OK
	}

	fprintf (stderr, "invalid stat command: '");
	for ( int i = 0 ; i < argc ; i++ ) fprintf (stderr, "%s ", argv[i]);
	fprintf (stderr, "'\n");
	
	return 1; // TCL_ERROR
}

void Stat::reset ()
{
	// TODO
	fprintf (stderr, "TODO\n");
	abort ();
}

bool
Stat::initialize (const char* filename)
{
	if ( fileout != 0 )
		fclose (fileout);
	fileout = fopen (filename, "w");
	// metrics.set_empty_key (0);
	return initialized();
}

double 
Stat::get (const char* metric, const int ID)
{
        // silent return if we are not collecting samples right now
        // or if we try to get a value of a non-initialized metric
        if ( collect == false || metrics.count(std::string(metric)) == 0 ) {
                return 0.0;
        }
        // Get the desciptor of the metric
        stat_desc& desc = metrics[std::string(metric)];
        // Get the descriptor of the measure
        typedef std::map<int, meas_descriptor>::const_iterator CI;
        CI p = desc.metrics.find(ID);
        // Check if the metrics exists
        if ( p == desc.metrics.end() ) {
                // The measure for the given ID has not been found
                return 0.0;
        }
        const meas_descriptor& meas = p->second;
        const SamplesAverage* const avg = meas.average;
        // Get the current measure value
        double x = 0.0;
        if ( desc.subtype == RATE ) {
                if ( avg->n() != 0 && duration > 0 ) x = avg->sum() / duration;
        } else if ( desc.subtype == CONTINUOUS ) {
                if ( avg->n() > 1 ) x = avg->sum() / meas.sumTimes;
        } else if ( desc.subtype == DISCRETE ) {
                if ( avg->n() != 0 ) x = avg->mean();
        } else if ( desc.subtype == COUNTER ) {
                if ( avg->n() != 0 ) x = avg->sum();
        }
        return x;
}

void
Stat::put (const char* metric, int ID, double x)
{
	// silent return if we are not collecting samples right now
	// or if we try to add a sample to a non-initialized metric
	if ( collect == false || metrics.count(std::string(metric)) == 0 ) return;

	stat_desc& desc = metrics[std::string(metric)];

	// print samples to an output trace file, if needed
	FileMap::iterator it = desc.trace.find (ID);
	if ( it != desc.trace.end() ) fprintf (it->second, "%f %f\n", NOW, x);

	meas_descriptor& meas = desc.metrics[ID];

	if ( desc.type == SCALAR ) {

		// check if the SamplesAverage object has been already initialized
		if ( ! meas.average ) meas.average = new SamplesAverage;

		// update the samples field, according to the metric subtype
		if ( desc.subtype == RATE ) {
			meas.average->add (x);
		} else if ( desc.subtype == CONTINUOUS ) {
			double& last = meas.lastTime; // alias
			if ( last >= 0.0 ) { 
				meas.average->add ( x * ( NOW - last) );
				meas.sumTimes += (NOW - last );
			}
			last = NOW;
		} else if ( desc.subtype == DISCRETE ) {
			meas.average->add (x);
		} else if ( desc.subtype == COUNTER ) {
			meas.average->add (x);
		} else {
			// there are not any other averaged metric subtypes
			abort ();
		}

	} else {
		// check if the SamplesAverage object has been already initialized
		if ( ! meas.distribution ) meas.distribution =
			new SamplesDist ( desc.nbins, desc.lower, desc.upper );

		// update the samples field, according to the metric subtype
		if ( desc.subtype == RATE ) {
			meas.distribution->add (x);
		} else if ( desc.subtype == CONTINUOUS ) {
			double& last = meas.lastTime; // alias
			if ( last >= 0.0 ) { 
				meas.distribution->add ( x, NOW - last );
				meas.sumTimes += (NOW - last );
			}
			last = NOW;
		} else if ( desc.subtype == DISCRETE ) {
			meas.distribution->add (x);
		} else {
			// there are not any other averaged metric subtypes
			abort ();
		}
	}

}

void 
Stat::addPostProcessMetric(TFunctor* functor)
{
        postProcessingFunctions_.push_back(functor);
}

void
Stat::computeAggregateStatistics() 
{
        // Alias
        typedef std::list< TFunctor* >::const_iterator CI;
        // Inkove the callback functions to compute aggregate metrics
        CI p = postProcessingFunctions_.begin();
        while ( p != postProcessingFunctions_.end() ) {
                // Invoke the callback function
                (*p)->call();
                // Get the next function
                p++;
        }
}

void
Stat::print ()
{
	DescMap::iterator it;
	MeasMap::iterator jt;

	if ( ! initialized () ) return;

	//
	// count the number of scalar and distribution metrics, respectively
	//
	
	// Rev. 20070414 Andrea
	// cerr << "Stat.cc: Processing composite statistics" << endl;
	// cerr << "This statistics are based on a combination of primitive statistics" << endl;
	computeAggregateStatistics();

	unsigned int nscalar = 0;
	unsigned int ndistribution = 0;

	for ( it = metrics.begin() ; it != metrics.end() ; ++it ) {
		if ( it->second.metrics.size() == 0 ) continue;
		if ( it->second.type == SCALAR ) ++nscalar;
		else ++ndistribution;
	}

	//
	// print data to the output file
	//

	// get the run identifier from the Simulator object
	unsigned int run = Simulator::run();

	// run ID
	fwrite (&run, sizeof(run), 1, fileout);

	// print the number of scalar metrics
	fwrite (&nscalar, sizeof(nscalar), 1, fileout);

	// exit when all the metrics have been printed out
	for ( it = metrics.begin() ; it != metrics.end() ; ++it ) {
		stat_desc* p = &(it->second); // alias
		const char* name = it->first.c_str(); // alias
		
		// skip any metric which does not have any samples or is a distribution
		if ( p->metrics.size() == 0 || p->type == DISTRIBUTION ) continue;

		// count the number of meaningful metrics
		// i.e. the CUMULATIVE and AVERAGE ones with at least one sample
		// and the TIME metrics with at least two samples
		// The COUNTER metrics are reported even if the number of samples
		// is zero.
		unsigned int how_many = 0;
		for ( jt = p->metrics.begin() ; jt != p->metrics.end() ; ++jt ) {
			SamplesAverage* avg = jt->second.average;
			if ( p->subtype == RATE || p->subtype == DISCRETE || p->subtype == COUNTER) {
				if ( avg->n() >= 1 ) how_many++;
			} else if ( p->subtype == CONTINUOUS ) {
				if ( avg->n() >= 2 ) how_many++;
			} else {
				// there is not other averaged metric subtype
				abort ();
			}
		}
		unsigned int len = strlen (name) + 1;

		fwrite (&how_many, sizeof(how_many), 1, fileout);
		fwrite (&len, sizeof(len), 1, fileout);
		fwrite (name, len, 1, fileout);

		for ( jt = p->metrics.begin() ; jt != p->metrics.end() ; ++jt ) {
			SamplesAverage* avg = jt->second.average;
			double x;
			// average the samples over the run duration
			if ( p->subtype == RATE ) {
                                if ( avg->n() == 0 ) continue;
				x = avg->sum() / duration;
			} else if ( p->subtype == CONTINUOUS ) {
			if ( avg->n() <= 1 ) continue;
				x = avg->sum() / jt->second.sumTimes;
			} else if ( p->subtype == DISCRETE ) {
				if ( avg->n() == 0 ) continue;
				x = avg->mean();
			} else if ( p->subtype == COUNTER ) {
				if ( avg->n() == 0 ) continue;
				x = avg->sum();
			}

			fwrite (&jt->first, sizeof(jt->first), 1, fileout);
			fwrite (&x, sizeof(x), 1, fileout);
		}
	}

	// print the number of distribution metrics
	fwrite (&ndistribution, sizeof(ndistribution), 1, fileout);

	// print out all the remaining distribution metrics
	for ( it = metrics.begin() ; it != metrics.end() ; ++it ) {
		stat_desc* p = &(it->second); // alias
		const char* name = it->first.c_str(); // alias
		
		// skip any metric which does not have any samples or is a scalar
		if ( p->metrics.size() == 0 || p->type == SCALAR ) continue;

		unsigned int ndx = 0;  // number of indices in this metric
		for ( jt = p->metrics.begin() ; jt != p->metrics.end() ; ++jt ) {
			SamplesDist* dist = jt->second.distribution;
			if ( ! dist->empty() > 0 ) ++ndx;
		}

		unsigned int len = strlen (name) + 1;
		unsigned int nbins = p->nbins;
		double binSize = ( p->upper - p->lower ) / (double)nbins;
		double lowerBd = p->lower;

		fwrite (&ndx, sizeof(ndx), 1, fileout);
		fwrite (&len, sizeof(len), 1, fileout);
		fwrite (name, len, 1, fileout);
		fwrite (&binSize, sizeof(binSize), 1, fileout);
		fwrite (&lowerBd, sizeof(lowerBd), 1, fileout);
		fwrite (&nbins, sizeof(nbins), 1, fileout);
		
		for ( jt = p->metrics.begin() ; jt != p->metrics.end() ; ++jt ) {
			SamplesDist* dist = jt->second.distribution;

			if ( dist->empty() ) continue;

			fwrite (&jt->first, sizeof(jt->first), 1, fileout);
		  for ( unsigned int k = 0 ; k < nbins ; k++ ) {
				double x = dist->getSamplesPerBin (k) / dist->n();
				fwrite (&x, sizeof(x), 1, fileout);
			}
		}
	}

	fflush (fileout);
}

// static object & members initialization
Stat Stat::stat;
Stat::DescMap Stat::metrics;
Stat::FunctionList Stat::postProcessingFunctions_;
double Stat::duration = 0.0;
double Stat::enabled = 0.0;
bool Stat::collect = false;
FILE* Stat::fileout = 0; 
