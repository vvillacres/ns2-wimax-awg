/*
 * TODO: INSERT LICENSE HERE
 */

#ifndef __NS2_VOIP_HEADER_H
#define __NS2_VOIP_HEADER_H

#include <voip_frame.h>

#include <app.h>
#include <object.h>

class VoipHeader : public Application {
public:
        //! Create an empty VoIP Header.
        VoipHeader ();

        //! Do Nothing
        ~VoipHeader() { }

        //! Return true if the object is ready to be started.
        bool initialized () { return initialized_; }

        //! Tcl interface.
        /*!
         * Tcl commands:
         * - $obj debug\n
         *   Enable debug to standard error.
         * - $obj nodebug\n
         *   Disable debug to standard error (default).
         */
        virtual int command (int argc, const char*const* argv);

        //! Produce frames for a new talkspurt.
        void recvPayload (int noBytes, VoipPayload* data);

protected:

        //! True if debug is enabled.
        bool debug_;

        //! True if the object is ready to produce frames.
        bool initialized_;

        //! True if header compression is enabled.
        // This variable will allow us to implement new header
        // compression policy based on real compression algorithms. 
        // Currently this variable is not used.
        bool compression_;

        //! Header size (RTP + UDP + IP) in bytes
        unsigned int size_;
};

#endif // __NS2_VOIP_HEADER_H
