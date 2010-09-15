/*
 * schedulingalgoproportionalfair.h
 *
 *  Created on: 10.08.2010
 *      Author: richter
 */

#ifndef SCHEDULINGALGOPROPORTIONALFAIR_H_
#define SCHEDULINGALGOPROPORTIONALFAIR_H_

#include "schedulingalgointerface.h"

class Connection;
class VirtualAllocation;

class SchedulingAlgoProportionalFair: public SchedulingAlgoInterface {
public:
	SchedulingAlgoProportionalFair();

	virtual ~SchedulingAlgoProportionalFair();

	/*
	 * Schedules all data connection according to the proportional fair algorithm
	 */
	virtual void scheduleConnections( VirtualAllocation* virtualAllocation, int freeSlots);

private:

	Connection* lastConnectionPtr_;


};

#endif /* SCHEDULINGALGOPROPORTIONALFAIR_H_ */
