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
 * CConfigCentralEntry.h
 *
 *  Created on: 04.01.2015
 *      Author: tobi
 */

#ifndef SRC_CONFIGURATION_CONFIGCENTRAL_CCONFIGCENTRALENTRY_H_
#define SRC_CONFIGURATION_CONFIGCENTRAL_CCONFIGCENTRALENTRY_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "IConfigCentralEntry.h"
#include "configuration/CConfigHelper.h"
#include "ConfigCentralHelpers.h"
/*** Single setting/configuration entry.
 *
 */
template<typename T>
class CConfigCentralEntry : public IConfigCentralEntry {

public:

    /** Constructor for mandatory parameters
     *
     * @param setting which setting
     * @param description description for the setting
     * @param store where to store the parsed value
     */
    CConfigCentralEntry(const char* setting, const char* description, T &store) :
        IConfigCentralEntry(setting, description), _have_default_set(false),
            _optional(false), _store(store)  {}

    /** Constructor for optional parameters
     *
     * @param setting which setting
     * @param description description for the setting
     * @param store where to store the parsed value
     * @param defaultvalue which value to use when the parameter was not found
     */
    CConfigCentralEntry(const char* setting, const char* description, T &store,
        T defaultvalue) :
        IConfigCentralEntry(setting, description), _have_default_set(true),
            _optional(true), _store(store), _defvalue(defaultvalue)   {}

    virtual ~CConfigCentralEntry() { }

    virtual bool CheckAndUpdateConfig(ILogger &logger, CConfigHelper &helper) const {

        if (_optional && _have_default_set) {
            return helper.CheckAndGetConfig(_setting.c_str(), _store, _defvalue, &logger);
        }
        else {
            return helper.CheckAndGetConfig(_setting, _store, &logger);
        }
    }

    std::string GetConfigSnippet() const
    {
		extern std::string _WrapForConfigSnippet(const std::string &description);

        std::string ret;
        // Wrap the desription
        ret = CConfigCentralHelpers::WrapForConfigSnippet(_description);

        std::stringstream ss;
        // print the optional / mandatory statement

        if (_optional) {
            assert(_have_default_set);
            // optional -- default must be set
            ss << "This setting is optional with a default value of "
                << _defvalue;
        } else {
            ss << "This setting is mandatory.\n";
        }

        ret += CConfigCentralHelpers::WrapForConfigSnippet(ss.str());

        // make a nice example
        if (_optional) ret += "# ";
        ret += this->_setting + " = ";
        if (_have_default_set) {
            //std::stringstream ss;
            ss << this->_defvalue;
            ret += ss.str();
        } else {
            ret += "<value>";
        }
        ret += ";\n";
        return ret;
    }

    T getDefvalue() const
    {
        return _defvalue;
    }

    void setDefvalue(T defvalue, bool is_optional=true)
    {
        _defvalue = defvalue;
        _optional = is_optional;
        _have_default_set = true;
    }

protected:
    bool _have_default_set;
    bool _optional;
    T &_store;
    T _defvalue;
};

// Forward declarations of the template specializations.
template<>
std::string CConfigCentralEntry<std::string>::GetConfigSnippet() const;

template <>
std::string CConfigCentralEntry<bool>::GetConfigSnippet() const;

#endif /* SRC_CONFIGURATION_CONFIGCENTRAL_CCONFIGCENTRALENTRY_H_ */
