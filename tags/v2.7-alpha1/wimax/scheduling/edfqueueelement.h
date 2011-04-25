/*
 * EdfQueueElement.h
 *
 *  Created on: 27.02.2010
 *      Author: elektron
 */

#ifndef EDFQUEUEELEMENT_H_
#define EDFQUEUEELEMENT_H_


class Connection;


class EdfQueueElement {
public:
	EdfQueueElement();
	EdfQueueElement(Connection * con, double deadline, int size);

	inline Connection * getConnection() const { return con_; }
	inline double getDeadline() const { return deadline_; }
	inline int getSize() const { return size_; }

	bool operator<(const EdfQueueElement& right) const;
	bool operator>(const EdfQueueElement& right) const;

private:

	Connection * con_;
	double deadline_;
	int size_;


};

#endif /* EDFQUEUEELEMENT_H_ */
