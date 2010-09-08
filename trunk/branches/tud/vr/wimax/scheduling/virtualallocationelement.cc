/*
 * VirtualAllocationElement.cpp
 *
 *  Created on: 07.09.2010
 *      Author: richter
 */

#include "virtualallocationelement.h"
#include "config.h"

VirtualAllocationElement::VirtualAllocationElement(Connection* connectionPtr, u_int32_t wantedMstrSize, u_int32_t wantedMrtrSize, int slotCapacity, int nbOfSlots, int nbOfBytes)
{
    // set member variables
    connectionPtr_ = connectionPtr;
    wantedMstrSize_ = wantedMstrSize;
    wantedMrtrSize_ = wantedMrtrSize;
    nbOfSlots_ = nbOfSlots;
    nbOfBytes_ = nbOfBytes;
}

VirtualAllocationElement::~VirtualAllocationElement()
{

}
