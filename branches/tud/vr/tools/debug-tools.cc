/**
   project: NS-2 Pisa
   filename: debug-tools.cc
	author: C. Cicconetti <c.cicconetti@iet.unipi.it>
	        A. Erta <alessandro.erta@imtlucca.it>
		A. Bacioccola <a.bacioccola@iet.unipi.it>
	year: 2006
   affiliation:
      Dipartimento di Ingegneria dell'Informazione
	   University of Pisa, Italy
   description:
	   Debug Tools Class
 */

// Debug Tools Includes
#include <debug-tools.h>

// NS-2 Includes
#include <packet.h>
#include <ip.h>

// STL Includes
#include <iostream>

namespace Tools {
        static class DebugClass : public TclClass {
        public:
                DebugClass() : TclClass("DebugTools") {}
                TclObject* create(int, const char*const*) {
                        return (new Debug);
                }
        } class_tools_debug;
}

#ifdef DEBUG_TOOLS

char Tools::Debug::buf_[NBUFS][BUFSIZE];
int Tools::Debug::cur_ = 0;
bool Tools::Debug::enabled_ = true;
std::map<std::string, bool> Tools::Debug::trace_;

using namespace Tools;
using namespace std;

int
Debug::command (int argc, const char*const* argv)
{
	if ( argc == 2 && strcmp (argv[1], "enable") == 0 ) {
		enabled_ = true;
		return TCL_OK;
	} else if ( argc == 2 && strcmp (argv[1], "disable") == 0 ) {
		enabled_ = false;
		return TCL_OK;
	} else if ( argc == 4 && strcmp (argv[1], "trace") == 0 ) {
		bool status;

		if ( strcmp (argv[3], "on") == 0 ) status = true;
		else if ( strcmp (argv[3], "off") == 0 ) status = false;
		else {
			fprintf (stderr, "the trace status of function '%s' must be "
					"either 'on' or 'off', not '%s'\n", argv[2], argv[3]);
			return TCL_ERROR;
		}

		trace (argv[2], status);
		return TCL_OK;
	}

	return TCL_ERROR;
}

char*
Debug::format (Packet* pkt)
{
	char* b = buf_[cur_];
	cur_ = ( cur_ + 1 == NBUFS ) ? 0 : ( cur_ + 1 );

	snprintf (b, BUFSIZE, "IP fid %d prio %d size %d src %d dst %d uid %d",
                        (HDR_IP(pkt))->flowid(),
                        (HDR_IP(pkt))->prio(),
                        (HDR_CMN(pkt))->size(),
                        (HDR_IP(pkt))->saddr(),
                        (HDR_IP(pkt))->daddr(),
                        (HDR_CMN(pkt))->uid() );
	return b;
}

#else

using namespace Tools;

int
Debug::command (int argc, const char*const* argv)
{
	return TCL_OK;
}

#endif // DEBUG_TOOLS