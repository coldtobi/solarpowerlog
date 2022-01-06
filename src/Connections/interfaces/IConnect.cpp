/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

Copyright (C) 2009-2012 Tobias Frost

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

/** \file IConnect.cpp
 *
 *  Created on: May 16, 2009
 *      Author: tobi
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>

#include "IConnect.h"
#include "configuration/Registry.h"

using namespace std;

IConnect::IConnect(const string& configurationname)
{
	ConfigurationPath = configurationname;
	_thread_is_running = false;
	_thread_term_request = false;

}

void IConnect::SetupLogger(const string &parentlogger, const string & spec)
{
	logger.Setup(parentlogger, spec);
}

IConnect::~IConnect()
{
    mutex.lock();
    if (_thread_is_running) {
        _thread_term_request = true;
        workerthread.interrupt();
    }
    mutex.unlock();
    workerthread.join();
}

void IConnect::StartWorkerThread(void)
{
	mutex.lock();
	if (!_thread_is_running) {
		workerthread = boost::thread(boost::bind(&IConnect::_main, this));
		_thread_is_running = true;
	}
	mutex.unlock();
}

bool IConnect::IsTermRequested(void)
{
	mutex.lock();
	bool ret = _thread_term_request;
	mutex.unlock();
	return ret;
}

void IConnect::Noop(ICommand* cmd)
{
    // Standard noop is just to schedule this work with no error set.
    cmd->addData(ICMD_ERRNO,(int)0);
    Registry::GetMainScheduler()->ScheduleWork(cmd);
}

bool IConnect::IsThreadRunning(void)
{
	mutex.lock();
	bool ret = _thread_is_running;
	mutex.unlock();
	return ret;
}
