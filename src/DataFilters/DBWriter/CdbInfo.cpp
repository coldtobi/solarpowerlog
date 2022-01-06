/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

 Copyright (C) 2009-2014 Tobias Frost

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
 * CdbInfo.cpp
 *
 *  Created on: Jul 19, 2014
 *      Author: tobi
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#include "porting.h"
#endif

#ifdef  HAVE_FILTER_DBWRITER

#include "CdbInfo.h"

Cdbinfo::~Cdbinfo()
{
    if (Value) delete Value;
};

Cdbinfo::Cdbinfo(std::string Capability, std::string Column) :
    Capability(Capability), Column(Column), Value(NULL),
        previously_subscribed(false), isSpecial(false)
{};


#endif


