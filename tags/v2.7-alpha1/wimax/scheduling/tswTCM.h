/*
 * tswTCM.h
 *                      nt@tud 07-2010
 *  Created on: 08.07.2010
 *      Author: tung
 */

#ifndef TSWTCM_H_
#define TSWTCM_H_

#include "serviceflowqosset.h"
#include "scheduler.h"
#include "connection.h"
#include <map>
#include <deque>
using namespace std;

//MAKRO MIN
#define MIN(a,b) ( (a)<= (b) ? (a) : (b) )
/*typedef enum { green, yellow, red }Data_color_;*/
// ---------------------Rahmendauer beträgt 5ms-----------------------------------
#define FRAMEDURATION 0.005 // in second

//----------------Prediction Data-------------------------------

struct LastAllocationSize{
				u_int32_t lastMrtr;
				u_int32_t lastMstr;
				double   timeStamp;
				};
// deque < TrafficRate > dequeTrafficRate;
//----Abbildungen von cid auf Prediction Data heiß  LastAllocationSize(mrtrSize; mstrSize;timeStamp;)------------

typedef map< int, LastAllocationSize > MapLastAllocationSize_t;
typedef MapLastAllocationSize_t::iterator LastAllocationSizeIt_t;
//typedef pair < int,LastAllocationSize > pair_t;

class tswTCM {

public:
	tswTCM();

	tswTCM( double frameDuration);

    virtual ~tswTCM();

     //------------------Gettting the Prediction Ddata---------------------
    void getDataSize(Connection *con,u_int32_t &mstrSize,u_int32_t &mrtrSize);

    //------------------Update the Allocation -----------------------
    void updateAllocation(Connection *con,u_int32_t &mstrSize,u_int32_t &mrtSize);

    //--------------------------Clear the Connection--------------------
    void clearConnection(Connection *con);

    //------------------Setting the Frame Duration----------------------
    void setFrameDuration(double frameDuration) {
    	frameDuration_ = frameDuration;}
   // MapLastAllocationSize_t updatemapLastAllocationSize_;

private :

    MapLastAllocationSize_t mapLastAllocationSize_ ; // Ich bekomme ein MAP von BS.
    double frameDuration_;


        };

#endif /* TSWTCM_H_ */
