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
 * CSputnikCmdBOIfSupported.h
 *
 *  Created on: 30.05.2012
 *      Author: tobi
 */

#ifndef CSPUTNIKCMDBOISSUPPORTED_H_
#define CSPUTNIKCMDBOISSUPPORTED_H_

#include "ISputnikCommandBackoffStrategy.h"

/// Backoff strategy that try to detect if a certain command is supported.
/// It will use an (simple) heuristic and try a specific command several times
/// (default 3 times) and if it is not answered the command is not issued anymore.
/// until the inverter reconnects.
class CSputnikCmdBOIfSupported: public ISputnikCommandBackoffStrategy
{
public:

    /// Constructor:
    /// retries -> how often to issue command before giving up
    CSputnikCmdBOIfSupported( int retries = 3,
        ISputnikCommandBackoffStrategy *next = NULL) :
        ISputnikCommandBackoffStrategy("BOIfSupported", next),
        triesleft(retries), triesleft_orig(retries), supported(false)
    {
        // do not repeat ourself more than once a day (roughly, 23,5h to avoid
        // timing shifts due to the seasons)
        // and disable the "repeation count"
        _logger.setSaMaxSuppressTime(23*3600 + 30*60);
        _logger.setSaMaxSuppressRepetitions(0);
    }

    virtual ~CSputnikCmdBOIfSupported() {};

    /// Should the command be considered?
     /// return false if not, true if yes.
     /// Call Interface first. If Interface returns "false", also return false.
     virtual bool ConsiderCommand();

     /// The command has been answered.
     virtual void CommandAnswered();

     /// The command has not been answered.
     virtual void CommandNotAnswered();

     /// Inverter disconnected, reset state.
     virtual void Reset();

private:
     int triesleft;
     int triesleft_orig;
     bool supported;

};

#endif /* CSPUTNIKCMDBOISSUPPORTED_H_ */
