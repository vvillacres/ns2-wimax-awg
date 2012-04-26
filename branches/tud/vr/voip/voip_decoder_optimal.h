/*
 * TODO: INSERT LICENSE HERE
 */

#ifndef __NS2_VOIP_DECODER_OPTIMAL_H
#define __NS2_VOIP_DECODER_OPTIMAL_H

#include <voip_decoder.h>
#include <map>
#include <vector>

class VoipDecoderOptimal : public VoipDecoder {
public:
	//! Create an empty VoIP decoder.
	VoipDecoderOptimal ();

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

	//! Talkspurt data structure.
	struct TalkData {
		//! Delay samples, in seconds.
		std::vector<double> delay_;
		//! Number of frames in this talkspurt.
		unsigned int nframes_;
		//! Number of frames received so far.
		unsigned int received_;
		//! Statistics collection flag
		bool statActive_;

		//! Create an empty talkspurt data structure.
		TalkData () {
			nframes_    = 0;
			received_   = 0;
			statActive_ = false;
		}
	};

	//! One talkspurt data structure for each talkspurt ID.
	std::map<unsigned int, TalkData> talkData_;
};

#endif // __NS2_VOIP_DECODER_OPTIMAL_H
