/*
 * trafficshapingnone.cc
 *
 *  Created on: 05.09.2010
 *      Author: richter
 */

#include "trafficshapingnone.h"
#include "connection.h"

TrafficShapingNone::TrafficShapingNone(double frameDuration) : TrafficShapingInterface( frameDuration) {
	// Nothing to do

}

TrafficShapingNone::~TrafficShapingNone() {
	// Nothing to do
}

/*
 * Returns wantedMstrSize and wantedMrtrSize as guideline for the scheduling algorithm
 */
MrtrMstrPair_t TrafficShapingNone::getDataSizes(Connection * connection, u_int32_t queuePayloadSize)
{
    MrtrMstrPair_t mrtrMstrPair;
    u_int32_t wantedMstrSize = queuePayloadSize;
    if ( connection->getFragmentBytes() > 0 ) {
    	wantedMstrSize = wantedMstrSize - u_int32_t( connection->getFragmentBytes() - HDR_MAC802_16_FRAGSUB_SIZE);
    }

    // QoS Parameter auslesen
    ServiceFlowQosSet* sfQosSet = connection->getServiceFlow()->getQosSet();
    assert( sfQosSet->getMinReservedTrafficRate() <= sfQosSet->getMaxSustainedTrafficRate());

    // assign size in the ratio of the MRTR / MSTR QoS parameters
    u_int32_t mrtrSize = int( ceil( double(sfQosSet->getMinReservedTrafficRate()) * frameDuration_ / 8.0 ) );
    u_int32_t wantedMrtrSize = u_int32_t ( ceil( wantedMstrSize * ( double(sfQosSet->getMinReservedTrafficRate()) / double(sfQosSet->getMaxSustainedTrafficRate())) )) ;

    // check if the whole queue belongs to MRTR demands
    if ( wantedMstrSize < mrtrSize) {
    	wantedMrtrSize = wantedMstrSize;
    }

    mrtrMstrPair.first = wantedMrtrSize;
    mrtrMstrPair.second = wantedMstrSize;

    return mrtrMstrPair;

}

/*
 * Occurred allocation is send back to the traffic policing algorithm to use this
 * values in the next call
 */
void TrafficShapingNone::updateAllocation(Connection *connection,u_int32_t realMstrSize,u_int32_t realMrtSize)
{
	// Nothing do to
	// Feadback is not needed
}
