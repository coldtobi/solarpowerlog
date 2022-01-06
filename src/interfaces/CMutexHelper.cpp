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

/** \file CMutexAutoLock.cpp
 *
 *  Created on: May 17, 2009
 *      Author: tobi
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "CMutexHelper.h"

CMutexAutoLock::CMutexAutoLock(boost::mutex &mutex, bool allow_recursive) :
    _mutex(mutex), _allow_recursive(allow_recursive), _locked(0)
{
    lock();
}

CMutexAutoLock::~CMutexAutoLock()
{
    if (_locked) _mutex.unlock();
}
