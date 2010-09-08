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

typedef map< int, VirtualAllocationElement > VirtualAllocationMap_t;
typedef VirtualAllocationMap_t::iterator VirtualAllocationMapIt_t;

class VirtualAllocation
{
public:
    VirtualAllocation();
    virtual ~VirtualAllocation();


    /*
     * Get number of Slots for Broadcast Burst
     */
    inline int getNbOfBroadcastSlots() {
        return nbOfBroadcastSlots_;
    }

    /*
     * Set number of Slots for Broadcast Burst
     */
    inline void setNbOfBroadcastSlots( int nbOfBroadcastSlots) {
        nbOfBroadcastSlots_ = nbOfBroadcastSlots;
    }

    /*
     * Get number of Byte for Broadcast Burst
     */
    inline int getNbOfBroadcastBytes() {
        return nbOfBroadcastBytes_;
    }

    /*
     * Set number of Byte for Broadcast Burst
     */
    inline void setNbOfBroadcastBytes( int nbOfBroadcastBytes) {
        nbOfBroadcastBytes_ = nbOfBroadcastBytes;
    }

    /*
     * Adds a new Virtual Allocation in the Map
     */
    void addAllocation( int cid);

    /*
     * Adds a new Virtual Allocation in the Map
     */
    void addAllocation( int cid, u_int32_t wantedMstrSize, u_int32_t wantedMrtrSize, int nbOfSlots = 0, int nbOfBytes = 0);

    /*
     * Returns true if an Entry of this CID exits and set the current Iterator
     */
    bool findCidEntry(int cid);

    /*
     * Returns the number of allocated slots for the current connection
     */
    int getCurrentNbOfSlots();

    /*
     * Returns the number of allocated bytes for the current connection
     */
    int getCurrentNbOfBytes();

    /*
     * Update values of the current virtual allocation
     */
    void updateAllocation( int nbOfslots, int nbOfBytes);


private:

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
