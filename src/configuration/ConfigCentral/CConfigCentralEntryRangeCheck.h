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
 * CConfigCentralEntryRangeCheck.h
 *
 *  Created on: 04.01.2015
 *      Author: tobi
 */

#ifndef SRC_CONFIGURATION_CONFIGCENTRAL_CCONFIGCENTRALENTRYRANGECHECK_H_
#define SRC_CONFIGURATION_CONFIGCENTRAL_CCONFIGCENTRALENTRYRANGECHECK_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ConfigCentralHelpers.h"
#include "CConfigCentralEntry.h"

/*** Single setting/configuration entry.
 *
 */
template<typename T>
class CConfigCentralEntryRangeCheck : public CConfigCentralEntry<T> {

public:

    /** Constructor for mandatory parameters with range check
     *
     * @param setting which setting
     * @param description description for the setting
     * @param store where to store the parsed value
     * @param minimum allowed value (including)
     * @param maximum allowed value (including)
     */
    CConfigCentralEntryRangeCheck(const char* setting, const char* description,
        T &store, const T &minimum, const T &maximum) :
            CConfigCentralEntry<T>(setting, description, store),
            _min(minimum), _max(maximum)
     { }

    /** Constructor for optional parameters with range check
     *
     * @param setting which setting
     * @param description description for the setting
     * @param store where to store the parsed value
     * @param defaultvalue which value to use when the parameter was not found
     * @param minimum allowed value (including)
     * @param maximum allowed value (including)
     */
    CConfigCentralEntryRangeCheck(const char* setting, const char* description,
        T &store, const T &defaultvalue, const T &minimum, const T &maximum) :
            CConfigCentralEntry<T>(setting, description, store, defaultvalue),
            _min(minimum), _max(maximum)

    { }

    virtual ~CConfigCentralEntryRangeCheck() { }

    virtual bool CheckAndUpdateConfig(ILogger &logger,
        CConfigHelper &helper) const
    {
        bool ret;
        T old = this->_store;

        ret = CConfigCentralEntry<T>::CheckAndUpdateConfig(logger, helper);

        if (!ret) {
            this->_store = old;
            return false;
        }

        if (this->_store > _max || this->_store < _min) {
            LOGERROR(logger, "Setting " << this->_setting <<
                " is out of range (allowed range: " << _min << " to " << _max);
            this->_store = old;
            return false;
        }
        return true;
    }

    virtual std::string GetConfigSnippet() const
    {
        extern std::string _WrapForConfigSnippet(
            const std::string &description);

        std::string ret;
        // Wrap the desription
        ret = CConfigCentralHelpers::WrapForConfigSnippet(this->_description);

        std::stringstream ss;
        // print the range
        ss << "Valid values are from " << _min << " to " << _max << ".\n";
        // print the optional / mandatory statement

        if (this->_optional) {
            assert(this->_have_default_set);
            // optional -- default must be set
            ss << "This setting is optional with a default value of "
                << this->_defvalue;
        } else {
            ss << "This setting is mandatory.\n";
        }

        ret += CConfigCentralHelpers::WrapForConfigSnippet(ss.str());

        // make a nice example
        if (this->_optional) ret += "# ";
        ret += this->_setting + " = ";
        if (this->_have_default_set) {
            std::stringstream ss;
            ss << this->_defvalue;
            ret += ss.str();
        } else {
            ret += "<value>";
        }
        ret += ";\n";
        return ret;
    }

private:
    T _min;
    T _max;
};

#endif /* SRC_CONFIGURATION_CONFIGCENTRAL_CCONFIGCENTRALENTRYRANGECHECK_H_ */
