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

/** \file CTimedWork.h
 *
 *  Created on: May 18, 2009
 *      Author: tobi
 */

#ifndef CTIMEDWORK_H_
#define CTIMEDWORK_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "patterns/ICommand.h"
#include "CWorkScheduler.h"
#include "boost/date_time/posix_time/posix_time.hpp"

#include <boost/thread.hpp>

#include "interfaces/CDebugHelper.h"

/** This class bundles a timed activity.
 *
 * The Timedwork are fed into a worker thread, which just waits for the time
 * to come and signal the event back to the CWorkScheduler object.
 *
 * On new requests, the worker thread will only be interrupted if the new event
 * expires earlier than the current one.
 *
 * \note: There was an bug in boost::thread in boost release 1.46 which
 * caused solarpowerlog to freeze after a while. This is why the code is
 * still littered with debug statements, enabled with the define
 * CTIMEDWORK_DEBUG (via CTimedWork.cpp or via C[XX]FLAGS)
 */
class CTimedWork
{
public:
    /** Constructor: Takes the scheduler to inform, the command to execute and the time when */
    explicit CTimedWork( CWorkScheduler *sch );

    virtual ~CTimedWork();

    /// Thread entry point.
    void run();

    /// Issue work in the future. (when ts elapsed)
    void ScheduleWork( ICommand *Command, struct timespec ts );

    /// Ask thread to terminate.
    void RequestTermination( void );

private:
    CTimedWork()
    : sch(NULL)
#ifdef CTIMEDWORK_DEBUG
        ,dhc("CTimedWork")
#endif
    { }

    void _main( void );

    /** Helper struct for the multimap -- to sort the entries. */
    struct time_compare
    {
        bool operator()( const boost::posix_time::ptime t1,
                const boost::posix_time::ptime t2 ) const
        {
            if (t1 > t2)
                return false;
            return true;
        }
    };

    /** holds the command and timing information.
     * The multimap is sorted accordingly. */
    std::multimap<boost::posix_time::ptime, ICommand*, time_compare> TimedCommands;

    /** it is attached to this scheduler.
     * (The scheduler keeps books of its processes)*/
    CWorkScheduler *sch;

    volatile bool terminate;

    boost::thread thread;

    boost::mutex mut;

#ifdef CTIMEDWORK_DEBUG
private:
    CDebugHelperCollection dhc;
    int work_received, work_completed;
    int thread_interrupts_count;
    int thread_interrupts_sighandler;
    int thread_interrupts_balance;
    int works_pending;
    int zero_waits;
    int thread_at_wait_point;
    int thread_wants_mutex;
    int ctimedwork_wants_mutex;
    int thread_has_mutex;
    int ctimedwork_has_mutex;
#endif

};

#endif /* CTIMEDWORK_H_ */
