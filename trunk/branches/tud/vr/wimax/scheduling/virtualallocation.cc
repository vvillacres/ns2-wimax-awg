/*
 * VirtualAllocation.cpp
 *
 *  Created on: 07.07.2010
 *      Author: richter
 */

#include <stdio.h>
#include <stdlib.h>
#include "virtualallocation.h"

VirtualAllocation::VirtualAllocation()
{

    // INitializes Broadcast Burst
    nbOfBroadcastSlots_ = 0;
    nbOfBroadcastBytes_ = 0;

    // Initializes Iterator with end of map
    mapIterator_ = virtualAllocationMap_.end();
}

VirtualAllocation::~VirtualAllocation()
{
    // Nothing to do
}

/*
 * Adds a new Virtual Allocation in the Map
 */
void VirtualAllocation::addAllocation( int cid)
{

    // Create new Virtual AllocationElement with 0 bytes and 0 slots
    VirtualAllocationElement newAllocationElement( cid, 0, 0);

    // Insert new Element into the map
    virtualAllocationMap_.insert ( pair<int,VirtualAllocationElement>( cid, newAllocationElement) );
}

/*
 * Adds a new Virtual Allocation in the Map
 */
void VirtualAllocation::addAllocation( int cid, u_int32_t wantedMstrSize, u_int32_t wantedMrtrSize, int nbOfSlots, int nbOfBytes)
{

    // Create new Virtual Allocation Element
    VirtualAllocationElement newAllocationElement( cid, wantedMstrSize, wantedMrtrSize, nbOfSlots, nbOfBytes);

    // Insert new Element into the map
    virtualAllocationMap_.insert ( pair<int,VirtualAllocationElement>( cid, newAllocationElement) );
}

/*
 * Returns true if an Entry of this CID exits and set the current Iterator
 */
bool VirtualAllocation::findCidEntry(int cid)
{

    mapIterator_ = virtualAllocationMap_.find( cid);
    return virtualAllocationMap_.end() != mapIterator_;

}

/*
 * Returns the number of allocated slots for the current connection
 */
int VirtualAllocation::getCurrentNbOfSlots()
{

    if ( virtualAllocationMap_.end() != mapIterator_ ) {
        return mapIterator_->second.getNbOfAllocatedSlots();
    } else {
        fprintf(stderr,"ERROR: Iterator not valid use findCidEntry() before");
        exit(5);
    }
}

/*
 * Returns the number of allocated bytes for the current connection
 */
int VirtualAllocation::getCurrentNbOfBytes()
{

    if ( virtualAllocationMap_.end() != mapIterator_ ) {
        return mapIterator_->second.getNbOfAllocatedBytes();
    } else {
        fprintf(stderr,"ERROR: Iterator not valid use findCidEntry() before");
        exit(5);
    }
}

/*
 * Update values of the current virtual allocation
 */
void VirtualAllocation::updateAllocation( int nbOfSlots, int nbOfBytes)
{

    if ( virtualAllocationMap_.end() != mapIterator_ ) {
        mapIterator_->second.setNbOfAllocatedSlots( nbOfSlots);
        mapIterator_->second.setNbOfAllocatedBytes( nbOfSlots);
    } else {
        fprintf(stderr,"ERROR: Iterator not valid use findCidEntry() before");
        exit(5);
    }

}
