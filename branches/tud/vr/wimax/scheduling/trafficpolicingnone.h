/*
 * trafficpolicingnone.h
 *
 *  Created on: 05.09.2010
 *      Author: richter
 */

#ifndef TRAFFICPOLICINGNONE_H_
#define TRAFFICPOLICINGNONE_H_

#include "trafficpolicinginterface.h"

class TrafficPolicingNone: public TrafficPolicingInterface {
public:
	TrafficPolicingNone( double frameDuration);
	virtual ~TrafficPolicingNone();

    /*
     * Returns wantedMstrSize and wantedMrtrSize as guideline for the scheduling algorithm
     */
    virtual MrtrMstrPair_t getDataSizes(Connection * connection);

    /*
     * Occurred allocation is send back to the traffic policing algorithm to use this
     * values in the next call
     */
    virtual void updateAllocation(Connection *connection,u_int32_t realMstrSize,u_int32_t realMrtSize);

};

#endif /* TRAFFICPOLICINGNONE_H_ */
