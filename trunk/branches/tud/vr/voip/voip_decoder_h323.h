/*
 * TODO: INSERT LICENSE HERE
 */

#ifndef __NS2_VOIP_DECODER_OPTIMAL_H
#define __NS2_VOIP_DECODER_OPTIMAL_H

#include <voip_decoder.h>
#include <map>
#include <vector>

class VoipDecoderH323 : public VoipDecoder {
public:
	//! Create an empty VoIP decoder.
	VoipDecoderH323 ();

	//! Tcl interface.
	int command (int argc, const char*const* argv);

protected:
	//! Collect statistics after playout buffering frame by frame.
	virtual void collectFrame (VoipFrame& frame);
	//! Collect statistics after playout buffering talkspurt by talkspurt.
	void collectTalkspurt (unsigned int nframes);
	//! Collect statistics after playout buffering at the end of simulation.
	void collectEnd ();

protected:

	/*
	 * Used for MOS calculation over the entire conversation
	 * rather than for each talkspurt.
	 */
	//! Counter for lost packets
	unsigned long endLost_;
	//! Counter for received packets
	unsigned long endRecv_;
	//! per-conversation packet delay
	double endDelay_;
	//! decoder time- used in collect talkspurt
	double codecTime_;
	
	//! prebuffering flag
	bool prebuffering_;
	
	//! \todo
	double minJitterTime_;
	//! \todo 
	double maxJitterTime_;
	//! \todo
	double currentJitterTime_;
	//! \todo
	double targetJitterTime_;
	//! \todo
	double consecutiveEarlyPacketStartTime_;
	//! \todo
	int jitterCalcPacketCount_;
	//! \todo
	double jitterCalc_;
	
	//! flag to enable immediate jitter reduction (otherwise jitter reduction is performed on-silence)
	bool doJitterReductionImmediately_;
	
	//! Talkspurt data structure.
	struct TalkData {
		//! Timestamps in seconds.
		std::vector<double> timestamp_;
		//! arrival times in seconds.
		std::vector<double> tick_;
		//! flag to signal first write of the talk
		bool doneFirstWrite_;
		//! current frame to send to the decoder
		int currentWrite_;
		//! last played out frame
		int lastWrite_;
		//! last received frame 
		int lastRead_;
		//! oldest frame still to be played out
		int oldest_;
		//! newest frame in dejitter buffer
		int newest_;
		//! Number of frames in this talkspurt.
		unsigned int nframes_;
		//! Number of frames received so far.
		unsigned int received_;
		//! Statistics collection flag
		bool statActive_;

		//! Create an empty talkspurt data structure.
		TalkData () {
			doneFirstWrite_ =false;
			nframes_    = 0;
			received_   = 0;
			currentWrite_ =-1;
			lastRead_   = -1;
			lastWrite_  = -1;
			oldest_	    = 0;
			newest_     = -1;
			statActive_ = false;
		}
	};

	//! One talkspurt data structure for each talkspurt ID.
	std::map<unsigned int, TalkData> talkData_;
};

#endif // __NS2_VOIP_DECODER_OPTIMAL_H
