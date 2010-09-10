/*
 * trafficpolicinginterface.h
 *
 * Defines a common Interface for different Traffic Policing Algorithms.
 *
 * These algorithms have to calculate the wantedMstrSize and the wantedMrtrSize
 * to fulfill the QoS Parameters Maximum Sustained Traffic Rate and Minimum Reserved Traffic Rate.
 *
 * This implementation follows the "Strategy Design Pattern".
 *
 *  Created on: 07.08.2010
 *      Author: richter
 */

#ifndef TRAFFICPOLICINGINTERFACE_H_
#define TRAFFICPOLICINGINTERFACE_H_

#include <sys/types.h>
#include <utility>

typedef std::pair< u_int32_t, u_int32_t> MrtrMstrPair_t;

class Connection;

class TrafficPolicingInterface
{
public:

    /*
     * Returns wantedMstrSize and wantedMrtrSize as guideline for the scheduling algorithm
     * through call by reference
     */
    virtual MrtrMstrPair_t getDataSizes(Connection * connection);

    /*
     * Occurred allocation is send back to the traffic policing algorithm to use this
     * values in the next call
     */
    virtual void updateAllocation(Connection *connection,u_int32_t realMstrSize,u_int32_t realMrtSize);

    virtual ~TrafficPolicingInterface();


protected:

    TrafficPolicingInterface(double frameDuration);


    /*
     * Saves the frame duration for usage in the child classes
     */
    double frameDuration_;


};

#endif /* TRAFFICPOLICINGINTERFACE_H_ */
