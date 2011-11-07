/*
 * schedulingdualequalfillul.h
 *
 *  Created on: 08.08.2010
 *      Author: richter
 */

#ifndef SCHEDULINGDUALEQUALFILLUL_H_
#define SCHEDULINGDUALEQUALFILLUL_H_

#include "schedulingalgointerface.h"

class Connection;
class VirtualAllocation;

class SchedulingDualEqualFillUl: public SchedulingAlgoInterface {
public:
	SchedulingDualEqualFillUl();

	virtual ~SchedulingDualEqualFillUl();

	/*
	 * Schedules all data connection according to the dual equal fill algorithm
	 */
	virtual void scheduleConnections( VirtualAllocation* virtualAllocation, int freeSlots);

private:

	Connection* nextConnectionPtr_;


};

#endif /* SCHEDULINGDUALEQUALFILLUL_H_ */
