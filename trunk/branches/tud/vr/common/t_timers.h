/**
 *  project: VoIP module for ns2
 *  filename: generic_timers.h
 *       author: C. Cicconetti <c.cicconetti@iet.unipi.it>
 *               A. Erta <alessandro.erta@imtlucca.it>
 *               A. Bacioccola <a.bacioccola@iet.unipi.it>
 *  year: 2007
 *  affiliation:
 *       Dipartimento di Ingegneria dell'Informazione
 *       University of Pisa, Italy
 *  description:
 *       Template timers (T_Timers)
 */

#ifndef __NS2_T_TIMERS_H
#define __NS2_T_TIMERS_H

#include <scheduler.h>
#include <object.h> // NOW definition

#include <queue>
#include <map>
#include <list>
#include <iostream>

//! Generic timer interface.
template<typename T>
class TTimer : public Handler {
public:
        //! Build an idle timer.
        TTimer (T* obj) { obj_ = obj;
                                busy_ = false;
                                start_ = 0;
                                expire_ = 0; }

        //! Do nothing.
        virtual ~TTimer () { }

        //! Handle the event.
        virtual void handle (Event* e);

        //! Start the timer. The timer is now busy.
        /**
         * If the timer has been already started, then probably there is
         * a programming error. Thus, we abort execution immediately.
         */
        virtual void start (double t);

        //! Stop the timer, if busy. The timer is now idle.
        virtual void stop ();

        //! Return true if the timer is busy.
        virtual bool busy () { return busy_; }

        //! Return true if the timer is idle.
        virtual bool idle () { return ! busy_; }

        //! Return the elapsed time from timer start.
        virtual double elapsed () { return NOW - start_; }

        //! Return the remaining time before expiration.
        virtual double remaining () { return expire_ - NOW; }

protected:
        //! Object for handling the event.
        T* obj_;

        //! Used for scheduling an event into the ns2 event scheduler
        Event intr_;

        //! True if the the timer has been already started.
        bool busy_;

        //! Last time the timer was started.
        double start_;

        //! Expire time.
        double expire_;
};

template<typename T>
void
TTimer<T>::start (double t)
{
        if ( busy_ ) abort();
        Scheduler::instance().schedule (this, &intr_, t);
        busy_ = true;
        start_ = NOW;
        expire_ = NOW + t;
}

template<typename T>
void
TTimer<T>::stop ()
{
        if ( busy_ ) Scheduler::instance().cancel (&intr_);
        busy_ = false;
}

template<typename T>
void
TTimer<T>::handle (Event* e)
{
        busy_ = false;
        obj_->handle ();
}

//! Generic multi-timer interface.
/*!
  A multi-timer is a timer that handles many events. Events are just added
  to the multi-timer, which will dispatch them all when needed. To
  distinguish between different events, if needed, the handler is called
  with an argument of a specified user data type.
  */
template<typename T, typename R>
class TMultiTimer : public Handler {
public:
	//! Build an idle multi-timer.
	TMultiTimer (T* obj) { obj_ = obj; busy_ = false; }
	//! Do nothing.
	virtual ~TMultiTimer () { }
	//! Handle the event.
	virtual void handle (Event* e);
	//! Add an event to the multi-timer, with a specified parameter.
	virtual void add (double t, R arg);
	//! Return true if there is at least one scheduled event.
	virtual bool busy () { return busy_; }
	//! Return true if there are no scheduled events.
	virtual bool idle () { return ! busy_; }
protected:
	//! Object for handling the event.
	T* obj_;
	//! Used for scheduling an event into the ns2 event scheduler
	Event intr_;
	//! True if there is at least one scheduled event.
	bool busy_;
	//! Priority queue to store the event list.
   /*!
     Note that finish times are stored as negative values, so as
     to have the earlierst event scheduled first.
     */
	std::priority_queue< std::pair<double,R> > prio_;
};

template<typename T, typename R>
void
TMultiTimer<T, R>::add (double t, R arg)
{
	double remaining = -1.0;

	// retrieve the ns2 event expire time
	if ( busy_ ) {
		std::pair<double, R> earliest = prio_.top();
		remaining = - ( NOW + earliest.first );
	}

	// add the event to the priority queue, along with its argument
	prio_.push (std::pair<double, R> (-(NOW + t), arg));

	// if this event finishes earlier than the earliest scheduled event, if any
	// then we have to reschedule the ns2 event
	if ( ! busy_ || t < remaining ) {
		if ( busy_ ) Scheduler::instance().cancel (&intr_);
		Scheduler::instance().schedule (this, &intr_, t);
	}

	busy_ = true;
}

template<typename T, typename R>
void
TMultiTimer<T, R>::handle (Event* e)
{
	// retrieve the earliest event to dispatch
	std::pair<double, R> first = prio_.top();

	// remove it
	prio_.pop ();

	// reschedule the timer to manage the earliest event, if any
	if ( prio_.empty() )
		busy_ = false;
	else {
		std::pair<double, R> second = prio_.top();
		Scheduler::instance().schedule (this, &intr_, - ( NOW + second.first ) );
	}

	// finally, handle the event
	obj_->handle (first.second);
}

//! Multi-timer enhanced.T
/*!
    Same as the multitime but events can be removed
 */

template<typename T, typename R>
class TMultiTimerPlus : public Handler {
public:
        //! Build an idle multi-timer.
        TMultiTimerPlus (T* obj) { obj_ = obj; busy_ = false; }
        //! Do nothing.
        virtual ~TMultiTimerPlus () { }
        //! Handle the event.
        virtual void handle (Event* e);
        //! Add an event to the multi-timer, with a specified parameter.
        virtual void add (double t, R arg);
        //! Remove an event. Note that event id are assumed to be unique
        virtual void remove ( R arg );
        //! Return true if there is at least one scheduled event.
        virtual bool busy () { return busy_; }
        //! Return true if the event id exists
        virtual bool busy (R arg);
        //! Return true if there are no scheduled events.
        virtual bool idle () { return ! busy_; }
protected:
        //! Object for handling the event.
        T* obj_;
        //! Used for scheduling an event into the ns2 event scheduler
        Event intr_;
        //! True if there is at least one scheduled event.
        bool busy_;
        //! The event list.
        std::list< std::pair<double,R> > ordList_;
        //! The removable list. Each event id matches with a pair.
        std::map< R, std::pair<double, R> > removeMap_;
};

template<typename T, typename R>
void
TMultiTimerPlus<T, R>::add (double t, R arg)
{
        double remaining = 1.0;

        // retrieve the ns2 event expire time
        if ( busy_ ) {
                std::pair<double, R> earliest = ordList_.front();
                remaining = ( earliest.first -  NOW );
        }

        // add the event to the priority queue, along with its argument
        ordList_.push_back (std::pair<double, R> ((NOW + t), arg));

        // the list should be ordered each time :-(
        ordList_.sort();

        // add the information to the removeMap_
        // If the element already exists abort
        if ( removeMap_.find(arg) != removeMap_.end() ) {
                fprintf(stderr, "MultiTimerPlus - Error\n");
                abort();
        }
        removeMap_[ arg ] = std::pair<double, R> ((NOW + t), arg);

        // if this event finishes earlier than the earliest scheduled event, if any
        // then we have to reschedule the ns2 event
        if ( ! busy_ || t < remaining ) {
                if ( busy_ ) Scheduler::instance().cancel (&intr_);
                Scheduler::instance().schedule (this, &intr_, t);
        }

        busy_ = true;
}
template<typename T, typename R>
bool
TMultiTimerPlus<T, R>::busy (R arg)
{
        if ( removeMap_.find(arg) != removeMap_.end() ) {
                return true;
        }
        else {
                return false;
        }
}

template<typename T, typename R>
void
TMultiTimerPlus<T, R>::handle (Event* e)
{
        // retrieve the earliest event to dispatch
        std::pair<double, R> first = ordList_.front();

        // remove it
        ordList_.pop_front ();

        // remove the information also in the removeMap_
        removeMap_.erase ( first.second );

        // reschedule the timer to manage the earliest event, if any
        if ( ordList_.empty() )
                busy_ = false;
        else {
                std::pair<double, R> second = ordList_.front();
                Scheduler::instance().schedule (this, &intr_,( second.first - NOW ) );
        }

        // finally, handle the event
        obj_->handle (first.second);
}

template<typename T, typename R>
void
TMultiTimerPlus<T, R>::remove( R arg )
{
        typename std::map< R, std::pair<double, R> >::iterator it = removeMap_.find( arg );

        if ( it == removeMap_.end() ) abort (); // not possible

        // check if the event is the earliest to be scheduled
        if ( it->second == ordList_.front() ) {
                // remove it
                ordList_.pop_front ();
                Scheduler::instance().cancel (&intr_);
                // schedule next event
                if ( ordList_.empty() )
                        busy_ = false;
                else {
                        std::pair<double, R> second = ordList_.front();
                        Scheduler::instance().schedule (this, &intr_, ( second.first - NOW ) );
                }
        } else ordList_.remove ( it->second );

        // remove from the remove list
        removeMap_.erase ( it );

}

#endif // __NS2_T_TIMERS_H 
