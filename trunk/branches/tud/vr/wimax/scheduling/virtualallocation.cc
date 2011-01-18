/*
 * VirtualAllocation.cpp
 *
 *  Created on: 07.07.2010
 *      Author: richter
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "virtualallocation.h"

VirtualAllocation::VirtualAllocation()
{

    // Initializes Broadcast Burst
    nbOfBroadcastSlots_ = 0;
    nbOfBroadcastBytes_ = 0;

    // Initializes Iterator with end of map
    mapIterator_ = virtualAllocationMap_.end();

}

VirtualAllocation::~VirtualAllocation()
{
    // Nothing to do
}

/**
 *  Increases the BroadcastBurst by added Bytes return the delta of used slots
 */
int VirtualAllocation::increaseBroadcastBurst( int addedBytes)
{
	//
	nbOfBroadcastBytes_ += addedBytes;
	int newSlots = int( ceil( double(nbOfBroadcastBytes_) / slotCapacityBroadcast_)) - nbOfBroadcastSlots_;
	nbOfBroadcastSlots_ += newSlots;
	return newSlots;
}


/*
 * Adds a new Virtual Allocation in the Map
 */
void VirtualAllocation::addAllocation( Connection* connection, u_int32_t wantedMstrSize, u_int32_t wantedMrtrSize, int slotCapacity, int nbOfBytes, int nbOfSlots, int nbOfCdmaSlots)
{

    // Create new Virtual Allocation Element
    VirtualAllocationElement newAllocationElement( connection, wantedMstrSize, wantedMrtrSize, slotCapacity, nbOfBytes, nbOfSlots, nbOfCdmaSlots);

    // Insert new Element into the map
    virtualAllocationMap_.insert ( pair< Connection*, VirtualAllocationElement>( connection, newAllocationElement) );
}

/*
 * Adds a new Virtrual Allocation for Cdma request in the Map
 */
void VirtualAllocation::addCdmaAllocation( Connection* connection, int slotCapacity, int nbOfCdmaSlots)
{
    // Create new Virtual Allocation Element
	VirtualAllocationElement newAllocationElement( connection, 0, 0, slotCapacity, 0, 0, nbOfCdmaSlots);

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
	printf("NextConnectionEntry - Size of Map: %d \n", virtualAllocationMap_.size());

	if ( ! virtualAllocationMap_.empty() ) {
		mapIterator_++;
		if ( virtualAllocationMap_.end() != mapIterator_ ) {
			return true;
		} else {
			// TODO: May cause problems if no Map Entry exists
			mapIterator_ = virtualAllocationMap_.begin();
			return false;
		}
	} else {
		return false;
	}
}

/*
 * Return the current connection
 */
Connection* VirtualAllocation::getConnection()
{
	if ( virtualAllocationMap_.end() != mapIterator_ ) {
		return mapIterator_->second.getConnectionPtr();
	} else {
	    fprintf(stderr,"ERROR: Iterator not valid use findCidEntry() before");
	    exit(5);
	}
}

/*
 * Returns wantedMrtrSize of the current connection
 */
u_int32_t VirtualAllocation::getWantedMrtrSize()
{
	if ( virtualAllocationMap_.end() != mapIterator_ ) {
			return mapIterator_->second.getWantedMrtrSize();
	} else {
		fprintf(stderr,"ERROR: Iterator not valid use findCidEntry() before");
		exit(5);
	}
}

/*
 * Returns wantedMstrSize of the current connection
 */
u_int32_t VirtualAllocation::getWantedMstrSize()
{

	if ( virtualAllocationMap_.end() != mapIterator_ ) {
		return mapIterator_->second.getWantedMstrSize();
	} else {
		fprintf(stderr,"ERROR: Iterator not valid use findCidEntry() before");
		exit(5);

	}
}

/*
 * Set Mrtr and Mstr Size
 */
void VirtualAllocation::updateWantedMrtrMstr( u_int32_t wantedMrtrSize, u_int32_t wantedMstrSize)
{
	if ( virtualAllocationMap_.end() != mapIterator_ ) {
		mapIterator_->second.setWantedMrtrSize( wantedMrtrSize);
		mapIterator_->second.setWantedMstrSize( wantedMstrSize);
	} else {
		fprintf(stderr,"ERROR: Iterator not valid use findCidEntry() before");
		exit(5);

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
 * Returns the number of allocated slots for the current connection
 */
int VirtualAllocation::getCurrentNbOfCdmaSlots()
{

    if ( virtualAllocationMap_.end() != mapIterator_ ) {
        return mapIterator_->second.getNbOfAllocatedCdmaSlots();
    } else {
        fprintf(stderr,"ERROR: Iterator not valid use findCidEntry() before");
        exit(5);
    }
}

/*
 * Set the number of allocated slots for the current connection
 */
void VirtualAllocation::setCurrentNbOfCdmaSlots( int nbOfCdmaSlots)
{

    if ( virtualAllocationMap_.end() != mapIterator_ ) {
        mapIterator_->second.setNbOfAllocatedCdmaSlots( nbOfCdmaSlots);
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
        mapIterator_->second.setNbOfAllocatedBytes( nbOfBytes);
    } else {
        fprintf(stderr,"ERROR: Iterator not valid use findCidEntry() before");
        exit(5);
    }

}
