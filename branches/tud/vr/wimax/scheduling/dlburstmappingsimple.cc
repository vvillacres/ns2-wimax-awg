/*
 * dlburstmappingsimple.cc
 *
 *  Created on: 25.04.2011
 *      Author: richter
 */

#include "dlburstmappingsimple.h"
#include "bsscheduler.h"
#include "virtualallocation.h"
#include "ofdmphy.h"
#include "framemap.h"
#include "burst.h"
#include "mac802_16.h"



DlBurstMappingSimple::DlBurstMappingSimple( Mac802_16 * mac, BSScheduler * bsScheduler) : DlBurstMappingInterface( mac, bsScheduler) {

}

DlBurstMappingSimple::~DlBurstMappingSimple() {
	// TODO Auto-generated destructor stub
}

/**
 * Mapping of Allocations into Dowlink Bursts
 */
mac802_16_dl_map_frame *
DlBurstMappingSimple::mapDlBursts( mac802_16_dl_map_frame * dlMap, VirtualAllocation * virtualAlloc, int totalDlSubchannels, int totalDlSymbols, int dlSymbolOffset, int dlSubchannelOffset)  {

    // map virtual allocations to DlMap
    dlMap->type = MAC_DL_MAP;
    dlMap->bsid = mac_->addr(); // its called in the mac_ object

    // for testing purpose
    int entireUsedSlots = 0;

    OFDMAPhy *phy = mac_->getPhy();
    FrameMap *map = mac_->getMap();

    // add broadcast burst
    int dlMapIeIndex = 0;
    mac802_16_dlmap_ie * dlMapIe = NULL;
    dlMapIe = &(dlMap->ies[dlMapIeIndex]);

    dlMapIe->cid = BROADCAST_CID;
    dlMapIe->n_cid = 1;
    dlMapIe->diuc = map->getDlSubframe()->getProfile (DIUC_PROFILE_2)->getIUC();
    dlMapIe->preamble = true;
    dlMapIe->start_time = dlSymbolOffset;
    dlMapIe->symbol_offset = dlSymbolOffset;
    dlMapIe->subchannel_offset = dlSubchannelOffset;
    dlMapIe->boosting = 0;
    dlMapIe->num_of_subchannels = virtualAlloc->getNbOfBroadcastSlots();
    dlMapIe->num_of_symbols = int( ceil( double( dlSubchannelOffset + virtualAlloc->getNbOfBroadcastSlots()) / totalDlSubchannels) * phy->getSlotLength(DL_));
    dlMapIe->repition_coding_indicator = 0;

    // update Symbol and Subchannel Offset
    dlSymbolOffset += int( floor( double( dlSubchannelOffset + virtualAlloc->getNbOfBroadcastSlots()) / totalDlSubchannels) * phy->getSlotLength(DL_));
    dlSubchannelOffset = ( dlSubchannelOffset + virtualAlloc->getNbOfBroadcastSlots()) % (totalDlSubchannels);

    // testing
    entireUsedSlots += virtualAlloc->getNbOfBroadcastSlots();

    // increase index
    dlMapIeIndex++;

    // get the first connection
    if ( virtualAlloc->firstConnectionEntry() )  {
        do {
            // get Connection
            Connection * currentCon = virtualAlloc->getConnection();
            int nbOfUsedSlots = virtualAlloc->getCurrentNbOfSlots();

            dlMapIe = &(dlMap->ies[dlMapIeIndex]);

            dlMapIe->cid = currentCon->get_cid();
            dlMapIe->n_cid = 1;
            dlMapIe->diuc = bsScheduler_->getDIUCProfile(mac_->getMap()->getDlSubframe()->getProfile(currentCon->getPeerNode()->getDIUC())->getEncoding());
            dlMapIe->preamble = false;
            dlMapIe->start_time = dlSymbolOffset;
            dlMapIe->symbol_offset = dlSymbolOffset;
            dlMapIe->subchannel_offset = dlSubchannelOffset;
            dlMapIe->boosting = 0;
            dlMapIe->num_of_subchannels = nbOfUsedSlots;
            dlMapIe->num_of_symbols = int( ceil( double( dlSubchannelOffset + nbOfUsedSlots) / totalDlSubchannels) * phy->getSlotLength(DL_));
            dlMapIe->repition_coding_indicator = 0;

            // update Symbol and Subchannel Offset
            dlSymbolOffset += int( floor( double( dlSubchannelOffset + nbOfUsedSlots) / totalDlSubchannels) * phy->getSlotLength(DL_));
            dlSubchannelOffset = ( dlSubchannelOffset + nbOfUsedSlots) % (totalDlSubchannels);

            // update traffic policing
            if ( currentCon->getType() == CONN_DATA) {
            //	trafficShapingAlgorithm_->updateAllocation( currentCon, virtualAlloc->getCurrentMrtrPayload(), virtualAlloc->getCurrentMstrPayload());
            }

            // testing
            entireUsedSlots += nbOfUsedSlots;


            // increase index
            dlMapIeIndex++;
        } while (virtualAlloc->nextConnectionEntry());
        // go to the next connection if existing
    }
    // set the number of information elements
    dlMap->nb_ies = dlMapIeIndex;

    printf("Number of Connection Scheduled %d , Number of Slots used %d \n", dlMapIeIndex, entireUsedSlots);



	return dlMap;
}
