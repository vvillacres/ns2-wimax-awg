/*
 * tswTCM.h
 *                      nt@tud 07-2010
 *  Created on: 08.07.2010
 *      Author: tung
 */

#ifndef TSWTCM_H_
#define TSWTCM_H_


#include "scheduler.h"
#include "connection.h"
#include "mac802_16.h"
#include <map>
using namespace std;

//MAKRO MIN
#define MIN(a,b) ( (a)<= (b) ? (a) : (b) )
/*typedef enum { green, yellow, red }Data_color_;*/
// ---------------------Rahmendauer beträgt 5ms-----------------------------------
#define FRAMEDURATION 0.005

//----------------Prediction Data-------------------------------

struct LastAllocationSize{
				u_int32_t mrtrSize;
				u_int32_t mstrSize;
				double timeStamp;
};
//----Abbildungen von cid auf Prediction Data heiß  LastAllocationSize(mrtrSize; mstrSize;timeStamp;)------------

typedef map< int, LastAllocationSize > MapLastAllocationSize_t;
typedef MapLastAllocationSize_t::iterator LastAllocationSizeIt_t;


class tswTCM {

public:
	 tswTCM();

	 tswTCM( double frameDuration);

     virtual ~tswTCM();

     //------------------Gettting the Prediction Ddata---------------------
    void getDataSize(Connection *con,u_int32_t &mstrsize,u_int32_t &mrtsize);

    //------------------Update the Allocation -----------------------
    void UpdateAllocation(Connection *con,u_int32_t &mstrsize,u_int32_t &mrtsize);

    //--------------------------Clear the Connection--------------------
    void ClearConnection(Connection *con);

    //------------------Setting the Frame Duration----------------------
    void setFrameDuration(double frameDuration) {
    	frameDuration_ = frameDuration;}

private :
    //void interneMethode () ;
    MapLastAllocationSize_t mapLastAllocationSize_; // Ich bekomme ein MAP von BS.
    double frameDuration_;
        };

#endif /* TSWTCM_H_ */
