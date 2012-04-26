/**
   project: measure
   filename: fairness.h
   author: C. Cicconetti <c.cicconetti@iet.unipi.it>
   year: 2006
   affiliation:
      Dipartimento di Ingegneria dell'Informazione
      University of Pisa, Italy
   description:
      Fairness measures collection module
*/

#ifndef ns_fairness_h
#define ns_fairness_h

#include <vector>

#include <object.h>

#include <t_timers.h>

//! Estimate fairness.
/*!
  Fairness is estimated via a set of timers, which can be set to expire
  at periodical interval with different periods. Each interval produces
  exactly one fairness measure, which is identified via an unsigned int.
  Identifiers are forced to be contiguous (since we use an array as the
  data structure to store them).

  The set of intervals is configured via Tcl.

  Fairness is computed according to Jain's fairness index.
  */
class FairnessEstimator : public TclObject {
	//! Different fairness indexes.
	enum Type { JAIN };

	//! Fairness descriptor.
	struct Descriptor {
		//! Array of measures, resized to actors_.
		std::vector<double> measures_;
		//! Sampling period, in seconds.
		double period_;
		//! Fairness index type.
		Type type_;
	};
	
	//! Timer to sample fairness.
	/*!
	  This timer is used to both send samples to Stat and reset the
	  collected measures.
	  */
	TMultiTimer<FairnessEstimator, unsigned int> timer_;

	//! Number of entities which produce samples. Configured via Tcl.
	unsigned int actors_;

	//! First numerical identifier for the Stat class. Configured via Tcl.
	unsigned int firstId_;

	//! Array of descriptors. Each element is added via a Tcl command.
	std::vector<Descriptor> descriptors_;

public:
	//! Create an empty object, which has to be initialized via Tcl.
	FairnessEstimator ();
	//! Do nothing.
	virtual ~FairnessEstimator () { }
	
	//! Tcl interface.
	virtual int command (int argc, const char*const* argv);

	//! Add a sample for a given actor (indexed from 0 to actors_ - 1).
	virtual void put (double sample, unsigned int actor);

	//! Handle timer expires.
	void handle (unsigned int ndx);
};

#endif // ns_fairness_h
