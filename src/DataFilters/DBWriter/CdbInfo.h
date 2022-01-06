/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

Copyright (C) 2009-2014 Tobias Frost

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

/* \file CdbInfo.h
 *
 *  Created on: Jul 19, 2014
 *      Author: tobi
 */

#ifndef CDBINFO_H_
#define CDBINFO_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "porting.h"
#endif

#ifdef  HAVE_FILTER_DBWRITER

#include <string>
#include "patterns/IValue.h"


/** Helper class to encapsulate the data for the db entry.
 *
 * This helper class bundles one set of information, one column-data tuple
 * for the database to be logged.
 *
 * Used only by CDBWriterHelper.
 */
class Cdbinfo
{
protected:
    friend class CDBWriterHelper;

    Cdbinfo(std::string Capability, std::string Column);

    ~Cdbinfo();

    /// String of the capability
    std::string Capability;

    /// Column of the table
    std::string Column;

    /// Copy of the current value.
    IValue *Value;

    /// this field is just to suppress a debug-message during observer pattern
    /// handling
    bool previously_subscribed;

    /// is the IValue a special "%" or "!" value (and therefore save to upcast)
    bool isSpecial;
};

#endif
#endif /* CDBINFO_H_ */
