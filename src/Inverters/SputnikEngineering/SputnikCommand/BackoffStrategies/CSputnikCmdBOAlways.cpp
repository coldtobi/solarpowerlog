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
 * CSputnikCmdBOAlways.cpp
 *
 *  Created on: 29.05.2012
 *      Author: tobi
 */

#include "CSputnikCmdBOAlways.h"


bool CSputnikCmdBOAlways::ConsiderCommand()
{
    bool ret = ISputnikCommandBackoffStrategy::ConsiderCommand();
    if (!ret) return true;
    return false;
}
