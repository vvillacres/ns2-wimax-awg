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

#define BWREQHEADERSIZE 6

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
void VirtualAllocation::addAllocation( Connection* connectionPtr, u_int32_t wantedMrtrSize, u_int32_t wantedMstrSize, int slotCapacity, int nbOfBytes, int nbOfSlots)
{

    // Create new Virtual Allocation Element
    VirtualAllocationElement newAllocationElement( connectionPtr, wantedMrtrSize, wantedMstrSize, slotCapacity, nbOfBytes, nbOfSlots, -1, -1);
    // Insert new Element into the map
    virtualAllocationMap_.insert ( pair< Connection*, VirtualAllocationElement>( connectionPtr, newAllocationElement) );
}

/*
 * Adds a new Virtrual Allocation for Cdma request in the Map
 */
void VirtualAllocation::addCdmaAllocation( Connection* connectionPtr, int slotCapacity, int nbOfSlots, short int cdmaTop, short int cdmaCode)
{
    // Create new Virtual Allocation Element
	VirtualAllocationElement newAllocationElement( connectionPtr, 0, 0, slotCapacity, BWREQHEADERSIZE, nbOfSlots, cdmaTop, cdmaCode);

    // Insert new Element into the map
    virtualAllocationMap_.insert ( pair< Connection*, VirtualAllocationElement>( connectionPtr, newAllocationElement) );
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
	printf("NextConnectionEntry - Size of Map: %d \n", int( virtualAllocationMap_.size()) );

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
	    fprintf(stderr,"ERROR getConnection: Iterator not valid use findCidEntry() before");
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

bool VirtualAllocation::isCdmaAllocation()
{

    if ( virtualAllocationMap_.end() != mapIterator_ ) {
        if ( mapIterator_->second.getCdmaTop() == -1 ) {
        	return false;
        } else {
        	return true;
        }
    } else {
        fprintf(stderr,"ERROR: Iterator not valid use findCidEntry() before");
        exit(5);
    }
}

short int VirtualAllocation::getCurrentCdmaTop()
{

    if ( virtualAllocationMap_.end() != mapIterator_ ) {
        return mapIterator_->second.getCdmaTop();
    } else {
        fprintf(stderr,"ERROR: Iterator not valid use findCidEntry() before");
        exit(5);
    }
}

short int VirtualAllocation::getCurrentCdmaCode()
{

    if ( virtualAllocationMap_.end() != mapIterator_ ) {
        return mapIterator_->second.getCdmaCode();
    } else {
        fprintf(stderr,"ERROR: Iterator not valid use findCidEntry() before");
        exit(5);
    }
}

/*
 * Update values of the current virtual allocation
 */
void VirtualAllocation::updateAllocation( int nbOfBytes, int nbOfSlots, u_int32_t allocatedMrtrPayload, u_int32_t allocatedMstrPayload)
{

    if ( virtualAllocationMap_.end() != mapIterator_ ) {
        mapIterator_->second.setNbOfAllocatedBytes( nbOfBytes);
        mapIterator_->second.setNbOfAllocatedSlots( nbOfSlots);
        mapIterator_->second.setAllocatedMrtrPayload( allocatedMrtrPayload);
        mapIterator_->second.setAllocatedMstrPayload( allocatedMstrPayload);
    } else {
        fprintf(stderr,"ERROR: Iterator not valid use findCidEntry() before");
        exit(5);
    }

}

/*
 * Reset Cdma Flags and Code
 */
void VirtualAllocation::resetCdma()
{
    if ( virtualAllocationMap_.end() != mapIterator_ ) {
        mapIterator_->second.setCdmaCode( -1);
        mapIterator_->second.setCdmaTop( -1);
    } else {
        fprintf(stderr,"ERROR: Iterator not valid use findCidEntry() before");
        exit(5);
    }
}

/*
 * Get the number of allocated mrtr payload for the current connection
 */
u_int32_t VirtualAllocation::getCurrentMrtrPayload( )
{

    if ( virtualAllocationMap_.end() != mapIterator_ ) {
        return mapIterator_->second.getAllocatedMrtrPayload( );
    } else {
        fprintf(stderr,"ERROR: Iterator not valid use findCidEntry() before");
        exit(5);
    }
}

/*
 * Set the number of allocated mrtr payload for the current connection
 */
void VirtualAllocation::setCurrentMrtrPayload( u_int32_t allocatedMrtrPayload)
{

    if ( virtualAllocationMap_.end() != mapIterator_ ) {
        mapIterator_->second.setAllocatedMrtrPayload( allocatedMrtrPayload);
    } else {
        fprintf(stderr,"ERROR: Iterator not valid use findCidEntry() before");
        exit(5);
    }
}

/*
 * Get the number of allocated mrtr payload for the current connection
 */
u_int32_t VirtualAllocation::getCurrentMstrPayload( )
{

    if ( virtualAllocationMap_.end() != mapIterator_ ) {
        return mapIterator_->second.getAllocatedMstrPayload( );
    } else {
        fprintf(stderr,"ERROR: Iterator not valid use findCidEntry() before");
        exit(5);
    }
}

/*
 * Set the number of allocated mrtr payload for the current connection
 */
void VirtualAllocation::setCurrentMstrPayload( u_int32_t allocatedMstrPayload)
{

    if ( virtualAllocationMap_.end() != mapIterator_ ) {
        mapIterator_->second.setAllocatedMstrPayload( allocatedMstrPayload);
    } else {
        fprintf(stderr,"ERROR: Iterator not valid use findCidEntry() before");
        exit(5);
    }
}
