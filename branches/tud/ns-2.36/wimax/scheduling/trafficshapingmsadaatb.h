/*
 * trafficshapingmsadaatb.h
 *
 *  Created on: 01.06.2015
 *      Author: richter
 */

#ifndef TRAFFICSHAPINGMSADAATB_H_
#define TRAFFICSHAPINGMSADAATB_H_

#include "trafficshapinginterface.h"
#include "trafficshapingmsadaa.h"
#include <map>
#include <deque>
using namespace std;


class TrafficShapingMsadaaTb: public TrafficShapingInterface
{
public:
    TrafficShapingMsadaaTb( double frameDuration);
    virtual ~TrafficShapingMsadaaTb();

    /*
     * Returns wantedMstrSize and wantedMrtrSize as guideline for the scheduling algorithm
     * through call by reference
     */
    virtual MrtrMstrPair_t getDataSizes(Connection * connection, u_int32_t queuePayloadSize);

    /*
     * Occurred allocation is send back to the traffic policing algorithm to use this
     * values in the next call
     */
    virtual void updateAllocation(Connection *connection,u_int32_t realMrtrSize,u_int32_t realMstrSize);

private :
    /*
     * Saves values for usage in the next call
     */
    MapLastBucketSize_t mapLastBucketSize_ ;


};

#endif /* TRAFFICSHAPINGTSWTCM_H_ */
