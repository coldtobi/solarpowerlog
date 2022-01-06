/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

 Copyright (C) 2015 Tobias Frost

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
 * CConfigCentralEntryText.cpp
 *
 *  Created on: 04.01.2015
 *      Author: tobi
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "CConfigCentralEntryText.h"
#include "ConfigCentralHelpers.h"

std::string CConfigCentralEntryText::GetConfigSnippet() const
{
   std::string ret =  CConfigCentralHelpers::WrapForConfigSnippet(_description);

   if (!_setting.empty() && !_example.empty()) {
       ret += "# " + _setting + " = " + _example +";\n";
   }

   return ret;
}
