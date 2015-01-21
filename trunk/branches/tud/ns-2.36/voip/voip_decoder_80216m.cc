/*
 * TODO: INSERT LICENSE HERE
 */

#include <voip_decoder_80216m.h>
#include <stat.h>
#include <math.h>
#include <stdio.h>

static class VoipDecoder80216mClass : public TclClass
{
        public:
                VoipDecoder80216mClass() : TclClass ( "Application/VoipDecoder80216m" ) {}
                TclObject* create ( int, const char*const* )
                {
                        return ( new VoipDecoder80216m );
                }
}
class_voip_decoder_80216m;

VoipDecoder80216m::VoipDecoder80216m ()
{}

int
VoipDecoder80216m::command ( int argc, const char*const* argv )
{
        return VoipDecoder::command ( argc, argv );
}

void
VoipDecoder80216m::collectFrame ( VoipFrame& frame )
{
	// If statistic collection is enabled 
	// Count the number of received frames
	if ( frame.statActive_ ) {
		Stat::put("voip_frames_recv",id_,1);
	}
}

void
VoipDecoder80216m::collectEnd ()
{
	// calculate cell outage
	
	double sent = Stat::get("voip_frames_sent",id_);
	double recv = Stat::get("voip_frames_recv",id_);
	
	if ( ! sent ) return;
	double loss = (sent-recv)/sent;

	// Is bidirectional enabled ?
	if ( peerId_ != -1 ) {
		// Only one speaking party collect statistics about conversation
		if ( id_ < peerId_ ) return;
		// If bidirectional is active then calculate cell outage
		// based on conversations instead that on flows
		double sent_peer = Stat::get("voip_frames_sent",peerId_);
		double recv_peer = Stat::get("voip_frames_recv",peerId_);

		if ( ! sent_peer ) return;
		double loss_peer = (sent_peer-recv_peer)/sent_peer;

		if ( ( loss > cellOutageLossThreshold_ ) || 
		     ( loss_peer > cellOutageLossThreshold_ ) )  {
			Stat::put("voip_cell_outage", cellId_ , 1);
		} else {
			Stat::put("voip_cell_outage", cellId_ , 0);
		}

	} else {
		if ( loss > cellOutageLossThreshold_ ) {
			Stat::put("voip_cell_outage", cellId_ , 1);
		} else {
			Stat::put("voip_cell_outage", cellId_ , 0);
		}
	}
}

