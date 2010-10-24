/* ----------------------------------------------------------------------------
 solarpowerlog
 Copyright (C) 2009  Tobias Frost

 This file is part of solarpowerlog.

 Solarpowerlog is free software; However, it is dual-licenced
 as described in the file "COPYING".

 For this file (InverterFactoryFactory.cpp), the license terms are:

 You can redistribute it and/or modify it under the terms of the GNU
 General Public License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Library General Public
 License along with this proramm; if not, see
 <http://www.gnu.org/licenses/>.
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

#if defined HAVE_INV_SPUTNIK
#include "Inverters/SputnikEngineering/CInverterFactorySputnik.h"
#endif

InverterFactoryFactory::InverterFactoryFactory() {
	// TODO Auto-generated constructor stub

}

IInverterFactory *InverterFactoryFactory::createInverterFactory(const string& manufactor)
{

#if defined HAVE_INV_SPUTNIK
	if (manufactor == "SPUTNIK_ENGINEERING") {
		return new CInverterFactorySputnik;
	}
#endif

	return NULL;
}

InverterFactoryFactory::~InverterFactoryFactory() {
	// TODO Auto-generated destructor stub
}
