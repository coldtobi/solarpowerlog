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

/*
 * CSputnikCmdBOTimed.h
 *
 *  Created on: 03.06.2012
 *      Author: tobi
 */

#ifndef CSPUTNIKCMDBOTIMED_H_
#define CSPUTNIKCMDBOTIMED_H_

#include "ISputnikCommandBackoffStrategy.h"
#include <boost/date_time/posix_time/posix_time.hpp>

/// Backoff strategy that will issue a command only once until the inverter
/// is reconnected.
/// Note: It will only cease issuing if the command was answered.
class CSputnikCmdBOTimed: public ISputnikCommandBackoffStrategy
{
public:
    CSputnikCmdBOTimed( const boost::posix_time::time_duration &interval, ISputnikCommandBackoffStrategy *next = NULL) :
            ISputnikCommandBackoffStrategy("BOTimed", next), interval(interval)
    { }

    virtual ~CSputnikCmdBOTimed() {};

    /// Should the command be considered?
    virtual bool ConsiderCommand();

    /// The command has been answered.
    virtual void CommandAnswered();

    /// Inverter disconnected, reset state.
    virtual void Reset();

private:
    boost::posix_time::time_duration interval;
    boost::posix_time::ptime last;
};

#endif /* CSPUTNIKCMDBOONCE_H_ */
