/*
 * VirtualAllocationElement.h
 *
 *  Created on: 07.09.2010
 *      Author: richter
 */

#ifndef VIRTUALALLOCATIONELEMENT_H_
#define VIRTUALALLOCATIONELEMENT_H_

#include <sys/types.h>

class Connection;

class VirtualAllocationElement
{
public:
    VirtualAllocationElement( Connection* connectionPtr, u_int32_t wantedMstrSize, u_int32_t wantedMrtrSize, int slotCapacity, int nbOfBytes, int nbOfSlots, int nbOfCdmaSlots );
    virtual ~VirtualAllocationElement();


    /*
     * Returns cid of the corresponding connection
     */
    inline Connection* getConnectionPtr() {
        return connectionPtr_;
    }

    /*
     * Set cid of the corresponding connection
     */
    inline void setConnectionPtr( Connection* connectionPtr) {
        connectionPtr_ = connectionPtr;
    }


    /*
     * Returns the maximum number of Bytes resulting of the Maximum Sustained Traffic Rate value
     */
    inline u_int32_t getWantedMstrSize() {
        return wantedMstrSize_;
    }

    /*
     * Set the maximum number of Bytes resulting of the Maximum Sustained Traffic Rate value
     */
    inline void setWantedMstrSize( u_int32_t wantedMstrSize) {
        wantedMstrSize_ = wantedMstrSize;
    }

    /*
     * Returns minimum Number of Bytes to fulfill the Minimum Reserved Traffic Rate value
     */
    inline u_int32_t getWantedMrtrSize() {
        return wantedMrtrSize_;
    }

    /*
     * Set minimum Number of Bytes to fulfill the Minimum Reserved Traffic Rate value
     */
    inline void setWantedMrtrSize( u_int32_t wantedMrtrSize) {
        wantedMrtrSize_ = wantedMrtrSize;
    }

    /*
     * Returns the transmitting capacity of one slot in Byte
     */
    inline int getSlotCapacity() {
    	return slotCapacity_;
    }

    /*
     * Set slot capacity
     */
    inline void setSlotCapacity(int slotCapacity) {
    	slotCapacity_ = slotCapacity;
    }


    /*
     * Returns the number of allocated bytes for the corresponding connection
     */
    inline int getNbOfAllocatedBytes() {
        return nbOfBytes_;
    }

    /*
     * Set the number of allocated bytes for the corresponding connection
     */
    inline void setNbOfAllocatedBytes( int nbOfBytes) {
        nbOfBytes_ = nbOfBytes;
    }


    /*
     * Returns the number of allocated slots for the corresponding connection
     */
    inline int getNbOfAllocatedSlots() {
        return nbOfSlots_;
    }

    /*
     * Set the number of allocated slots for the corresponding connection
     */
    inline void setNbOfAllocatedSlots( int nbOfSlots) {
        nbOfSlots_ = nbOfSlots;
    }

    /*
     * Returns the number of allocated slots for the corresponding connection
     */
    inline int getNbOfAllocatedCdmaSlots() {
        return nbOfCdmaSlots_;
    }

    /*
     * Set the number of allocated slots for the corresponding connection
     */
    inline void setNbOfAllocatedCdmaSlots( int nbOfCdmaSlots) {
        nbOfCdmaSlots_ = nbOfCdmaSlots;
    }



private:

    /*
     * Pointer to the corresponding connection
     */
    Connection* connectionPtr_;

    /*
     * Maximum Number of Bytes resulting of the Maximum Sustained Traffic Rate value
     */
    u_int32_t wantedMstrSize_;

    /*
     * Minimum Number of Bytes to fulfill the Minimum Reserved Traffic Rate value
     */
    u_int32_t wantedMrtrSize_;

    /*
     * Slot capacity
     */
    int slotCapacity_;

    /*
     * Number of allocated Slots
     */
    int nbOfSlots_;

    /*
     * Number of allocated Bytes
     */
    int nbOfBytes_;

    /*
     * Number of allocated Slots for CDMA request
     */
    int nbOfCdmaSlots_;


};

#endif /* VIRTUALALLOCATIONELEMENT_H_ */
