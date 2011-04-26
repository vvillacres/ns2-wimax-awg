/*
 * anfrage.h
 *
 *  Created on: 2010-6-30
 *      Author: Administrator
 */

#ifndef ANFRAGE_H_
#define ANFRAGE_H_

#include <iostream>
#include "connection.h"

using namespace std;

class anfrage {
public:
	anfrage();
	anfrage(int cid,Connection * con, int slot,int diuc);
	virtual ~anfrage();

int get_cid(){return cid_;}
void  set_cid(int cid);

Connection * get_connection(){return con_;}
void set_connection(Connection * con);

int get_slot(){return slot_;}
void set_slot(int slot);

int get_diuc(){return diuc_;}
void set_diuc(int diuc);

void displayAnfrage() const;


private:

	int cid_;
	Connection * con_;
	int slot_;
	int diuc_;
};

#endif /* ANFRAGE_H_ */
