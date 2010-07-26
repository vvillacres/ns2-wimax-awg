/*
 * sumDequetswTCM.h
 *
 *  Created on: 19.07.2010
 *      Author: tung
 */

#ifndef SUMDEQUETSWTCM_H_
#define SUMDEQUETSWTCM_H_


#include "scheduler.h"
#include "connection.h"
#include "mac802_16.h"
#include <map>
#include <deque>
using namespace std;

//MAKRO MIN
#define MIN(a,b) ( (a) <= (b) ? (a) : (b) )
/*typedef enum { green, yellow, red }Data_color_;*/
// ---------------------Rahmendauer beträgt 5ms-----------------------------------
#define FRAMEDURATION 0.005 // in second

//----------------Prediction Data-------------------------------
struct AllocationElement{
			u_int32_t mrtrSize;
			u_int32_t mstrSize;
			double timeStamp;
		};
struct AllocationList{
			u_int32_t sumMrtrSize;
			u_int32_t sumMstrSize;
			deque <AllocationElement> * ptrDequeAllocationElement;
		};
//-------------------------deque < TrafficRate > dequeTrafficRate-----------------------------------------;
//----Abbildungen von cid auf Prediction Data heiß  LastAllocationSize(mrtrSize; mstrSize;timeStamp;)------------

typedef map< int, AllocationList > MapLastAllocationList_t;
typedef MapLastAllocationSize_t::iterator AllocationListIt_t;


class sumDequetswTCM {
public:
	sumDequetswTCM();
	virtual ~sumDequetswTCM();

    //------------------Gettting the Prediction Ddata---------------------
    void getDataSize(Connection *con, u_int32_t &mstrSize, u_int32_t &mrtrSize);

    //------------------Update the Allocation -----------------------
    void updateAllocation(Connection *con,u_int32_t mstrSize,u_int32_t mrtSize);

    //--------------------------Clear the Connection--------------------
    void clearConnection(Connection *con);

    //------------------Setting the Frame Duration----------------------
    void setFrameDuration(double frameDuration) {
    	frameDuration_ = frameDuration;}

private :
    // Kommentar ???
    MapLastAllocationSize_t mapLastAllocationSize_ ;
    double frameDuration_;

};

#endif /* SUMDEQUETSWTCM_H_ */
