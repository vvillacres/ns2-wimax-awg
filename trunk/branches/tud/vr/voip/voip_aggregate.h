/*
 * TODO: INSERT LICENSE HERE
 */

#ifndef __NS2_VOIP_AGGREGATE_H
#define __NS2_VOIP_AGGREGATE_H

class VoipHeader;

#include <voip_frame.h>
#include <app.h>
#include <object.h>

class VoipAggregate : public Application {
public:
	//! Create an empty VoIP aggregate object.
	VoipAggregate ();

	//! Return true if the object is ready to be started.
	bool initialized () { return initialized_; }
	
	//! Tcl interface.
	/*!
	 * Tcl commands:
	 * - $obj debug\n
	 *   Enable debug to standard error.
	 * - $obj nodebug\n
	 *   Disable debug to standard error (default).
	 * - $obj size $s \n
	 *   Force the size of the aggregated payload to $s bytes.
	 * - $obj nframes $n\n
	 *   Aggregate (at most) $n frames into the same payload.
	 */
	virtual int command (int argc, const char*const* argv);

	//! Receive a new frame from the encoder.
	void recv (VoipFrame& frame);

protected:
        //! Payload size, in bytes.
        /*!
         * If size_ is equal to zero, then the payload size is set to the
         * sum of the contained frames.
         */
        unsigned int size_;

        //! Number of frames to aggregate into a payload.
        unsigned int nframes_;

        //! Number of pending frames;
        unsigned int pending_;

        //! Current payload size. Only used when size_ is equal to zero.
        unsigned int pendingSize_;

        //! Payload waiting to be sent.
        VoipPayload* payload_;

        //! Pointer to the VoIP Header Module
        VoipHeader* header_;

        //! True if debug is enabled.
        bool debug_;

        //! True if the object is ready to produce frames.
        bool initialized_;
};

#endif // __NS2_VOIP_AGGREGATE_H
