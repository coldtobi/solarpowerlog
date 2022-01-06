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
 * CSputnikCommandSoftwareVersion.cpp
 *
 *  Created on: 19.05.2012
 *      Author: tobi
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "CSputnikCommandSoftwareVersion.h"
#include "configuration/ILogger.h"

/// command string for Software Version
static const std::string SWV("SWV");
/// command string for BuildVersion
static const std::string BDN("BDN");
/// command string for both
static const std::string BDNSWV("BDN;SWV");

CSputnikCommandSoftwareVersion::CSputnikCommandSoftwareVersion(ILogger &logger,
        IInverterBase *inv, const std::string & capname,
        ISputnikCommandBackoffStrategy *backoff ) :
        ISputnikCommand(logger, BDNSWV, 0, inv, capname, backoff),
        got_buildversion(false), got_swversion(false), sw(0), build(0)
{
}

int CSputnikCommandSoftwareVersion::GetMaxAnswerLen(void) {
    if ( !got_swversion && ! got_buildversion ) return 9+7;
    if ( !got_swversion) return 9;
    return 7;
}

const std::string& CSputnikCommandSoftwareVersion::GetCommand(void) {
    if ( !got_swversion && ! got_buildversion ) return BDNSWV;
    if ( !got_swversion) return (SWV);
    return BDN;
}

unsigned int CSputnikCommandSoftwareVersion::GetCommandLen(void) {
    return GetCommand().length();
}

bool CSputnikCommandSoftwareVersion::IsHandled(const std::string& token) {
    if ( token == SWV) return true;
    if ( token == BDN) return true;
    return false;
}

bool CSputnikCommandSoftwareVersion::ConsiderCommand() {
    return strat->ConsiderCommand();
}

bool CSputnikCommandSoftwareVersion::handle_token(
    const std::vector<std::string>& tokens) {

    if ( tokens.size() != 2)
        return false;

    if ( tokens[0] == SWV ) {
        sw = strtoul(tokens[1].c_str(), NULL, 16);
        if (sw) got_swversion = true;
    }
    else if ( tokens[0] == BDN ) {
        build = strtoul(tokens[1].c_str(), NULL, 16);
        if (build) got_buildversion = true;
    }
    else {
        return false;
    }

    // do not assemble the capability without the sw version.
    if (!got_swversion) return true;

    std::string strsw;
    strsw = sw/10 + "." + sw%10;
    if ( got_buildversion ) {
        strsw += "Build " + build;
    }

    CapabilityHandling<std::string>(strsw);

    // only tell the backoff strategy that we have been answered if we got
    // both information.
    // The Inverter will tell it later, if one sub-command is not available
    // (as this will be reissued and then detected as not answered.
    // This will catch also if we make "progress", but never getting the full
    // information (However, all Inverters I know support this command...)
    if (got_swversion && got_buildversion) this->strat->CommandAnswered();
    return true;
}

void CSputnikCommandSoftwareVersion::InverterDisconnected()
{
    got_buildversion = false;
    got_swversion = false;
    ISputnikCommand::InverterDisconnected();
}
