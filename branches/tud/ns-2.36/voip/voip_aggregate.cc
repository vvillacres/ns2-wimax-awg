/*
 * TODO: INSERT LICENSE HERE
 */

#include <voip_aggregate.h>
#include <voip_frame.h>
#include <voip_header.h>
#include <math.h>
#include <stdio.h>

static class VoipAggregateClass : public TclClass {
public:
   VoipAggregateClass() : TclClass("Application/VoipAggregate") {}
   TclObject* create(int, const char*const*) {
      return (new VoipAggregate);
   }
} class_voip_aggregate;

VoipAggregate::VoipAggregate ()
{
        size_        = 0;
        nframes_     = 0;
        initialized_ = false;
        debug_       = false;
        payload_     = 0;
        header_      = 0;
        pending_     = 0;
        pendingSize_ = 0;

        agent_       = 0;       // base class member
}

int
VoipAggregate::command (int argc, const char*const* argv)
{
        if ( argc == 2  && strcmp (argv[1], "debug") == 0 ) {
                debug_ = true;
                return TCL_OK;

        } else if ( argc == 2  && strcmp (argv[1], "nodebug") == 0 ) {
                debug_ = false;
                return TCL_OK;

        } else if ( argc >= 3 && strcmp (argv[1], "size") == 0 ) {
                size_ = atoi (argv[2]);
                return TCL_OK;

        } else if ( argc == 3  && strcmp (argv[1], "header") == 0 ) {
                header_ = (VoipHeader*) TclObject::lookup (argv[2]);
                return TCL_OK;

        } else if ( argc >= 3 && strcmp (argv[1], "nframes") == 0 ) {
                nframes_ = atoi (argv[2]);
                if ( nframes_ == 0 ) {
                        fprintf (stderr, "Invalid number of aggregated frames '%d'\n",
                                 nframes_);
                        return TCL_ERROR;
                }
                return TCL_OK;
        }
        return Application::command (argc, argv);
}

void
VoipAggregate::recv (VoipFrame& frame)
{
        if ( debug_ ) fprintf (stderr,
                "%.9f frame received %d,%d (total %d) timestamp %f\n",
                        NOW, frame.talkspurt_, frame.frame_, frame.nframes_, frame.timestamp_);

        // Update the number of pending frames.
        ++pending_;

        // If this is the first frame of the payload, create a new payload.
        if ( pending_ == 1 ) payload_ = new VoipPayload;

        // Add the frame to the pending payload.
        (*payload_) += frame;

        // Update the current size of the payload.
        pendingSize_ += frame.size_;

        // If the payload is full, send the frame to the agent.
        if ( pending_ == nframes_ || frame.frame_ == ( frame.nframes_ - 1 ) ) {

                // Select the size to be advertised for the payload.
                unsigned int size = ( size_ == 0 ) ? pendingSize_ : size_;

                if ( header_ ) {
                        header_->recvPayload( size, payload_ );
                } else {
                        // Send the message to the agent.
                        agent_->sendmsg (size, (AppData*)payload_);
                }

                // Reset the status variables.
                payload_ = 0;
                pending_ = 0;
                pendingSize_ = 0;
        }
}
