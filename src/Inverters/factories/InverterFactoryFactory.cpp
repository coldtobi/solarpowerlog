/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

 Copyright (C) 2009-2015 Tobias Frost

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

/** \file InverterFactoryFactory.cpp
 *
 *  Created on: May 21, 2009
 *      Author: tobi
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "InverterFactoryFactory.h"
#include "IInverterFactory.h"

#if defined HAVE_INV_SPUTNIK || defined HAVE_INV_SPUTNIKSIMULATOR
#include "Inverters/SputnikEngineering/CInverterFactorySputnik.h"
#endif


#if defined HAVE_INV_DUMMY
#include "Inverters/DummyInverter/CInverterFactoryDummy.h"
#endif

InverterFactoryFactory::InverterFactoryFactory() {
	// TODO Auto-generated constructor stub

}

IInverterFactory *InverterFactoryFactory::createInverterFactory(const string& manufacturer)
{

#if defined HAVE_INV_SPUTNIK || defined HAVE_INV_SPUTNIKSIMULATOR
    if (manufacturer == INV_MANU_SPUTNIK) {
        return new CInverterFactorySputnik;
    }
#endif

#if defined HAVE_INV_DUMMY
    if ( manufacturer == INV_MANU_DUMMY) {
        return new CInverterFactoryDummy;
    }
#endif

	return NULL;
}

InverterFactoryFactory::~InverterFactoryFactory() {
	// TODO Auto-generated destructor stub
}
