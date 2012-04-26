/**
   project: measure
   filename: fairness.cc
   author: C. Cicconetti <c.cicconetti@iet.unipi.it>
   year: 2006
   affiliation:
      Dipartimento di Ingegneria dell'Informazione
      University of Pisa, Italy
   description:
      Fairness measures collection module
*/

#include <fairness.h>

#include <stat.h>

static class FairnessEstimatorClass : public TclClass {
public:
   FairnessEstimatorClass() : TclClass("FairnessEstimator") {}
   TclObject* create(int, const char*const*) {
      return (new FairnessEstimator);
   }
} class_fairness_estimator;


FairnessEstimator::FairnessEstimator () : timer_ (this)
{
	actors_ = 0;
	firstId_ = 0;
}

int
FairnessEstimator::command (int argc, const char*const* argv)
{
	if ( argc == 3 && strcmp (argv[1], "actors") == 0 ) {
		if ( actors_ > 0 ) {
			fprintf (stderr, "You cannot change the number of actors\n");
			return TCL_ERROR;
		} else if ( atoi(argv[2]) <= 0 ) {
			fprintf (stderr, "Invalid actors number '%s'. "
					"Choose a number greater than or equal to 1\n", argv[2]);
			return TCL_ERROR;
		}
		actors_ = (unsigned int) atoi (argv[2]);
		return TCL_OK;

	} else if ( argc == 3 && strcmp (argv[1], "first-id") == 0 ) {
		if ( atoi(argv[2]) <= 0 ) {
			fprintf (stderr, "Invalid first Stat ID '%s'. "
					"Choose a number greater than or equal to 0\n", argv[2]);
			return TCL_ERROR;
		}
		firstId_ = (unsigned int) atoi (argv[2]);
		return TCL_OK;

	} else if ( argc == 4 && strcmp (argv[1], "add") == 0 ) {
		// add a new element to the array of descriptors
		descriptors_.resize (descriptors_.size() + 1);

		// get a reference to the new element
		Descriptor& buf = descriptors_.back();
		
		if ( strcmp (argv[2], "jain") == 0 ) {
			buf.type_ = JAIN;
		} else {
			fprintf (stderr, "Invalid fairness index '%s'. "
					"Choose 'jain'\n", argv[2]);
			return TCL_ERROR;
		}

		if ( atof (argv[3]) <= 0 ) {
			fprintf (stderr, "Invalid sampling period '%s'. "
					"Choose a positive number of milliseconds.\n", argv[3]);
			return TCL_ERROR;
		}
		buf.period_ = 1e-3 * atof (argv[3]);

		if ( actors_ == 0 ) {
			fprintf (stderr, "The number of actors must be initialized "
					"befor you add fairness measures\n");
			return TCL_ERROR;
		}
		buf.measures_.resize (actors_);

		timer_.add (buf.period_, descriptors_.size() - 1);

		return TCL_OK;
	}
	return TCL_ERROR;
}

void
FairnessEstimator::put (double sample, unsigned int actor)
{
	// ignore actors which lie outside the specified range
	if ( actor > ( actors_ - 1 ) ) return;

	for ( unsigned int i = 0 ; i < descriptors_.size() ; i++ )
		descriptors_[i].measures_[actor] += sample;
}

void
FairnessEstimator::handle (unsigned int ndx)
{
	// current descriptor
	Descriptor& current = descriptors_[ndx];

	// compute the fairness index for the expired descriptor
	if ( current.type_ == JAIN ) {

		//
		// we compute the Jain's fairness index, which is equal to the
		// square of the measures' sum divided by the sum of square of the
		// measures times the number of measures
		//
		// the index range is thus [0, 1]
		//

		double numerator = 0.0;
		double denominator = 0.0;
		for ( unsigned int i = 0 ; i < actors_ ; i++ ) {
			double& x = current.measures_[i];
			numerator += x;
			denominator += x * x;
			x = 0.0;            // remove each measure after use
		}
		denominator *= actors_;
		numerator *= numerator;

		// if the denominator is zero, then we do not insert any sample
		if ( denominator > 0 )
			Stat::put ("fairness", firstId_ + ndx, numerator/denominator);
	}

	// restart the timer
	timer_.add (current.period_, ndx);
}
