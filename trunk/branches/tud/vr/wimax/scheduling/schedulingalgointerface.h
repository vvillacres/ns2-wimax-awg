/*
 * schedulingalgointerface.h
 *
 *	Abstract interface for all scheduling algorithms
 *	This implementation follows the Strategy design pattern,
 *	to allow different dynamically configured behaviors
 *
 *  Created on: 08.08.2010
 *      Author: richter
 */

#ifndef SCHEDULINGALGOINTERFACE_H_
#define SCHEDULINGALGOINTERFACE_H_

class VirtualAllocation;

class SchedulingAlgoInterface {
public:

	/*
	 * Calls the scheduling algorithm to schedule all data connection
	 * Results are
	 */
	virtual void scheduleConnections( VirtualAllocation* virtualAllocation, int freeSlots) = 0;

	virtual ~SchedulingAlgoInterface();


protected:

	SchedulingAlgoInterface();


};

#endif /* SCHEDULINGALGOINTERFACE_H_ */
