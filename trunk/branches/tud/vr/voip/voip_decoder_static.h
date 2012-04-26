/*
 * TODO: INSERT LICENSE HERE
 */

#ifndef __NS2_VOIP_DECODER_STATIC_H
#define __NS2_VOIP_DECODER_STATIC_H

#include <voip_decoder.h>

#include <t_timers.h>
#include <queue>


class VoipDecoderStatic : public VoipDecoder {
public:
        //! Create an empty VoIP decoder.
        VoipDecoderStatic ();

        //! Tcl interface.
        int command (int argc, const char*const* argv);

        //! Periodic playout timer
        void handle();

protected:
        //! Collect statistics after playout buffering frame by frame.
        void collectFrame (VoipFrame& frame);

protected:
        //! Frame buffer
        std::priority_queue< VoipFrame > buffer_;

        //! Next frame to be played out
        unsigned int nextFrame_;

        //! Length of the current talk (in frames)
        unsigned int talkLength_;

        //! Playout sample rate (in ms)
        double playoutRate_;

        //! Playout initial delay
        /** Delay of the first sample of the talkspurt
         */
        double initialDelay_;

        //! Playout buffer size.
        /** It is expressed in terms of number of VoIP samples, not in
         *  number of IP Packets!
         */
        int bufferSize_;

        //! Current buffer occupancy
        int bufferOccupancy_;

        //! T_Timer
        /** The timer is used to manage playout samples
         */
        TTimer<VoipDecoderStatic> timer_;
};

#endif // __NS2_VOIP_DECODER_STATIC_H
