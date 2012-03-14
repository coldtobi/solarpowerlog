/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

Copyright (C) 2009-2012 Tobias Frost

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

#include <semaphore.h>
#include <map>

#include <boost/thread/mutex.hpp>

#include "interfaces/CDebugHelper.h"

class ICommand;
class ICommandTarget;
class CTimedWork;

using namespace std;


/** This class implements the work scheduler:
 *
 * Objects derived from CommandTarget can schedule work to be done:
 *
 * call again later:
 * (when they are not able to complete it immediatly)
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

	void ScheduleWork(ICommand *Command);

	/** Schedule a work for later */
	void ScheduleWork(ICommand *Commmand, struct timespec ts);

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

	CTimedWork *timedwork;

	list<ICommand*> CommandsDue;

#if 0
	struct timepec_compare
	{
		 bool operator()(const struct timespec t1, const struct timespec t2) const
		  {
			 if(t1.tv_sec < t2.tv_sec) return true;
			 if(t1.tv_sec > t2.tv_sec) return false;
			 if(t1.tv_nsec < t2.tv_nsec) return true;
			 return false;
		  };
	};

	multimap<struct timespec, ICommand*, timepec_compare> TimedCommands;
#endif

	/** get the next new command in the list.
	 * (Thread safe)*/
	ICommand *getnextcmd(void);

private:
	sem_t semaphore;

protected:
	boost::mutex mut;

private:
	CDebugHelperCollection dhc;
	int works_received;
	int works_completed;
	int works_timed_scheduled;

};

#endif /* CWORKSCHEDULER_H_ */
