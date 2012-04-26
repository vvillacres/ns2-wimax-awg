/*
 * TODO: INSERT LICENSE HERE
 */

#ifndef __NS2_VOIP_BIDIRECTIONAL_H
#define __NS2_VOIP_BIDIRECTIONAL_H

#include <object.h>
#include <t_timers.h>
#include <ranvar.h>
#include <list>

class VoipSource;

class VoipBidirectional : public TclObject {
public:
        //! Create an empty VoIP bidirectional module.
        VoipBidirectional ();

        //! Return true if the object is ready to be started.
        bool initialized () { return ( nsources_ >= 1 ); } 

        //! Tcl interface.
        /*!
         * Tcl commands:
         * - $obj debug\n
         *   Enable debug to standard error.
         * - $obj nodebug\n
         *   Disable debug to standard error (default).
	 * - $obj source
	 *   Link a new voip-source to $obj
	 * - $obj start
	 *   Start traffic generation
	 * - $obj stop
	 *   Stop traffic generation
         */
        virtual int command (int argc, const char*const* argv);

        //! Generate a new talkspurt.
        virtual void next (VoipSource* source, double& talk, double& silence)
			  { abort(); }

protected:
        //! Pointers to the VoIP sources
        VoipSource* source_[2];
	//! Current number of sources. When initialized is equal to two.
	unsigned int nsources_;
	//! A list of <talkspurt,silence> pairs for each speaking party
	std::list< std::pair<double, double> > talkspurtsList_[2];
	//! True if debug is enabled.
	bool debug_;
};

/*
 *  Finite State Machine:
 *
 *   /-------------\        p1        /--------------\
 *  |               | -------------> |                |
 *  | Single Talk A |       p2       | Mutual silence |
 *  |               | <------------- |                |
 *   \-------------/                  \--------------/
 *     / \     |                        / \      |
 *      |      |                         |       |
 *      | 1-p3 | 1-p1                    | 1-p4  | 1-p2
 *      |      |                         |       |
 *      |     \ /                        |      \ /
 *   /-------------\        p3        /--------------\
 *  |               | -------------> |                |
 *  | Double Talk   |       p4       | Single Talk B  |
 *  |               | <------------- |                |
 *   \-------------/                  \--------------/
 *
 *
 *  E[Single Talk]    = 1.004 s
 *  E[Double Talk]    = 0.228 s
 *  E[Mutual Silence] = 0.508 s
 * 
 */
class VoipBidirectionalItuP59 : public VoipBidirectional {
	enum State { TALK_A, TALK_B, DOUBLE_TALK, MUTUAL_SILENCE };

public:
	//! Create a default bi-directional source according to ITU-T P.59.
	VoipBidirectionalItuP59();

	//! Tcl interface.
	//virtual int command (int argc, const char*const* argv);

	//! Generate a new talkspurt.
	//virtual void next (VoipSource* source, double& talk, double& silence) {}

protected:
	//! Timer to schedule talkspurts.
	//TTimer<VoipBidirectionalItuP59> timer_;
	//! Random variable for single talk user A.
	ExponentialRandomVariable singleTalkA_;
	//! Random variable for single talk user B.
	ExponentialRandomVariable singleTalkB_;
	//! Random variable for double talk.
	ExponentialRandomVariable doubleTalk_;
	//! Random variable for mutual silence.
	ExponentialRandomVariable mutualSilence_;
	
	//! Current state.
	State state_;

	//! TODO

};

#define N_STATES 8
class VoipBidirectionalModifiedBrady : public VoipBidirectional {
	enum State { TALK_A, 
		     DOUBLE_TALK_A_INTERRUPTED,
		     DOUBLE_TALK_B_INTERRUPTED,
		     MUTUAL_SILENCE_A_LAST,
		     MUTUAL_SILENCE_B_LAST,
		     TALK_B,
		     SHORT_SILENCE_A_TALKS,
		     SHORT_SILENCE_B_TALKS };

	//! Return the next state of the chain
	State nextState ( double rn );

public:
	VoipBidirectionalModifiedBrady ();

	~VoipBidirectionalModifiedBrady () {
		for (unsigned int i=0; i<N_STATES; i++ ) 
			delete erv_[i];
		delete uniform_;
	}
	
	/*!
	 * Generate a list of talkspurt,silence pairs
	 * for each speaking party
	 */
	void updateTalkspurtsList();

	//! Return the durations of next talkspurt and silence periods
        virtual void next (VoipSource* source, double& talk, double& silence);

protected:
	//! Transition rate matrix
	double Q_[N_STATES][N_STATES];
	        
	//! A random variable for each state
	ExponentialRandomVariable* erv_[N_STATES];

	//! Random variable for state transitions
	UniformRandomVariable* uniform_;

	//! Embedded Markov Chain (Jump Chain)
	double P_[N_STATES][N_STATES];

	//! Current state
	State state_;

	// Talkspurt duration for source A 
	double A_talkspurt_duration_;

	// Talkspurt duration for source B
	double B_talkspurt_duration_;

	// Silence duration for source A 
	double A_silence_duration_;

	// Silence duration for source B
	double B_silence_duration_;
	
};

#endif // __NS2_VOIP_BIDIRECTIONAL_H
