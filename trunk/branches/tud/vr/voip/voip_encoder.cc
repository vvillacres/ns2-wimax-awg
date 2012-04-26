/*
 * TODO: INSERT LICENSE HERE
 */

#include <voip_encoder.h>
#include <voip_frame.h>
#include <voip_aggregate.h>
#include <voip_header.h>
#include <stat.h>
#include <math.h>
#include <stdio.h>

static class VoipEncoderClass : public TclClass {
public:
        VoipEncoderClass() : TclClass("Application/VoipEncoder") {}
        TclObject* create(int, const char*const*) {
                return (new VoipEncoder);
        }
} class_voip_encoder;

VoipEncoder::VoipEncoder () : timer_ (this)
{
    size_           = 0;
    inter_          = 0;
    initialized_    = false;
    debug_          = false;
    header_         = 0;
    aggregate_      = 0;
    agent_          = 0;        // base class member
}

int
VoipEncoder::command (int argc, const char*const* argv)
{
    if ( argc == 2  && strcmp (argv[1], "debug") == 0 ) {
        debug_ = true;
        return TCL_OK;

    } else if ( argc == 2  && strcmp (argv[1], "nodebug") == 0 ) {
        debug_ = false;
        return TCL_OK;

    } else if ( argc == 2 && strcmp (argv[1], "inter") == 0 )  {
        Tcl::instance ().resultf ("%f", inter_);
        return TCL_OK;

    } else if ( argc == 3  && strcmp (argv[1], "aggregate") == 0 ) {
        aggregate_ = (VoipAggregate*) TclObject::lookup (argv[2]);
        return TCL_OK;

    } else if ( argc == 3  && strcmp (argv[1], "header") == 0 ) {
        header_ = (VoipHeader*) TclObject::lookup (argv[2]);
        return TCL_OK;

    } else if ( argc == 3 && strcmp (argv[1], "id") == 0 ) {
        id_ = atoi(argv[2]);
        if (id_ < 0) {
            fprintf (stderr, "VoIP Encoder: Invalid id(%d)\n", id_);
            return TCL_ERROR;
        }
        return TCL_OK;

    } else if ( argc >= 3 && strcmp (argv[1], "codec") == 0 ) {
        if ( argc == 3 && strcmp (argv[2], "G.711") == 0 ) {
            size_ = 80;
            inter_ = 0.01;
            initialized_ = true;
            return TCL_OK;

        } else if ( argc == 3 && strcmp (argv[2], "GSM.AMR") == 0 ) {
            size_ = 32;
            inter_ = 0.02;
            initialized_ = true;
            return TCL_OK;

        } else if ( argc == 3 && strcmp (argv[2], "G.729A") == 0 ) {
            size_ = 20;
            inter_ = 0.02;
            initialized_ = true;
            return TCL_OK;

        } else if ( argc == 3 && strcmp (argv[2], "G.723.1") == 0 ) {
            size_ = 24;
            inter_ = 0.03;
            initialized_ = true;
            return TCL_OK;

        } else if ( argc == 3 && strcmp (argv[2], "GSM.EFR") == 0 ) {
            size_ = 32;
            inter_ = 0.02;
            initialized_ = true;
            return TCL_OK;
        //
        // Add *here* your codec.
        //
        // if ( argc == X && strcmp (argv[2], "X") == 0 ) {
        // 	return TCL_OK;
        //
        } else {
            fprintf (stderr, "Unknown codec '%s'\n", argv[2]);
            return TCL_ERROR;
        }
        return TCL_OK;
    }
    return Application::command (argc, argv);
}

void
VoipEncoder::handle (unsigned int count)
{
    if ( ! initialized () ) {
        fprintf (stderr, "VoIP encoder not initialized");
        abort ();
    }

    if ( debug_ ) fprintf (stderr,
        "%.9f new frame (%d bytes) for talkspurt %d, %d to go\n",
            NOW, size_, count, remaining_[count]);

    // Create a new frame.
    VoipFrame frame (size_, count,
        total_[count] - remaining_[count], total_[count]);

    // Count the number of frames sent
    Stat::put("voip_frames_sent",id_,1);

    // Send the frame to the aggregate object, if present.
    if ( aggregate_ ) {
        aggregate_->recv (frame);

    // Otherwise, send it to the agent.
    } else {
        // Create a new payload.
        VoipPayload* payload = new VoipPayload;
        (*payload) += frame;

        // Send the frame to the header module, if present.
        if ( header_ ) {
            header_->recvPayload(size_, payload);
        } else {
            // Send out the payload to the agent.
            agent_->sendmsg (size_, (AppData*)payload);
        }
    }

    // Update the remaining frames to be generated.
    --remaining_[count];

    // If this is not the last frame to generate, re-start the timer.
    if ( remaining_[count] > 0 ) timer_.add (inter_, count);
}

void
VoipEncoder::talkspurt (unsigned int count, double duration)
{
    if ( debug_ ) fprintf (stderr,
        "%.9f flow id %u talkspurt %d start (duration %f)\n",
            NOW, id_, count, duration);

    unsigned int remaining = (unsigned int) round ( duration / inter_ );
    if ( remaining == 0 ) remaining = 1;

    // Update the remaining frames to be generated.
    total_[count] = remaining;
    remaining_[count] = remaining;

    // Generate the first frame of this talkspurt.
    handle (count);
}
