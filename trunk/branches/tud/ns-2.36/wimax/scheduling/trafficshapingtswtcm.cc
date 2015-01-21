/*
 * trafficshapingtswtcm.cpp
 *
 *  Created on: 07.09.2010
 *      Author: richter
 */

#include "trafficshapingtswtcm.h"
#include "serviceflowqosset.h"
#include "serviceflow.h"
#include "connection.h"

TrafficShapingTswTcm::TrafficShapingTswTcm( double frameDuration )  : TrafficShapingInterface( frameDuration)
{
    // Nothing to do

}

TrafficShapingTswTcm::~TrafficShapingTswTcm()
{
    // Nothing to do
}

/*
 * Returns wantedMstrSize and wantedMrtrSize as guideline for the scheduling algorithm
 * through call by reference
 */
MrtrMstrPair_t TrafficShapingTswTcm::getDataSizes(Connection *connection, u_int32_t queuePayloadSize)
{
    LastAllocationSizeIt_t mapIterator;

    // get the CID of the current connection
    int currentCid = connection->get_cid();

    // load QoS Parameter set
    ServiceFlowQosSet* sfQosSet = connection->getServiceFlow()->getQosSet();

    // search for an map entry of the current cid
    mapIterator = mapLastAllocationSize_.find(currentCid);

    u_int32_t wantedMrtrSize;
    u_int32_t wantedMstrSize;

    if ( mapIterator == mapLastAllocationSize_.end()) {
        // there is no entry for this CID -> calculate default sizes

        // round up the mrtr size as it is a guaranteed rate
        wantedMrtrSize = u_int32_t( ceil( double(sfQosSet->getMinReservedTrafficRate()) * frameDuration_ / 8.0 ) );

        // round down the mstr rate as it is the upper border
        wantedMstrSize = u_int32_t( floor( double(sfQosSet->getMaxSustainedTrafficRate()) * frameDuration_/ 8.0 ) );

        // fix rounding errors
        if ( wantedMstrSize < wantedMrtrSize) {
        	wantedMstrSize = wantedMrtrSize;
        }

    } else {
    	// there is an entry for this CID

    	// load entries from the map
        u_int32_t lastMrtr = mapIterator->second.lastMrtr;
        u_int32_t lastMstr = mapIterator->second.lastMstr;
        double lastTimeStamp = mapIterator->second.timeStamp;

        // get time base converted in seconds
        double timeBase = double(sfQosSet->getTimeBase()) * 1e-3;

        // calculate time since last call
        double timeDelta = timeBase - (NOW - lastTimeStamp);
        if ( timeDelta < 0) {
        	// if this functions was not called for a longer time than timeBase negative values can occure
        	timeDelta = 0.0;
        }
        // debug timeDelta shall never exceed timeBate
        assert( timeBase > timeDelta);

        /*
         * S_max <Byte> = T_b <second> * MRTR <Bit/second> - (T_b <second> - (current_Time - last_Time) <second> * last_MRTR <Bit/second> )
         */
        wantedMrtrSize = u_int32_t( ceil( ((timeBase * sfQosSet->getMinReservedTrafficRate()) - ( timeDelta * lastMrtr)) / 8.0) );

        /*
         * S_max <Byte> = T_b <second> * MSTR <Bit/second> - (T_b <second> - (current_Time - last_Time) <second> * last_MRTR <Bit/second> )
         */
        wantedMstrSize = u_int32_t( floor( ((timeBase * sfQosSet->getMaxSustainedTrafficRate()) - ( timeDelta * lastMstr)) / 8.0) );

        // fix possible rounding errors
        if ( wantedMstrSize < wantedMrtrSize) {
        	wantedMstrSize = wantedMrtrSize;
        }
    }

    // get Maximum Traffic Burst Size for this connection
    u_int32_t maxTrafficBurst = sfQosSet->getMaxTrafficBurst();

    //
    if ( (connection->getFragmentBytes() > 0) && ( connection->queueLength() > 0) ) {
    	queuePayloadSize -= ( connection->getFragmentBytes() - HDR_MAC802_16_SIZE - HDR_MAC802_16_FRAGSUB_SIZE );
    }

    // min function Mrtr according to IEEE 802.16-2009
    wantedMrtrSize = MIN( wantedMrtrSize, maxTrafficBurst);
    wantedMrtrSize = MIN( wantedMrtrSize, queuePayloadSize);

    // min function for Mstr
    wantedMstrSize = MIN( wantedMstrSize, maxTrafficBurst);
    wantedMstrSize = MIN( wantedMstrSize, queuePayloadSize);

    MrtrMstrPair_t mrtrMstrPair;
    mrtrMstrPair.first = wantedMrtrSize;
    mrtrMstrPair.second = wantedMstrSize;

    return mrtrMstrPair;
}

/*
 * Occurred allocation is send back to the traffic policing algorithm to use this
 * values in the next call
 */
void TrafficShapingTswTcm::updateAllocation(Connection *con,u_int32_t realMrtrSize,u_int32_t realMstrSize)
{

    LastAllocationSizeIt_t mapIterator;

    // get the CID of the current connection
    int currentCid = con->get_cid();

    // get Windows Size < millisecond > from QoS Parameter Set and convert it in seconds
    double timeBase =  con->getServiceFlow()->getQosSet()->getTimeBase() * 1e-3 ;

    // find entry for the current connection
    mapIterator = mapLastAllocationSize_.find(currentCid);


    if ( mapIterator == mapLastAllocationSize_.end()) {
    	// no entry was found --> create new entry


        // Calculate Mrtr and Mstr which was achieved in this frame (Ak)
        u_int32_t currentMrtr = u_int32_t( ceil( double(realMrtrSize) * 8.0 / frameDuration_ ));
        u_int32_t currentMstr = u_int32_t( floor( double(realMstrSize) * 8.0 / frameDuration_ ));

        // fix possible rounding errors
        if ( currentMstr < currentMrtr) {
        	currentMstr = currentMrtr;
        }

        // create new map element
        LastAllocationSize lastAllocationSize;
        lastAllocationSize.lastMrtr = currentMrtr;
        lastAllocationSize.lastMstr = currentMstr;
        lastAllocationSize.timeStamp = NOW;

        // insert new map element
        mapLastAllocationSize_.insert( pair<int,LastAllocationSize>( currentCid, lastAllocationSize) );


    } else	{
    	// entry for the current connection found

        // get values
        u_int32_t lastMrtr = mapIterator->second.lastMrtr;
        u_int32_t lastMstr = mapIterator->second.lastMstr;

        // calculate time different since the last call (T_D)
        double timeDelta = NOW - mapIterator->second.timeStamp;


        /*--------------------------die aktuelle MRTR für die Info-Datenrate---------------------------------------------------------//
         * A_k <Bit/second> = ( T_b <second>_double * last_MRTR <Bit/s>_u_int32_t + last_DataSize <Bit>_u_int32_t ) / ( T_b <second>_double + nT_f <second > )
         */
        u_int32_t currentMrtr = u_int32_t( ceil(( lastMrtr * timeBase + realMrtrSize * 8.0 ) / ( timeBase + timeDelta )));
        /*--------------------------die aktuelle MSTR für die Info-Datenrate---------------------------------------------------------//
         * A_k <Bit/second> = ( T_b <second>_double * last_MSTR <Bit/s>_u_int32_t + last_DataSize <Bit>_u_int32_t ) / ( T_b <second> + T_f <second > )
         */
        u_int32_t currentMstr = u_int32_t( floor(( lastMstr * timeBase + realMstrSize * 8.0 ) / ( timeBase + timeDelta)));

        // fix possible rounding errors
        if ( currentMstr < currentMrtr) {
        	currentMstr = currentMrtr;
        }

        //  write new values to map of mstr and mrtr rate back to the map
        mapIterator->second.lastMrtr = currentMrtr;
        mapIterator->second.lastMstr = currentMstr;
        mapIterator->second.timeStamp = NOW;


    }
}
