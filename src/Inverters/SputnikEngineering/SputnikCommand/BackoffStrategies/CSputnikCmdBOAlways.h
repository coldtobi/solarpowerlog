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

/*
 * CSputnikCmdBOAlways.h
 *
 *  Created on: 29.05.2012
 *      Author: tobi
 */

#ifndef CSPUTNIKCMDBOALWAYS_H_
#define CSPUTNIKCMDBOALWAYS_H_

#include "ISputnikCommandBackoffStrategy.h"

/// Backoff Algorithm which "always" will issue a command.
/// (note: implementation example -- a bare ISputnikCommandBackoffStrategy
/// will behave the same.

class CSputnikCmdBOAlways : public ISputnikCommandBackoffStrategy
{
public:
    virtual ~CSputnikCmdBOAlways() {}

    CSputnikCmdBOAlways( ISputnikCommandBackoffStrategy *next = NULL ) :
            ISputnikCommandBackoffStrategy("BOAlways",next)
    { }
};

#endif /* CSPUTNIKCMDBOALWAYS_H_ */
