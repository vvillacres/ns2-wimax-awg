/*
 * VirtualAllocation.h
 *
 * Container Class to save all values during scheduling process
 *
 *  Created on: 07.07.2010
 *      Author: richter
 */

#ifndef VIRTUALALLOCATION_H_
#define VIRTUALALLOCATION_H_

#include <map>
#include "virtualallocationelement.h"
using namespace std;

class Connection;

typedef map< Connection*, VirtualAllocationElement > VirtualAllocationMap_t;
typedef VirtualAllocationMap_t::iterator VirtualAllocationMapIt_t;

class VirtualAllocation
{
public:
    VirtualAllocation();
    virtual ~VirtualAllocation();


    /*
     * Get number of Slots for Broadcast Burst
     */
    int getNbOfBroadcastSlots() const {
        return nbOfBroadcastSlots_;
    }

    /*
     * Set number of Slots for Broadcast Burst
     */
    void setNbOfBroadcastSlots( int nbOfBroadcastSlots) {
        nbOfBroadcastSlots_ = nbOfBroadcastSlots;
    }

    /*
     * Get number of Byte for Broadcast Burst
     */
    int getNbOfBroadcastBytes() const {
        return nbOfBroadcastBytes_;
    }

    /*
     * Set number of Byte for Broadcast Burst
     */
    inline void setNbOfBroadcastBytes( int nbOfBroadcastBytes) {
        nbOfBroadcastBytes_ = nbOfBroadcastBytes;
    }

    /*
     * Get the slot capacity of the broadcast burst
     */
    int getBroadcastSlotCapacity() const {
        return slotCapacityBroadcast_;
    }

    /*
     * Set number of Byte for Broadcast Burst
     */
    inline void setBroadcastSlotCapacity( int slotCapacityBroadcast) {
    	slotCapacityBroadcast_ = slotCapacityBroadcast;
    }

    /**
     *  Increases the BroadcastBurst by added Bytes return the delta of used slots
     */
    int increaseBroadcastBurst( int addedBytes);

    /*
     * Adds a new Virtual Allocation in the Map
     */
    void addAllocation( Connection* connectionPtr, u_int32_t wantedMrtrSize, u_int32_t wantedMstrSize, int slotCapacity, int nbOfBytes = 0, int nbOfSlots = 0);


    /*
     * Adds a new Virtrual Allocation for Cdma request in the Map
     */
    void addCdmaAllocation( Connection* connectionPtr, int slotCapacity, int nbOfSlots, int cdmaTop, int cdmaCode);

    /*
     * Returns true if an Entry of this connection exits and set the current Iterator
     */
    bool findConnectionEntry( Connection* connectionPtr);

    /*
     * Return true if the first Entry has a connection and set the Iterator
     */
    bool firstConnectionEntry();

    /*
     * Returns true if next Entry exists and set the Iterator
     * If the end is reached the Iterator is set to the next Element
     */
    bool nextConnectionEntry();

    /*
     * Return the current connection
     */
    Connection* getConnection();

    /*
     * Returns wantedMrtrSize of the current connection
     */
    u_int32_t getWantedMrtrSize();

    /*
     * Returns wantedMstrSize of the current connection
     */
    u_int32_t getWantedMstrSize();

    /*
     * Set Mrtr and Mstr Size
     */
    void updateWantedMrtrMstr( u_int32_t wantedMrtrSize, u_int32_t wantedMstrSize);

    /*
     * Returns the slot capacity in byte of the connection
     */
    int getSlotCapacity();

    /*
     * Returns the number of allocated bytes for the current connection
     */
    int getCurrentNbOfBytes();

    /*
     * Returns the number of allocated bytes for the current connection
     */
    void setCurrentNbOfBytes( int nbOfBytes);

    /*
     * Update values of the current virtual allocation
     */
    void updateAllocation( int nbOfSlots, int nbOfBytes, u_int32_t allocatedMrtrPayload, u_int32_t allocatedMstrPayload);

    /*
     * Returns the number of allocated slots for the current connection
     */
    int getCurrentNbOfSlots();

    /*
     * Set the number of allocated slots for the current connection
     */
    void setCurrentNbOfSlots( int nbOfSlots);


    bool isCdmaAllocation();

    int getCurrentCdmaTop();

    int getCurrentCdmaCode();


    u_int32_t getCurrentMrtrPayload ();
    void setCurrentMrtrPayload( u_int32_t allocatedMrtrPayload);
    u_int32_t getCurrentMstrPayload ();
    void setCurrentMstrPayload( u_int32_t allocatedMstrPayload);


private:

    /*
     * Capacity in Byte of a broadcast slot
     */
    int slotCapacityBroadcast_;

    /*
     * Number of Slots for Broadcast Burst
     */
    int nbOfBroadcastSlots_;

    /*
     * Number of Bytes for Broadcast Burst
     */
    int nbOfBroadcastBytes_;

    /*
     * All elements are saved in an STL Map container
     */
    VirtualAllocationMap_t virtualAllocationMap_;

    /*
     * Iterator to the current element
     */
    VirtualAllocationMapIt_t mapIterator_;

};

#endif /* VIRTUALALLOCATION_H_ */
