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
	lastConnectionPtr_ = NULL;

}

SchedulingProportionalFairUl::~SchedulingProportionalFairUl() {
	// TODO Auto-generated destructor stub
}

void SchedulingProportionalFairUl::scheduleConnections( VirtualAllocation* virtualAllocation, int freeSlots)
{

	int nbOfConnections = 0;
	u_int32_t sumOfWantedBytes = 0;

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
		if ( lastConnectionPtr_ != NULL ) {
			// find last connection
			virtualAllocation->findConnectionEntry( lastConnectionPtr_);
			// get next connection
			virtualAllocation->nextConnectionEntry();
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
				int requestedAllocation = virtualAllocation->getConnection()->getBw();
				int allocatedBytes = requestedAllocation;


				// calculate fragmentation of the last scheduled packet
				if ( requestedAllocation > maximumBytes) {
					// reduce to maximum allocation
					allocatedBytes = maximumBytes;
				}
				if ( allocatedBytes >= requestedAllocation) {
					// reduce number of connection with mrtr demand
					nbOfConnections--;
				}


				// Calculate Allocated Slots
				allocatedSlots = int( ceil( double(allocatedBytes) / virtualAllocation->getSlotCapacity()) );

				// update freeSlots
				freeSlots -= ( allocatedSlots - virtualAllocation->getCurrentNbOfSlots());

				// check for debug
				assert( freeSlots >= 0);

				// update container
				virtualAllocation->updateAllocation( allocatedSlots, allocatedBytes, 0, allocatedBytes );

				// decrease loop counter
				conThisRound--;
			}
		}

		// save last served connection for the next round
		lastConnectionPtr_ = virtualAllocation->getConnection();
	}

}
