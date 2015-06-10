/*
 * trafficshapingmsadda.h
 *
 *  Created on: 01.06.2015
 *      Author: richter
 */

#ifndef TRAFFICSHAPINGMSADAA_H_
#define TRAFFICSHAPINGMSADAA_H_

#include "trafficshapinginterface.h"
#include <map>
#include <deque>
using namespace std;




//----------------Prediction Data-------------------------------

struct LastBucketSize {
    int lastCirToken;
    int lastEirToken;
    double   timeStamp;
};


typedef map< int, LastBucketSize > MapLastBucketSize_t;
typedef MapLastBucketSize_t::iterator LastBucketSizeIt_t;

class TrafficShapingMsadaa: public TrafficShapingInterface
{
public:
    TrafficShapingMsadaa( double frameDuration);
    virtual ~TrafficShapingMsadaa();

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
