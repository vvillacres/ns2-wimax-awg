/*
 * frame.h
 *
 *  Created on: 2010-7-1
 *      Author: Administrator
 */

#ifndef FRAME_H_
#define FRAME_H_
#include <iostream>
#include <vector>
#include "oscaburst.h"
#include "anfrage.h"
#include "freearea.h"

using namespace std;

class frame {

public:

	frame();
	frame(int width,int height, int dlSymbolOffset, int dlSubchannelOffset);
	virtual ~frame();

	int get_widthOfFrame(){return widthOfFrame_;}
	int get_heightOfFrame(){return heightOfFrame_;}

	void set_widthOfFrame(int width);
	void set_heightOfFrame(int height);

	int get_frameArea(){return frameArea_;}
	void set_frameArea(int width,int height);

	int get_restwidth(){return restwidth_;}
	void set_restwidth(int restwidth);

	float get_overheadarea(){return overhead_area;}
	void set_overheadarea(int overheadarea);

	int get_overheadX(){return overhead_x;}
	int get_overheadY(){return overhead_y;}
	void set_overheadX(int x);
	void set_overheadY(int y);

	float get_estimate_overhead(){return estimate_overhead;}
	void set_estimate_overhead(int estimateoverhead);

	int get_usedSlot(){return usedSlot_;}
	void set_usedSlot(int usedSlot);

	int get_noOfBurst(){return noOfBurst_;}
    int get_used_Area(){return usedArea_;}

    int get_waste_area(){return waste_area;}
    void set_waste_area(int wasteArea);

    vector<OscaBurst*> get_burstList(){return burstList_;}
    void addToBurstList(OscaBurst * burstObject);
    void displaynewburst(OscaBurst * burstObject);

    vector<freearea *> freeareaList;
    void addToFreeareaList(int xCorr, int yCorr, int width, int height);
    void deleteFromFreeareaList(vector<freearea *>*freeareaList,int i);
    void displayFreearea();
    bool get_collision(){return collision;}
    void set_collision(bool a);

private:

    int widthOfFrame_;
    int heightOfFrame_;
    int noOfBurst_;
    int frameArea_;
    int usedArea_;
    int usedSlot_;  //really total slot
    int waste_area;
    float overhead_area; //bits
    int overhead_x;
    int overhead_y;
    int restwidth_;
    bool collision;
    float estimate_overhead;
    vector<OscaBurst *> burstList_;


};

#endif /* FRAME_H_ */
