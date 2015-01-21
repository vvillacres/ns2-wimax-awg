/*
 * dlburstmappingocsa.cc
 *
 *  Created on: 26.04.2011
 *      Author: richter
 */

#include "dlburstmappingocsa.h"
#include "ocsaalgorithm.h"

#include "bsscheduler.h"
#include "virtualallocation.h"
#include "ofdmphy.h"
#include "framemap.h"
#include "burst.h"
#include "mac802_16.h"

#include "oscaburst.h"
#include "anfrage.h"
#include "frame.h"

DlBurstMappingOcsa::DlBurstMappingOcsa( Mac802_16 * mac, BSScheduler * bsScheduler) : DlBurstMappingInterface( mac, bsScheduler) {

}

DlBurstMappingOcsa::~DlBurstMappingOcsa() {
	// TODO Auto-generated destructor stub
}

/**
 * Mapping of Allocations into Dowlink Bursts
 */
mac802_16_dl_map_frame *
DlBurstMappingOcsa::mapDlBursts( mac802_16_dl_map_frame * dlMap, VirtualAllocation * virtualAlloc, int totalDlSubchannels, int totalDlSymbols, int dlSymbolOffset, int dlSubchannelOffset)  {

    // map virtual allocations to DlMap
    dlMap->type = MAC_DL_MAP;
    dlMap->bsid = mac_->addr(); // its called in the mac_ object


	OFDMAPhy *phy = mac_->getPhy();
	FrameMap *map = mac_->getMap();

	// add broadcast burst
	int dlMapIeIndex = 0;
	mac802_16_dlmap_ie * dlMapIe = NULL;
	dlMapIe = &(dlMap->ies[dlMapIeIndex]);


	// fill Anfrage Liste
	vector<anfrage *> anfrageList;

    frame * frameObject = NULL;

    if ( virtualAlloc->firstConnectionEntry()) {

    	do {

    		Connection * currentCon = virtualAlloc->getConnection();
    		printf("Demand of Connection %d with %d Slots and DIUC %d \n", currentCon->get_cid(), virtualAlloc->getCurrentNbOfSlots(),
    				bsScheduler_->getDIUCProfile(mac_->getMap()->getDlSubframe()->getProfile(currentCon->getPeerNode()->getDIUC())->getEncoding()));
    		if ( virtualAlloc->getCurrentNbOfSlots() > 0) {
    			anfrageList.push_back( new anfrage( currentCon->get_cid(), currentCon, virtualAlloc->getCurrentNbOfSlots(),
    					bsScheduler_->getDIUCProfile(mac_->getMap()->getDlSubframe()->getProfile(currentCon->getPeerNode()->getDIUC())->getEncoding()) ));
    		}

    	} while ( virtualAlloc->nextConnectionEntry());


		// for testing purpose
		int entireUsedSlots = 0;

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

		printf("Broadcast Burst x %d y %d numOfSubchannels %d numOfSymbol %d \n", dlSymbolOffset, dlSubchannelOffset,  dlMapIe->num_of_subchannels,  dlMapIe->num_of_symbols);

	    // increase index
	    dlMapIeIndex++;

		// update Symbol and Subchannel Offset
		dlSymbolOffset += int( floor( double( dlSubchannelOffset + virtualAlloc->getNbOfBroadcastSlots()) / totalDlSubchannels) * phy->getSlotLength(DL_));
		dlSubchannelOffset = ( dlSubchannelOffset + virtualAlloc->getNbOfBroadcastSlots()) % (totalDlSubchannels);

		// testing
		entireUsedSlots += virtualAlloc->getNbOfBroadcastSlots();

        OcsaAlgorithm osca;
        osca.sortAnfrageList( &anfrageList);

        frameObject = new frame( (totalDlSymbols - 1) / 2, totalDlSubchannels, (dlSymbolOffset + 1) / 2 , dlSubchannelOffset);

        // call mapping algorithm                          // v max_burst_slots
        osca.finishToPackingFrame( &anfrageList, frameObject, 0);


		vector<OscaBurst *> burstList;
		vector<OscaBurst *>::iterator burstListIt;


		burstList = frameObject->get_burstList();

		vector<anfrage *> mappedConnections;
		vector<anfrage *>::iterator mappedConIt;

		// testing
		entireUsedSlots += virtualAlloc->getNbOfBroadcastSlots();

		for (burstListIt=burstList.begin(); burstListIt != burstList.end(); ++burstListIt) {


			mappedConnections = (*burstListIt)->get_anfrageList();

			int numberOfCid = 0;

			for (mappedConIt=mappedConnections.begin(); mappedConIt != mappedConnections.end(); ++mappedConIt ) {
				numberOfCid++;
				entireUsedSlots += (*mappedConIt)->get_slot();
			}
			assert( numberOfCid == 1);

			// take first connection
			mappedConIt = mappedConnections.begin();

			assert( (*burstListIt)->get_height() <= (*burstListIt)->get_yCorr());

			dlMapIe = &(dlMap->ies[dlMapIeIndex]);
			dlMapIe->cid = (*mappedConIt)->get_cid();
			dlMapIe->n_cid = 1;
			dlMapIe->diuc = bsScheduler_->getDIUCProfile(mac_->getMap()->getDlSubframe()->getProfile((*mappedConIt)->get_connection()->getPeerNode()->getDIUC())->getEncoding());
			dlMapIe->preamble = false;
			dlMapIe->start_time = ((*burstListIt)->get_xCorr() - (*burstListIt)->get_width()) * 2 + dlSymbolOffset;
			dlMapIe->symbol_offset = ((*burstListIt)->get_xCorr() - (*burstListIt)->get_width()) * 2 + dlSymbolOffset;
			dlMapIe->subchannel_offset = (*burstListIt)->get_yCorr() -  (*burstListIt)->get_height();
			dlMapIe->boosting = 0;
			dlMapIe->num_of_subchannels = (*burstListIt)->get_height();
			dlMapIe->num_of_symbols = (*burstListIt)->get_width() * 2;
			dlMapIe->repition_coding_indicator = 0;

			// debug
			Connection * con = (*mappedConIt)->get_connection();
			printf("Handle connection %d with Slots %d through Burst with x %d y %d w %d h %d area %d\n",
					con->get_cid(), (*mappedConIt)->get_slot(), ((*burstListIt)->get_xCorr() - (*burstListIt)->get_width()) * 2 + dlSymbolOffset, (*burstListIt)->get_yCorr() -  (*burstListIt)->get_height(),
					(*burstListIt)->get_width() * 2, (*burstListIt)->get_height(), (*burstListIt)->get_width() * (*burstListIt)->get_height());

		    // increase index
		    dlMapIeIndex++;
		}

	    printf("Number of Connection Scheduled %d , Number of Slots used %d \n", dlMapIeIndex, entireUsedSlots);
    } else {
    	// Only Broadcast Burst


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

		printf("Broadcast Burst x %d y %d numOfSubchannels %d numOfSymbol %d \n", dlSymbolOffset, dlSubchannelOffset,  dlMapIe->num_of_subchannels,  dlMapIe->num_of_symbols);

		// increase index
		dlMapIeIndex++;
    }



    // set the number of information elements
    dlMap->nb_ies = dlMapIeIndex;

    delete frameObject;



	return dlMap;
}
