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

/** \file CWorkScheduler.cpp
 *
 *  Created on: May 17, 2009
 *      Author: tobi
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <boost/exception/all.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include "CWorkScheduler.h"

#include "patterns/ICommand.h"

#include "CTimedWork.h"

#include "semaphore.h"
#include "interfaces/CMutexHelper.h"

#include "patterns/ICommandTarget.h"
#include "configuration/Registry.h"

using namespace std;

CWorkScheduler::CWorkScheduler() :
    dhc("CWorkScheduler")
{
    works_received = 0;
    works_completed = 0;
    works_timed_scheduled = 0;

    dhc.Register(new CDebugObject<void*>("instance", this));
    dhc.Register(new CDebugObject<int>("works_received", works_received));
    dhc.Register(new CDebugObject<int>("works_completed", works_completed));
    dhc.Register(
        new CDebugObject<int>("works_timed_scheduled", works_timed_scheduled));

    sem_init(&semaphore, 0, 0);

    // generate thread for the timed work facility.
    timedwork = new CTimedWork(this);

    timedwork->run();
}

CWorkScheduler::~CWorkScheduler()
{
    delete timedwork;
    broadcast_subscribers.clear();

    sem_destroy(&semaphore);
}

bool CWorkScheduler::DoWork(bool block)
{
    if (!block) {
        CMutexAutoLock cma(mut);
        if (CommandsDue.empty()) {
            return false;
        }
    }

    sem_wait(&semaphore);
    ICommand *cmd = getnextcmd();
    if (cmd->getCmd() > BasicCommands::CMD_BROADCAST_MAX) {
        cmd->execute();
    } else {
        if (!broadcast_subscribers.empty()) {
            LOGDEBUG_SA(Registry::GetMainLogger(), LOG_SA_HASH("CWSSubscriber"),
                "Handling broadcast-event cmd=" << cmd->getCmd() <<
                " subscribers=" << broadcast_subscribers.size());
            std::set<ICommandTarget*>::iterator it;
            for (it = broadcast_subscribers.begin();
                it != broadcast_subscribers.end(); it++) {
                (*it)->ExecuteCommand(cmd);
            }
        } else {
            LOGDEBUG_SA(Registry::GetMainLogger(), LOG_SA_HASH("CWSSubscriber"),
                "Handling broadcast-event cmd=" << cmd->getCmd() <<
                " NO subscribers");
        }
    }
    delete cmd;
    return true;
}

void CWorkScheduler::RegisterBroadcasts(ICommandTarget* target, bool subscribe)
{
    if (subscribe) {
        broadcast_subscribers.insert(target);
    } else {
        broadcast_subscribers.erase(target);
    }
}

ICommand *CWorkScheduler::getnextcmd(void)
{
    // Obtain Mutex to make sure...
    CMutexAutoLock cma(mut);
    ICommand *cmd = CommandsDue.front();
    CommandsDue.pop_front();
    works_completed++;
    return cmd;
}

bool CWorkScheduler::ScheduleWork(ICommand *Command, bool tryonly)
{
    // assert if a broadcast event has a ITarget set. (This indicates a bug)
    //LOGERROR(Registry::GetMainLogger(),"cmd=" <<Command->getCmd() << " trgt="<< Command->getTrgt());

    assert(
        !((Command->getCmd() <= BasicCommands::CMD_BROADCAST_MAX
            && Command->getTrgt())));

    if (Command->getCmd() >= BasicCommands::CMD_BROADCAST_MAX
        && !Command->getTrgt()) {
        // Fire-and-Forget commmand. Just delete it.
        delete Command;
        return true;
    }

    if (tryonly) {
        if (!mut.try_lock()) return false;
    } else {
        mut.lock();
    }

    if (Command->getCmd() <= BasicCommands::CMD_BROADCAST_MAX) {
        LOGDEBUG(Registry::GetMainLogger(),
            "Broadcast event accepted cmd=" << Command->getCmd());
    }

    CommandsDue.push_back(Command);
    works_received++;
    mut.unlock();
    sem_post(&semaphore);
    return true;
}

void CWorkScheduler::ScheduleWork(ICommand *Command, struct timespec ts)
{
    // assert if a broadcast event has a ITarget set. (This indicates a bug)
    assert(
        !((Command->getCmd() <= BasicCommands::CMD_BROADCAST_MAX
            && Command->getTrgt())));

    if (Command->getCmd() >= BasicCommands::CMD_BROADCAST_MAX
        && !Command->getTrgt()) {
        // Fire-and-Forget command. Just delete it.
        delete Command;
        return;
    }

    works_timed_scheduled++;
    timedwork->ScheduleWork(Command, ts);
}
