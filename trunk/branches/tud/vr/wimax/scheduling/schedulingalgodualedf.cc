/*
 * schedulingalgodualedf.cc
 *
 *  Created on: 08.08.2010
 *      Author: richter
 */

#include "schedulingalgodualedf.h"
#include "virtualallocation.h"
#include "connection.h"
#include "packet.h"
#include "edfqueueelement.h"
#include <queue>

#define DEADLINE_IDLE 1e9
#define DEADLINE_NRTPS 1e6

SchedulingAlgoDualEdf::SchedulingAlgoDualEdf() {
	// Nothing to do

}

SchedulingAlgoDualEdf::~SchedulingAlgoDualEdf() {
	// TODO Auto-generated destructor stub
}

void SchedulingAlgoDualEdf::scheduleConnections( VirtualAllocation* virtualAllocation, int freeSlots)
{
	// Save Packets according to there deadline in EdfListElements
    // list for MRTR allocations
    std::priority_queue <EdfQueueElement, deque<EdfQueueElement>, greater<EdfQueueElement> > edfMrtrQueue;
    // list for MSTR allocations
    std::priority_queue <EdfQueueElement, deque<EdfQueueElement>, greater<EdfQueueElement> > edfMstrQueue;

    double currentTime = NOW;

	// for debug
	if ( NOW >=  11.008166  ) {
		printf("Breakpoint reached \n");
	}

	// update totalNbOfSlots_ for statistic
	assert( ( 0 < movingAverageFactor_) && ( 1 > movingAverageFactor_) );
	totalNbOfSlots_ = ( totalNbOfSlots_ * ( 1 - movingAverageFactor_)) + ( freeSlots * movingAverageFactor_);

	// count number of allocated slots for mrtr demands
	int mrtrSlots = 0;
	// count the slots used to fulfill MSTR demands
	int mstrSlots = 0;


	// check if any connections have data to send
	if ( virtualAllocation->firstConnectionEntry()) {


		if ( lastConnectionPtr_ != NULL) {
			if ( ! virtualAllocation->findConnectionEntry( lastConnectionPtr_)) {
				// connection not found -> get first connection
				virtualAllocation->firstConnectionEntry();
			}
		}
		Connection * firstConnectionPtr = virtualAllocation->getConnection();

		// run ones through whole the map
		do {
			Connection * currentCon = virtualAllocation->getConnection();

			if ( currentCon->getType() == CONN_DATA ) {
				int maxAllocationSize =  int( virtualAllocation->getWantedMstrSize());

				// only for active connections
				if ( maxAllocationSize > 0) {

					ServiceFlowQosSet * qosSet = currentCon->getServiceFlow()->getQosSet();

					// get Maximum Latency in ms for this connection
					double maxLatency =  double( qosSet->getMaxLatency()) / 1e3;
					if ( maxLatency > 0 ) {
						// a higher priority will be served first
						maxLatency -= ( 1e-6 * qosSet->getTrafficPriority());
					} else {
						// set deadline to infinity
						maxLatency = DEADLINE_NRTPS - ( 1e-6 * qosSet->getTrafficPriority());
					}

					// get fragmented bytes to calculate first packet size
					int fragmentedBytes = currentCon->getFragmentBytes();

					EdfQueueElement edfElement;

					Packet * currentPacket;
					currentPacket = currentCon->get_queue()->head();

					//debug
					// printf("Current Connection has %d packet with %d bytes \n", currentCon->get_queue()->length(), currentCon->get_queue()->byteLength());

					int packetSize;


					while (( currentPacket != NULL) && ( maxAllocationSize > 0 )) {
						// calculate packetSize
						packetSize = HDR_CMN(currentPacket)->size() - fragmentedBytes;
						// only touch packets which are admitted
						// Allocation size is based on payload
						maxAllocationSize -= packetSize - HDR_MAC802_16_SIZE;
						// create new queue element for this packet
						edfElement = EdfQueueElement( currentCon, ( HDR_CMN(currentPacket)->timestamp() + maxLatency) - currentTime, packetSize );
						// save this queue element
						edfMrtrQueue.push( edfElement);
						// reset fragmented bytes
						fragmentedBytes = 0;
						// next packet
						currentPacket = currentPacket->next_;
					}
				}
			}
			// start next time with the last connection
			lastConnectionPtr_ = virtualAllocation->getConnection();

			// handle next connection
			virtualAllocation->nextConnectionEntry();
			// compare if all connections have been served
		} while ( virtualAllocation->getConnection() != firstConnectionPtr );

		/*
		 * Allocation of Slots for fulfilling the MRTR demands
		 */


		while ( ( ! edfMrtrQueue.empty() ) && ( freeSlots > 0 ) ) {

			// get connection of this packet
			Connection * currentCon = edfMrtrQueue.top().getConnection();

			// get already assigned bytes
			int allocBytes =  virtualAllocation->getCurrentNbOfBytes();
			int allocPayload = virtualAllocation->getCurrentMrtrPayload();

			int wantedMrtrSize;
			int wantedMstrSize;

			// find this connection in the allocation container
			if ( virtualAllocation->findConnectionEntry( currentCon) ) {
				wantedMrtrSize = virtualAllocation->getWantedMrtrSize();
				wantedMstrSize = virtualAllocation->getWantedMstrSize();
			} else {
				fprintf(stderr,"ERROR DEDF: This connection should have an entry ! \n");
				exit(6);
			}

			// are there free mrtr size
			if (( wantedMrtrSize - allocPayload) > 0 ) {

				// get slot capacity
				int slotCapacity = virtualAllocation->getSlotCapacity();

				// size of the current packet
				int newAllocBytes = edfMrtrQueue.top().getSize();
				int newAllocPayload = newAllocBytes - HDR_MAC802_16_SIZE;

				// reduce to maximum amount of addition allocations
				if ( newAllocPayload > (wantedMrtrSize - allocPayload)) {
					newAllocPayload = wantedMrtrSize - allocPayload;
					newAllocBytes = newAllocPayload + HDR_MAC802_16_SIZE;
				}

				// number of additional slots for this connection
				int newSlots = int( ceil( double(newAllocBytes + allocBytes) / slotCapacity )) - virtualAllocation->getCurrentNbOfSlots();


				if ( freeSlots < newSlots) {
					// not enough free resources
					newSlots = freeSlots;
					// recalculate new allocations
					newAllocBytes = (newSlots + virtualAllocation->getCurrentNbOfSlots()) * slotCapacity - allocBytes;
					newAllocPayload = newAllocBytes - HDR_MAC802_16_SIZE;
				}
				// update free slots
				freeSlots -= newSlots;

				// update mrtrSlots
				mrtrSlots += newSlots;

				if (( ( edfMrtrQueue.top().getSize() - HDR_MAC802_16_SIZE) > (wantedMrtrSize - allocPayload)) && ( wantedMstrSize > wantedMrtrSize) ){
					// has connection Mstr addmitments
					// create new element for the MSTR Queue
					EdfQueueElement newQueueElement( edfMrtrQueue.top().getConnection(), edfMrtrQueue.top().getDeadline(),
							edfMrtrQueue.top().getSize() - newAllocBytes + HDR_MAC802_16_SIZE + HDR_MAC802_16_FRAGSUB_SIZE);
					edfMstrQueue.push( newQueueElement);
				}

				// debug
				// printf("Packet from Connection %d with %d Bytes and %f ms deadline served using %d new slots \n", edfMrtrQueue.top().getConnection()->get_cid(), edfMrtrQueue.top().getSize(), edfMrtrQueue.top().getDeadline() * 1e3, newSlots);

				// debug
				assert( wantedMrtrSize <= (newAllocPayload + allocPayload));

				// update virtual allocation container
				virtualAllocation->updateAllocation( newSlots + virtualAllocation->getCurrentNbOfSlots(), newAllocBytes + allocBytes, newAllocPayload + allocPayload, newAllocPayload + allocPayload);

				// remove packet from queue
				edfMrtrQueue.pop();

			} else {
				// no free mrtr size
				if ( ( wantedMstrSize - allocPayload) > 0) {
					// add packet to mstr queue
					edfMstrQueue.push( edfMrtrQueue.top());
				}
				// remove it from mrtr queue
				edfMrtrQueue.pop();

			}
		} // end while


		while ( ( ! edfMstrQueue.empty() ) && ( freeSlots > 0 ) ) {

			// get connection of this packet
			Connection * currentCon = edfMstrQueue.top().getConnection();

			// get already assigned bytes
			int allocBytes =  virtualAllocation->getCurrentNbOfBytes();
			int allocPayload = virtualAllocation->getCurrentMstrPayload();

			int wantedMstrSize;

			// find this connection in the allocation container
			if ( virtualAllocation->findConnectionEntry( currentCon) ) {
				wantedMstrSize = virtualAllocation->getWantedMstrSize();
			} else {
				fprintf(stderr,"ERROR DEDF: This connection should have an entry ! \n");
				exit(6);
			}

			// are there free mstr size
			if (( wantedMstrSize - allocPayload) > 0 ) {

				// get slot capacity
				int slotCapacity = virtualAllocation->getSlotCapacity();

				// size of the current packet
				int newAllocBytes = edfMstrQueue.top().getSize();
				int newAllocPayload = newAllocBytes - HDR_MAC802_16_SIZE;

				// reduce to maximum amount of addition allocations
				if ( newAllocPayload > (wantedMstrSize - allocPayload)) {
					newAllocPayload = wantedMstrSize - allocPayload;
					newAllocBytes = newAllocPayload + HDR_MAC802_16_SIZE;
				}

				// number of additional slots for this connection
				int newSlots = int( ceil( double(newAllocBytes + allocBytes) / slotCapacity )) - virtualAllocation->getCurrentNbOfSlots();


				if ( freeSlots < newSlots) {
						// not enough free resources
						newSlots = freeSlots;
						// recalculate new allocations
						newAllocBytes = (newSlots + virtualAllocation->getCurrentNbOfSlots()) * slotCapacity - allocBytes;
						newAllocPayload = newAllocBytes - HDR_MAC802_16_SIZE;
				}

				// update free slots
				freeSlots -= newSlots;

				// update mstrSlots
				mstrSlots += newSlots;

				// debug
				assert( wantedMstrSize <= (newAllocPayload + allocPayload));

				// update virtual allocation container
				virtualAllocation->updateAllocation( newSlots + virtualAllocation->getCurrentNbOfSlots(), newAllocBytes + allocBytes, virtualAllocation->getCurrentMrtrPayload(), newAllocPayload + allocPayload);


			}

			// remove packet from queue
			edfMstrQueue.pop();

		} // end while

	}

	// update usedMrtrSlots_ for statistic
	usedMrtrSlots_ = ( usedMrtrSlots_ * ( 1 - movingAverageFactor_)) + ( mrtrSlots * movingAverageFactor_);
	// update usedMstrSlots_ for statistic
	usedMstrSlots_ = ( usedMstrSlots_ * ( 1 - movingAverageFactor_)) + ( mstrSlots * movingAverageFactor_);

}
