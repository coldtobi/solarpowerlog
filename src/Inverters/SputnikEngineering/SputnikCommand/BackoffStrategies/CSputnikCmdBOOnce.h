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
 * CSputnikCmdBOOnce.h
 *
 *  Created on: 30.05.2012
 *      Author: tobi
 */

#ifndef CSPUTNIKCMDBOONCE_H_
#define CSPUTNIKCMDBOONCE_H_

#include "ISputnikCommandBackoffStrategy.h"

/// Backoff strategy that will issue a command only once until the inverter
/// is reconnected.
/// Note: It will only cease issuing if the command was answered.
class CSputnikCmdBOOnce: public ISputnikCommandBackoffStrategy
{
public:
    CSputnikCmdBOOnce( ISputnikCommandBackoffStrategy *next = NULL ) :
            ISputnikCommandBackoffStrategy("BOOnce", next), issued(false)
    { }

    virtual ~CSputnikCmdBOOnce() {};

    /// Should the command be considered?
    virtual bool ConsiderCommand();

    /// The command has been answered.
    virtual void CommandAnswered();

    /// Inverter disconnected, reset state.
    virtual void Reset();

private:
    bool issued;
};

#endif /* CSPUTNIKCMDBOONCE_H_ */
