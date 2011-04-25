/*
 * dlburstmappinginterface.cc
 *
 *  Created on: 25.04.2011
 *      Author: richter
 */

#include "dlburstmappinginterface.h"

DlBurstMappingInterface::DlBurstMappingInterface( Mac802_16 * mac, BSScheduler * bsScheduler) {
	mac_ = mac;
	bsScheduler_ = bsScheduler;

}

DlBurstMappingInterface::~DlBurstMappingInterface() {
	// TODO Auto-generated destructor stub
}
