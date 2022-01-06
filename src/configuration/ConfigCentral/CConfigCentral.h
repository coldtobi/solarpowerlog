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

/* \file CConfigCentral.h
 *
 *  Created on: 02.01.2015
 *      Author: tobi
 *
 */


#ifndef SRC_CONFIGURATION_CCONFIGCENTRAL_H_
#define SRC_CONFIGURATION_CCONFIGCENTRAL_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <list>
#include <string>

#include <boost/shared_ptr.hpp>

#include "configuration/ILogger.h"
#include "configuration/CConfigHelper.h"

#include "CConfigCentralEntry.h"
#include "CConfigCentralEntryRangeCheck.h"
#include "CConfigCentralEntryText.h"

/** Configuration Helper Class for better checking and documenting
 * configuration options for solarpowerlog
 *
 * It is cumbersome to keep documentation and code in sync. This class
 * should couple documentation with the code, and also make the checking of
 * the configuration and configuration error reporting more straight forward.
 *
 * The class is designed to be instantiated within the class to be configured,
 * so that the values can be directly placed into the class' variables (via
 * pointers)
 *
 */
class CConfigCentral {

public:

    CConfigCentral() {};

    virtual ~CConfigCentral() {

    }


    /** Add an textual entry (which can be used when dumping the help
     * to have an header, prequel text or like or parameters which are
     * (reported) aliases to others (like the typo "manufactor" instead of
     * "manufacturer")
     *
     * \param parameter parameter linked to this entry. Might be NULL.
     * \param description description, text...
     * \param example everyone loves examples...
     */
    CConfigCentral& operator()(const char* parameter, const char *description,
        const char *example = NULL)
    {
        boost::shared_ptr<IConfigCentralEntry> p((IConfigCentralEntry*)
            new CConfigCentralEntryText(parameter, description, example));
            l.push_back(p);
            return *this;
        }

    /** Add an setting describing entry (mandatory version)
     *
     * @param parameter setting's name
     * @param description setting's description
     * @param store where to store the value
     * @return object, so that the operator can be cascaded.
     */
    template<typename T>
    CConfigCentral& operator()(const char* parameter, const char* description,
        T &store)
    {
        boost::shared_ptr<IConfigCentralEntry>
        p((IConfigCentralEntry*) new CConfigCentralEntry<T>(parameter,
                description, store));
        l.push_back(p);
        return *this;
    }

    /** Add an setting describing entry (optional version with default value)
      *
      * @param parameter setting's name
      * @param description setting's description
      * @param store where to store the value
      * @param defaultvalue to use when the setting was not found.
      * @return object, so that the operator can be cascaded.
      */
    template<typename T>
    CConfigCentral& operator()(const char* parameter, const char* description,
        T &store, const T &defaultvalue)
    {
        boost::shared_ptr<IConfigCentralEntry>
        p((IConfigCentralEntry*) new CConfigCentralEntry<T>(parameter,
                description, store, defaultvalue));
        l.push_back(p);

        return *this;
    }

    /** Add an setting describing entry (mandatory version) with range-check
     *
     * @param parameter setting's name
     * @param description setting's description
     * @param store where to store the value
     * @param minimum range limit (including)
     * @param maximum range limit (including
     * @return object, so that the operator can be cascaded.
     */
    template<typename T>
    CConfigCentral& operator()(const char* parameter, const char* description,
            T &store, const T &minimum, const T &maximum)
        {
        boost::shared_ptr<IConfigCentralEntry> p(
            (IConfigCentralEntry*)new CConfigCentralEntryRangeCheck<T>(
                parameter, description, store, minimum, maximum));
        l.push_back(p);
        return *this;
        }

    /** Add an setting describing entry (optional version) with range-check
     *
     * @param parameter setting's name
     * @param description setting's description
     * @param store where to store the value
     * @param defaultvalue to use when setting has not been found
     * @param minimum range limit (including)
     * @param maximum range limit (including
     * @return object, so that the operator can be cascaded.
     */
    template<typename T>
    CConfigCentral& operator()(const char* parameter, const char* description,
        T &store, const T &defaultvalue, const T &minimum, const T &maximum)
    {
        boost::shared_ptr<IConfigCentralEntry> p(
            (IConfigCentralEntry*)new CConfigCentralEntryRangeCheck<T>(
                parameter, description, store, defaultvalue, minimum, maximum));
        l.push_back(p);

        return *this;
    }


    /** Parse the configuration and use the supplied logger to print errors.
     *
     * Store values into their targets (if given)
     *
     * @return true on success, false on config errors.
     */
    bool CheckConfig(ILogger &logCConfigCentralEntryger, const std::string &configpath);

    /** Get a configuration snippet */
    std::string GetConfigSnippet(void);

    /** Set or replace an example */
    template<typename T>
    void SetExample(const char *token, T newdefault, bool is_optional= true){
        std::list<boost::shared_ptr<IConfigCentralEntry> >::iterator it;
        CConfigCentralEntry<T> *ptr;
        IConfigCentralEntry *p;
        for (it = l.begin(); it != l.end(); it++) {
            p = (*it).get();
            try {
                 if ( 0 == strcmp(p->getSetting().c_str(),token)) {
                    ptr = dynamic_cast<CConfigCentralEntry<T> *>(p);
                    if (ptr) ptr->setDefvalue(newdefault, is_optional);
                    return;
                }
            } catch(const std::bad_cast& ex) {
                // means a bug in the code!
               assert(false);
            }
        }

        // bug: one tries to add an example/default for an unknown setting.
        assert(false);
    }


private:
    std::list<boost::shared_ptr< IConfigCentralEntry> > l;
};

#endif /* SRC_CONFIGURATION_CCONFIGCENTRAL_H_ */
