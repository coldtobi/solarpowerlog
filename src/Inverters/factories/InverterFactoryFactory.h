/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

 Copyright (C) 2009-2015 Tobias Frost

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

/** \file InverterFactoryFactory.h
 *
 *  Created on: May 21, 2009
 *      Author: tobi
 */

#ifndef INVERTERFACTORYFACTORY_H_
#define INVERTERFACTORYFACTORY_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>

#if defined HAVE_INV_SPUTNIK || defined HAVE_INV_SPUTNIKSIMULATOR
#define INV_MANU_SPUTNIK "SPUTNIK_ENGINEERING"
#else
#define INV_MANU_SPUTNIK
#endif

#ifdef HAVE_INV_DUMMY
#define INV_MANU_DUMMY "DUMMY_INVERTER"
#else
#define INV_MANU_DUMMY
#endif

class IInverterFactory;

using namespace std;

/** IInverterFactoryFactory
 *
 * This class implements a factory-factory pattern.
 * The generated object is -- depending on the manufacturer of the inverter --
 * a factory able to generate the final object.
 *
 */
class InverterFactoryFactory {
public:
    static IInverterFactory * createInverterFactory(const string& manufacturer);

private:
	InverterFactoryFactory();
public:
	virtual ~InverterFactoryFactory();
};

#endif /* INVERTERFACTORYFACTORY_H_ */
