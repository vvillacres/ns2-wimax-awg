/*
 * TODO: INSERT LICENSE HERE
 */

#include <voip_header.h>
#include <stdio.h>

static class VoipHeaderClass : public TclClass {
        public:
                VoipHeaderClass() : TclClass("Application/VoipHeader") {}
                TclObject* create(int, const char*const*) {
                        return (new VoipHeader);
                }
} class_voip_header;

VoipHeader::VoipHeader()
{
        debug_           = false;
        initialized_     = false;
        compression_     = false;
        size_            = 40;

        agent_           = 0;        // base class member
}

int
VoipHeader::command (int argc, const char*const* argv)
{
        if ( argc == 2  && strcmp (argv[1], "debug") == 0 ) {
                debug_ = true;
                return TCL_OK;

        } else if ( argc == 2  && strcmp (argv[1], "nodebug") == 0 ) {
                debug_ = false;
                return TCL_OK;

        } else if ( argc == 2  && strcmp (argv[1], "compression") == 0 ) {
                compression_ = true;
                size_ = 6;
                return TCL_OK;

	} else if ( argc == 3  && strcmp (argv[1], "compression") == 0 ) {
				compression_ = true;
				size_ = atoi(argv[2]);
				return TCL_OK;

        } else if ( argc == 2  && strcmp (argv[1], "nocompression") == 0 ) {
                compression_ = false;
                size_ = 40;
                return TCL_OK;

        } else if ( argc == 2  && strcmp (argv[1], "algorithm") == 0 ) {
                fprintf(stderr, "Header Compression Algorithm. To Be Implemented\n");
                return TCL_ERROR;
        }

        return Application::command (argc, argv);
}

void 
VoipHeader::recvPayload (int size, VoipPayload* data)
{
        int payloadSize = size_ + size;
	agent_->sendmsg(payloadSize, (AppData*)data);
}
