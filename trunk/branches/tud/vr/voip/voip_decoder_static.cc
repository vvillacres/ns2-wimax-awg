/*
 * TODO: INSERT LICENSE HERE
 */

#include <voip_decoder_static.h>
#include <stat.h>
#include <math.h>
#include <stdio.h>

static class VoipDecoderStaticClass : public TclClass {
public:
        VoipDecoderStaticClass() : TclClass("Application/VoipDecoderStatic") {}
        TclObject* create(int, const char*const*) {
                return (new VoipDecoderStatic);
        }
} class_voip_decoder_static;

VoipDecoderStatic::VoipDecoderStatic (): timer_(this)
{
        bufferOccupancy_ = 0;
        bufferSize_ = 10;
        initialDelay_ = 0.005;
        playoutRate_ = 0.020;
        talkLength_ = 0;
        nextFrame_ = 0;
        currentTalk_ = 0;
}

void
VoipDecoderStatic::collectFrame (VoipFrame& frame)
{
        if (debug_) {
                fprintf(stderr, "%.7f Received Frame [%d, %d of %d]\n",NOW, frame.talkspurt_, frame.frame_, frame.nframes_);
                fprintf(stderr, "\tBuffer Occupancy: %d / %d\n", bufferOccupancy_, bufferSize_);
        }
        // Check if the frame belongs to an old talkspurt
        if ( frame.talkspurt_ < currentTalk_) {
                // Drop the frame
                //Stat::put(VOIP_BUFFER_OUT_OF_TIME_DROP, id_, 1.0);
                return;
        }
        // Check if there is space in the buffer and add the VoIP frames.
        if (bufferOccupancy_ == bufferSize_) {
                // A new frame is arrived and there is no free space in the
                // buffer. 
                // Check if the frame should be added or dropped, if added
                // drop the first arrived frame in the buffer
                if ( frame.talkspurt_ == currentTalk_ && frame.frame_ < buffer_.top().frame_  ) {
                        // Ignore the frame, is too old.
                        //Stat::put(VOIP_BUFFER_OUT_OF_TIME_DROP, id_, 1.0);
                        return;
                }
                // Drop a frame from the head of the buffer 
                buffer_.pop();
                // Now store the packet into the buffer
                // The bufferOccupancy_ remains the same
                buffer_.push(frame);
                // Note that, in this case nextFrame_ is manually set.
                nextFrame_ = buffer_.top().frame_;
                // Check if the next frame belongs to a new talkspurt
                if (currentTalk_ < buffer_.top().talkspurt_) {
                        collectTalkspurt(talkLength_);
                        delaySum_ = 0;
                        delayNum_ = 0;
                        currentTalk_ = buffer_.top().talkspurt_;
                        talkLength_ = buffer_.top().nframes_;
                        nextFrame_ = 0;
                        // A new talkspurt will begin
                        // Check if the timer has been set
                        if ( timer_.idle() ) {
                                // It should never happen
                                timer_.start(initialDelay_);
                        }
                }
                //Stat::put(VOIP_BUFFER_OVERFLOW_DROP, id_, 1.0);
        }
        else {
                // Now store the packet into the buffer
                buffer_.push(frame);
                bufferOccupancy_++;
        }
        // If the buffer was empty, start the playout timer and set the next
        // sequence number to be played to the sequence number of this packet
        if (bufferOccupancy_ == 1) {
                // Check if the playout of this talkspurt has been already
                // started. If yes, do nothing
                if ( timer_.idle() ) {
                        timer_.start( initialDelay_ );
                        currentTalk_ = frame.talkspurt_;
                        // Note that, we set the next sample to be played to
                        // the first sample of the talkspurt.
                        // In fact, we enter here if and only if the buffer
                        // was empty and no talkspurt was under playing.
                        nextFrame_ = 0;
                        talkLength_ = frame.nframes_;
                }
        }
}

void
VoipDecoderStatic::handle() 
{
        if (debug_) {
                fprintf (stderr, "%.7f It's time to playout: [%d, %d]\n", NOW, currentTalk_, nextFrame_);
        }
        // When playout of a talkspurt is started, a frame is exctracted
        // from the buffer every period. If no frames are found a loss is
        // assumed.

        // Drop all the old samples in the buffer
        if (bufferOccupancy_ > 0) {
                VoipFrame frame = buffer_.top();
                while ( frame.frame_ < nextFrame_ ) {
                        if (debug_) {
                                fprintf(stderr, "\tFound out-of-time sample [%d, %d]\n", frame.talkspurt_, frame.frame_);
                        }
                        buffer_.pop();
                        bufferOccupancy_ --;
                        //Stat::put(VOIP_BUFFER_OUT_OF_TIME_DROP, id_, 1.0);
                        if ( bufferOccupancy_ > 0 ) {
                                frame = buffer_.top();
                        }
                        else {
                                break;
                        }
                }
        }
        // If there are still frames in the buffer, try to playout the first
        // frame
        if (bufferOccupancy_ > 0) {
                VoipFrame frame = buffer_.top();
                // If talkLength_ is equal to zero, it means that no frames
                // have been received belonging to current talkspurt.
                if (talkLength_ == 0 && frame.talkspurt_ > currentTalk_) {
                        // Assume that one or more talkspurts have been not
                        // received
                        currentTalk_ = frame.talkspurt_;
                        nextFrame_ = 0;
                        talkLength_ = frame.nframes_;
                }
                if (debug_) {
                        fprintf (stderr, "%.7f Handling sample [%d, %d]\n", NOW, frame.talkspurt_, frame.frame_);
                        fprintf (stderr, "\tExpected frame: [%d, %d]\n", currentTalk_, nextFrame_);
                }
                if ( frame.frame_ == nextFrame_ ) {
                        if (debug_) {
                                fprintf (stderr, "%.7f Playing sample [%d, %d]\n", NOW, frame.talkspurt_, frame.frame_);
                        }
                        // Playout the frame
                        const double delay = NOW - frame.timestamp_;
                        //Stat::put (VOIP_FRAME_DELAY, id_, delay);
                        ++delayNum_;
                        delaySum_ += delay;
                        // This assignment assures that if at least one
                        // frame of a talkspurt is received the talkLength_ 
                        // contains the length of the talkspurt.
                        talkLength_ = frame.nframes_;
                        // Remove the frame from the buffer
                        buffer_.pop();
                        bufferOccupancy_--;
                }
        }
        // Update the next frame variable
        nextFrame_++;
        // Check if this is the last frame of the talkspurt
        if ( nextFrame_ == talkLength_ ) {
                collectTalkspurt(talkLength_);
                delaySum_ = 0;
                delayNum_ = 0;
                nextFrame_ = 0;
                currentTalk_++;
                if ( bufferOccupancy_ > 0 ) {
                        if (buffer_.top().talkspurt_ == currentTalk_) {
                                talkLength_ = buffer_.top().nframes_;
                        }
                        else {
                                // There are no information about the next
                                // talkspurt.
                                // Note that, this means that all the frames
                                // in the buffer are related to a future
                                // talkspurts. e.g. the talkspurt after
                                // the current one.
                                talkLength_ = 0;
                        }
                        // Reschedule the timer for the next talkspurt
                        timer_.start ( initialDelay_ );
                }
                // Otherwise, wait for a new talkspurt before restart
                // the timer
                else if (debug_) {
                        fprintf(stderr, "%.7f The buffer is empty, wait for a new frame\n", NOW);
                }
        }
        else {
                timer_.start ( playoutRate_ );
        }
}

int
VoipDecoderStatic::command (int argc, const char*const* argv)
{
        if ( argc == 3 && strcmp (argv[1], "id") == 0 ) {
                id_ = atoi(argv[2]);
                // Initialize some counter metrics
                //Stat::put(VOIP_BUFFER_OUT_OF_TIME_DROP, id_, 0.0);
                //Stat::put(VOIP_BUFFER_OVERFLOW_DROP, id_, 0.0);
                return VoipDecoder::command (argc, argv);

        } else if ( argc == 3 && strcmp ( argv[1], "buffer-size" ) == 0 ) {
                bufferSize_ = atoi ( argv[2] );
                if ( bufferSize_ <= 0 ) {
                        fprintf(stderr, "VoipDecoderStatic: Invalid buffer size: %d\n", bufferSize_);
                        return TCL_ERROR;
                }
                return TCL_OK;

        } else if ( argc == 3 && strcmp ( argv[1], "playout-rate" ) == 0 ) {
                playoutRate_ = atof ( argv[2] );
                if ( playoutRate_ <= 0 ) {
                        fprintf(stderr, "VoipDecoderStatic: Invalid playout rate %f\n", playoutRate_);
                        return TCL_ERROR;
                }
                return TCL_OK;

        } else if ( argc == 3 && strcmp ( argv[1], "initial-delay" ) == 0 ) {
                initialDelay_ = atof ( argv[2] );
                if ( playoutRate_ <= 0 ) {
                        fprintf(stderr, "VoipDecoderStatic: Invalid initial delay %f\n", initialDelay_);
                        return TCL_ERROR;
                }
                return TCL_OK;
        }

        // For more detail, please refer to ITU-T Recommendation G.113
        else if ( argc == 3 && strcmp ( argv[1], "emodel" ) == 0 ) {
                if ( strcmp ( argv[2], "G.711" ) == 0 ) {
                        playoutRate_ = 0.010;
                        return VoipDecoder::command(argc, argv);

                } else if ( strcmp ( argv[2], "G.729A_VAD" ) == 0 ) {
                        playoutRate_ = 0.020;
                        return VoipDecoder::command(argc, argv);

                } else if ( strcmp ( argv[2], "G.723.1_VAD" ) == 0 ) {
                        playoutRate_ = 0.030;
                        return VoipDecoder::command(argc, argv);

                } else if ( strcmp ( argv[2], "GSM.EFR" ) == 0 ) {
                        playoutRate_ = 0.020;
                        return VoipDecoder::command(argc, argv);

                } else if ( strcmp ( argv[2], "GSM.AMR" ) == 0 ) {
                        playoutRate_ = 0.020;
                        return VoipDecoder::command(argc, argv);

                } else {
                        fprintf(stderr, "VoipDecoder - Unsupported Codec\n");
                        return TCL_ERROR;
                }
        }

        return VoipDecoder::command (argc, argv);
}

