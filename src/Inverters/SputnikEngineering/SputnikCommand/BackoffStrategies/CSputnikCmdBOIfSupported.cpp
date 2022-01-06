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

/*
 * CSputnikCmdBOOnce.cpp
 *
 *  Created on: 30.05.2012
 *      Author: tobi
 */

#include "CSputnikCmdBOIfSupported.h"

bool CSputnikCmdBOIfSupported::ConsiderCommand()
{
    bool ret = ISputnikCommandBackoffStrategy::ConsiderCommand();
    if (!ret)
        return ret;
    if (triesleft || supported){
        if (supported) {
            LOGINFO_SA(_logger,LOG_SA_HASH("BOIF_consider"),"Command is supported.");
        } else {
            LOGTRACE_SA(_logger, __COUNTER__, "triesleft="
                << triesleft);
        }
        return true;
    }

    LOGINFO_SA(_logger,LOG_SA_HASH("BOIF_consider"),
        "Command is not supported ");
    return false;
}

void CSputnikCmdBOIfSupported::CommandAnswered()
{

    ISputnikCommandBackoffStrategy::CommandAnswered();
    LOGDEBUG_SA(_logger, LOG_SA_HASH("BOIF_answered"), "Command answered");
    supported = true;
}

void CSputnikCmdBOIfSupported::CommandNotAnswered()
{
    ISputnikCommandBackoffStrategy::CommandNotAnswered();
    LOGDEBUG_SA(_logger, LOG_SA_HASH("BOIF_answered"), "Command not answered. Tries left: " << triesleft);
    if (triesleft) {
        triesleft--;
    }
}

void CSputnikCmdBOIfSupported::Reset()
{
    ISputnikCommandBackoffStrategy::Reset();
    LOGDEBUG_SA(_logger, LOG_SA_HASH("BOIF_answered"), "Reset");
    triesleft = triesleft_orig;
    supported = false;
}
