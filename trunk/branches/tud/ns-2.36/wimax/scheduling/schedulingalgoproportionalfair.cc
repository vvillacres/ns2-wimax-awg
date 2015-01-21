/*
 * schedulingalgoproportionalfair.cc
 *
 *  Created on: 10.08.2010
 *      Author: richter
 */

#include "schedulingalgoproportionalfair.h"
#include "virtualallocation.h"
#include "connection.h"
#include "packet.h"

SchedulingAlgoProportionalFair::SchedulingAlgoProportionalFair() {
	// Initialize member variables
	nextConnectionPtr_ = NULL;

}

SchedulingAlgoProportionalFair::~SchedulingAlgoProportionalFair() {
	// TODO Auto-generated destructor stub
}

void SchedulingAlgoProportionalFair::scheduleConnections( VirtualAllocation* virtualAllocation, int freeSlots)
{

	// update totalNbOfSlots_ for statistic
	assert( ( 0 < movingAverageFactor_) && ( 1 > movingAverageFactor_) );
	totalNbOfSlots_ = ( totalNbOfSlots_ * ( 1 - movingAverageFactor_)) + ( freeSlots * movingAverageFactor_);

	int nbOfMstrConnections = 0;
	int nbOfUnscheduledConnections = 0;
	u_int32_t sumOfWantedMstrBytes = 0;

	// count number of allocated slots for mrtr demands
	int mrtrSlots = 0;
	// count the slots used to fulfill MSTR demands
	int mstrSlots = 0;


	// check if any connections have data to send
	if ( virtualAllocation->firstConnectionEntry()) {

		// run ones through whole the map
		do {
			if ( virtualAllocation->getConnection()->getType() == CONN_DATA ) {

				// count the connection and the amount of data which can be schedules due to the mstr rates
				if ( virtualAllocation->getWantedMstrSize()  > 0) {

					nbOfMstrConnections++;
					sumOfWantedMstrBytes +=  virtualAllocation->getWantedMstrSize();
				}
				printf("Connection CID %d Demand MSTR %d \n", virtualAllocation->getConnection()->get_cid(), virtualAllocation->getWantedMstrSize());
			}
		} while ( virtualAllocation->nextConnectionEntry() );

		nbOfUnscheduledConnections = nbOfMstrConnections;


		/*
		 * Allocation of Slots for fulfilling the MSTR demands
		 */


		// get first data connection
		if ( nextConnectionPtr_ != NULL ) {
			// find last connection
			if ( ! virtualAllocation->findConnectionEntry( nextConnectionPtr_)) {
				// connection not found -> get first connection
				virtualAllocation->firstConnectionEntry();
			}
		} else {
			// get first connection
			virtualAllocation->firstConnectionEntry();
		}


		while ( ( nbOfMstrConnections > 0 ) && ( freeSlots > 0 ) ) {

			// number of Connections which can be served in this iteration
			int conThisRound = nbOfMstrConnections;

			int nbOfSlotsPerConnection = 0;

			// divide slots equally for the first round
			if ( virtualAllocation->getBroadcastSlotCapacity() != 0) {
				// Downlink direction --> reserve space for dl-map
				//int nbOfSlotsPerConnection = freeSlots / nbOfMstrConnections;
				nbOfSlotsPerConnection = (freeSlots - ( ceil( double( nbOfUnscheduledConnections * DL_MAP_IE_SIZE) / virtualAllocation->getBroadcastSlotCapacity() ))) / nbOfMstrConnections;
			} else {
				// Uplink direction
				nbOfSlotsPerConnection = freeSlots / nbOfMstrConnections;
			}

			// only one slot per connection left
			if ( nbOfSlotsPerConnection <= 0 ) {
				nbOfSlotsPerConnection = 1;
			}


			while ( ( conThisRound > 0) && ( freeSlots > 0) ) {

				// go to connection which has unfulfilled  Mstr demands
				while ( ( virtualAllocation->getConnection()->getType() != CONN_DATA ) ||
						( virtualAllocation->getWantedMstrSize() <= u_int32_t( virtualAllocation->getCurrentMstrPayload() ) ) )  {
					// next connection

					// TODO: may cause problems ugly
					virtualAllocation->nextConnectionEntry();
				}

				// this connection gets up to nbOfSlotsPerConnection slots

				// calculate the corresponding number of bytes
				int allocatedSlots = nbOfSlotsPerConnection + virtualAllocation->getCurrentNbOfSlots();
				int maximumBytes = allocatedSlots * virtualAllocation->getSlotCapacity();

				// get fragmented bytes to calculate first packet size
				int fragmentedBytes = virtualAllocation->getConnection()->getFragmentBytes();


				// get first packed
				Packet * currentPacket = virtualAllocation->getConnection()->get_queue()->head();
				int allocatedBytes = 0;
				int allocatedPayload = 0;
				int wantedMrtrSize = virtualAllocation->getWantedMrtrSize();
				int wantedMstrSize = virtualAllocation->getWantedMstrSize();

				while ( ( currentPacket != NULL) && ( (allocatedBytes < maximumBytes) && ( allocatedPayload < wantedMstrSize) ) ) {

					int packetSize = HDR_CMN(currentPacket)->size();

					if (fragmentedBytes > 0) {

						// payload is packet size - already send byte - Generice Header - Fragmentation Subheader
						allocatedPayload += ( packetSize - fragmentedBytes - HDR_MAC802_16_SIZE - HDR_MAC802_16_FRAGSUB_SIZE );

						// packet size - already send bytes
						allocatedBytes += ( packetSize - fragmentedBytes );
						fragmentedBytes = 0;
					} else {
						allocatedPayload += packetSize - HDR_MAC802_16_SIZE;
						allocatedBytes += packetSize;
					}
					// get next packet
					currentPacket = currentPacket->next_;
				}



				// calculate fragmentation of the last scheduled packet
				if ( allocatedBytes > maximumBytes) {
					// one additional fragmentation subheader has to be considered
					allocatedPayload -= ( (allocatedBytes - maximumBytes) + HDR_MAC802_16_FRAGSUB_SIZE);
					if ( allocatedPayload < 0) {
						// avoid rounding errors
						allocatedPayload = 0;
					}
					allocatedBytes = maximumBytes;
					// is demand fulfilled ?
					if ( allocatedPayload >= wantedMstrSize) {
						// reduce payload
						if ( (allocatedPayload - wantedMstrSize) > HDR_MAC802_16_FRAGSUB_SIZE) {
							allocatedBytes -= ( (allocatedPayload - wantedMstrSize) - HDR_MAC802_16_FRAGSUB_SIZE);
						}
						allocatedPayload = wantedMstrSize;
						// reduce number of connection with mstr demand
						nbOfMstrConnections--;
						// avoids, that connection is called again
						virtualAllocation->updateWantedMrtrMstr( 0, 0);
					}
				} else {
					// is demand fulfilled due to allocated bytes or all packets
					if (( allocatedPayload >= wantedMstrSize) || ( currentPacket == NULL)) {
						// reduce number of connection with mstr demand
						nbOfMstrConnections--;
						// avoids, that connection is called again
						virtualAllocation->updateWantedMrtrMstr( 0, 0);

						// consider fragmentation due to traffic policing
						if ( allocatedPayload > wantedMstrSize) {
							// reduce payload
							if ( (allocatedPayload - wantedMstrSize) > HDR_MAC802_16_FRAGSUB_SIZE) {
								allocatedBytes -= ( (allocatedPayload - wantedMstrSize) - HDR_MAC802_16_FRAGSUB_SIZE);
							}
							allocatedPayload = wantedMstrSize;
						}
					}
				}


				// Calculate Allocated Slots
				allocatedSlots = int( ceil( double(allocatedBytes) / virtualAllocation->getSlotCapacity()) );


				int oldSlots = virtualAllocation->getCurrentNbOfSlots();
				if (( oldSlots == 0 ) && (virtualAllocation->getBroadcastSlotCapacity() != 0 )) {
					// new dl-map ie is needed

					// debug for QPSK 1/2
					assert( freeSlots >= 1);
					// increase broadcast burst
					freeSlots -= virtualAllocation->increaseBroadcastBurst( DL_MAP_IE_SIZE);

					// debug
					assert( freeSlots >= 0);

					// decrease unscheduled connections
					nbOfUnscheduledConnections--;
				}

				// calculate new assigned slots
				int newSlots = ( allocatedSlots - oldSlots);


				// update freeSlots
				freeSlots -= newSlots;

				// check for debug
				assert( freeSlots >= 0);



				if ( allocatedPayload <= wantedMrtrSize ) {
					// update only mrtrSlots
					mrtrSlots += newSlots;

					// update container
					virtualAllocation->updateAllocation( allocatedBytes, allocatedSlots,  u_int32_t( allocatedPayload), u_int32_t( allocatedPayload));

				} else {
					// update mrtrSlot and mstrSlots
					int newMrtrSlots = int( ceil( double( wantedMrtrSize ) / virtualAllocation->getSlotCapacity())) - virtualAllocation->getCurrentNbOfSlots();

					if ( newMrtrSlots > 0 ) {
						// update mrtrSlots and mstrSlots
						mrtrSlots += newMrtrSlots;
						mstrSlots += (newSlots - newMrtrSlots);
					} else {
						// update only mstrSlots
						mstrSlots += newSlots;
					}

					// update container
					virtualAllocation->updateAllocation( allocatedBytes, allocatedSlots,  u_int32_t( wantedMrtrSize), u_int32_t( allocatedPayload));
				}


				// decrease loop counter
				conThisRound--;

				// get next connection
				virtualAllocation->nextConnectionEntry();
				nextConnectionPtr_ = virtualAllocation->getConnection();
			}
		}
	} // END: check if any connections have data to send


	// update usedMrtrSlots_ for statistic
	usedMrtrSlots_ = ( usedMrtrSlots_ * ( 1 - movingAverageFactor_)) + ( mrtrSlots * movingAverageFactor_);
	// update usedMstrSlots_ for statistic
	usedMstrSlots_ = ( usedMstrSlots_ * ( 1 - movingAverageFactor_)) + ( mstrSlots * movingAverageFactor_);

}

