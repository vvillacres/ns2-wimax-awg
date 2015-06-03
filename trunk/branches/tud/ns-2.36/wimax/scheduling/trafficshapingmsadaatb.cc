/*
 * trafficshapingmsadaatb.cpp
 *
 *  Created on: 01.06.2015
 *      Author: richter
 */

#include "trafficshapingmsadaatb.h"
#include "serviceflowqosset.h"
#include "serviceflow.h"
#include "connection.h"

//MAKRO MIN for usage by the traffic shaping algorithm
#define MINTS(a,b) ( (a)<= (b) ? (a) : (b) )

TrafficShapingMsadaaTb::TrafficShapingMsadaaTb( double frameDuration )  : TrafficShapingInterface( frameDuration)
{
    // Nothing to do

}

TrafficShapingMsadaaTb::~TrafficShapingMsadaaTb()
{
    // Nothing to do
}

/*
 * Returns wantedMstrSize and wantedMrtrSize as guideline for the scheduling algorithm
 * through call by reference
 */
MrtrMstrPair_t TrafficShapingMsadaaTb::getDataSizes(Connection *connection, u_int32_t queuePayloadSize)
{
    LastBucketSizeIt_t mapIterator;

    // get the CID of the current connection
    int currentCid = connection->get_cid();

    // load QoS Parameter set
    ServiceFlowQosSet* sfQosSet = connection->getServiceFlow()->getQosSet();

    // search for an map entry of the current cid
    mapIterator = mapLastBucketSize_.find(currentCid);

    u_int32_t wantedMrtrSize;
    u_int32_t wantedMstrSize;

    int cirToken;
    int eirToken;

    // calculate token bucket sizes in sec
    int timeBase = sfQosSet->getTimeBase()* 1e-3;

    int cirBucketSize = int( ceil(double(sfQosSet->getMinReservedTrafficRate()) / 8.0 * timeBase));
    int eirBucketSize = int( floor(double(sfQosSet->getMaxSustainedTrafficRate() - sfQosSet->getMinReservedTrafficRate()) / 8.0 * timeBase));



    if ( mapIterator == mapLastBucketSize_.end()) {
        // there is no entry for this CID ->  Calculated new Tokens

        // new cir tokens for the first time
    	cirToken = int( ceil( double(sfQosSet->getMinReservedTrafficRate()) * frameDuration_ / 8.0 ) );
    	cirToken = MINTS(cirToken, cirBucketSize);

        // new eir tokens for the first time
    	eirToken = int( floor( double(sfQosSet->getMaxSustainedTrafficRate() - sfQosSet->getMinReservedTrafficRate()) * frameDuration_/ 8.0 ) );
    	eirToken = MINTS(eirToken, eirBucketSize);

        // create new map element
        LastBucketSize lastBucketSize;
        lastBucketSize.lastCirToken = cirToken;
        lastBucketSize.lastEirToken = cirToken;
        lastBucketSize.timeStamp = NOW;

        // insert new map element
        mapLastBucketSize_.insert( pair<int,LastBucketSize>( currentCid, lastBucketSize) );


    } else {
    	// there is an entry for this CID

    	// load entries from the map
        cirToken = mapIterator->second.lastCirToken;
        eirToken = mapIterator->second.lastEirToken;

        // elapsed time
        double deltaTime = (NOW - mapIterator->second.timeStamp);

        // max mrtrToken
        cirToken += int( ceil( deltaTime * double(sfQosSet->getMinReservedTrafficRate() / 8.0)) );
        cirToken = MINTS(cirToken, cirBucketSize);

        // max mstrToken
        eirToken += int( floor( deltaTime * double(sfQosSet->getMaxSustainedTrafficRate() - sfQosSet->getMinReservedTrafficRate()) / 8.0));
        eirToken = MINTS(eirToken, eirBucketSize);

        //  write new token sizes into map
        mapIterator->second.lastCirToken = cirToken;
        mapIterator->second.lastEirToken = eirToken;
        mapIterator->second.timeStamp = NOW;

    }

    wantedMrtrSize = cirToken;
    wantedMstrSize = cirToken + eirToken;

    // get Maximum Traffic Burst Size for this connection
    u_int32_t maxTrafficBurst = sfQosSet->getMaxTrafficBurst();

    // min function Mrtr according to IEEE 802.16-2009
    wantedMrtrSize = MINTS( wantedMrtrSize, maxTrafficBurst);
    wantedMrtrSize = MINTS( wantedMrtrSize, queuePayloadSize);

    // min function for Mstr
    wantedMstrSize = MINTS( wantedMstrSize, maxTrafficBurst);
    wantedMstrSize = MINTS( wantedMstrSize, queuePayloadSize);

    MrtrMstrPair_t mrtrMstrPair;
    mrtrMstrPair.first = wantedMrtrSize;
    mrtrMstrPair.second = wantedMstrSize;

    return mrtrMstrPair;
}

/*
 * Occurred allocation is send back to the traffic policing algorithm to use this
 * values in the next call
 */
void TrafficShapingMsadaaTb::updateAllocation(Connection *con,u_int32_t realMrtrSize,u_int32_t realMstrSize)
{

    LastBucketSizeIt_t mapIterator;

    // get the CID of the current connection
    int currentCid = con->get_cid();

    // load QoS Parameter set
    ServiceFlowQosSet* sfQosSet = con->getServiceFlow()->getQosSet();

    // find entry for the current connection
    mapIterator = mapLastBucketSize_.find(currentCid);


    if ( mapIterator == mapLastBucketSize_.end()) {
    	// no entry was found --> should never happen

    	assert(false);

        // create new map element
        LastBucketSize lastBucketSize;
        lastBucketSize.lastCirToken = 0;
        lastBucketSize.lastEirToken = 0;
        lastBucketSize.timeStamp = NOW;

        // insert new map element
        mapLastBucketSize_.insert( pair<int,LastBucketSize>( currentCid, lastBucketSize) );


    } else	{
    	// entry for the current connection found
    	assert( realMrtrSize <= realMstrSize);

        //  write new values to map of mstr and mrtr rate back to the map
        mapIterator->second.lastCirToken -= realMrtrSize;
        mapIterator->second.lastEirToken -= (realMstrSize - realMrtrSize);


    }
}
