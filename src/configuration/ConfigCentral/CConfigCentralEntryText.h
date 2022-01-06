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
 * CConfigCentralEntryText.h
 *
 *  Created on: 04.01.2015
 *      Author: tobi
 */

#ifndef SRC_CONFIGURATION_CONFIGCENTRAL_CCONFIGCENTRALENTRYTEXT_H_
#define SRC_CONFIGURATION_CONFIGCENTRAL_CCONFIGCENTRALENTRYTEXT_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string>

#include "IConfigCentralEntry.h"


/** Class for a description, without parameter.
 *
 * note: to get a
 */
class CConfigCentralEntryText : public IConfigCentralEntry {
public:
    CConfigCentralEntryText(const char *setting, const char *description,
        const char *example) :
        IConfigCentralEntry(setting, description)
    {
        if (example) _example = example;
    }

    virtual bool CheckAndUpdateConfig(ILogger &, CConfigHelper &) const {
        return true;
    }

    virtual std::string GetConfigSnippet() const;

    virtual ~CConfigCentralEntryText() { }

private:
    std::string _example;
};

#endif /* SRC_CONFIGURATION_CONFIGCENTRAL_CCONFIGCENTRALENTRYTEXT_H_ */
