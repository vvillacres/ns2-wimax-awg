/*
 * frame.cpp
 *
 *  Created on: 2010-7-1
 *      Author: Administrator
 */
#include "frame.h"

frame::frame()
{
    widthOfFrame_ = 0;
    heightOfFrame_ = 0;
    frameArea_ = 0;
    usedArea_ = 0;
    usedSlot_ = 0;
    waste_area =0;
    noOfBurst_ = 0;
    overhead_area = 4*48 + 104;  //bits
    overhead_x = 1;
    overhead_y = 4;
    restwidth_ = 13;
    collision = false;
    estimate_overhead =0;
}

frame::frame(int width,int height, int dlSymbolOffset, int dlSubchannelOffset )
{
	widthOfFrame_ = width;
	heightOfFrame_ = height;
	frameArea_ = width * height;
	usedArea_ = 0;
	usedSlot_ = 0;
	waste_area =0;
	noOfBurst_ = 0;
	  overhead_area = 4*48 + 104; //bits
	    overhead_x = dlSymbolOffset;
	    overhead_y = dlSubchannelOffset;
	    restwidth_ = width - dlSymbolOffset;
	    collision = false;
	    estimate_overhead =0;
}

frame::~frame()
{

	while (!burstList_.empty())
	{
		delete burstList_.back();
		burstList_.pop_back();
	}

	while (!freeareaList.empty())
	{
		delete freeareaList.back();
		freeareaList.pop_back();
	}
}

void frame::set_widthOfFrame(int width)
{
	widthOfFrame_ = width;
	frameArea_ = width * heightOfFrame_;
}

void frame::set_heightOfFrame(int height)
{
	heightOfFrame_ = height;
	frameArea_ = widthOfFrame_ * height;
}

void frame::set_frameArea(int width, int height)
{
	widthOfFrame_ = width;
	heightOfFrame_ = height;
	frameArea_ = width * height;
}

void frame::set_estimate_overhead(int estimateoverhead)
{
	estimate_overhead = estimateoverhead;
}

void frame::set_restwidth(int restwidth)
{
   restwidth_ = restwidth;
}

void frame::set_overheadarea(int overheadarea)
{
	overhead_area = overheadarea;
}

void frame::set_overheadX(int x)
{
	overhead_x = x;
}

void frame::set_overheadY(int y)
{
	overhead_y = y;
}

void frame::set_usedSlot(int usedSlot)
{
	usedSlot_ = usedSlot;
}

void frame::set_waste_area(int wasteArea)
{
	waste_area += wasteArea;
}

void frame::set_collision(bool a)
{
     collision =a;
}

void frame::addToBurstList(OscaBurst * burstObject)
{
	burstList_.push_back(burstObject);
	usedArea_ += burstList_.back()->get_area();
	usedSlot_ += burstList_.back()->get_slot();
	noOfBurst_ = noOfBurst_ + 1;
}

void frame::displaynewburst(OscaBurst * burstObject){
	cout<<"---------------new burst:----------------"<<endl;
	cout<<"(X,Y)="<<burstList_.back()->get_xCorr()<<","<<burstList_.back()->get_yCorr()<<"    ";
	cout<<"(width,height)="<<burstList_.back()->get_width()<<","<<burstList_.back()->get_height()<<"    ";
	cout<<"area="<<burstList_.back()->get_area()<<"    "<<endl;
	cout<<"this burst: "<<endl; burstList_.back()->displayBurst();
	cout<<"in this Frame have "<<get_restwidth()<<" rest width"<<endl;
	cout<<"in this Frame have "<<get_overheadarea()<<" overhead used"<<endl;
	cout<<"overhead x= "<<get_overheadX()<<" ,overhead y="<<get_overheadY()<<endl;
	cout<<"over location(waste) in this frame= "<<get_waste_area()<<endl;
	displayFreearea();
	//cout<<"---------------end this burst------------- "<<endl;

}

void frame::addToFreeareaList(int xCorr, int yCorr, int width, int height)
{
	freeareaList.push_back(new freearea(xCorr, yCorr, width, height));

}

void frame::deleteFromFreeareaList(vector<freearea *>*freeareaList,int i)
{

	delete freeareaList->at(i);
	freeareaList->erase(freeareaList->begin() + i);
}

void frame::displayFreearea()
{
	    cout<<"Free area: "<<endl;
	    unsigned int i=0;
	    for (i=0;i != freeareaList.size();i++)
	    {
	    	cout<<"(x,y) = "<<freeareaList.at(i)->get_x()<<","<<freeareaList.at(i)->get_y()<<" ,width="<<freeareaList.at(i)->get_width()<<" ,height="<<freeareaList.at(i)->get_height()<<" ,free area="<<freeareaList.at(i)->get_area()<<endl;
	    }
}






