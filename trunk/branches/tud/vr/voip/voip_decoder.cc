/*
 * TODO: INSERT LICENSE HERE
 */

#include <voip_decoder.h>
#include <stat.h>
#include <math.h>
#include <stdio.h>

static class VoipDecoderClass : public TclClass {
public:
        VoipDecoderClass() : TclClass("Application/VoipDecoder") { }
        TclObject* create(int, const char*const*) {
                return (new VoipDecoder);
        }
} class_voip_decoder;

VoipDecoder::VoipDecoder () :
	functor_ (this, &VoipDecoder::collectEnd)
{
        debug_       = false;
        agent_       = 0;
        id_          = -1;
        cellId_      = 0;
        peerId_      = -1;

        // E-Model parameter for GSM AMR 12.2 Kb/s codec.
        emodel_Ie_ = 5;
        emodel_Bpl_ = 10;
        emodel_A_ = 5;
        emodel_Ro_ = 93.2;
        mosThreshold_ = 3.0;
        cellOutageMosThreshold_ = 0.10;
	cellOutageLossThreshold_ = 0.02;

        firstFrame_ = true;
        currentFrame_ = 0;
        currentTalk_ = 0;
        currentTalkSize_ = 0;
        delaySum_ = 0;
        delayNum_ = 0;

	nextDecoder_ = 0;

        Stat::addPostProcessMetric(&functor_);
}

int
VoipDecoder::command (int argc, const char*const* argv)
{
        if ( argc == 2  && strcmp (argv[1], "debug") == 0 ) {
                debug_ = true;
                return TCL_OK;

        } else if ( argc == 2  && strcmp (argv[1], "nodebug") == 0 ) {
                debug_ = false;
                return TCL_OK;

	} else if ( argc == 3  && strcmp (argv[1], "next-decoder") == 0 ) {
		nextDecoder_ = (VoipDecoder*) TclObject::lookup (argv[2]);
		return TCL_OK;


	} else if ( argc == 3 && strcmp ( argv[1], "mos-threshold" ) == 0 ) {
		mosThreshold_ = atof(argv[2]);
		if (mosThreshold_ < 0) {
			fprintf (stderr, "E-Model: Invalid MOS threshold '%f'", 
					mosThreshold_);
			return TCL_ERROR;
		}
		return TCL_OK;

		} else if ( argc == 3 && strcmp ( argv[1], "cell-mos-threshold" ) == 0 ) {
			cellOutageMosThreshold_ = atof(argv[2]);
			if (cellOutageMosThreshold_ < 0 || cellOutageMosThreshold_ > 1.0) {
				fprintf (stderr, "E-Model: Invalid Cell Outage MOS threshold '%f'", 
						cellOutageMosThreshold_);
				return TCL_ERROR;
			}
			return TCL_OK;
	
	} else if ( argc == 3 && strcmp ( argv[1], "cell-loss-threshold" ) == 0 ) {
		cellOutageLossThreshold_ = atof(argv[2]);
		if ( cellOutageLossThreshold_ < 0 || cellOutageLossThreshold_ > 1.0) {
			fprintf (stderr, "E-Model: Invalid Cell Outage Loss threshold '%f'", 
					cellOutageLossThreshold_);
			return TCL_ERROR;

		}
		return TCL_OK;

        } else if ( argc == 3 && strcmp (argv[1], "id") == 0 ) {
                id_ = atoi(argv[2]);
                if (id_ < 0) {
                        fprintf (stderr, "Invalid id '%d'\n", id_);
			return TCL_ERROR;

                }
                return TCL_OK;

        } else if ( argc == 3 && strcmp (argv[1], "cell-id") == 0 ) {
                cellId_ = atoi(argv[2]);
                if (cellId_ < 0) {
                        fprintf (stderr, "Invalid cell id '%d'\n", cellId_);
                        return TCL_ERROR;
                }
                return TCL_OK;

        } else if ( argc == 3 && strcmp (argv[1], "peer-id") == 0 ) {
                peerId_ = atoi(argv[2]);
                if (peerId_ < 0) {
                        fprintf (stderr, "Invalid peer id '%d'\n", peerId_);
                        return TCL_ERROR;
                }
                return TCL_OK;

        } else if ( argc == 4 && strcmp ( argv[1], "emodel" ) == 0 ) {
                if ( strcmp ( argv[2], "Ie" ) == 0 ) {
                                emodel_Ie_ = atof(argv[3]);
                                if (emodel_Ie_ < 0) {
                                                fprintf (stderr, "E-Model: Invalid Ie '%f'", emodel_Ie_);
                                                return TCL_ERROR;
                                }

                } else if ( strcmp ( argv[2], "Bpl" ) == 0 ) {
                                emodel_Bpl_ = atof(argv[3]);
                                if (emodel_Bpl_ < 0) {
                                                fprintf (stderr, "E-Model: Invalid Bpl '%f'", emodel_Bpl_);
                                                return TCL_ERROR;
                                }

                } else if ( strcmp ( argv[2], "A" ) == 0 ) {
                                emodel_A_ = atof(argv[3]);
                                if (emodel_A_ < 0) {
                                                fprintf (stderr, "E-Model: Invalid A '%f'", emodel_A_);
                                                return TCL_ERROR;
                                }

                } else if ( strcmp ( argv[2], "Ro" ) == 0 ) {
                                emodel_Ro_ = atof(argv[3]);
                                if (emodel_Ro_ < 0) {
                                                fprintf (stderr, "E-Model: Invalid Ro '%f'", emodel_Ro_);
                                                return TCL_ERROR;
                                }
                }
                        return TCL_OK;
        }

        // For more detail, please refer to ITU-T Recommendation G.113
        else if ( argc == 3 && strcmp ( argv[1], "emodel" ) == 0 ) {
                if ( strcmp ( argv[2], "G.711" ) == 0 ) {
                        emodel_Ie_ = 0;
                        emodel_Bpl_ = 4.3;
                        return TCL_OK;

                } else if ( strcmp ( argv[2], "G.729A" ) == 0 ) {
                        emodel_Ie_ = 11.0;
                        emodel_Bpl_ = 19.0;
                        return TCL_OK;

                } else if ( strcmp ( argv[2], "G.723.1" ) == 0 ) {
                        emodel_Ie_ = 15.0;
                        emodel_Bpl_ = 16.1;
                        return TCL_OK;

                } else if ( strcmp ( argv[2], "GSM.EFR" ) == 0 ) {
                        emodel_Ie_ = 5.0;
                        emodel_Bpl_ = 10.0;
                        return TCL_OK;

                } else if ( strcmp ( argv[2], "GSM.AMR" ) == 0 ) {
                        // GSM_AMR uses the same value of GSM_EFR
                        emodel_Ie_ = 5.0;
                        emodel_Bpl_ = 10.0;
                        return TCL_OK;

                } else {
                        fprintf(stderr, "VoipDecoder - Unsupported Codec\n");
                        return TCL_ERROR;
                }
        }

        return Application::command (argc, argv);
}

void
VoipDecoder::process_data (int size, AppData* data)
{
	VoipPayload* payload = (VoipPayload*) data;

        while ( payload->nframes() > 0 ) {
                VoipFrame frame(--(*payload));
		if ( nextDecoder_ != 0 ) nextDecoder_->collectFrame(frame);
                collectFrame (frame);
                if ( debug_ ) {
                        fprintf (stderr, "%.9f payload received", NOW);
                        fprintf (stderr, " frame %d,%d (total %d) timestamp %f",
                        frame.talkspurt_, frame.frame_,
                                frame.nframes_, frame.timestamp_);
                        fprintf (stderr, "\n");
                }
        }
}

void
VoipDecoder::collectFrame (VoipFrame& frame)
{
	if ( currentTalk_ > frame.talkspurt_ ) return;

        bool newTalkspurt = false;
        if ( currentTalk_ < frame.talkspurt_ || firstFrame_ ) {
                collectTalkspurt (currentTalkSize_);
                currentTalk_     = frame.talkspurt_;
                currentFrame_    = frame.frame_;
                currentTalkSize_ = frame.nframes_;
                delayNum_        = 0;
                delaySum_        = 0;
                newTalkspurt     = true;
        }

        const double delay = NOW - frame.timestamp_;
        if ( frame.frame_ >= ( currentFrame_ + 1 ) || newTalkspurt || firstFrame_ ) {
                ++delayNum_;
                delaySum_ += delay;
                firstFrame_ = false;
        }
        currentFrame_ = frame.frame_;
}

double
VoipDecoder::eModel (double delay, double loss)
{
	double delayms = 1000 * delay;

	// Compute the Id parameter
	int u = ( (delayms - 177.3) > 0 ? 1: 0 );
	double id = 0.0;
	id = 0.024 * delayms + 0.11 * (delayms - 177.3) * u;

	// Packet loss p in %
	double p = loss * 100;
	// Compute the Ie,eff parameter
	double ie_eff = emodel_Ie_ + (95 - emodel_Ie_) * p / (p + emodel_Bpl_);

	// Compute the R factor
	double Rfactor = emodel_Ro_ - id - ie_eff + emodel_A_;

	// Compute the MOS value
	double mos = 0.0;

	if (Rfactor < 0) {
                mos = 1.0;
	}
	else if (Rfactor > 100) {
                mos = 4.5;
	}
	else {
                mos = 1 + 0.035 * Rfactor + 7 * pow(10, -6) * Rfactor * (Rfactor - 60) * (100 - Rfactor);
	}
	mos = ( mos < 1 ) ? 1 : mos;

	return mos;
}
