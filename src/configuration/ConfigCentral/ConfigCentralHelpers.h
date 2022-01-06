/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

 Copyright (C) 2015 Tobias Frost

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
 * ConfigCentralHelpers.h
 *
 *  Created on: 04.01.2015
 *      Author: tobi
 */

#ifndef SRC_CONFIGURATION_CONFIGCENTRAL_CONFIGCENTRALHELPERS_H_
#define SRC_CONFIGURATION_CONFIGCENTRAL_CONFIGCENTRALHELPERS_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>

/** some helper functions.
 *
 */
class CConfigCentralHelpers
{
private:
    // private constructor -- we do not want objects of this.
    CConfigCentralHelpers() {}
    virtual ~CConfigCentralHelpers() {}

public:
    /// wrap the string at WRAP_AT characters.
    /// add "# " before each line to mimic configuration syntax
    static std::string WrapForConfigSnippet(const std::string &description);

    /// defines where to wrap.
    static const int WRAP_AT=79;
};
#endif /* SRC_CONFIGURATION_CONFIGCENTRAL_CONFIGCENTRALHELPERS_H_ */

