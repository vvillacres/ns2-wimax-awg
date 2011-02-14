/*
 * trafficshapinginterface.cpp
 *
 *  Created on: 07.08.2010
 *      Author: richter
 */


#include "trafficshapinginterface.h"
#include <stdio.h>

TrafficShapingInterface::TrafficShapingInterface(double frameDuration)
{
    /*
     * Set member variables
     */
    frameDuration_ = frameDuration;
}

TrafficShapingInterface::~TrafficShapingInterface()
{
    // Nothing to do
}

MrtrMstrPair_t TrafficShapingInterface::getDataSizes(Connection* connection)
{
	printf("ERROR !!!!!!!!! \n");
	// overwritten by subclass
	MrtrMstrPair_t dummyPair;
	dummyPair.first = 0;
	dummyPair.second = 0;
	return dummyPair;
}


void TrafficShapingInterface::updateAllocation(Connection* connection,u_int32_t realMstrSize,u_int32_t realMrtSize)
{
	// overwritten by subclass
}
