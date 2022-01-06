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
 * CSputnikCmdBOTimed.cpp
 *
 *  Created on: 03.06.2012
 *      Author: tobi
 */

#include "CSputnikCmdBOTimed.h"

bool CSputnikCmdBOTimed::ConsiderCommand() {
    bool ret = ISputnikCommandBackoffStrategy::ConsiderCommand();
    if (!ret) return false;

    if (last == boost::posix_time::not_a_date_time) {
        LOGDEBUG_SA(_logger, LOG_SA_HASH("BO-Timed_Consider"),
            "BO-Timed: Considering -- first call");
        return true;
    }

    if (last + interval >= boost::posix_time::second_clock::local_time()) {
        LOGDEBUG_SA(_logger, LOG_SA_HASH("BO-Timed_Consider"),
            "BO-Timed: Considering -- due");
        return true;
    }

    LOGDEBUG_SA(_logger, LOG_SA_HASH("BO-Timed_Consider"),
        "BO-Timed: not yet due. Due at " << (last+interval));

    return false;
}

void CSputnikCmdBOTimed::CommandAnswered() {
    LOGDEBUG_SA(_logger, LOG_SA_HASH("BO-Timed-Logic"),"BO-Timed: Answered");
    last = boost::posix_time::second_clock::local_time();
}

void CSputnikCmdBOTimed::Reset() {
    LOGDEBUG_SA(_logger, LOG_SA_HASH("BO-Timed-Logic"),"BO-Timed: Reset");
    last = boost::posix_time::not_a_date_time;
}
