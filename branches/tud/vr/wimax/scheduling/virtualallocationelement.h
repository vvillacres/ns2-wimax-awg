/*
 * VirtualAllocationElement.h
 *
 *  Created on: 07.09.2010
 *      Author: richter
 */

#ifndef VIRTUALALLOCATIONELEMENT_H_
#define VIRTUALALLOCATIONELEMENT_H_

//#include <sys/types.h>
#include "wimaxscheduler.h"

class VirtualAllocationElement
{
public:
    VirtualAllocationElement( int cid, int wantedMstrSize = 0, int wantedMrtrSize = 0, int nbOfSlots = 0, int nbOfBytes = 0);
    virtual ~VirtualAllocationElement();


    /*
     * Returns cid of the corresponding connection
     */
    inline int getCid() {
        return cid_;
    }

    /*
     * Set cid of the corresponding connection
     */
    inline void setCid( int cid) {
        cid_ = cid;
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
     * Returns the number of allocated slots for the corresponding connection
     */
    inline int getNbOfAllocatedSlots() {
        return nbOfSlots_;
    }

    /*
     * Returns the number of allocated bytes for the corresponding connection
     */
    inline int getNbOfAllocatedBytes() {
        return nbOfBytes_;
    }



    /*
     * Set the number of allocated slots for the corresponding connection
     */
    inline void setNbOfAllocatedSlots( int nbOfSlots) {
        nbOfSlots_ = nbOfSlots;
    }

    /*
     * Set the number of allocated bytes for the corresponding connection
     */
    inline void setNbOfAllocatedBytes( int nbOfBytes) {
        nbOfBytes_ = nbOfBytes;
    }


private:

    /*
     * CID of the corresponding Connection
     */
    int cid_;

    /*
     * Maximum Number of Bytes resulting of the Maximum Sustained Traffic Rate value
     */
    u_int32_t wantedMstrSize_;

    /*
     * Minimum Number of Bytes to fulfill the Minimum Reserved Traffic Rate value
     */
    u_int32_t wantedMrtrSize_;

    /*
     * Number of allocated Slots
     */
    int nbOfSlots_;

    /*
     * Number of allocated Bytes
     */
    int nbOfBytes_;


};

#endif /* VIRTUALALLOCATIONELEMENT_H_ */
