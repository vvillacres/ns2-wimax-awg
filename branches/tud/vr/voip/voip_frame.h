/*
 * TODO: INSERT LICENSE HERE
 */

#ifndef __NS2_VOIP_FRAME_H
#define __NS2_VOIP_FRAME_H

#include <ns-process.h>
#include <object.h>
#include <list>
#include <stat.h>

struct VoipFrame {
        //! Talkspurt identifier.
        unsigned int talkspurt_;
        //! Frame identifier within talkspurt.
        unsigned int frame_;
        //! Number of frames in this talkspurt.
        unsigned int nframes_;
        //! Size, in bytes of this frame.
        unsigned int size_;
        //! Frame generation timestamp, in seconds.
        double timestamp_;
	//! Statistics collection flag
	bool statActive_;

        //! Create an empty VoIP frame.
        VoipFrame (unsigned int size = 0,
                   unsigned int talkspurt = 0,
                   unsigned int frame = 0,
                   unsigned int nframes = 0) {
                size_ = size;
                talkspurt_ = talkspurt;
                frame_ = frame;
                nframes_ = nframes;
                timestamp_ = NOW;
		statActive_ = Stat::active();
        }

        //! Stric weak ordering among VoIP frames
        bool operator<(const VoipFrame& frame) const {
                if ( talkspurt_ > frame.talkspurt_ ) {
                        return true;
                }
                else if ( talkspurt_ == frame.talkspurt_ && frame_ > frame.frame_ ) {
                        return true;
                }
                return false;
        }
};

struct VoipPayload : public AppData {
public:
        typedef std::list<VoipFrame> List;

        //! Create a VoIP payload with no frames.
        VoipPayload () : AppData (VOIP_DATA) { nframes_ = 0; }
        //! Copy constructor.
        VoipPayload (VoipPayload& p) : AppData (p) {
                nframes_ = p.nframes_;
                frames_ = p.frames_;
        }

        //! Deallocate the payload.
        ~VoipPayload () { }

        //! Add a VoIP frame.
        void operator+= (const VoipFrame& frame) {
                frames_.push_back (frame);
                ++nframes_;
        }
        //! Pop the first VoIP frame and return it.
        VoipFrame operator-- () {
                if ( nframes_ == 0 ) abort ();
                VoipFrame first = frames_.front();
                frames_.pop_front ();
                --nframes_;
                return first;
        }
        //! Get the number of frames.
        unsigned int nframes () { return nframes_; }

        //! Return the memory occupancy (required by the base class).
        int size() const { return sizeof(VoipPayload); }

        //! Copy this object (required by the base class).
        AppData* copy() { return (new VoipPayload(*this)); }

private:
        //! Number of VoIP frames contained into this payload.
        unsigned int nframes_;
        //! List of VoIP frames contained into this payload.
        List frames_;
};

#endif // __NS2_VOIP_FRAME_H
