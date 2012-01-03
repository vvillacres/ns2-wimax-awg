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
    VirtualAllocationElement( Connection* connectionPtr, u_int32_t wantedMrtrSize, u_int32_t wantedMstrSize, int slotCapacity, int nbOfBytes, int nbOfSlots, bool isCdmaAlloc, int cdmaTop, int cdmaCode);
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


    inline bool isCdmaAlloc() {
    	return isCdmaAlloc_;
    }


    inline void setCdmaAlloc( bool isCdmaAlloc) {
    	isCdmaAlloc_ = isCdmaAlloc;
    }

    /*
     * Returns the number of allocated slots for the corresponding connection
     */
    inline int getCdmaTop() {
        return cdmaTop_;
    }

    /*
     * Set the number of allocated slots for the corresponding connection
     */
    inline void setCdmaTop( int cdmaTop) {
        cdmaTop_ = cdmaTop;
    }

    /*
     * Returns the number of allocated slots for the corresponding connection
     */
    inline int getCdmaCode() {
        return cdmaCode_;
    }

    /*
     * Set the number of allocated slots for the corresponding connection
     */
    inline void setCdmaCode( int cdmaCode) {
        cdmaCode_ = cdmaCode;
    }

    /*
     * Returns minimum Number of allocated Bytes assigned to fulfill the Minimum Reserved Traffic Rate value
     */
    inline u_int32_t getAllocatedMrtrPayload() {
        return allocatedMrtrPayload_;
    }

    /*
     * Set minimum Number of Bytes to fulfill the Minimum Reserved Traffic Rate value
     */
    inline void setAllocatedMrtrPayload( u_int32_t allocatedMrtrPayload) {
        allocatedMrtrPayload_ = allocatedMrtrPayload;
    }

    /*
     * Returns the maximum number of Bytes resulting of the Maximum Sustained Traffic Rate value
     */
    inline u_int32_t getAllocatedMstrPayload() {
        return allocatedMstrPayload_;
    }

    /*
     * Set the maximum number of Bytes resulting of the Maximum Sustained Traffic Rate value
     */
    inline void setAllocatedMstrPayload( u_int32_t allocatedMstrPayload) {
        allocatedMstrPayload_ = allocatedMstrPayload;
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
     * Flag for CDMA Allocations
     */
    bool isCdmaAlloc_;

    /*
     * Used CDMA slot ( CDMA Top)
     */
    int cdmaTop_;

    /*
     * Used CDMA code
     */
    int cdmaCode_;

    /*
     * fulfilled mrtr payload
     */
    u_int32_t allocatedMrtrPayload_;

    /*
     * fulfilled mstr payload
     */
    u_int32_t allocatedMstrPayload_;
};

#endif /* VIRTUALALLOCATIONELEMENT_H_ */
