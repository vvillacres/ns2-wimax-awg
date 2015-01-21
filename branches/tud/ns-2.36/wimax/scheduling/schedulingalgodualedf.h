/*
 * schedulingalgodualedf.h
 *
 *  Created on: 08.08.2010
 *      Author: richter
 */

#ifndef SCHEDULINGALGODUALEDF_H_
#define SCHEDULINGALGODUALEDF_H_

#include "schedulingalgointerface.h"

class Connection;
class VirtualAllocation;

class SchedulingAlgoDualEdf: public SchedulingAlgoInterface {
public:
	SchedulingAlgoDualEdf();

	virtual ~SchedulingAlgoDualEdf();

	/*
	 * Schedules all data connection according to the dual earliest deadline first algorithm
	 */
	virtual void scheduleConnections( VirtualAllocation* virtualAllocation, int freeSlots);

private:

	Connection * lastConnectionPtr_;

};

#endif /* SCHEDULINGALGODUALEDF_H_ */
