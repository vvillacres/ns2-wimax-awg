/*
 * TODO: INSERT LICENSE HERE
 */

#include <voip_decoder_atzori.h>
#include <stat.h>
#include <math.h>
#include <stdio.h>

static class VoipDecoderAtzoriClass : public TclClass
{
        public:
                VoipDecoderAtzoriClass() : TclClass ( "Application/VoipDecoderAtzori" ) {}
                TclObject* create ( int, const char*const* )
                {
                        return ( new VoipDecoderAtzori );
                }
}
class_voip_decoder_atzori;

VoipDecoderAtzori::VoipDecoderAtzori ()
{
	endLost_ = 0;
	endRecv_ = 0;
	endDelay_ = 0;
	prevDelay_=0.0;
}

int
VoipDecoderAtzori::command ( int argc, const char*const* argv )
{
        return VoipDecoder::command ( argc, argv );
}

void
VoipDecoderAtzori::collectFrame ( VoipFrame& frame )
{
        // Access/add the current talkspurt data.
        TalkData& talk = talkData_[frame.talkspurt_];

        // If this is the first frame of this talkspurt, then update data.
        if ( talk.nframes_ == 0 )
        {
		talk.statActive_ = Stat::active();
                talk.nframes_ = frame.nframes_;
                talk.delay_.resize ( talk.nframes_, -1.0 );
        }

        // Update the number of received frames.
        if ( talk.delay_[frame.frame_] < 0 ) ++talk.received_;

        // Add/update the delay sample for the current frame.
        double delay = NOW - frame.timestamp_;
        if ( talk.delay_[frame.frame_] < 0 || talk.delay_[frame.frame_] > delay )
        {
                talk.delay_[frame.frame_] = delay;
        }

        if ( talk.nframes_ == talk.received_ )
        {
                // If the last frame of this talkspurt has been added, then
                // compute the measures for this talkspurt.
                collectTalkspurt ( frame.talkspurt_ );

                // Remove this talkspurt.
                talkData_.erase ( frame.talkspurt_ );
        }
}

void
VoipDecoderAtzori::collectTalkspurt ( unsigned int nframes )
{
	  // Access the current talkspurt data.
	  TalkData& talk = talkData_[nframes];

	  if ( ! talk.statActive_ ) return;

	  double talkPer =0.0;
	  double talkReceived =0.0;
	  
	  for ( unsigned int i = 0 ; i < talk.nframes_ ; i++ ) 
	  {
			if ( talk.delay_[i] < 0 || talk.delay_[i] > prevDelay_ ) {
				++endLost_;
				++ talkPer;
			}
			else {
			      endDelay_ += talk.delay_[i] + prevDelay_;
			      ++endRecv_;
			      ++ talkReceived;
		      }
	  }
	  
	  talkPer/=talk.nframes_;
	  double talkMos = eModel ( prevDelay_, talkPer);
	    
	  Stat::put ("voip_playout_talkspurt_delay", id_, prevDelay_);
	  Stat::put ("voip_playout_talkspurt_per", id_, talkPer );
	  Stat::put ("voip_mos_distribution", id_, talkMos);
	  Stat::put ("voip_satisfaction", id_,( talkMos >= mosThreshold_ ) ? 1.0 : 0);

	  
	  double bestMos  = -1.0;
	  double bestDelay = 0.0;

	  // Get the optimal value of the delay for this talkspurt.
	  for ( unsigned int i = 0 ; i < talk.nframes_ ; i++ )
	  {
				 double per   = 0;
				 double delay = talk.delay_[i];
				 if ( delay < 0 ) continue;

				 // Compute the packet loss with this delay.
				 for ( unsigned int j = 0 ; j < talk.nframes_ ; j++ )
				 {
					 if ( talk.delay_[j] < 0 || talk.delay_[j] > delay ) {
						 ++per;
					 }
				 }
				 per /= talk.nframes_;

				 double mos = eModel ( delay, per );

				 if ( bestMos < 0 || mos > bestMos )
				 {
							bestMos = mos;	
							bestDelay = delay;
				 }
	  }
	  //storing best playout delay for next talkspurt
	  prevDelay_=bestDelay;
}

void
VoipDecoderAtzori::collectEnd ()
{
        unsigned int max = 0;
        std::map<unsigned int, TalkData>::iterator it;
        for ( it = talkData_.begin () ; it != talkData_.end () ; ++it )
        {
                if ( it == talkData_.begin() || it->first > max )
                        max = it->first;
        }
        talkData_.erase ( max );
        for ( it = talkData_.begin () ; it != talkData_.end () ; ++it )
        {
                collectTalkspurt ( it->first );
        }

		  // :TODO: add comment
		  if ( peerId_ == -1 || id_ > peerId_ ) {
			  if ( Stat::get ("voip_satisfaction", id_) >=
					    cellOutageMosThreshold_ &&
				    ( peerId_ == -1 || Stat::get ("voip_satisfaction", peerId_) >=
					    cellOutageMosThreshold_ ) )
				  Stat::put ("voip_cell_satisfaction", cellId_, 1.0);
			  else
				  Stat::put ("voip_cell_satisfaction", cellId_, 0);
		  }

	// Calculate MOS based on the entire conversation delay and loss
	if ( endRecv_ != 0 ) {
		endDelay_ /= endRecv_;
		double endPer = ((double) endLost_) / (endLost_ + endRecv_ );
		double endMos = eModel ( endDelay_ , endPer );
		Stat::put("voip_mos_conversation", id_, endMos);
	}
        // Collect Gloabl metrics
        // Call the base class collect End function.
        VoipDecoder::collectEnd();
}
