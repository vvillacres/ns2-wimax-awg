/*
 * EdfListElement.cpp
 *
 *  Created on: 27.02.2010
 *      Author: elektron
 */

#include "edfqueueelement.h"

EdfQueueElement::EdfQueueElement()
{
	con_ = 0; // NULL
	deadline_ = 0.0;
	size_ = 0;
}

EdfQueueElement::EdfQueueElement(Connection * con, double deadline, int size)
{
	con_ = con;
	deadline_ = deadline;
	size_ = size;
}

bool EdfQueueElement::operator<(const EdfQueueElement& right) const
{
	return deadline_ < right.deadline_;
}

bool EdfQueueElement::operator>(const EdfQueueElement& right) const
{
	return deadline_ > right.deadline_;
}
