/*
 * VirtualAllocationElement.cpp
 *
 *  Created on: 07.09.2010
 *      Author: richter
 */

#include "virtualallocationelement.h"

VirtualAllocationElement::VirtualAllocationElement(int cid, int wantedMstrSize, int wantedMrtrSize, int nbOfSlots, int nbOfBytes)
{
    // set member variables
    cid_ = cid;
    wantedMstrSize_ = wantedMstrSize;
    wantedMrtrSize_ = wantedMrtrSize;
    nbOfSlots_ = nbOfSlots;
    nbOfBytes_ = nbOfBytes;
}

VirtualAllocationElement::~VirtualAllocationElement()
{

}
