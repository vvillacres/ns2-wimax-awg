/*
 * oscaburst.h
 *
 *  Created on: 26.04.2011
 *      Author: richter
 */

#ifndef OSCABURST_H_
#define OSCABURST_H_

#include <iostream>
#include <vector>
#include "anfrage.h"
#include "connection.h"
using namespace std;

class OscaBurst {
public:
	OscaBurst();
	OscaBurst(int xCorr, int yCorr, int width, int height);
	virtual ~OscaBurst();


	int get_xCorr(){return xCorr_;}
	int get_yCorr(){return yCorr_;}
	int get_width(){return width_;}
	int get_height(){return height_;}
	int get_area(){return width_*height_;}
	int get_slot(){return slot_;}
	int get_diuc();
	vector<anfrage*> get_anfrageList(){return anfrageList_;}

	void set_x(int xCorr);
	void set_y(int yCorr);
	void set_width(int width);
	void set_height(int height);
	void set_slot(int slot);
	void addToAnfrageList(anfrage* one_anfrage);
	void displayBurst() const;


private:
	int xCorr_,yCorr_;
	int width_;
	int height_;
	int slot_;
	vector<anfrage *> anfrageList_;

};

#endif /* OSCABURST_H_ */
