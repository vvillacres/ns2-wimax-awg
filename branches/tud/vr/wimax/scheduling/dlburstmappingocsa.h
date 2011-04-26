/*
 * dlburstmappingocsa.h
 *
 *  Created on: 26.04.2011
 *      Author: richter
 */

#ifndef DLBURSTMAPPINGOCSA_H_
#define DLBURSTMAPPINGOCSA_H_

#include "dlburstmappinginterface.h"

class DlBurstMappingOcsa: public DlBurstMappingInterface {
public:
	DlBurstMappingOcsa( Mac802_16 * mac, BSScheduler * bsScheduler);
	virtual ~DlBurstMappingOcsa();


	/**
	 * Mapping of Allocations into Dowlink Bursts
	 */
	virtual mac802_16_dl_map_frame * mapDlBursts( mac802_16_dl_map_frame * dlMap, VirtualAllocation * virtualAlloc, int totalDlSubchannels, int totalDlSymbols, int dlSymbolOffset, int dlSubchannelOffset);
};

#endif /* DLBURSTMAPPINGOCSA_H_ */
