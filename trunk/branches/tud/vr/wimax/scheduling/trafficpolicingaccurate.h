/*
 * trafficpolicingaccurate.h
 *
 *  Created on: 07.09.2010
 *      Author: richter
 */

#ifndef TRAFFICPOLICINGACCURATE_H_
#define TRAFFICPOLICINGACCURATE_H_

#include "trafficpolicinginterface.h"
#include <map>
#include <deque>
using namespace std;

//MAKRO MIN
#define MIN(a,b) ( (a) <= (b) ? (a) : (b) )

//----------------Prediction Data-------------------------------
struct AllocationElement {
    u_int32_t mrtrSize;
    u_int32_t mstrSize;
    double timeStamp;
};
struct AllocationList {
    u_int32_t sumMrtrSize;
    u_int32_t sumMstrSize;
    deque <AllocationElement> * ptrDequeAllocationElement;
};
//-------------------------deque < TrafficRate > dequeTrafficRate-----------------------------------------;
//----Abbildungen von cid auf Prediction Data heiß  LastAllocationSize(mrtrSize; mstrSize;timeStamp;)------------

typedef map< int, AllocationList > MapLastAllocationSize_t;
typedef MapLastAllocationSize_t::iterator AllocationSizeIt_t;

class Connection;

class TrafficPolicingAccurate: public TrafficPolicingInterface
{
public:
    TrafficPolicingAccurate(double frameDuration_);
    virtual ~TrafficPolicingAccurate();

    /*
     * Calculate predicted mrtr and msrt sizes
     */
    virtual void getDataSize(Connection *con, u_int32_t &wantedMstrSize, u_int32_t &wantedMrtrSize);

    /*
     * Sends occurred allocation back to traffic policing
     */
    virtual void updateAllocation(Connection *con,u_int32_t mstrSize,u_int32_t mrtSize);
private :
    /*
     * Saves the last occurred allocations
     */
    MapLastAllocationSize_t mapLastAllocationSize_ ;
};

#endif /* TRAFFICPOLICINGACCURATE_H_ */
