/*
 * TODO: INSERT LICENSE HERE
 */

#include <voip_bidirectional.h>
#include <voip_source.h>
#include <ranvar.h>
#include <stdio.h>
#include <stat.h>

static class VoipBidirectionalClass : public TclClass {
public:
   VoipBidirectionalClass() : TclClass("VoipBidirectional") {}
   TclObject* create(int, const char*const*) {
      return (new VoipBidirectional);
   }
} class_voip_bidirectional;

VoipBidirectional::VoipBidirectional ()
{
	source_[0]   = 0;
	source_[1]   = 0;
	nsources_    = 0;
	debug_       = false;
	talkspurtsList_[0].clear();		
	talkspurtsList_[1].clear();		
}

int
VoipBidirectional::command (int argc, const char*const* argv)
{
	if ( argc == 2  && strcmp (argv[1], "debug") == 0 ) {
		debug_ = true;
		return TCL_OK;

	} else if ( argc == 2  && strcmp (argv[1], "nodebug") == 0 ) {
		debug_ = false;
		return TCL_OK;

	} else if ( argc == 2 && strcmp (argv[1], "start") == 0 ) {
		if ( ! initialized () ) {
			fprintf (stderr, "VoIP bidirectional not initialized");
			return TCL_ERROR;
		} 
		if ( nsources_ == 0 ) {
			fprintf (stderr, "There's nothing to start\n");
			return TCL_ERROR;
		}
		if ( source_[0] ) source_[0]->handle ();
		if ( source_[1] ) source_[1]->handle ();
		return TCL_OK;
	} else if ( argc == 2 && strcmp (argv[1], "stop") == 0 ) {
		if ( source_[0] ) source_[0]->stop ();
		if ( source_[1] ) source_[1]->stop ();
		return TCL_OK;

	} else if ( argc == 3 && strcmp (argv[1], "source") == 0 ) {
		if ( nsources_ == 2 ) {
			fprintf (stderr, "You cannot bound more than TWO sources to this object\n");
			return TCL_ERROR;
		}
		source_[nsources_] = (VoipSource*) TclObject::lookup (argv[2]);
		++nsources_;
		return TCL_OK;

	}
	return TCL_ERROR;
}

static class VoipBidirectionalItuP59Class : public TclClass {
public:
   VoipBidirectionalItuP59Class() : TclClass("VoipBidirectionalItuP59") {}
   TclObject* create(int, const char*const*) {
      return (new VoipBidirectionalItuP59);
   }
} class_voip_bidirectional_itu_p59;

VoipBidirectionalItuP59::VoipBidirectionalItuP59 () :
	//timer_ (this),
	singleTalkA_   (1.004),
	singleTalkB_   (1.004),
	doubleTalk_    (0.228),
	mutualSilence_ (0.508)
{
	// Nihil.
}

static class VoipBidirectionalModifiedBradyClass : public TclClass {
public:
	VoipBidirectionalModifiedBradyClass() : TclClass("VoipBidirectionalModifiedBrady") {}
	TclObject* create(int, const char*const*) {
		return (new VoipBidirectionalModifiedBrady);
	}
} class_voip_bidirectional_modified_brady;

VoipBidirectionalModifiedBrady::VoipBidirectionalModifiedBrady ()
{
	// Fill transition rate matrix of zero
	for (int i=0; i<N_STATES; i++ ) {
		for(int j=0; j<N_STATES; j++) {
			Q_[i][j] = 0.0;
		}
	}
	// Set transition rates
	Q_[0][3] = Q_[5][4] =  0.83305;
	Q_[0][6] = Q_[5][7] =  5.48900;
	Q_[2][0] = Q_[1][5] =  2.15720;
	Q_[1][0] = Q_[2][5] =  2.32450;
	Q_[6][0] = Q_[7][5] = 27.62000;
	Q_[3][0] = Q_[4][5] =  2.22220;
	Q_[4][0] = Q_[3][5] =  1.04380;
	Q_[0][1] = Q_[5][2] = Q_[6][1] = Q_[7][2] = 0.27853;

	double sum;
	for (int i=0; i<N_STATES; i++) {
		sum = 0.0;
		// Calculate
		for (int j=0; j<N_STATES; j++)
			sum += Q_[i][j];
		// Create a Random Variable for each state
		erv_[i] = new ExponentialRandomVariable ( 1/(sum) );
		// Fill the Embedded Markov Chain (Jump Chain)
		for (int j=0; j<N_STATES; j++) 
			P_[i][j] = Q_[i][j]/sum;
	}
	// Start from the state in which A and B are silent and A spoke last
	state_ = MUTUAL_SILENCE_A_LAST;

	// Initialize the random variable used to determine state transitions
	uniform_ = new UniformRandomVariable ( 0, 1 );

	A_talkspurt_duration_ = 0.0;
	B_talkspurt_duration_ = 0.0;
	A_silence_duration_ = 0.0;
	B_silence_duration_ = 0.0;
}

VoipBidirectionalModifiedBrady::State
VoipBidirectionalModifiedBrady::nextState ( double rn ) {
	
	double sum = 0;
	State newState = TALK_A;

	for (int j=0; j<N_STATES; j++) {
		sum += P_[state_][j];
		if ( rn <= sum ) {
			newState = (State) j;
			break;
		}
	}

	return newState;
}

void 
VoipBidirectionalModifiedBrady::next (VoipSource* source, double& talk, double& silence) 
{
	int sourceNum;
	if ( source == source_[0] ) 
		sourceNum = 0;
	else if ( source == source_[1] )
		sourceNum = 1;
	else {
		fprintf ( stderr, "%.9f VoipBidirectional::next() Wrong source!\n", NOW);
		abort();
	}

	// If the list of talkspurts for this source is empty, update it
	if ( talkspurtsList_[sourceNum].empty() ) 
		updateTalkspurtsList();

	// Now at least one pair is present in the list, 
	// then returns its values and pop it
	std::pair< double, double >& tmp = talkspurtsList_[sourceNum].front();
	talk = tmp.first;
	silence = tmp.second;
	if ( debug_ )
		fprintf ( stderr, "%.9f VoipBidirectional::next() " 
				  "source=%u, talk=%f, silence=%f!\n", 
				  NOW, sourceNum, talk, silence);
	talkspurtsList_[sourceNum].pop_front();
}

void 
VoipBidirectionalModifiedBrady::updateTalkspurtsList() 
{
	// Exit if not initialized	
	if ( ! initialized () ) {
		fprintf (stderr, "VoIP bidirectional not initialized");
		abort ();
	}

	if ( debug_ ) 
		fprintf ( stderr, "%.9f updateTalkspurtList()\n", NOW );

	// true if party A has at least a <talk,silence> pair
	bool flag_A = false;
	// true if party B has at least a <talk,silence> pair
	bool flag_B = false;

	do {
		// Get duration of current status
		double state_duration = erv_[state_]->value();
		// Calculate the next state using Jump Matrix
		State next_state = nextState( uniform_->value() );

		switch ( state_ ) {
			// Modified Brady model's state 1
			case TALK_A:
				// If A begin a talkspurt then it must be 
				// long at least 15ms
				if ( next_state == MUTUAL_SILENCE_A_LAST )
					state_duration += 0.015;
				break;
			// Modified Brady model's state 6
			case TALK_B:
				// If B begin a talkspurt then it must be
				// long at least 15ms
				if ( next_state == MUTUAL_SILENCE_B_LAST ) 
					state_duration += 0.015;
				break;
			// Modified Brady model's state 7
			case SHORT_SILENCE_A_TALKS:
			// Modified Brady model's state 8
			case SHORT_SILENCE_B_TALKS:
				break;
			// Modified Brady model's state 4
			case MUTUAL_SILENCE_A_LAST:
			// Modified Brady model's state 5
			case MUTUAL_SILENCE_B_LAST:
				// Mutual silence duration is at least 200ms
				state_duration += 0.2;
				break;
			// Modified Brady model's state 2
			case DOUBLE_TALK_A_INTERRUPTED:
				// If B begin a talkspurt (interrupting A) then
				// it must be long at least 15ms
				if ( next_state == TALK_A ) 
					state_duration += 0.015;
				break;
			// Modified Brady model's state 3
			case DOUBLE_TALK_B_INTERRUPTED:
				// If A begin a talkspurt (interrupting A) then
				// it must be long at least 15ms
				if ( next_state == TALK_B ) 
					state_duration += 0.015;
				break;
			default:
			       	fprintf ( stderr, "VoipBidirectional: cannot be here!\n" );
				abort();
		}

		if ( debug_ ) fprintf (stderr, "%.9f new voip-bidirectional state (%s), duration %f\n",
				NOW, 
				(state_ == TALK_A)? "A talk":
				(state_ == TALK_B)? "B talk":
				(state_ == MUTUAL_SILENCE_A_LAST)? "Mutual silence, A spoke last":
				(state_ == MUTUAL_SILENCE_B_LAST)? "Mutual silence, B spoke last":
				(state_ == SHORT_SILENCE_A_TALKS)? "Short silence while A talks":
				(state_ == SHORT_SILENCE_B_TALKS)? "Short silence while B talks":
				(state_ == DOUBLE_TALK_A_INTERRUPTED)? "Double talk, B interrupt A":
				/*(state_ == DOUBLE_TALK_B_INTERRUPTED)?*/ "Double talk, A interrupt B",
				state_duration);

		// Construct a lists of talkspurts for A and B 
		// and collect some statistics useful for debug (if enabled)
		if ( state_ == TALK_A || 			// 1
		     state_ == DOUBLE_TALK_A_INTERRUPTED || 	// 2
		     state_ == DOUBLE_TALK_B_INTERRUPTED) {	// 3
			// Party A is speaking
			if ( A_silence_duration_ > 0.0 ) {
				// End of a silence period (for A) detected
				// Add a new <talkspurt,silence> pair for A to the list
				if ( debug_)
					fprintf( stderr, "%.9f New <Talkspurt,Silence>="
							 "<%.9f,%.9f> pair for A\n",
							 NOW, A_talkspurt_duration_, 
							 A_silence_duration_);
				talkspurtsList_[0].push_back( std::make_pair( A_talkspurt_duration_, 
									 A_silence_duration_ ) );
				flag_A = true;
				A_silence_duration_ = 0.0;
				A_talkspurt_duration_ = 0.0;
			}
 			A_talkspurt_duration_ += state_duration;
		}
		else {						// 4,5,6,7,8
			// Party A is silent
			A_silence_duration_ += state_duration;
		}

		if ( state_ == TALK_B || 			// 6
		     state_ == DOUBLE_TALK_A_INTERRUPTED || 	// 2
		     state_ == DOUBLE_TALK_B_INTERRUPTED) {	// 3
			// Party B is speaking
			if ( B_silence_duration_ > 0.0 ) {
				// End of a silence period (for A) detected
				// Add a new <talkspurt,silence> pair for B to the list
				if ( debug_ )
					fprintf( stderr, "%.9f New <Talkspurt,Silence>="
					 		 "<%.9f,%.9f> pair for B\n",
							 NOW, B_talkspurt_duration_, 
							 B_silence_duration_);
				// Do not add a pair to the second party list if only one 
				// source is attached to the Bidirectional object.
				if ( nsources_ == 2 ) 
					talkspurtsList_[1].push_back( std::make_pair( B_talkspurt_duration_, 
										 B_silence_duration_ ) );
				flag_B = true;
				B_silence_duration_ = 0.0;
				B_talkspurt_duration_ = 0.0;
			}
			B_talkspurt_duration_ += state_duration;
		}
		else {						// 1,4,5,7,8
			// Party B is silent
			B_silence_duration_ += state_duration;
		}

	if ( debug_ ) 
		Stat::put ( "voip_state_duration" , state_, state_duration );

		// Evolve to the new state
		state_ = next_state;

	} while ( ! (flag_A && flag_B) );
}

/*
void
VoipBidirectional::handle ()
{
	if ( ! initialized () ) {
		fprintf (stderr, "VoIP bidirectional not initialized");
		abort ();
	}

	double talk;
	double silence;

	// If there is a bidirectional module bound to this object
	// then get the next talkspurt and silence periods from it.
	if ( bidirectional_ ) {
		bidirectional_->next (this, talk, silence);

	// Otherwise, draw the talkspurt and silence period duration randomly
	// from the random variables depending on the source model.
	} else {
		talk = talk_->value();
		silence = silence_->value();
	}

	if ( debug_ ) fprintf (stderr, "%.9f new talkspurt %f, %f (ID %d)\n",
		NOW, talk, silence, count_);

	// Tell the encoder that a new talkspurt is to be generated.
	encoder_->talkspurt (count_, talk);

	// Update the talkspurt counter.
	++count_;

	// Reschedule this function to be called after the silence period.
	timer_.start (talk + silence);
}
*/
