/*
 * schedulingalgodualequalfill.h
 *
 *  Created on: 08.08.2010
 *      Author: richter
 */

#ifndef SCHEDULINGALGODUALEQUALFILL_H_
#define SCHEDULINGALGODUALEQUALFILL_H_

#include "schedulingalgointerface.h"

class Connection;
class VirtualAllocation;

class SchedulingAlgoDualEqualFill: public SchedulingAlgoInterface {
public:
	SchedulingAlgoDualEqualFill();

	virtual ~SchedulingAlgoDualEqualFill();

	/*
	 * Schedules all data connection according to the dual equal fill algorithm
	 */
	virtual void scheduleConnections( VirtualAllocation* virtualAllocation, int freeSlots);

private:

	Connection* lastConnectionPtr_;


};

#endif /* SCHEDULINGALGODUALEQUALFILL_H_ */
