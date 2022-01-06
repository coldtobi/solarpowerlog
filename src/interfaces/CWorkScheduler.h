/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

Copyright (C) 2009-2014 Tobias Frost

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 ----------------------------------------------------------------------------
 */

/** \file CWorkScheduler.h
 *
 *  Created on: May 17, 2009
 *      Author: tobi
 */

#ifndef CWORKSCHEDULER_H_
#define CWORKSCHEDULER_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "porting.h"
#endif

#include <time.h>
#include <list>
#include <set>

#include <semaphore.h>

#include <boost/thread/mutex.hpp>

#include "interfaces/CDebugHelper.h"

class ICommand;
class ICommandTarget;
class CTimedWork;

/** This class implements the work scheduler:
 *
 * Objects derived from CommandTarget can schedule work to be done:
 *
 * call again later:
 * (when they are not able to complete it immediately)
 *
 * call again at ....
 * (when they expect to do some work in some specific time
 *
 * \warning for all actions, the Objects will be passed by reference. At this
 * point of time, CWorkScheduler is owner of the object and will destroy it later,
 * when used.
 *
*/
class CWorkScheduler {

	friend class CTimedWork;

public:
	CWorkScheduler();
	virtual ~CWorkScheduler();

	/** Schedule an immediate work
	 *
	 * \param Command to be issued
	 * \param tryonly does not issue the work if the underlying mutex cannot be
	 * obtained immediately (useful if called from e.g signal handler)
	 *
	 * \returns true if work has been scheduled, false if not
	 * It is guaranteed to return true if tryonly is false.
	*/
	bool ScheduleWork(ICommand *Command, bool tryonly=false);

	/** Schedule a work for later */
	void ScheduleWork(ICommand *Commmand, struct timespec ts);

	/** Register for broadcast events.
	 *
	 * There (will) be some broadcast events, and if your datafilter/inverter
	 * is interested in those.
	 *
	 * See the file BasicCommands.h for defined broadcast events, but those
	 * events are only broadcasted through the mainscheduler.
	 *
	 * A specialty about broadcast events is, that they will have no callback
	 * in the ICommand. This is asserted by ScheduleWork().
	 *
	 * \param target which wants to receive the broadcast events
	 * \param subscribe set to false if you want to unsubscribe to the events.
	 *
	 * \note solarpowerlog will only issue
	*/
	void RegisterBroadcasts(ICommandTarget *target, bool subscribe=true);

	/** Call this method to do dispatch due work.
	 * Note: Returns after each piece of work has been done!
	 *
	 * returns true, if work has been done, false, if no work was available.
	 *
	 * \param block if false (default), it will return if there is no work, else it will
	 * wait for work.
	*/
	bool DoWork(bool block=false);

private:

	/// Stores the CTimedWork Object, the handler for works to be executed in
	/// at a specific time.
	CTimedWork *timedwork;

	/// Stores the pending work
	std::list<ICommand*> CommandsDue;

	/** get the next new command in the list.
	 * (Thread safe)*/
	ICommand *getnextcmd(void);

private:
	/// Semaphore to wait on until work arrives (probably via timed works
	sem_t semaphore;

	/// stores for the mainscheduler the list of broadcast subscribers
	std::set<ICommandTarget*> broadcast_subscribers;

protected:
	/// Mutex to protect against concurrent accesses.
	boost::mutex mut;

private:
	CDebugHelperCollection dhc;
	int works_received;
	int works_completed;
	int works_timed_scheduled;

};

#endif /* CWORKSCHEDULER_H_ */
