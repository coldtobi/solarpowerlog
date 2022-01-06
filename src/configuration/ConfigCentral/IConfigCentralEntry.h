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
 * IConfigCentralEntry.h
 *
 *  Created on: 04.01.2015
 *      Author: tobi
 */

#ifndef SRC_CONFIGURATION_CONFIGCENTRAL_ICONFIGCENTRALENTRY_H_
#define SRC_CONFIGURATION_CONFIGCENTRAL_ICONFIGCENTRALENTRY_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>

class CConfigHelper;
class ILogger;

/** Interface class for config entries.
 *
 */
class IConfigCentralEntry {
public:
    IConfigCentralEntry(const char* setting, const char* description)
    {
        if (setting) _setting = setting;
        if (description) _description = description;
    }

    virtual ~IConfigCentralEntry() { }

    /** Check the config entry and if ok update the target value.
     *
     * @param logger where to log errors
     * @param helper where to retrive the settings.
     * @return true if setting ok, false if something is wrong. The reason is
     *    logged using LOGERROR(logger,...)
     */
    virtual bool CheckAndUpdateConfig(ILogger &logger, CConfigHelper &helper) const = 0;

    /** Get the configuration options pretty-printed to automatically generate
     * config snippets.
     */
    virtual std::string GetConfigSnippet() const = 0;

    const std::string& getSetting() const
    {
        return _setting;
    }


    /** Get the configuration options pretty-printed to suitable for help2man
     */
    // virtual std::string& GetHelp() const = 0;

protected:
    std::string _setting;
    std::string _description;

};

#endif /* SRC_CONFIGURATION_CONFIGCENTRAL_ICONFIGCENTRALENTRY_H_ */
