/*
 * dlburstmappingsimple.h
 *
 *  Created on: 25.04.2011
 *      Author: richter
 */

#ifndef DLBURSTMAPPINGSIMPLE_H_
#define DLBURSTMAPPINGSIMPLE_H_

#include "dlburstmappinginterface.h"

class DlBurstMappingSimple: public DlBurstMappingInterface {
public:
	DlBurstMappingSimple( Mac802_16 * mac, BSScheduler * bsScheduler);
	virtual ~DlBurstMappingSimple();

	/**
	 * Mapping of Allocations into Dowlink Bursts
	 */
	virtual mac802_16_dl_map_frame * mapDlBursts( mac802_16_dl_map_frame * dlMap, VirtualAllocation * virtualAlloc, int totalDlSubchannels, int totalDlSymbols, int dlSymbolOffset, int dlSubchannelOffset);
};

#endif /* DLBURSTMAPPINGSIMPLE_H_ */
