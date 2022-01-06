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

/** \file CMutexAutoLock.h
 *
 *  Created on: May 17, 2009
 *      Author: tobi
 */

#ifndef CMUTEXHELPER_H_
#define CMUTEXHELPER_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <boost/thread/mutex.hpp>

/** Mutex Wrapper
 *
 * The library makes you keeping track when you entered the
 * Mutex. This generates the problem, that if the function
 * is left without considering the lock, the lock remains forever.
 *
 * This class helps in this case: Just create an object of it,
 * passing the "this" pointer to the constructor
 * (assuming your class is derived from Mutex) and enjoy the lock.
 *
 * The lock is automatically destroyed on destrucing the object.
 */
class CMutexAutoLock {
public:
    /** Construct the MutexAutoLock object and obtain lock to the mutex.
     *
     * @param mutex object to control
     * @param allow_recursive whether additional lock() and unlock()
     *  should be accounted. if false, every call to lock() and unlock()
     *  will lock and unlock, if true, the mutex will only be unlocked if
     *  the amount of calls to lock() matches the unlock()s -- and on object
     *  destruction.
     */
    CMutexAutoLock(boost::mutex &mutex, bool allow_recursive = false);

    /** Destructor. Will also unlock the mutex if held during destruction
     *
     */
    virtual ~CMutexAutoLock();

	/** Unlock a previously locked mutex
	 *
	 * In recursive mode, only unlocked when the number of calls to lock()
	 * matches the ones to unlock()
	 */
    void unlock(void)
    {
        if (_locked) {
            _locked--;
            if (0 == _locked || !_allow_recursive) {
                _mutex.unlock();
                _locked = 0;
            }
        }
    }

    /** Lock the mutex
     *
     * \note This will block if the mutex is already held by e.g another
     * instance.
     */
    void lock(void)
    {
        if (!_locked) _mutex.lock();
        _locked++;
    }

private:
    /// mutex object
    boost::mutex &_mutex;
    /// store state whether recursive mode or not
    bool _allow_recursive;
    /// current lock state -- u
    int _locked;

};

#endif /* CMUTEXHELPER_H_ */
