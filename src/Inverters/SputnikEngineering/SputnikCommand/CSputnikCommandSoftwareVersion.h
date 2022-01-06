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
 * CSputnikCommandSoftwareVersion.h
 *
 *  Created on: 19.05.2012
 *      Author: tobi
 */

#ifndef CSPUTNIKCOMMANDSOFTWAREVERSION_H_
#define CSPUTNIKCOMMANDSOFTWAREVERSION_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ISputnikCommand.h"
#include "Inverters/SputnikEngineering/SputnikCommand/BackoffStrategies/ISputnikCommandBackoffStrategy.h"

/** Special implementation for the Inverter's Firmware Version property.
 *
 * To get that information, two commands are required. (SWV and BDN)
 *
 * \sa ISputnikCommand
 */
class CSputnikCommandSoftwareVersion : public ISputnikCommand
{
public:
    CSputnikCommandSoftwareVersion(ILogger &logger, IInverterBase *inv,
        const std::string & capname, ISputnikCommandBackoffStrategy *backoff =
            NULL);

    virtual ~CSputnikCommandSoftwareVersion() {}

    virtual bool ConsiderCommand();

    virtual int GetMaxAnswerLen(void);

    virtual const std::string& GetCommand(void);

    virtual unsigned int GetCommandLen(void);

    virtual bool IsHandled(const std::string &token);

    virtual bool handle_token(const std::vector<std::string> & tokens);

    virtual void InverterDisconnected();

private:
    /// to track if we've got the build version already
    bool got_buildversion;
    /// to track if we've got the software version already
    bool got_swversion;
    /// var to save our result.
    long sw,build;
};

#endif
