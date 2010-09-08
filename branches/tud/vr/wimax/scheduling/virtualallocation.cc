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
void VirtualAllocation::addAllocation( Connection* connection, u_int32_t wantedMstrSize, u_int32_t wantedMrtrSize, int slotCapacity, int nbOfSlots, int nbOfBytes)
{

    // Create new Virtual Allocation Element
    VirtualAllocationElement newAllocationElement( connection, wantedMstrSize, wantedMrtrSize, slotCapacity, nbOfSlots, nbOfBytes);

    // Insert new Element into the map
    virtualAllocationMap_.insert ( pair< Connection*, VirtualAllocationElement>( connection, newAllocationElement) );
}

/*
 * Returns true if an Entry of this CID exits and set the current Iterator
 */
bool VirtualAllocation::findConnectionEntry( Connection* connection)
{

    mapIterator_ = virtualAllocationMap_.find( connection);
    return virtualAllocationMap_.end() != mapIterator_;

}

/*
 * Return true if the first Entry has a connection and set the Iterator
 */
bool VirtualAllocation::firstConnectionEntry()
{
	mapIterator_ = virtualAllocationMap_.begin();
	return virtualAllocationMap_.end() != mapIterator_;
}

/*
 * Returns true if next Entry exists and set the Iterator
 * If the end is reached the Iterator is set to the next Element
 */
bool VirtualAllocation::nextConnectionEntry()
{
	++mapIterator_;
	if ( virtualAllocationMap_.end() != mapIterator_ ) {
		return true;
	} else {
		// TODO: May cause problems if no Map Entry exists
		mapIterator_ = virtualAllocationMap_.end();
		return false;
	}
}


/*
 * Returns the slot capacity in byte of the connection
 */
int VirtualAllocation::getSlotCapacity()
{
	if ( virtualAllocationMap_.end() != mapIterator_ ) {
		return mapIterator_->second.getSlotCapacity();
	} else {
	    fprintf(stderr,"ERROR: Iterator not valid use findCidEntry() before");
	    exit(5);
	}
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
 * Set the number of allocated slots for the current connection
 */
void VirtualAllocation::setCurrentNbOfSlots( int nbOfSlots)
{

    if ( virtualAllocationMap_.end() != mapIterator_ ) {
        mapIterator_->second.setNbOfAllocatedSlots( nbOfSlots);
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
 * Returns the number of allocated bytes for the current connection
 */
void VirtualAllocation::setCurrentNbOfBytes( int nbOfBytes)
{

    if ( virtualAllocationMap_.end() != mapIterator_ ) {
        mapIterator_->second.setNbOfAllocatedBytes( nbOfBytes);
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
