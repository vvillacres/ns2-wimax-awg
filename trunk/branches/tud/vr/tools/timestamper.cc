/* 
 *  Copyright (C) 2006 Dip. Ing. dell'Informazione, University of Pisa, Italy
 *  http://info.iet.unipi.it/~cng/ns2measure/ns2measure.html
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA, USA
 */

/* Timestamper connector
   This code has been developed within the EuQoS project of the EU IST 6th Framework Programme. 
   The "timestamper" connector sits at the **head** of a link, (at least, provided nothing else is 
   attached to the head of that link afterwards), and adds a timestamp to packets already marked by
   an end-to-end tagger only (also see e2e{t,m}.{cc,h} files). If packets have NOT been marked by an 
   end-to-end tagger, it simply does nothing.

   Giovanni Stea (g.stea@iet.unipi.it), University of Pisa, Italy, June 2005. 
*/


#ifndef ns_timestamper_h
#define ns_timestamper_h

#include <connector.h>
#include <packet.h>

class timestamper : public Connector {
	
protected:
	virtual void recv(Packet *, Handler *);
};


void timestamper::recv(Packet *p, Handler *)
{
	hdr_cmn *ch=hdr_cmn::access(p);
	int howmany=ch->e2e_timestamp_no();

	if (howmany>0 && howmany<TSTAMP_NO)
	// only packets which have ALREADY been timestamped by a sending agent should be considered

	{
		ch->e2e_timestamp(howmany)=Scheduler::instance().clock();
		ch->e2e_timestamp_no()++;
	}
	target_->recv(p);
}


static class timestamperClass : public TclClass {
public:
	timestamperClass() : TclClass ("timestamper") {}
	TclObject* create(int,const char*const*) {
		return (new timestamper());
	}
}class_timestamper;


#endif
