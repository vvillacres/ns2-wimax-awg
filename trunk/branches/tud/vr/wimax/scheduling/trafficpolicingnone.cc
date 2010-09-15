/*
 * trafficpolicingnone.cc
 *
 *  Created on: 05.09.2010
 *      Author: richter
 */

#include "trafficpolicingnone.h"
#include "connection.h"

TrafficPolicingNone::TrafficPolicingNone(double frameDuration) : TrafficPolicingInterface( frameDuration) {
	// Nothing to do

}

TrafficPolicingNone::~TrafficPolicingNone() {
	// Nothing to do
}

/*
 * Returns wantedMstrSize and wantedMrtrSize as guideline for the scheduling algorithm
 */
MrtrMstrPair_t TrafficPolicingNone::getDataSizes(Connection * connection)
{
    MrtrMstrPair_t mrtrMstrPair;
    mrtrMstrPair.first = u_int32_t ( connection->queueByteLength());
    mrtrMstrPair.second = u_int32_t ( connection->queueByteLength());

    return mrtrMstrPair;

}

/*
 * Occurred allocation is send back to the traffic policing algorithm to use this
 * values in the next call
 */
void TrafficPolicingNone::updateAllocation(Connection *connection,u_int32_t realMstrSize,u_int32_t realMrtSize)
{
	// Nothing do to
	// Feadback is not needed
}
