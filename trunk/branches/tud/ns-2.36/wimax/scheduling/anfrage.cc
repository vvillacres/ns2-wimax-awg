/*
 * anfrage.cpp
 *
 *  Created on: 2010-6-30
 *      Author: Administrator
 */

#include "anfrage.h"

anfrage::anfrage()
{
	cid_ = 0;
	con_ = NULL;
	slot_ = 0;
	diuc_ = 0;
}

anfrage::anfrage(int cid, Connection * con, int slot,int diuc)
{
	cid_ = cid;
	con_ = con;
	slot_ = slot;
	diuc_ = diuc;
}

anfrage::~anfrage()
{

}

void anfrage::set_cid(int cid)
{
	cid_ = cid;
}

void anfrage::set_connection(Connection * con)
{
	con_ = con;
}

void anfrage::set_slot(int slot)
{
	slot_ = slot;
}

void anfrage::set_diuc(int diuc)
{
	diuc_ = diuc;
}

void anfrage::displayAnfrage() const
{

	cout <<"CID: "<< cid_ <<"  Slot: "<<slot_<<"  DIUC: "<<diuc_<<endl;
}

