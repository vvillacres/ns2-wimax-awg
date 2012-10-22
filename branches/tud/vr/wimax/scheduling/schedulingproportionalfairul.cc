/*
 * schedulingalgoproportionalfair.cc
 *
 *  Created on: 10.08.2010
 *      Author: richter
 */

#include "schedulingproportionalfairul.h"
#include "virtualallocation.h"
#include "connection.h"
#include "packet.h"

SchedulingProportionalFairUl::SchedulingProportionalFairUl() {
	// Initialize member variables
	nextConnectionPtr_ = NULL;

}

SchedulingProportionalFairUl::~SchedulingProportionalFairUl() {
	// TODO Auto-generated destructor stub
}

void SchedulingProportionalFairUl::scheduleConnections( VirtualAllocation* virtualAllocation, int freeSlots)
{

	/* Calculate the number of data connections, the number of wanted Mrtr Bytes and the sum of wanted Mstr Bytes
	 * This could be done more efficient in bsscheduler.cc, but this would be a algorithm specific extension and
	 * would break the algorithm independent interface.
	 */

	// update totalNbOfSlots_ for statistic
	assert( ( 0 < movingAverageFactor_) && ( 1 > movingAverageFactor_) );
	totalNbOfSlots_ = ( totalNbOfSlots_ * ( 1 - movingAverageFactor_)) + ( freeSlots * movingAverageFactor_);

	int nbOfConnections = 0;
	u_int32_t sumOfWantedBytes = 0;

	// count number of allocated slots for mrtr demands
	int mrtrSlots = 0;
	// count the slots used to fulfill MSTR demands
	int mstrSlots = 0;

	// check if any connections have data to send
	if ( virtualAllocation->firstConnectionEntry()) {

		// run ones through whole the map
		do {
			if ( virtualAllocation->getConnection()->getType() == CONN_DATA ) {

				// count the connection and the amount of data which has to be scheduled
				if ( virtualAllocation->getWantedMstrSize() > 0 ) {

					nbOfConnections++;
					sumOfWantedBytes += virtualAllocation->getWantedMstrSize();
				}
			}
		} while ( virtualAllocation->nextConnectionEntry() );


		/*
		 * Allocation of Slots for fulfilling the demands
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

		while ( ( nbOfConnections > 0 ) && ( freeSlots > 0 ) ) {

			// number of Connections which can be served in this iteration
			int conThisRound = nbOfConnections;

			// divide slots equally for the first round
			int nbOfSlotsPerConnection = freeSlots / nbOfConnections;

			// only one slot per connection left
			if ( nbOfSlotsPerConnection <= 0 ) {
				nbOfSlotsPerConnection = 1;
			}


			while ( ( conThisRound > 0) && ( freeSlots > 0) ) {

				// go to connection which has unfulfilled  Mrtr demands
				while ( ( virtualAllocation->getConnection()->getType() != CONN_DATA ) ||
						( virtualAllocation->getWantedMstrSize() <= u_int32_t( virtualAllocation->getCurrentNbOfBytes() ) ) )  {
					// next connection
					virtualAllocation->nextConnectionEntry();
				}

				// this connection gets up to nbOfSlotsPerConnection slots

				// calculate the corresponding number of bytes
				int allocatedSlots = nbOfSlotsPerConnection + virtualAllocation->getCurrentNbOfSlots();
				int maximumBytes = allocatedSlots * virtualAllocation->getSlotCapacity();

				// get currently requested bytes
				int wantedSize = virtualAllocation->getWantedMstrSize();

				// calculate fragmentation of the last scheduled packet
				if ( maximumBytes >= wantedSize ) {
					// limit allocation to demand
					maximumBytes = wantedSize;
					// reduce number of connections
					nbOfConnections--;
				}



				// Calculate Allocated Slots
				allocatedSlots = int( ceil( double(maximumBytes) / virtualAllocation->getSlotCapacity()) );

				// calculate new assigned slots
				int newSlots = ( allocatedSlots - virtualAllocation->getCurrentNbOfSlots());

				// update free Slots
				freeSlots -= newSlots;

				// check for debug
				assert( freeSlots >= 0);

				// update container
				if ( u_int32_t( maximumBytes) <= virtualAllocation->getWantedMrtrSize() ) {
					// only MRTR has been fulfilled
					virtualAllocation->updateAllocation( maximumBytes, allocatedSlots,  u_int32_t( maximumBytes), u_int32_t( maximumBytes));
					// increase mrtr slots
					mrtrSlots += newSlots;

				} else {
					// mrtr and mstr has been fulfilled
					int newMrtrSlots =	ceil ( double(virtualAllocation->getWantedMrtrSize()) / virtualAllocation->getSlotCapacity() ) - virtualAllocation->getCurrentNbOfSlots();
					if ( newMrtrSlots > 0) {
						mrtrSlots += newMrtrSlots;
						mstrSlots += ( newSlots - newMrtrSlots);
					} else {
						mstrSlots += newSlots;
					}
					// updateContainer
					virtualAllocation->updateAllocation( maximumBytes, allocatedSlots,  virtualAllocation->getWantedMrtrSize(), u_int32_t( maximumBytes));

				}

				// decrease loop counter
				conThisRound--;

				// get next connection
				virtualAllocation->nextConnectionEntry();

				// save last served connection to begin with in the next frame
				nextConnectionPtr_ = virtualAllocation->getConnection();
			}
		}
	}

	// update usedMrtrSlots_ for statistic
	usedMrtrSlots_ = ( usedMrtrSlots_ * ( 1 - movingAverageFactor_)) + ( mrtrSlots * movingAverageFactor_);
	// update usedMstrSlots_ for statistic
	usedMstrSlots_ = ( usedMstrSlots_ * ( 1 - movingAverageFactor_)) + ( mstrSlots * movingAverageFactor_);

}
