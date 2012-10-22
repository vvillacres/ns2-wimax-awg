/*
 * schedulingproportionalfairul.h
 *
 *  Created on: 10.08.2010
 *      Author: richter
 */

#ifndef SCHEDULINGPROPORTIONALFAIRUL_H_
#define SCHEDULINGPROPORTIONALFAIRUL_H_

#include "schedulingalgointerface.h"

class Connection;
class VirtualAllocation;

class SchedulingProportionalFairUl: public SchedulingAlgoInterface {
public:
	SchedulingProportionalFairUl();

	virtual ~SchedulingProportionalFairUl();

	/*
	 * Schedules all data connection according to the proportional fair algorithm
	 */
	virtual void scheduleConnections( VirtualAllocation* virtualAllocation, int freeSlots);

private:

	Connection* nextConnectionPtr_;


};

#endif /* SCHEDULINGPROPORTIONALFAIRUL_H_ */
