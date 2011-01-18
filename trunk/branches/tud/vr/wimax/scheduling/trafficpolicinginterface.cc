/*
 * trafficpolicinginterface.cpp
 *
 *  Created on: 07.08.2010
 *      Author: richter
 */


#include "trafficpolicinginterface.h"
#include <stdio.h>

TrafficPolicingInterface::TrafficPolicingInterface(double frameDuration)
{
    /*
     * Set member variables
     */
    frameDuration_ = frameDuration;
}

TrafficPolicingInterface::~TrafficPolicingInterface()
{
    // Nothing to do
}

MrtrMstrPair_t TrafficPolicingInterface::getDataSizes(Connection* connection)
{
	printf("ERROR !!!!!!!!! \n");
	// overwritten by subclass
}


void TrafficPolicingInterface::updateAllocation(Connection* connection,u_int32_t realMstrSize,u_int32_t realMrtSize)
{
	// overwritten by subclass
}
