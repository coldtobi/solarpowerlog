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
 * CSputnikCommandSYS.h
 *
 *  Created on: 23.05.2012
 *      Author: tobi
 */

#ifndef CSPUTNIKCOMMANDSYS_H_
#define CSPUTNIKCOMMANDSYS_H_

#include "ISputnikCommand.h"
#include "Inverters/SputnikEngineering/SputnikCommand/BackoffStrategies/ISputnikCommandBackoffStrategy.h"
#include "configuration/ILogger.h"

class CSputnikCommandSYS : public ISputnikCommand
{
public:
    CSputnikCommandSYS( ILogger &logger, IInverterBase *inv,
            ISputnikCommandBackoffStrategy *backoff = NULL );

    virtual ~CSputnikCommandSYS() {}

    virtual bool handle_token(const std::vector<std::string> & tokens);

    virtual void InverterDisconnected();

private:
    unsigned int laststatuscode;
    unsigned int secondparm_sys;
};

#endif /* CSPUTNIKCOMMANDSYS_H_ */
