/*
 * freearea.h
 *
 *  Created on: 02.08.2010
 *      Author: dong
 */

#ifndef FREEAREA_H_
#define FREEAREA_H_
class freearea {

public:
	freearea();
	freearea(int xCorr,int yCorr,int width,int height);
	virtual ~freearea();

	int get_x(){return x_;}
	int get_y(){return y_;}

    int get_width(){return width_;}
	int get_height(){return height_;}
	int get_area(){return width_*height_;}

	void set_x(int xCorr);
	void set_y(int yCorr);
	void set_width(int width);
	void set_height(int height);



private:


	int x_,y_;
	int width_;
	int height_;

};

#endif /* FREEAREA_H_ */


