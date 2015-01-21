/**
   project: NS-2 Pisa
   filename: debug-tools.h
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

#ifndef __NS2_DEBUG_TOOLS_H
#define __NS2_DEBUG_TOOLS_H

// NS-2 Includes
#include <packet.h> //! \todo Includere il file per la classe TclObject!

// STL Includes
#include <list>
#include <map>
#include <stdio.h>
#include <string>
#include <vector>

//! This macro can be used to hard-disable debug and save computational time
#define DEBUG_TOOLS

#ifdef DEBUG_TOOLS
namespace Tools {
        //! Useful debug static class.
        /*!
        The 'format' functions return a single-line (non-'\n' terminated) sequence.

        On the other hand, the 'print' functions print a longer description
        on a given output stream file. An optional argument allows the caller
        to specify a string that has to be added at the beginning of each line.
        */
        class Debug : public TclObject {
                enum { NBUFS = 10 };
                enum { BUFSIZE = 1024 };
                //! Static buffer returned by format functions.
                static char buf_[NBUFS][BUFSIZE];
                //! Current buffer.
                static int cur_;
                //! True if debug is enabled.
                static bool enabled_;
                //! Map of function names to flag (true means this function is traced).
                static std::map<std::string, bool> trace_;

        public:
                //! Do nothing.
                Debug () { }
                //! Do nothing.
                virtual ~Debug () { }

                //! Print a line with
                /** ns2 packet information.
                 */
                static char* format ( Packet* pkt );

                //! Get/set the enabled flag.
                static bool& enabled () { return enabled_; }

                //! Return true if this function is traced and debug is enabled.
                static bool trace (const char* f) {
                        return ( enabled_ &&
                                 trace_.count(std::string(f)) == 1 &&
                                 trace_[std::string(f)] == true ) ? true : false;
                }

                //! Set the trace true/false status of a function name.
                static void trace (const char* f, bool status) {
                        trace_[std::string(f)] = status;
                }

        protected:
                //! Tcl interface.
                virtual int command (int argc, const char*const* argv);
        };
}

#else

namespace Tools {

        class Debug : public TclObject {
        public:
                Debug () { }
                virtual ~Debug () { }

                static char* format ( Packet* pkt ){return "";}

                static bool enabled () { return false; }
                static bool trace (const char* f) { return false; }
                static void trace (const char* f, bool status) { }

        protected:

                virtual int command (int argc, const char*const* argv);
        };

}
#endif // DEBUG_TOOLS

#endif // __NS2_DEBUG_TOOLS_H
