/*
 * VirtualAllocationElement.cpp
 *
 *  Created on: 07.09.2010
 *      Author: richter
 */

#include "virtualallocationelement.h"
#include "config.h"

VirtualAllocationElement::VirtualAllocationElement(Connection* connectionPtr, u_int32_t wantedMrtrSize, u_int32_t wantedMstrSize, int slotCapacity, int nbOfBytes, int nbOfSlots, int nbOfCdmaSlots)
{
    // set member variables
    connectionPtr_ = connectionPtr;
    wantedMrtrSize_ = wantedMrtrSize;
    wantedMstrSize_ = wantedMstrSize;
    slotCapacity_ = slotCapacity;
    nbOfBytes_ = nbOfBytes;
    nbOfSlots_ = nbOfSlots;
    nbOfCdmaSlots_ = nbOfCdmaSlots;
    allocatedMrtrPayload_ = 0;
    allocatedMstrPayload_ = 0;

}

VirtualAllocationElement::~VirtualAllocationElement()
{

}
