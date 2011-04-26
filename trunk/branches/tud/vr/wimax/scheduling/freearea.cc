/*
 * freearea.cpp
 *
 *  Created on: 02.08.2010
 *      Author: dong
 */
#include "freearea.h"

freearea::freearea()
{
	x_ = 0;
	y_ = 0;
	width_ = 0;
	height_ = 0;

}

freearea::freearea (int xCorr,int yCorr,int width,int height)
{
	x_ = xCorr;
	y_ = yCorr;
	width_ = width;
	height_ = height;
}

freearea::~freearea() {

}

void freearea::set_x(int xCorr){
	x_ = xCorr;

}

void freearea::set_y(int yCorr){
	y_ = yCorr;

}

void freearea::set_width(int width)
{
	width_ = width;
}

void freearea::set_height(int height)
{
   height_ = height;
}


