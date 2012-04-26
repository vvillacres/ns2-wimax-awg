/**
   project: measure
   filename: stat.cc
   author: C. Cicconetti <c.cicconetti@iet.unipi.it>
   year: 2006
   affiliation:
      Dipartimento di Ingegneria dell'Informazione
      University of Pisa, Italy
   description:
      body of ns2 SamplesDist and Stat classes
*/

#ifndef ns_stat_h
#define ns_stat_h

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <tfunctor.h>
#include <list>

#include <map>
#include <string>

//#include <google/dense_hash_map>

/* ======================================================================
   SamplesAverage class
   ====================================================================== */

//! Samples average class.
class SamplesAverage {
	/*
    * Public members.
	 */
public:
	//! Build an empty SamplesAverage object.
	SamplesAverage ();
	//! Return the smallest sample.
	double minSample () const { return minS; }
	//! Return the largest sample.
	double maxSample () const { return maxS; }
	//! Return the avereage.
	double mean () const { return sumS / nsamples; }
	//! Return the sum of samples.
	double sum () const { return sumS; }
	//! Return the number of samples.
	unsigned int n () const { return nsamples; }
	//! Return true if no samples have been added.
	bool empty () const { return ( nsamples == 0 ) ? true : false; }
	//! Add a sample.
	void add (double x);
private:
	//! Number of samples.
	unsigned int nsamples;
	//! Smallest sample.
	double minS;
	//! Largest sample.
	double maxS;
	//! Sum of samples.
	double sumS;
};

/* ======================================================================
   SamplesDist class
   ====================================================================== */

//! Samples distribution class.
class SamplesDist {
	/*
    * Public members.
	 */
public:
	//! Create an empty SamplesDist object.
	SamplesDist ( unsigned int n = 0, double l = 0.0 , double u = 0.0 );

	//! Reinitialize the object. Stored samples are dropped.
	void initialize ( unsigned int n, double l, double u );
	//! Clean all collected samples.
	void reset ();

	//! Add sample y to bin x.
	void add ( double x, double y = 1.0 );

	//! Get the minimum sample. If no samples => FP_NAN.
	double minSample () const { return minS; }
	//! Get the maximum sample. If no samples => FP_NAN.
	double maxSample () const { return maxS; }
	//! Get the samples mean. If no samples => FP_NAN.
	double mean () const;

	//! Return true if no samples have been added.
	bool empty () const { return ( nsamples == 0 ) ? true : false; }
	//! Get the number of samples.
	double n () const { return nsamples; }

	//! Return the number of underflow samples.
	double underflow () const { return n_underflow; }
	//! Return the number of overflow samples.
	double overflow () const { return n_overflow; }
	//! Return the bin size.
	double bin_size () const { return size; }
	//! Return the number of bins.
	unsigned int getNBins () const { return nbins; }

	//! Return the number of samples in a bin.
	double getSamplesPerBin (unsigned int i) const { return bucket[i]; }

	/*
    * Private members.
	 */
private:
	//! Copy construction forbidden.
	SamplesDist (const SamplesDist&);
	//! Assignment forbidden.
	const SamplesDist& operator= (const SamplesDist&);

	//! Return the upper bound of the i-th bucket.
	double getValue (int i) const;

	/*
    * Data structure.
	 */
private:
	//! Buckets of samples.
	/*!
	  Each bucket i stores the number of samples falling into the interval
	     [lower + i * size, lower + (i+1) * size)
	  where size is ( ( upper - lower ) / N_BUCKETS ).
	  Samples falling behind this interval are dropped.
	  */
	double* bucket;
	//! Number of bins.
	unsigned int nbins;
	//! Lower bound of the samples to collect.
	double lower;
	//! Upper bound of the samples to collect.
	double upper;
	//! Number of underflow samples.
	double n_underflow;
	//! Number of overflow samples.
	double n_overflow;
	//! Bucket size.
	double size;
	//! Smallest sample.
	double minS;
	//! Largest sample.
	double maxS;
	//! Number of samples.
	double nsamples;
};

/* ======================================================================
   Stat class
   ====================================================================== */

//! Gather statistics.
/*!
  TODO documentation.
*/

class Stat {
public:
	//! Metric subtype (cumulative, time-averaged, averaged).
	enum SubType { RATE = 0, CONTINUOUS, DISCRETE, TRACE, COUNTER, UNKNOWN };

	//! Metric type, scalar or distribution.
	enum MetricType { SCALAR, DISTRIBUTION };

	//! Create an empty Stat object.
	/*!
	  Data structure are allocated using create_arrays() and initialized using
	  reinitialize_arrays().
	  */
   Stat ();

protected:

	//! Only (static) member. It enforces the allocation of only one Stat object.
	static Stat stat;

   /*
    *   PUBLIC METHODS
    */
public:
	//! Deallocate data structures that were allocated using create_arrays().
   // ~Stat ();

	//! Tcl command interface.
	static int command (int argc, const char*const* argv);

        //! Reset statistics.
        static void reset ();
	
	//! Start/stop collecting samples.
	static void active (bool a) { collect = a; }
	//! Return active flag.
	static bool active () { return collect; }

	//! Return true if the output file has been correctly opened.
	static bool initialized () { return (fileout == 0) ? false : true; }
	//! Open the output file. Return true if successful.
	static bool initialize (const char* filename);

	//! Insert a new sample.
	static void put (const char* metric, int ID, double x);

        //! Get the current value of a metric
        static double get (const char* metric, const int ID);

        //! Add a post processing metric
        static void addPostProcessMetric(TFunctor* functor);

        //! \todo To be commented
        static void computeAggregateStatistics();

	//! Print out the collected statistics.
	static void print ();

	//! Return a reference to the current run duration.
	static double& runDuration () { return duration; }

	//! Return a reference to the last time that collection was enabled.
	static double& lastEnabled () { return enabled; }

public:
	//! Metric descriptor.
	struct meas_descriptor {
		//! Last sampling time. Only meaningful with TIME metrics.
		double lastTime;
		//! Sum of the sampling times. Only meaningful with TIME metrics.
		double sumTimes;
		//! Distribution object.
		SamplesDist* distribution;
		//! Average object.
		SamplesAverage* average;

		//! Create an empty averaged metric descriptor.
		meas_descriptor () :
			lastTime (-1.0), sumTimes (0), distribution (0), average (0) { }
		//! Return true only if initialized, false otherwise.
		bool initialized () { return ( distribution || average ) ? true : false; }
		//! Delete the metric descriptor.
		~meas_descriptor () {
			if ( distribution ) delete distribution;
			if ( average ) delete average;
		}
	};

	typedef std::map<int, meas_descriptor> MeasMap;
	typedef std::map<int, FILE*> FileMap;

	//! Metric descriptor.
	struct stat_desc {
		//! Metric type, scalar or distribution.
		MetricType type;
		//! Metric subtype, only meaningful for averaged metrics.
		SubType subtype;

		//! Lower bound for distribution metrics.
		double lower;
		//! Upper bound for distribution metrics.
		double upper;
		//! Number of bins for distribution metrics.
		int nbins;

		//! Map of metric descriptors.
		MeasMap metrics;

		//! Map of output trace file descriptors. For trace measures.
		FileMap trace;

		//! Create an empty metric descriptor.
		stat_desc () : type (SCALAR), subtype (UNKNOWN),
							lower (0.0), upper (0.0), nbins (0) { }

	private:
		// stat_desc (const stat_desc&);
		stat_desc& operator= (const stat_desc&);
	};


	// typedef google::dense_hash_map<std::string, stat_desc> DescMap;
	typedef std::map<std::string, stat_desc> DescMap;

        //! Alias for a list of function pointers
        typedef std::list< TFunctor* > FunctionList;
	
   /*
    *    DATA STRUCTURE
    */
private:
	//! Array of metric descriptors. This is the main data structure.
	static DescMap metrics;

        //! List of callback function that will be called before print the
        //! metrics
        static FunctionList postProcessingFunctions_;

	//! True if statistics collecting is on.
	static bool collect;

	//! File descriptor to store statistics.
	static FILE* fileout;

	//! Current run duration.
	static double duration;

	//! Last time that statistics collection was enabled.
	static double enabled;
};

#endif // ns_stat_h 
