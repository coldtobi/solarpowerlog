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

/** \file CInverterFactorySputnik.cpp
 *
 *  Created on: May 20, 2009
 *      Author: tobi
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "porting.h"
#endif

#include "Inverters/SputnikEngineering/CInverterFactorySputnik.h"
#include "Inverters/SputnikEngineering/CInverterSputnikSSeries.h"
#include "CInverterSputnikSSeriesSimulator.h"

using namespace std;
#if defined HAVE_INV_SPUTNIK || defined HAVE_INV_SPUTNIKSIMULATOR

static const string supportedmodels =
#if defined HAVE_INV_SPUTNIK
		"S-Series: \tModels 2000S, 3000S, 4200S, 6000S and similar\n "
#endif
#if defined HAVE_INV_SPUTNIKSIMULATOR
        "Simulator:\tModels a S-Series Inverter\n";
#else
;
#endif

CInverterFactorySputnik::CInverterFactorySputnik() {
	// TODO Auto-generated constructor stub
}

/** Instanciates the right inverter class */
IInverterBase *CInverterFactorySputnik::Factory(const string & type,
		const string& name, const string & configurationpath) {

#if defined HAVE_INV_SPUTNIK
	if (type == "S-Series") {
		return new CInverterSputnikSSeries(name, configurationpath);
	}
#endif
#if defined HAVE_INV_SPUTNIKSIMULATOR
	if (type == "Simulator") {
	    return new CInverterSputnikSSeriesSimulator(name,configurationpath);
	}
#endif

	return NULL;
}

CInverterFactorySputnik::~CInverterFactorySputnik() {
	// TODO Auto-generated destructor stub
}

/** Return a string describing the available models for this factory.
 * This should be inforamative for the user. */
const string & CInverterFactorySputnik::GetSupportedModels() const {
	return supportedmodels;
}

#endif
