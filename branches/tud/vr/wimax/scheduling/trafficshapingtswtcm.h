/*
 * trafficshapingtswtcm.h
 *
 *  Created on: 07.09.2010
 *      Author: tung & richter
 */

#ifndef TRAFFICSHAPINGTSWTCM_H_
#define TRAFFICSHAPINGTSWTCM_H_

#include "trafficshapinginterface.h"
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

class TrafficShapingTswTcm: public TrafficShapingInterface
{
public:
    TrafficShapingTswTcm( double frameDuration);
    virtual ~TrafficShapingTswTcm();

    /*
     * Returns wantedMstrSize and wantedMrtrSize as guideline for the scheduling algorithm
     * through call by reference
     */
    virtual MrtrMstrPair_t getDataSizes(Connection *connection);

    /*
     * Occurred allocation is send back to the traffic policing algorithm to use this
     * values in the next call
     */
    virtual void updateAllocation(Connection *connection,u_int32_t realMstrSize,u_int32_t realMrtSize);

private :
    /*
     * Saves values for usage in the next call
     */
    MapLastAllocationSize_t mapLastAllocationSize_ ;


};

#endif /* TRAFFICSHAPINGTSWTCM_H_ */
