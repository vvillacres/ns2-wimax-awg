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



	// check if any connections have data to send
	if ( virtualAllocation->firstConnectionEntry()) {

		// run ones through whole the map
		do {
			Connection * currentCon = virtualAllocation->getConnection();

			if ( currentCon->getType() == CONN_DATA ) {
				u_int32_t maxAllocationSize =  virtualAllocation->getWantedMstrSize();

				// only for active connections
				if ( maxAllocationSize > 0) {

					ServiceFlowQosSet * qosSet = currentCon->getServiceFlow()->getQosSet();

					// get Maximum Latency for this connection
					double maxLatency =  double( qosSet->getMaxLatency());
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

					int packetSize;


					while (( currentPacket != 0) && ( maxAllocationSize > 0 )) {
						// calculate packetSize
						packetSize = HDR_CMN(currentPacket)->size() - fragmentedBytes;
						// only touch packets which are admitted
						maxAllocationSize -= packetSize;
						// create new queue element for this packet
						edfElement = EdfQueueElement( currentCon, ( HDR_CMN(currentPacket)->timestamp() + maxLatency) - currentTime, packetSize );
						// save this queue element
						edfMrtrQueue.push( edfElement);
						// reset fragmentet bytes
						fragmentedBytes = 0;
					}
				}
			}
		} while ( virtualAllocation->nextConnectionEntry() );


		/*
		 * Allocation of Slots for fulfilling the MRTR demands
		 */


		while ( ( ! edfMrtrQueue.empty() ) && ( freeSlots > 0 ) ) {

			// get connection of this packet
			Connection * currentCon = edfMrtrQueue.top().getConnection();

			// get already assigned bytes
			int allocBytes =  virtualAllocation->getCurrentNbOfBytes();

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
			if (( wantedMrtrSize - allocBytes) > 0 ) {

				// get slot capacity
				int slotCapacity = virtualAllocation->getSlotCapacity();

				// size of the current packet
				int newAllocBytes = edfMrtrQueue.top().getSize();

				// reduce to maximum amount of addition allocations
				if ( newAllocBytes > (wantedMrtrSize - allocBytes)) {
					newAllocBytes = wantedMrtrSize - allocBytes;
				}

				// number of additional slots for this connection
				int newSlots = int( ceil( double(newAllocBytes + allocBytes) / slotCapacity ));


				if ( freeSlots < newSlots) {
					// not enough free resources
					newSlots = freeSlots;
					// recalculate new allocations
					newAllocBytes = (newSlots + virtualAllocation->getCurrentNbOfSlots()) * slotCapacity - allocBytes;
				}
				// update free slots
				freeSlots -= newSlots;

				if (( newAllocBytes > (wantedMrtrSize - allocBytes)) && ( wantedMstrSize > ( newAllocBytes + allocBytes) )){
					// has connection Mstr addmitments
					// create new element for the MSTR Queue
					EdfQueueElement newQueueElement( edfMrtrQueue.top().getConnection(), edfMrtrQueue.top().getDeadline(),
							edfMrtrQueue.top().getSize() - newAllocBytes + HDR_MAC802_16_SIZE + HDR_MAC802_16_FRAGSUB_SIZE);
					edfMstrQueue.push( newQueueElement);
				}

				// update virtual allocation container
				virtualAllocation->updateAllocation( newSlots + virtualAllocation->getCurrentNbOfSlots(), newAllocBytes + virtualAllocation->getCurrentNbOfBytes());

				// remove packet from queue
				edfMrtrQueue.pop();

			} else {
				// no free mrtr size
				if ( ( wantedMstrSize - allocBytes) > 0) {
					// add packet to mstr queue
					edfMstrQueue.push( edfMrtrQueue.top());
				}
				// remove it from mrtr queue
				edfMrtrQueue.pop();

			}
		} // end while


		while ( ( ! edfMstrQueue.empty() ) && ( freeSlots > 0 ) ) {

			// get connection of this packet
			Connection * currentCon = edfMrtrQueue.top().getConnection();

			// get already assigned bytes
			int allocBytes =  virtualAllocation->getCurrentNbOfBytes();

			int wantedMstrSize;

			// find this connection in the allocation container
			if ( virtualAllocation->findConnectionEntry( currentCon) ) {
				wantedMstrSize = virtualAllocation->getWantedMstrSize();
			} else {
				fprintf(stderr,"ERROR DEDF: This connection should have an entry ! \n");
				exit(6);
			}

			// are there free mrtr size
			if (( wantedMstrSize - allocBytes) > 0 ) {

				// get slot capacity
				int slotCapacity = virtualAllocation->getSlotCapacity();

				// size of the current packet
				int newAllocBytes = edfMrtrQueue.top().getSize();

				// reduce to maximum amount of addition allocations
				if ( newAllocBytes > (wantedMstrSize - allocBytes)) {
					newAllocBytes = wantedMstrSize - allocBytes;
				}

				// number of additional slots for this connection
				int newSlots = int( ceil( double(newAllocBytes + allocBytes) / slotCapacity ));


				if ( freeSlots < newSlots) {
						// not enough free resources
						newSlots = freeSlots;
						// recalculate new allocations
						newAllocBytes = (newSlots + virtualAllocation->getCurrentNbOfSlots()) * slotCapacity - allocBytes;
				}

				// update free slots
				freeSlots -= newSlots;

				// update virtual allocation container
				virtualAllocation->updateAllocation( newSlots + virtualAllocation->getCurrentNbOfSlots(), newAllocBytes + virtualAllocation->getCurrentNbOfBytes());


			}

			// remove packet from queue
			edfMrtrQueue.pop();

		} // end while

	}

}
