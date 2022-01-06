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

/*
 * CAsyncCommand.h
 *
 *  Created on: Dec 14, 2009
 *      Author: tobi
 */

#ifndef CASYNCCOMMAND_H_
#define CASYNCCOMMAND_H_

#include <semaphore.h>
#include "patterns/ICommand.h"
#include "configuration/Registry.h"

class CAsyncCommand
{
public:
    enum Commando
    {
        DISCONNECT, /// Tear down a connection
        CONNECT, /// Connect
        SEND, /// Send data
        RECEIVE, /// Try to receive data
        ACCEPT   /// "Server-Mode" for inbound connections.
    };

    /** Constructor which create the object
     *
     * \param c Commando to be used
     * \param callback ICommand used as callback. Must not be NULL.
     *
     * \note since solarpowerlog 0.25, the synchronous interface is no longer
     * supported: So ICommand must no longer be NULL (will be asserted!)
     */
    CAsyncCommand(enum Commando cmd, ICommand *pcallback) :
        c(cmd), callback(pcallback)
    {
        assert(pcallback);
    }

    /** Destructor */
    ~CAsyncCommand()
    { }

    /** Handle this jobs completion by notifying the sender
     */
    inline void HandleCompletion(void)
    {
        Registry::GetMainScheduler()->ScheduleWork(callback);
    }

    /** Stores the command what to do */
    enum Commando c;

    /** callback for completion handling
     * In this ICommand, the comand data is stored, results and data...
     */
    ICommand *callback;

};
#endif /* CASYNCCOMMAND_H_ */
