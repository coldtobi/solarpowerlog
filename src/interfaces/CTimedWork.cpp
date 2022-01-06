/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

Copyright (C) 2009-2014 Tobias Frost

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 ----------------------------------------------------------------------------
*/
/** \file CTimedWork.cpp
 *
 *  Created on: May 18, 2009
 *      Author: tobi
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// #define CTIMEDWORK_DEBUG

#include <iostream>
#include <list>
#include <time.h>

#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "CTimedWork.h"
#include "interfaces/CMutexHelper.h"
#include "configuration/Registry.h"

CTimedWork::CTimedWork( CWorkScheduler *sch ) :
        sch(sch), terminate(false)
#ifdef CTIMEDWORK_DEBUG
, dhc("CTimedWork")
#endif
{

#ifdef CTIMEDWORK_DEBUG

    this->work_completed=0; this->work_received=0;
    zero_waits = 0;
    dhc.Register(new CDebugObject<int>("work_received",work_received));
    dhc.Register(new CDebugObject<int>("work_completed",work_completed));

    thread_interrupts_count = thread_interrupts_sighandler = thread_interrupts_balance = 0;
    dhc.Register(new CDebugObject<int>("thread_interrupts_count",thread_interrupts_count));
    dhc.Register(new CDebugObject<int>("thread_interrupts_sighandler",thread_interrupts_sighandler));
    dhc.Register(new CDebugObject<int>("thread_interrupts_balance",thread_interrupts_balance));
    dhc.Register(new CDebugObject<int>("works_pending",works_pending));
    dhc.Register(new CDebugObject<int>("zero_waits",zero_waits));

    thread_at_wait_point = 0;
    thread_wants_mutex = 0;
    thread_has_mutex = 0;
    ctimedwork_has_mutex = 0;
    ctimedwork_wants_mutex = 0;

    dhc.Register(new CDebugObject<int>("thread_at_wait_point",thread_at_wait_point));
    dhc.Register(new CDebugObject<int>("thread_wants_mutex",thread_wants_mutex));
    dhc.Register(new CDebugObject<int>("ctimedwork_wants_mutex",ctimedwork_wants_mutex));
    dhc.Register(new CDebugObject<int>("thread_has_mutex",thread_wants_mutex));
    dhc.Register(new CDebugObject<int>("ctimedwork_has_mutex",ctimedwork_wants_mutex));

#endif

}

CTimedWork::~CTimedWork()
{
    if (!terminate)
        RequestTermination();
    thread.join();

    std::multimap<boost::posix_time::ptime, ICommand*, time_compare>::iterator it;
    for(it = TimedCommands.begin(); it != TimedCommands.end(); it++) {
        delete (*it).second;
    }
    TimedCommands.clear();
}

// Called on execution of the thread.
void CTimedWork::run()
{
    thread = boost::thread(boost::bind(&CTimedWork::_main, this));
}

void CTimedWork::ScheduleWork( ICommand *Command, struct timespec ts )
{
    bool need_interrupt = false;
    boost::posix_time::ptime first;
    boost::posix_time::ptime n =
            boost::posix_time::microsec_clock::local_time();
#ifdef CTIMEDWORK_DEBUG
    LOGDEBUG(Registry::GetMainLogger(),"NOW:\t" << n);
#endif

    boost::posix_time::seconds s(ts.tv_sec);
    boost::posix_time::millisec ms(ts.tv_nsec / (1000 * 1000));
    n = n + s + ms;
    {
#ifdef CTIMEDWORK_DEBUG
        ctimedwork_wants_mutex=1;
#endif
        CMutexAutoLock m(mut);
#ifdef CTIMEDWORK_DEBUG
        ctimedwork_wants_mutex=1;ctimedwork_has_mutex=1;
        this->work_received++;
#endif

        if (0 == TimedCommands.size()) {
#ifdef CTIMEDWORK_DEBUG
            LOGDEBUG(Registry::GetMainLogger(),"Q empty");
#endif
            need_interrupt = true;
        } else {
            first = (TimedCommands.begin())->first;
#ifdef CTIMEDWORK_DEBUG
            LOGDEBUG(Registry::GetMainLogger(),"first\t" << first << "\tnew " << n << " \t\tdelta " << n-first );
#endif
            if (first > n) {
                need_interrupt = true;
            }
        }

        TimedCommands.insert(
                pair<boost::posix_time::ptime, ICommand*>(n, Command));

        first = (TimedCommands.begin())->first;

#ifdef CTIMEDWORK_DEBUG
        if (need_interrupt)
            LOGDEBUG(Registry::GetMainLogger(),"new 1st\t" << first );
#endif

#ifdef CTIMEDWORK_DEBUG
        ctimedwork_wants_mutex=0;
#endif
        m.unlock(); // should not be needed, but paranoid me...
#ifdef CTIMEDWORK_DEBUG
        ctimedwork_has_mutex=0;
#endif
    }

#ifdef CTIMEDWORK_DEBUG
    (volatile int)thread_interrupts_balance++;
    thread_interrupts_count++;
#endif

    if (need_interrupt) {
#ifdef CTIMEDWORK_DEBUG
    	LOGDEBUG(Registry::GetMainLogger(),"interrupted");
#endif
        thread.interrupt();
    }

#ifdef CTIMEDWORK_DEBUG
    (volatile int)thread_interrupts_balance--;
#endif
}

void CTimedWork::_main()
{
    boost::posix_time::ptime n, w;
    boost::posix_time::time_duration s;

    while (!terminate) {

#ifdef CTIMEDWORK_DEBUG
        this->thread_wants_mutex=1;
#endif
        mut.lock();
#ifdef CTIMEDWORK_DEBUG
        this->thread_has_mutex=1;
#endif
        // get now
        n = boost::posix_time::microsec_clock::local_time();

#if 0
        cerr << "Now: " << (boost::posix_time::ptime)n << endl;
#endif
#if 0
        cerr << "TIMED WORKS EXPIRATION" << endl;
        multimap<boost::posix_time::ptime, ICommand*>::iterator it;
        for (it = TimedCommands.begin(); it != TimedCommands.end(); it++) {
            cerr << (*it).first << endl;
        }
        cerr << "TIMED WORKS LIST END" << endl;
#endif

#ifdef CTIMEDWORK_DEBUG
        this->works_pending = TimedCommands.size();
#endif

        if (!TimedCommands.empty()) {
            w = (TimedCommands.begin())->first;
            // cerr << "Waiting: " << to_simple_string(w) << endl;

            if (w > n) {
                s = w - n;
                // cerr << "Difference: " << to_simple_string(s)  << endl;
            } else {
                ICommand *cmd = (TimedCommands.begin())->second;
                TimedCommands.erase(TimedCommands.begin());

#ifdef CTIMEDWORK_DEBUG
                this->works_pending = TimedCommands.size();
                work_completed++;
                this->thread_wants_mutex=0;
#endif
                mut.unlock();
#ifdef CTIMEDWORK_DEBUG
                this->thread_has_mutex=0;
#endif

                sch->ScheduleWork(cmd);
                continue;
            }
        } else {
#ifdef CTIMEDWORK_DEBUG
            this->zero_waits++; // wait requests for empty queues
#endif
            s = boost::posix_time::hours(1);
        }

#ifdef CTIMEDWORK_DEBUG
        this->thread_wants_mutex=0;
#endif
        mut.unlock();
#ifdef CTIMEDWORK_DEBUG
        this->thread_has_mutex=0;
#endif

        try {
            // cerr << "sleeping for " << boost::posix_time::to_simple_string(s) << endl;
#ifdef CTIMEDWORK_DEBUG
            this->thread_at_wait_point=1;
#endif
            boost::this_thread::sleep(s);
#ifdef CTIMEDWORK_DEBUG
            this->thread_at_wait_point=0;
#endif
        } catch (boost::thread_interrupted &e) {
            // sleep was interrupted, probably by new work.
#ifdef CTIMEDWORK_DEBUG
            thread_interrupts_sighandler++;
#endif
        }
    }
}

void CTimedWork::RequestTermination( void )
{
    terminate = true;
    thread.interrupt();
}

