/*
 * TODO: INSERT LICENSE HERE
 */

#include <voip_source.h>
#include <voip_encoder.h>
#include <voip_bidirectional.h>
#include <ranvar.h>
#include <stdio.h>

#include <stat.h>

static class VoipSourceClass : public TclClass {
public:
    VoipSourceClass() : TclClass("VoipSource") { }
    TclObject* create(int, const char*const*) {
        return (new VoipSource);
    }
} class_voip_source;

VoipSource::VoipSource () : timer_(this), talk1_(1.423, 0.824), silence1_(0.899, 1.089)
{
    encoder_        = 0;
    talk_           = 0;
    silence_        = 0;
    count_          = 0;
    debug_          = false;
    stat_           = false;
    fid_            = 0;
    bidirectional_  = 0;
}

int
VoipSource::command (int argc, const char*const* argv)
{
    if ( argc == 3  && strcmp (argv[1], "stat") == 0 ) {
        stat_ = true;
        fid_ = atoi(argv[2]);
        return TCL_OK;
        
    } else if ( argc == 2  && strcmp (argv[1], "debug") == 0 ) {
        debug_ = true;
        return TCL_OK;

    } else if ( argc == 2  && strcmp (argv[1], "nodebug") == 0 ) {
        debug_ = false;
        return TCL_OK;

    } else if ( argc == 2  && strcmp (argv[1], "start") == 0 ) {
        handle ();
        return TCL_OK;

    } else if ( argc == 2  && strcmp (argv[1], "stop") == 0 ) {
        stop ();
        return TCL_OK;

    } else if ( argc == 3 && strcmp (argv[1], "encoder") == 0 ) {
        encoder_ = (VoipEncoder*) TclObject::lookup (argv[2]);
        return TCL_OK;

    } else if ( argc == 3 && strcmp (argv[1], "bidirectional") == 0 ) {
        bidirectional_ = (VoipBidirectional*) TclObject::lookup (argv[2]);
        return TCL_OK;

    } else if ( argc >= 3 && strcmp (argv[1], "model") == 0 ) {
        if ( argc == 5 && strcmp (argv[2], "exponential") == 0 ) {

            // Retrieve and check input parameters from Tcl.
            double talk = atof (argv[3]);
            double silence = atof (argv[4]);
            if ( talk <= 0 || silence <= 0 ) {
                fprintf (stderr, "Invalid exponential parameters talk '%f', silence '%f'\n", talk, silence);
                return TCL_ERROR;
            }

            // Create and initialize the random variables
            // to schedule talkspurts and silence periods.
            //! \todo
            //! Andrea: Dynamic cast seems to introduce some strange
            //! behaviour! CHECK CHECK CHECK.
            talk_ = new ExponentialRandomVariable;
            dynamic_cast<ExponentialRandomVariable*>(talk_)->setavg (talk);
            silence_ = new ExponentialRandomVariable;
            dynamic_cast<ExponentialRandomVariable*>(silence_)->setavg (silence);
            return TCL_OK;

            //
            // Add *here* your VoIP source model.
            //
            // if ( argc == X && strcmp (argv[2], "X") == 0 ) {
            // 	return TCL_OK;
            //
        } else if ( argc == 3 && strcmp (argv[2], "one-to-one") == 0 ) {
            // Retrieve and check input parameters from Tcl.
            // Create and initialize the random variables
            // to schedule talkspurts and silence periods.
            talk_ = new WeibullRandomVariable(1.423, 0.824);
            silence_ = new WeibullRandomVariable(0.899, 1.089);
            return TCL_OK;

        } else if ( argc == 3 && strcmp (argv[2], "many-to-many") == 0 ) {
            // Retrieve and check input parameters from Tcl.
            // Create and initialize the random variables
            // to schedule talkspurts and silence periods.
            talk_ = new WeibullRandomVariable(2.184, 0.435);
            silence_ = new WeibullRandomVariable(3.093, 0.450);
            return TCL_OK;

        } else if ( argc == 3 && strcmp (argv[2], "one-to-many") == 0 ) {
            // Retrieve and check input parameters from Tcl.
            // Create and initialize the random variables
            // to schedule talkspurts and silence periods.
            talk_ = new WeibullRandomVariable(23.952, 1.278);
            silence_ = new WeibullRandomVariable(3.941, 0.820);
            return TCL_OK;

        } else if ( argc == 3 && strcmp (argv[2], "many-to-one") == 0 ) {
            // Retrieve and check input parameters from Tcl.
            // Create and initialize the random variables
            // to schedule talkspurts and silence periods.
            talk_ = new WeibullRandomVariable(3.342, 0.732);
            silence_ = new WeibullRandomVariable(44.267, 0.432);
            return TCL_OK;

        } else if ( argc == 7 && strcmp (argv[2], "weibull-custom") == 0 ) {
            talk_    = new WeibullRandomVariable( atof(argv[3]), atof(argv[4]) );
            silence_ = new WeibullRandomVariable( atof(argv[5]), atof(argv[6]) );
            return TCL_OK;
	    } else {
		    fprintf (stderr, "Unknown VoIP source model '%s'\n", argv[2]);
            return TCL_ERROR;
        }
	    return TCL_OK;
    }
    return TCL_ERROR;
}

void
VoipSource::handle ()
{
    if ( ! initialized () ) {
        fprintf (stderr, "VoIP source not initialized");
        abort ();
    }

    double talk    = 0.0;
    double silence = 0.0;

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

	if ( talk > 0.0 ) {
        // Tell the encoder that a new talkspurt is to be generated.
        encoder_->talkspurt (count_, talk);

	    // Update the talkspurt counter.
        ++count_;
		// Collect some statistics useful for debug purposes
		if ( stat_ )
        {
			Stat::put ("voip_talkspurt_duration", 0, talk );
            //Stat::put (VOIP_DUR_FID_TALK, fid_, talk);
        }
	}

	// Collect some statistics useful for debug purposes
	if ( silence > 0.0 && stat_ )
    {
		Stat::put ("voip_silence_duration", 0, silence );
        Stat::put ("voip_dur_fid_silence", fid_, silence);
    }
    
    // Reschedule this function to be called after the silence period.
    timer_.start (talk + silence);
}
