/*
 * TODO: INSERT LICENSE HERE
 */

#ifndef __NS2_VOIP_DECODER_80216M_H
#define __NS2_VOIP_DECODER_80216M_H

#include <voip_decoder.h>
#include <map>
#include <vector>

class VoipDecoder80216m : public VoipDecoder {
public:
	//! Create an empty VoIP decoder.
	VoipDecoder80216m ();

	//! Tcl interface.
	int command (int argc, const char*const* argv);

protected:
	//! Collect statistics after playout buffering frame by frame.
	virtual void collectFrame (VoipFrame& frame);
	//! Collect statistics after playout buffering talkspurt by talkspurt.
	//void collectTalkspurt (unsigned int nframes);
	//! Collect statistics after playout buffering at the end of simulation.
	void collectEnd ();
};

#endif // __NS2_VOIP_DECODER_80216M_H
