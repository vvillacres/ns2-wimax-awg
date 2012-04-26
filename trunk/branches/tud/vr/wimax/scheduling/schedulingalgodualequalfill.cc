/*
 * schedulingalgodualequalfill.cc
 *
 *  Created on: 08.08.2010
 *      Author: richter
 */

#include "schedulingalgodualequalfill.h"
#include "virtualallocation.h"
#include "connection.h"
#include "packet.h"

SchedulingAlgoDualEqualFill::SchedulingAlgoDualEqualFill() {
	// Initialize member variables
	nextConnectionPtr_ = NULL;

}

SchedulingAlgoDualEqualFill::~SchedulingAlgoDualEqualFill() {
	// TODO Auto-generated destructor stub
}

void SchedulingAlgoDualEqualFill::scheduleConnections( VirtualAllocation* virtualAllocation, int freeSlots)
{
	/* Calculate the number of data connections, the number of wanted Mrtr Bytes and the sum of wanted Mstr Bytes
	 * This could be done more efficient in bsscheduler.cc, but this would be a algorithm specific extension and
	 * would break the algorithm independent interface.
	 */

	if ( NOW > 11.0067) {
		printf("debug %f \n", NOW);
	}

	// update totalNbOfSlots_ for statistic
	assert( ( 0 < movingAverageFactor_) && ( 1 > movingAverageFactor_) );
	totalNbOfSlots_ = ( totalNbOfSlots_ * ( 1 - movingAverageFactor_)) + ( freeSlots * movingAverageFactor_);

	int nbOfMrtrConnections = 0;
	int nbOfMstrConnections = 0;
	u_int32_t sumOfWantedMrtrBytes = 0;
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

				// Debugging propose
				// MRTR size is always less or equal to MSTR size
				assert( virtualAllocation->getWantedMrtrSize() <= virtualAllocation->getWantedMstrSize());

				// count the connection and the amount of data which has to be scheduled to fullfill the mrtr rates
				if ( virtualAllocation->getWantedMrtrSize() > 0 ) {

					nbOfMrtrConnections++;
					sumOfWantedMrtrBytes += virtualAllocation->getWantedMrtrSize();
				}

				// count the connection and the amount of data which can be schedules due to the mstr rates
				if ( (virtualAllocation->getWantedMstrSize() -  virtualAllocation->getWantedMrtrSize()) > 0) {

					nbOfMstrConnections++;
					sumOfWantedMstrBytes += ( virtualAllocation->getWantedMstrSize() - virtualAllocation->getWantedMrtrSize());
				}
				printf("Connection CID %d Demand MRTR %d MSTR %d \n", virtualAllocation->getConnection()->get_cid(), virtualAllocation->getWantedMrtrSize(), virtualAllocation->getWantedMstrSize());
			}
		} while ( virtualAllocation->nextConnectionEntry() );


		/*
		 * Allocation of Slots for fulfilling the MRTR demands
		 */

		// for debugging purpose
		bool test;

		// get first data connection
		if ( nextConnectionPtr_ != NULL ) {
			// find last connection
			if ( ! virtualAllocation->findConnectionEntry( nextConnectionPtr_)) {
				// connection not found -> get first connection
				test = virtualAllocation->firstConnectionEntry();
				assert( test);
			}
		} else {
			// get first connection
			test = virtualAllocation->firstConnectionEntry();
			assert( test);
		}


		while ( ( nbOfMrtrConnections > 0 ) && ( freeSlots > 0 ) ) {

			// number of Connections which can be served in this iteration
			int conThisRound = nbOfMrtrConnections;

			// divide slots equally for the first round
			int nbOfSlotsPerConnection = freeSlots / nbOfMrtrConnections;

			// only one slot per connection left
			if ( nbOfSlotsPerConnection <= 0 ) {
				nbOfSlotsPerConnection = 1;
			}


			while ( ( conThisRound > 0) && ( freeSlots > 0) ) {

				// go to connection which has unfulfilled  Mrtr demands
				while ( ( virtualAllocation->getConnection()->getType() != CONN_DATA ) ||
						( virtualAllocation->getWantedMrtrSize() <= u_int32_t( virtualAllocation->getCurrentMrtrPayload() ) ) )  {
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

				while ( ( currentPacket != NULL) && ( (allocatedBytes < maximumBytes) && ( allocatedPayload < wantedMrtrSize) ) ) {

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
					if ( allocatedPayload >= wantedMrtrSize) {
						// reduce payload
						if ( (allocatedPayload - wantedMrtrSize) > HDR_MAC802_16_FRAGSUB_SIZE) {
							allocatedBytes -= ( (allocatedPayload - wantedMrtrSize) - HDR_MAC802_16_FRAGSUB_SIZE);
						}
						allocatedPayload = wantedMrtrSize;
						// reduce number of connection with mrtr demand
						nbOfMrtrConnections--;
						// avoids, that connection is called again
						virtualAllocation->updateWantedMrtrMstr( 0, virtualAllocation->getWantedMstrSize());
					}
				} else {
					// is demand fulfilled due to allocated bytes or all packets
					if (( allocatedPayload >= wantedMrtrSize) || ( currentPacket == NULL)) {
						// reduce number of connection with mrtr demand
						nbOfMrtrConnections--;
						// avoids, that connection is called again
						virtualAllocation->updateWantedMrtrMstr( 0, virtualAllocation->getWantedMstrSize());

						// consider fragmentation due to traffic policing
						if ( allocatedPayload > wantedMrtrSize) {
							// reduce payload
							if ( (allocatedPayload - wantedMrtrSize) > HDR_MAC802_16_FRAGSUB_SIZE) {
								allocatedBytes -= ( (allocatedPayload - wantedMrtrSize) - HDR_MAC802_16_FRAGSUB_SIZE);
							}
							allocatedPayload = wantedMrtrSize;
						}
					} else {
						if (allocatedPayload < 0) {
							// avoid rounding errors
							allocatedPayload = 0;
						}
					}
				}
				// Calculate Allocated Slots
				allocatedSlots = int( ceil( double(allocatedBytes) / virtualAllocation->getSlotCapacity()) );

				int newSlots = ( allocatedSlots - virtualAllocation->getCurrentNbOfSlots());

				// debug
				printf(" %d new Mrtr Slots for Connection CID %d \n", newSlots, virtualAllocation->getConnection()->get_cid() );

				// update freeSlots
				freeSlots -= newSlots;
				// update mrtrSlots
				mrtrSlots += newSlots;

				// check for debug
				assert( freeSlots >= 0);

				// update container
				virtualAllocation->updateAllocation( allocatedBytes, allocatedSlots,  u_int32_t( allocatedPayload), u_int32_t( allocatedPayload));

				// decrease loop counter
				conThisRound--;

				// get next connection
				virtualAllocation->nextConnectionEntry();
				nextConnectionPtr_ = virtualAllocation->getConnection();
			}
		}

		// check if there are free Slots after Mrtr allocation
		if ( freeSlots > 0) {

			// count mstr connections
			nbOfMstrConnections = 0;
			virtualAllocation->firstConnectionEntry();
			// run ones through whole the map
			do {
				if ( virtualAllocation->getConnection()->getType() == CONN_DATA ) {

					// Debugging propose
					// MRTR size is always less or equal to MSTR size
					assert( virtualAllocation->getWantedMrtrSize() <= virtualAllocation->getWantedMstrSize());

					// count the connection and the amount of data which can be schedules due to the mstr rates
					if ( (virtualAllocation->getWantedMstrSize() >  virtualAllocation->getCurrentMrtrPayload())) {

						nbOfMstrConnections++;
					}
				}
			} while ( virtualAllocation->nextConnectionEntry() );

			// get next connection to be served
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

			/*
			 * Allocation of Slots for fulfilling the MSTR demands
			 */

			while ( ( nbOfMstrConnections > 0 ) && ( freeSlots > 0 ) ) {

				// number of Connections which can be served in this iteration
				int conThisRound = nbOfMstrConnections;

				// divide slots equally for the first round
				int nbOfSlotsPerConnection = freeSlots / nbOfMstrConnections;

				// only one slot per connection left
				if ( nbOfSlotsPerConnection <= 0 ) {
					nbOfSlotsPerConnection = 1;
				}

				while ( ( conThisRound > 0) && ( freeSlots > 0 ) ) {

					int i = 0;
					// go to connection which has unfulfilled  Mstr demands
					while  ( ( virtualAllocation->getConnection()->getType() != CONN_DATA ) 	||
							( ( virtualAllocation->getWantedMstrSize() <= u_int32_t( virtualAllocation->getCurrentMstrPayload()) ) && (i < 3 )) ){

						// next connection

						if ( ! virtualAllocation->nextConnectionEntry() ) {
							// 	count loops due to rounding errors
							i++;
						}
						// debug
						assert(i < 3);
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
						// has demand fulfilled
						if ( allocatedPayload >= wantedMstrSize) {
							// reduce payload
							if ( (allocatedPayload - wantedMstrSize) > HDR_MAC802_16_FRAGSUB_SIZE) {
								allocatedBytes -= ( (allocatedPayload - wantedMstrSize) - HDR_MAC802_16_FRAGSUB_SIZE);
							}
							allocatedPayload = wantedMstrSize;
							// reduce number of connection with mrtr demand
							nbOfMstrConnections--;
							// avoids, that connection is called again
							virtualAllocation->updateWantedMrtrMstr( 0, 0);
						}
					} else {
						// is demand fulfilled due to allocated bytes or all packets in the queue are scheduled
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
						} else {
							if (allocatedPayload < 0) {
								// avoid rounding errors
								allocatedPayload = 0;
							}
						}
					}
					// Calculate Allocated Slots
					allocatedSlots = int( ceil( double(allocatedBytes) / virtualAllocation->getSlotCapacity()) );

					// calculate new assigned slots
					int newSlots = ( allocatedSlots - virtualAllocation->getCurrentNbOfSlots());

					// debug
					printf(" %d new Mstr Slots for Connection CID %d \n", newSlots, virtualAllocation->getConnection()->get_cid() );

					// update freeSlots
					freeSlots -= newSlots;
					// update mstrSlots
					mstrSlots += newSlots;

					u_int32_t allocatedMrtrPayload = virtualAllocation->getCurrentMrtrPayload();
					// check for debug
					assert( freeSlots >= 0);
					assert( allocatedPayload >= 0);

					// update container
					virtualAllocation->updateAllocation( allocatedBytes, allocatedSlots,  allocatedMrtrPayload, u_int32_t(allocatedPayload));

					// decrease loop counter
					conThisRound--;

					// get next connection
					virtualAllocation->nextConnectionEntry();
					nextConnectionPtr_ = virtualAllocation->getConnection();
				}

			} // END Mstr allocation loop

		} // END: check if there are free Slots after Mrtr allocation

	} // END: check if any connections have data to send


	// update usedMrtrSlots_ for statistic
	usedMrtrSlots_ = ( usedMrtrSlots_ * ( 1 - movingAverageFactor_)) + ( mrtrSlots * movingAverageFactor_);
	// update usedMstrSlots_ for statistic
	usedMstrSlots_ = ( usedMstrSlots_ * ( 1 - movingAverageFactor_)) + ( mstrSlots * movingAverageFactor_);

}
