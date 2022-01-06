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

/** \file CInverterFactorySputnik.h
 *
 *  Created on: May 20, 2009
 *      Author: tobi
 */

#ifndef CINVERTERFACTORYSPUTNIK_H_
#define CINVERTERFACTORYSPUTNIK_H_


/** \fixme Factory for the Sputnik inverters
 *
 * Creates the object handling the inverters for the manufacturer Sputnik
 * Engineering.
 *
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined HAVE_INV_SPUTNIK || defined HAVE_INV_SPUTNIKSIMULATOR

#include "Inverters/factories/IInverterFactory.h"

using namespace std;

/** Factory class for sputnik inverters. */
class CInverterFactorySputnik: public IInverterFactory {

	virtual IInverterBase * Factory(const string& type, const string& name,
			const string & configurationpath);

	virtual const string & GetSupportedModels() const;

public:
	CInverterFactorySputnik();
	virtual ~CInverterFactorySputnik();
};

#endif

#endif /* CINVERTERFACTORYSPUTNIK_H_ */
