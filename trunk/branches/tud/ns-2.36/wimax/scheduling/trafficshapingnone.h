/*
 * trafficshapingnone.h
 *
 *  Created on: 05.09.2010
 *      Author: richter
 */

#ifndef TRAFFICSHAPINGNONE_H_
#define TRAFFICSHAPINGNONE_H_

#include "trafficshapinginterface.h"

class TrafficShapingNone: public TrafficShapingInterface {
public:
	TrafficShapingNone( double frameDuration);
	virtual ~TrafficShapingNone();

    /*
     * Returns wantedMstrSize and wantedMrtrSize as guideline for the scheduling algorithm
     */
	virtual MrtrMstrPair_t getDataSizes(Connection * connection, u_int32_t queuePayloadSize);

    /*
     * Occurred allocation is send back to the traffic policing algorithm to use this
     * values in the next call
     */
    virtual void updateAllocation(Connection *connection,u_int32_t realMstrSize,u_int32_t realMrtSize);

};

#endif /* TRAFFICSHAPINGNONE_H_ */
