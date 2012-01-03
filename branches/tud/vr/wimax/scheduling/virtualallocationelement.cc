/*
 * VirtualAllocationElement.cpp
 *
 *  Created on: 07.09.2010
 *      Author: richter
 */

#include "virtualallocationelement.h"
#include "config.h"

VirtualAllocationElement::VirtualAllocationElement(Connection* connectionPtr, u_int32_t wantedMrtrSize, u_int32_t wantedMstrSize, int slotCapacity, int nbOfBytes, int nbOfSlots, bool isCdmaAlloc, int cdmaTop, int cdmaCode)
{
    // set member variables
    connectionPtr_ = connectionPtr;
    wantedMrtrSize_ = wantedMrtrSize;
    wantedMstrSize_ = wantedMstrSize;
    slotCapacity_ = slotCapacity;
    nbOfBytes_ = nbOfBytes;
    nbOfSlots_ = nbOfSlots;
    isCdmaAlloc_ = isCdmaAlloc;
    cdmaTop_ = cdmaTop;
    cdmaCode_ = cdmaCode;
    allocatedMrtrPayload_ = 0;
    allocatedMstrPayload_ = 0;

}

VirtualAllocationElement::~VirtualAllocationElement()
{

}
