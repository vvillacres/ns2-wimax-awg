/*
 * schedulingalgointerface.cc
 *
 *  Created on: 08.08.2010
 *      Author: richter
 */

#include "schedulingalgointerface.h"

SchedulingAlgoInterface::SchedulingAlgoInterface()
{
	// init of variabels
	totalNbOfSlots_ = 400.0;
	usedMrtrSlots_ = 0.0;
	usedMstrSlots_ = 0.0;
	movingAverageFactor_ = 0.1;

}

SchedulingAlgoInterface::~SchedulingAlgoInterface()
{
	// Nothing to do
}

frameUsageStat_t SchedulingAlgoInterface::getUsageStatistic()
{
	frameUsageStat_t frameUsage;
	frameUsage.totalNbOfSlots = totalNbOfSlots_;
	frameUsage.usedMrtrSlots = usedMrtrSlots_;
	frameUsage.usedMstrSlots = usedMstrSlots_;

	return frameUsage;

}
