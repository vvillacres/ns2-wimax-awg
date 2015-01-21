/*
 * TODO: INSERT LICENSE HERE
 */

#ifndef __NS2_VOIP_ENCODER_H
#define __NS2_VOIP_ENCODER_H

#include <app.h>
#include <object.h>
#include <t_timers.h>
#include <map>

class VoipAggregate;
class VoipHeader;

class VoipEncoder : public Application {
public:
        //! Create an empty VoIP source.
        VoipEncoder ();

        //! Return true if the object is ready to be started.
        bool initialized () { return initialized_; }

        //! Tcl interface.
        /*!
                * Tcl commands:
                * - $obj debug\n
                *   Enable debug to standard error.
                * - $obj nodebug\n
                *   Disable debug to standard error (default).
                * - $obj codec $code_type
                *   Select the codec to be used
                *   The followinf codecs are actually implemented:
                *   <ul>
                *    <li>
                *     G.711 - (64kbps): 80 bytes VoIP frame, 100 frames per second.
                *    </li>
                *    <li>
                *     GSM AMR - (12.2kbps): 32 bytes VoIP frame, 50 frames per second.
                *              Note that, 244 bits payload, 4 bit padding, 1 byte EOF
                *    </li>
                *   </ul>
                */
        virtual int command (int argc, const char*const* argv);

        //! Produce frames for a new talkspurt.
        void talkspurt (unsigned int count, double duration);

        //! Generate a new talkspurt.
        void handle (unsigned int count);

protected:
        //! Timer to schedule frames.
        /*!
                * The timer is called at the beginning of each talkspurt.
                */
        TMultiTimer<VoipEncoder, unsigned int> timer_;

        //! Frame size, in bytes.
        unsigned int size_;

        //! Interval between two consecutive frames, in seconds.
        double inter_;

        //! Number of remaining frames to generate for a given talkspurt.
        std::map<unsigned int, unsigned int> remaining_;

        //! Number of frames to generate for a given talkspurt.
        std::map<unsigned int, unsigned int> total_;

        //! Statistic Metric ID
        int id_;

        //! Aggregate object, if present.
        VoipAggregate* aggregate_;

        //! Pointer to the VoIP Header Module, if present
        VoipHeader* header_;

        //! True if debug is enabled.
        bool debug_;

        //! True if the object is ready to produce frames.
        bool initialized_;
};

#endif // __NS2_VOIP_ENCODER_H
