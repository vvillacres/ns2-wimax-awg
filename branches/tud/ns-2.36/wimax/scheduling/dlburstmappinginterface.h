/*
 * dlburstmappinginterface.h
 *
 *  Created on: 25.04.2011
 *      Author: richter
 */

#ifndef DLBURSTMAPPINGINTERFACE_H_
#define DLBURSTMAPPINGINTERFACE_H_

#include "mac802_16pkt.h"

// Forward declarations
class VirtualAllocation;
class Mac802_16;
class BSScheduler;

class DlBurstMappingInterface {
public:
	DlBurstMappingInterface( Mac802_16 * mac, BSScheduler * bsScheduler);
	virtual ~DlBurstMappingInterface();

	/**
	 * Mapping of Allocations into Dowlink Bursts
	 */
	virtual mac802_16_dl_map_frame * mapDlBursts( mac802_16_dl_map_frame * dlMap, VirtualAllocation * virtualAlloc, int totalDlSubchannels, int totalDlSymbols, int dlSymbolOffset, int dlSubchannelOffset) = 0;

protected:

    /**
     * The Mac layer
     */
    Mac802_16 * mac_;

    /**
     * Callback Pointer to the BSScheduler object
     */
    BSScheduler * bsScheduler_;
};

#endif /* DLBURSTMAPPINGINTERFACE_H_ */
