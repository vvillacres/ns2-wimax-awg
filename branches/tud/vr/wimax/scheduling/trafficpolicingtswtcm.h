/*
 * trafficpolicingtswtcm.h
 *
 *  Created on: 07.09.2010
 *      Author: tung & richter
 */

#ifndef TRAFFICPOLICINGTSWTCM_H_
#define TRAFFICPOLICINGTSWTCM_H_

#include "trafficpolicinginterface.h",
#include "wimaxscheduler.h"
#include "connection.h"
#include <map>
#include <deque>
using namespace std;


//MAKRO MIN
#define MIN(a,b) ( (a)<= (b) ? (a) : (b) )

//----------------Prediction Data-------------------------------

struct LastAllocationSize {
    u_int32_t lastMrtr;
    u_int32_t lastMstr;
    double   timeStamp;
};
// deque < TrafficRate > dequeTrafficRate;
//----Abbildungen von cid auf Prediction Data heiß  LastAllocationSize(mrtrSize; mstrSize;timeStamp;)------------

typedef map< int, LastAllocationSize > MapLastAllocationSize_t;
typedef MapLastAllocationSize_t::iterator LastAllocationSizeIt_t;

class TrafficPolicingTswTcm: public TrafficPolicingInterface
{
public:
    TrafficPolicingTswTcm( double frameDuration);
    virtual ~TrafficPolicingTswTcm();

    /*
     * Returns wantedMstrSize and wantedMrtrSize as guideline for the scheduling algorithm
     * through call by reference
     */
    virtual void getDataSizes(Connection *con, u_int32_t &wantedMstrSize, u_int32_t &wantedMrtrSize);

    /*
     * Occurred allocation is send back to the traffic policing algorithm to use this
     * values in the next call
     */
    virtual void updateAllocation(Connection *con,u_int32_t realMstrSize,u_int32_t realMrtSize);

private :
    /*
     * Saves values for usage in the next call
     */
    MapLastAllocationSize_t mapLastAllocationSize_ ;


};

#endif /* TRAFFICPOLICINGTSWTCM_H_ */
