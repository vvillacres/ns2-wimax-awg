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

struct frameUsageStat_t {
    double totalNbOfSlots;
    double usedMrtrSlots;
    double usedMstrSlots;
};


class VirtualAllocation;

class SchedulingAlgoInterface {
public:

	SchedulingAlgoInterface();

	virtual ~SchedulingAlgoInterface();

	/*
	 * Calls the scheduling algorithm to schedule all data connection
	 * Results are
	 */
	virtual void scheduleConnections( VirtualAllocation* virtualAllocation, int freeSlots) = 0;

	/*
	 * Returns the moving averages of the frame utilization
	 */
	virtual frameUsageStat_t getUsageStatistic();



protected:

	/*
	 * Moving average of the frame size for data allocation
	 */
	double totalNbOfSlots_;

	/*
	 * Moving average of the number of slots used for MRTR based allocations
	 */
	double usedMrtrSlots_;

	/*
	 * Moving average of the number of slots used for MSTR based allocations
	 */
	double usedMstrSlots_;

	/*
	 * Factor that defines the influence of new values for the moving average
	 * Must be a value 0 < value < 1
	 */
	double movingAverageFactor_;
};

#endif /* SCHEDULINGALGOINTERFACE_H_ */
