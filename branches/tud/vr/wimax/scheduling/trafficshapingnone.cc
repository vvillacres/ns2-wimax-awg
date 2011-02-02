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
MrtrMstrPair_t TrafficShapingNone::getDataSizes(Connection * connection)
{
    MrtrMstrPair_t mrtrMstrPair;
    int wantedSize = connection->queuePayloadLength();
    if ( connection->getFragmentBytes() > 0 ) {
    	wantedSize = wantedSize - connection->getFragmentBytes() - HDR_MAC802_16_FRAGSUB_SIZE;
    }
    mrtrMstrPair.first = u_int32_t ( wantedSize);
    mrtrMstrPair.second = u_int32_t ( wantedSize);

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
