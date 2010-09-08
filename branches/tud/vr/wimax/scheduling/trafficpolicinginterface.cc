/*
 * trafficpolicinginterface.cpp
 *
 *  Created on: 07.08.2010
 *      Author: richter
 */

#include "trafficpolicinginterface.h"

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
