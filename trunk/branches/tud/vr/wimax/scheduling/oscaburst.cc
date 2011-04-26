/*
 * oscaburst.cpp
 *
 *  Created on: 26.04.2011
 *      Author: richter
 */

#include "oscaburst.h"


OscaBurst::OscaBurst()
{
	xCorr_ = 0; yCorr_ = 0;
	width_ = 0;
	height_ = 0;
	slot_ = 0;
}


OscaBurst::OscaBurst(int xCorr, int yCorr, int width, int height)
{
	    xCorr_ = xCorr; yCorr_ = yCorr;
		width_ = width;
		height_ = height;
		slot_ = 0;
}

OscaBurst::~OscaBurst()
{
	while (!anfrageList_.empty())
		{
		delete anfrageList_.back();
		anfrageList_.pop_back();
		}
}


int OscaBurst::get_diuc(){
	   int diuc = 0;
	   diuc =anfrageList_.at(0)->get_diuc();

	    return diuc;
}

void OscaBurst::set_x(int xCorr){
	xCorr_=xCorr;
}

void OscaBurst::set_y(int yCorr){
	yCorr_=yCorr;
}

void OscaBurst::set_width(int width)
{
	width_ = width;
}

void OscaBurst::set_height(int height)
{
   height_ = height;
}

void OscaBurst::set_slot(int slot)
{
	slot_ = slot;
}


void OscaBurst::addToAnfrageList(anfrage* one_anfrage)
{
	anfrageList_.push_back(one_anfrage);
	slot_ += one_anfrage->get_slot();
}

void OscaBurst::displayBurst() const
{

	unsigned int i=0;
    for (i=0;i != anfrageList_.size();i++)
    {
    	anfrageList_.at(i)->displayAnfrage();
    }
}
