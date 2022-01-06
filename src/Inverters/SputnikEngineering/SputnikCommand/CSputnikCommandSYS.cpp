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
 * CSputnikCommandSYS.cpp
 *
 *  Created on: 23.05.2012
 *      Author: tobi
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>

#include "CSputnikCommandSYS.h"
#include "Inverters/Capabilites.h"


static const struct
{
    unsigned int code;
    enum InverterStatusCodes status;
    const char *description;
} statuscodes[] = {
    { 20001, FEEDING, "Operating" },
    { 20002, NOT_FEEDING_OK, "Solar radiation too low" },
    { 20003, NOT_FEEDING_OK, "Inverter starting up" },
    { 20004, FEEDING_MPP, "Feeding on MPP" },
    { 20005, FEEDING, "Feeding. Fan on" },
    { 20006, FEEDING_MAXPOWER, "Feeding. Inverter at power limit" },
    { 20008, FEEDING, "Feeding" },
    { 20009, FEEDING_MAXPOWER, "Feeding. Current limitation (DC)" },
    { 20010, FEEDING_MAXPOWER, "Feeding. Current limitation (AC)" },
    { 20011, FEEDING_WARNING, "Test mode" },
    { 20012, FEEDING_WARNING, "Remotely controlled" },
    { 20013, NOT_FEEDING_OK, "Restart delay" },

    { 20050, STATUS_UNAVAILABLE, "Firmware update" },
    { 20051, FEEDING, "Operating (200051)" },

    { 20101, NOT_FEEDING_ERROR, "Inverter error (20101)" },
    { 20102, NOT_FEEDING_ERROR, "Inverter error (20102)" },
    { 20103, NOT_FEEDING_ERROR, "Inverter error (20103)" },
    { 20104, NOT_FEEDING_ERROR, "Inverter error (20104)" },
    { 20105, NOT_FEEDING_ERROR, "Isolation error (20105) " },
    { 20106, NOT_FEEDING_ERROR, "Isolation error (20106)" },
    { 20107, NOT_FEEDING_ERROR, "Inverter error (20107)" },
    { 20108, NOT_FEEDING_ERROR, "Inverter error (20108)" },
    { 20109, NOT_FEEDING_ERROR, "DC Voltage too high (20109)" },
    { 20110, NOT_FEEDING_ERROR, "Internal voltage too high." },
    { 20111, NOT_FEEDING_ERROR, "Surge" },
    { 20112, NOT_FEEDING_ERROR, "Overload." },
    { 20113, NOT_FEEDING_ERROR, "Inverter error (20113)" },
    { 20114, NOT_FEEDING_ERROR, "Error current too high." },
    { 20115, NOT_FEEDING_EXTEVENT, "Off-grid" },
    { 20116, NOT_FEEDING_EXTEVENT, "Grid frequency too high" },
    { 20117, NOT_FEEDING_EXTEVENT, "Grid frequency too low" },
    { 20118, FEEDING_WARNING, "Islanding -- Operating in island mode." },
    { 20119, NOT_FEEDING_EXTEVENT, "Grid quality too low" },
    { 20120, NOT_FEEDING_ERROR, "Inverter error (20120)" },
    { 20121, NOT_FEEDING_ERROR, "Inverter error (20120)" },
    { 20122, NOT_FEEDING_EXTEVENT, "Grid voltage too high" },
    { 20123, NOT_FEEDING_EXTEVENT, "Grid voltage too low" },
    { 20124, NOT_FEEDING_EXTEVENT, "Over temperature" },
    { 20125, STATUS_UNAVAILABLE, "Asymmetric grid currents" },
    { 20126, STATUS_UNAVAILABLE, "Error external input 1" },
    { 20127, STATUS_UNAVAILABLE, "Error external input 2" },
    { 20128, NOT_FEEDING_ERROR, "Inverter error (20128)" },
    { 20129, STATUS_UNAVAILABLE, "Phase sequence wrong" },
    { 20130, STATUS_UNAVAILABLE, "Wrong Device" },
    { 20131, STATUS_UNAVAILABLE, "Main switch off" },
    { 20132, STATUS_UNAVAILABLE, "Diode over temperature" },
    { 20133, NOT_FEEDING_ERROR, "Inverter error (20133)" },
    { 20134, STATUS_UNAVAILABLE, "Fan broken" },
    { 20135, NOT_FEEDING_ERROR, "Inverter error (20135)" },
    { 20136, NOT_FEEDING_ERROR, "Inverter error (20136)" },
    { 20137, NOT_FEEDING_ERROR, "Inverter error (20137)" },

    { 20139, NOT_FEEDING_ERROR, "Inverter error (20139)" },
    { 20140, NOT_FEEDING_ERROR, "Inverter error (20140)" },
    { 20141, NOT_FEEDING_ERROR, "Inverter error (20141)" },
    { 20142, NOT_FEEDING_ERROR, "Inverter error (20142)" },
    { 20143, NOT_FEEDING_ERROR, "Inverter error (20143)" },
    { 20144, NOT_FEEDING_ERROR, "Inverter error (20144)" },
    { 20145, STATUS_UNAVAILABLE, "Frequency change rate too high" },
    { 20146, NOT_FEEDING_ERROR, "Inverter error (20146)" },

    { 20163, NOT_FEEDING_ERROR, "Inverter error (20163)" },
    { 20164, NOT_FEEDING_ERROR, "Error current to high." },
    { 20165, NOT_FEEDING_EXTEVENT, "Off-grid (20165)" },
    { 20166, NOT_FEEDING_EXTEVENT, "Grid frequency too high (20166)" },
    { 20167, NOT_FEEDING_EXTEVENT, "Grid frequency too low (20167)" },
    { 20168, FEEDING_WARNING, "Islanding -- Operating in island mode (20168)" },
    { 20169, NOT_FEEDING_EXTEVENT, "Grid quality too low (20169)" },
    { 20170, NOT_FEEDING_ERROR, "Inverter error (20170)" },

    { 20172, NOT_FEEDING_EXTEVENT, "Grid voltage too high (20172)" },
    { 20173, NOT_FEEDING_EXTEVENT, "Grid voltage too low (20173)" },

    { 20175, STATUS_UNAVAILABLE, "Asymmetric Currents (20175)" },

    { 20186, STATUS_UNAVAILABLE, "Inverter error (20186)" },
    { 20187, STATUS_UNAVAILABLE, "Invalid Firmware" },
    { 20188, STATUS_UNAVAILABLE, "Inverter error (20188)" },

    { 20190, STATUS_UNAVAILABLE, "Too low results" },
    { 20198, STATUS_UNAVAILABLE, "Inverter error (20198)" },
    { 20199, STATUS_UNAVAILABLE, "Unknown error (20199)" },

    { 0xFFFF, STATUS_UNAVAILABLE, "UNKNOWN -- Please file a bug "
        "with as much information as you have, and please read the display of your inverter." }
};

CSputnikCommandSYS::CSputnikCommandSYS(ILogger &logger, IInverterBase *inv,
    ISputnikCommandBackoffStrategy *backoff) :
        ISputnikCommand(logger, "SYS", 10, inv,
            CAPA_INVERTER_STATUS_NAME " and " CAPA_INVERTER_STATUS_READABLE_NAME, backoff),
        laststatuscode(0), secondparm_sys(0)
{}

bool CSputnikCommandSYS::handle_token(const std::vector<std::string>& tokens) {

    if (tokens.size() != 3) return false;
    char *ptmp = NULL;

    unsigned long status = strtol(tokens[1].c_str(), &ptmp, 16);
    if (!status || status == ULONG_MAX || ptmp == tokens[1].c_str()) return false;
    unsigned long status2 = strtol(tokens[2].c_str(), &ptmp, 16);
    if (ptmp == tokens[1].c_str()) return false;

    if (status2 && status2 != secondparm_sys) {
        secondparm_sys = status2;
        LOGINFO(inverter->logger, "Received an unknown SYS response. Please file a bug"
                << " along with the following: " << tokens[0] << ","
                << tokens[1] << "," << tokens[2] << " and check the Inverter's display for more information.");
    }

    int i = 0;
    do {
        if (statuscodes[i].code == status)
            break;
    } while (statuscodes[++i].code != 0xffff );

    if (laststatuscode != 0xffff && statuscodes[i].code
            == 0xffff ) {
        LOGINFO(inverter->logger, "SYS reported an (too us) unknown status code of "
                << tokens[0] << "=" << tokens[1] << "," << tokens[2]
        );
        LOGINFO (inverter->logger,
                " PLEASE file along with all information you have, for example,"
                << " reading the display of the inverter and of the info given above."
        );
    }
    laststatuscode = statuscodes[i].code;
    CapabilityHandling<CAPA_INVERTER_STATUS_TYPE>(status,
        CAPA_INVERTER_STATUS_NAME);

    CapabilityHandling<CAPA_INVERTER_STATUS_READABLE_TYPE>(
        statuscodes[i].description, CAPA_INVERTER_STATUS_READABLE_NAME);

    this->strat->CommandAnswered();
    return true;
}

void  CSputnikCommandSYS::InverterDisconnected() {
    CCapability *cap;

    cap = inverter->GetConcreteCapability(CAPA_INVERTER_STATUS_NAME);
    if (cap) cap->getValue()->Invalidate();
    cap = inverter->GetConcreteCapability(CAPA_INVERTER_STATUS_READABLE_NAME);
    if (cap) cap->getValue()->Invalidate();

    ISputnikCommand::InverterDisconnected();
}

