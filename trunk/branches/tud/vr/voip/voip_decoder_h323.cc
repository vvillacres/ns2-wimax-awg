/*
 * TODO: INSERT LICENSE HERE
 */

#include <voip_decoder_h323.h>
#include <stat.h>
#include <math.h>
#include <stdio.h>

#define CODEC_PERIOD 0.020

/**How much time must elapse with lower jitter before jitter
   buffer size is reduced */
#define DECREASE_JITTER_PERIOD 5000 // milliseconds

/* Percentage of current jitter buffer size that constitutes a "genuinely" smaller
jitter */
#define LOWER_JITTER_MAX_PCNT   80

/* Minimum number of packets that constitute a reliable sample for setting a lower
jitter buffer target */
#define DECREASE_JITTER_MIN_PACKETS 50

static class VoipDecoderH323Class : public TclClass
{
        public:
                VoipDecoderH323Class() : TclClass ( "Application/VoipDecoderH323" ) {}
                TclObject* create ( int, const char*const* )
                {
                        return ( new VoipDecoderH323 );
                }
}
class_voip_decoder_optimal;

VoipDecoderH323::VoipDecoderH323 ()
{
	endLost_ = 0;
	endRecv_ = 0;
	endDelay_ = 0;
	codecTime_=0.0;
	// dejitter starts from prebuffering
	prebuffering_=true;
	
	consecutiveEarlyPacketStartTime_ = 0;
	jitterCalcPacketCount_ = 0;
	jitterCalc_ = 0;
	
	minJitterTime_=0.001;
	maxJitterTime_=0.020;
	//! \todo - configure from tcl
	doJitterReductionImmediately_=true;
	
	currentJitterTime_=minJitterTime_;
	targetJitterTime_ = currentJitterTime_;
}

int
VoipDecoderH323::command ( int argc, const char*const* argv )
{
	if ( argc == 3 && strcmp ( argv[1], "min-jitter" ) == 0 ) {
		minJitterTime_ = atof(argv[2]);
		return TCL_OK;
	} else if ( argc == 3 && strcmp ( argv[1], "max-jitter" ) == 0 ) {
		maxJitterTime_ = atof(argv[2]);
		return TCL_OK;
	}
        return VoipDecoder::command ( argc, argv );
}

void
VoipDecoderH323::collectFrame ( VoipFrame& frame )
{
  
        // Access/add the current talkspurt data.
        TalkData& talk = talkData_[frame.talkspurt_];
	
        // If this is the first frame of this talkspurt, then update data.
        if ( talk.nframes_ == 0 )
        {
		talk.statActive_ = Stat::active();
                talk.nframes_ = frame.nframes_;
                talk.timestamp_.resize ( talk.nframes_, -1.0 );
		talk.tick_.resize ( talk.nframes_, -1.0 );
        }

        // Update the number of received frames.
        ++talk.received_;

        // record timestamp and arrival time
        talk.timestamp_[frame.frame_] = frame.timestamp_;
	talk.tick_[frame.frame_] =NOW;
        

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
VoipDecoderH323::collectTalkspurt ( unsigned int nframes )
{
	  // Access the current talkspurt data.
	  TalkData& talk = talkData_[nframes];

	  if ( ! talk.statActive_ ) return;

	  double per =0.0;
	  double delay=-1.0;
	  // set the codec start of operation at first frame arrival
	  // all actions that would have been performed while dejitter buffer
	  // was empty are useless
	  codecTime_=talk.tick_[0];
	  for ( ; (codecTime_<=talk.tick_[talk.received_-1]); codecTime_+=CODEC_PERIOD )
	  {	
		
		for (unsigned int i =0 ; i<talk.nframes_;++i)
		{
		  if (talk.tick_[i]<=codecTime_)  
		  {
		    talk.newest_=i;
		  }  
		  else break;
		}
		// buffer empty condition
		if (talk.newest_<0 ) 
		{
		    prebuffering_=true;
		    currentJitterTime_=targetJitterTime_;
		    continue;
		}
		
		double oldestTimestamp = talk.timestamp_[talk.oldest_];
		double newestTimestamp = talk.timestamp_[talk.newest_];

// 		 If there is an opportunity (due to silence in the buffer) to implement a desired 
// 		reduction in the size of the jitter buffer, effect it 

		if (targetJitterTime_ < currentJitterTime_ && (newestTimestamp - oldestTimestamp) < currentJitterTime_ ) 
		{
		    currentJitterTime_ = ( targetJitterTime_ > (newestTimestamp - oldestTimestamp)) ?
                          targetJitterTime_ : (newestTimestamp - oldestTimestamp);
		
		}
		
		/* See if time for this packet, if our oldest frame is older than the
		required age, then use it. */
		
		if (prebuffering_) 
		{
		  // Reset jitter baseline
		  talk.lastWrite_=-1;
		  
		  // If oldest frame has not been in the buffer long enough, don't return anything yet
		    if ((codecTime_ - talk.tick_[talk.oldest_])  < currentJitterTime_ / 2) 
		    {
		      // go ahead in codec "for" time sequence
		      continue;
		    }
		    
		    //else deactivate prebuffering
		    prebuffering_=false;
		}
		
		// Detatch oldest packet from the list, put into parking space
	      
		talk.currentWrite_ = talk.oldest_;
		if (talk.oldest_ < talk.newest_)  talk.oldest_ ++;
		
		// Calculate the jitter contribution of this frame
		// - don't count if start of a talk burst
		if (talk.currentWrite_<=0) 
		{
		  talk.lastWrite_ = -1;  
		}
		
		if (talk.lastWrite_ > -1) 
		{
		  double thisJitter = 0;
		
		  if (talk.timestamp_[talk.currentWrite_]  < talk.timestamp_[talk.lastWrite_] ) 
		  {
		    //Not too sure how to handle this situation...
		    thisJitter = 0;
		  }
		  else if (talk.tick_[talk.currentWrite_]  < talk.tick_[talk.lastWrite_]) 
		  {
		    //Not too sure how to handle this situation either!
		    thisJitter = 0;
		  } else {  
		      thisJitter = (talk.tick_[talk.currentWrite_] - talk.tick_[talk.lastWrite_] + 
		      talk.timestamp_[talk.lastWrite_] - talk.timestamp_[talk.currentWrite_]);
		  }
		  
		  if (thisJitter < 0) thisJitter *=(-1);
		  thisJitter *=2; //currentJitterTime needs to be at least TWICE the maximum jitter

		  if (thisJitter >  currentJitterTime_ * LOWER_JITTER_MAX_PCNT / 100) 
		  {
		    targetJitterTime_ = currentJitterTime_;
	    
		    consecutiveEarlyPacketStartTime_ = codecTime_;
		    jitterCalcPacketCount_ = 0;
		    jitterCalc_ = 0;
		  } else {
		    if ( thisJitter >  jitterCalc_ )
			  jitterCalc_ = thisJitter;
			  jitterCalcPacketCount_++;

		      //If it's bigger than the target we're currently trying to set, adapt that target.
		      //Note: this will never make targetJitterTime larger than currentJitterTime due to
		      //previous if condition
		      if (thisJitter >  targetJitterTime_ * LOWER_JITTER_MAX_PCNT / 100) 
		      {
			targetJitterTime_ = thisJitter * 100 / LOWER_JITTER_MAX_PCNT;
		      } 
		  }
	      }
	      
	      // writing here frame to codec, if it is the first one, record here delay
	      
	      talk.lastWrite_=talk.currentWrite_;
	      
	      if (talk.oldest_ == talk.newest_)
	      {  // buffer play end.
		  talk.newest_ = -1;
		  talk.oldest_ = -1;
	      }   
	      else {
      // 	    talk.oldest_ ->prev = NULL;
		// If exceeded current jitter buffer time delay
		if ((talk.timestamp_[talk.newest_] - talk.timestamp_[talk.currentWrite_]) > currentJitterTime_)
		{
		    consecutiveEarlyPacketStartTime_ = codecTime_;
		    jitterCalcPacketCount_ = 0;
		    jitterCalc_ = 0;
		    talk.lastWrite_ = -1;
		    
		    // If we haven't yet written a frame, we get one free overrun
		    if (!talk.doneFirstWrite_) 
		    {
	      
			while ((talk.timestamp_[talk.newest_] - talk.timestamp_[talk.currentWrite_]) > currentJitterTime_) 
			{
			    // remove current Write frame
			    talk.currentWrite_ = talk.oldest_;
			    if (talk.oldest_<=talk.newest_)
			    {  
				talk.oldest_++;
			      // increasing packet error rate due to frame deletion.
			      per++;
			    }
			    if (talk.oldest_>talk.newest_ ) 
			    {
			      talk.oldest_= -1;
			      talk.newest_= -1;
			      break;
			    }
		      }
	      
			talk.doneFirstWrite_ = TRUE;
			if (delay<0) 
			{
			    delay=codecTime_-talk.timestamp_[talk.currentWrite_] ;
			}
			// continuing the codec main loop
			continue;
		}
		
	 // See if exceeded maximum jitter buffer time delay, waste them if so
	  while ((talk.timestamp_[talk.newest_] - talk.timestamp_[talk.currentWrite_]) > maxJitterTime_) 
	  {
	      currentJitterTime_ = maxJitterTime_;
	      // remove current Write frame
	      talk.currentWrite_ = talk.oldest_;
	      if (talk.oldest_<=talk.newest_)
	      {
		talk.oldest_++; 

	      // increasing packet error rate due to frame deletion.
	      per++;
	      }
	      
	      if (talk.oldest_>talk.newest_ ) 
	      {
		talk.oldest_= -1;
		talk.newest_= -1;
		break;
	      }
	  }
	  
	  
	  // Now change the jitter time to cope with the new size
    // unless already set to maxJitterTime
      if (newestTimestamp - talk.timestamp_[talk.currentWrite_] > currentJitterTime_) 
          currentJitterTime_ = newestTimestamp - talk.timestamp_[talk.currentWrite_];

      targetJitterTime_ = currentJitterTime_;
     }
  }
  
  
  if (( (codecTime_ - consecutiveEarlyPacketStartTime_) > DECREASE_JITTER_PERIOD) &&
       (jitterCalcPacketCount_ >= DECREASE_JITTER_MIN_PACKETS))
    {
      jitterCalc_ = jitterCalc_ * 100 / LOWER_JITTER_MAX_PCNT;
      if (jitterCalc_ < targetJitterTime_ / 2) jitterCalc_ = targetJitterTime_ / 2;
      if (jitterCalc_ < minJitterTime_) jitterCalc_ = minJitterTime_;
      targetJitterTime_ = jitterCalc_;
      jitterCalc_ = 0;
      jitterCalcPacketCount_ = 0;
      consecutiveEarlyPacketStartTime_ = codecTime_;
    }


    /* If using immediate jitter reduction (rather than waiting for silence opportunities)
  then trash oldest frames as necessary to reduce the size of the jitter buffer */
  if (targetJitterTime_ < currentJitterTime_ &&
      doJitterReductionImmediately_ &&
      talk.newest_ > -1) {
    while ((talk.timestamp_[talk.newest_] - talk.timestamp_[talk.currentWrite_]) > targetJitterTime_)
    {
      // Throw away the newest entries
      if (talk.newest_>=talk.oldest_)
      {
	  talk.newest_--; 
	  talk.received_--; 
	  
	      // increasing packet error rate due to frame deletion.
	      per++;
      }      
      
      
      // Reset jitter calculation baseline
      talk.lastWrite_ = -1;
      

      
      if (talk.newest_ < 0) 
      {
          talk.oldest_ = -1;
	  talk.received_= 0;
          break;
      }
    }

    currentJitterTime_ = targetJitterTime_;
  }

  talk.doneFirstWrite_ = TRUE;
  if (delay<0) 
  {
      delay=codecTime_- talk.timestamp_[talk.currentWrite_];
  }
	 
}// end codec loop
	 
	 per/=talk.nframes_;
	 
	 double mos = eModel ( delay , per );
	 
	 Stat::put ("voip_playout_talkspourt_delay", id_, delay);
	 Stat::put ("voip_playout_talkspourt_per", id_, per );
	 Stat::put ("voip_mos_distribution", id_, mos);
	  Stat::put ("voip_satisfaction", id_,( mos >= mosThreshold_ ) ? 1.0 : 0);

}

void
VoipDecoderH323::collectEnd ()
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
		Stat::put("voip_mos_converstion", id_, endMos);
	}
        // Collect Gloabl metrics
        // Call the base class collect End function.
        VoipDecoder::collectEnd();
}
