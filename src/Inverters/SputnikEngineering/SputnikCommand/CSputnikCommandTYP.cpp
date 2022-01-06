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
 * CSputnikCommandTYP.cpp
 *
 *  Created on: 26.05.2012
 *      Author: tobi
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "CSputnikCommandTYP.h"

static const struct
{
    const int typ;
    const char *description;
} model_lookup[] =
    {
      { 65534, "Solarpowerlog Sputnik Simulator"},
      { 2001, "SolarMax 2000 E" },
      { 3001, "SolarMax 3000 E" },
      { 4001, "SolarMax 4000 E" },
      { 6000, "SolarMax 6000 E" },
      { 2010, "SolarMax 2000 C" },
      { 3010, "SolarMax 3000 C" },
      { 4010, "SolarMax 4000 C" },
      { 4200, "SolarMax 4200 C" },
      { 6010, "SolarMax 6000 C" },
      { 20010, "SolarMax 2000 S" },
      { 20020, "SolarMax 3000 S" },
      { 20030, "SolarMax 4200 S" },
      { 20040, "SolarMax 6000 S" },
      { 20202, "SolarMax 10MT" },
      { 20210, "SolarMax 10MTV2" },
      { 20, "SolarMax 20 C" },
      { 25, "SolarMax 25 C" },
      { 30, "SolarMax 30 C" },
      { 35, "SolarMax 35 C" },
      { 50, "SolarMax 50 C" },
      { 80, "SolarMax 80 C" },
      { 100, "SolarMax 100 C" },
      { 300, "SolarMax 300 C" },
      { -1, "UKNOWN MODEL. PLEASE FILE A BUG WITH THE REPORTED ID,"
            "ALONG WITH ALL INFOS YOU HAVE" }
    };

CSputnikCommandTYP::CSputnikCommandTYP(ILogger &logger, IInverterBase *inv,
    ISputnikCommandBackoffStrategy *backoff)
    : ISputnikCommand(logger, "TYP", 9, inv, CAPA_INVERTER_MODEL, backoff) {
}

bool CSputnikCommandTYP::handle_token(const std::vector<std::string>& tokens) {
    string strmodel;
    unsigned int i = 0;

    // Check syntax
    if (tokens.size() != 2) return false;

    int model = strtoul(tokens[1].c_str(), NULL, 16);

    do {
        if (model_lookup[i].typ == model) break;
    } while (model_lookup[++i].typ != -1);

    if (model_lookup[i].typ == -1) {
        LOGINFO(inverter->logger,
            "Identified a " << model_lookup[i].description);
        LOGINFO(inverter->logger,
            "Received TYP was " << tokens[0] << "=" << tokens[1]);
    }

    CapabilityHandling<CAPA_INVERTER_MODEL_TYPE>(model_lookup[i].description);
    this->strat->CommandAnswered();
    return true;
}
