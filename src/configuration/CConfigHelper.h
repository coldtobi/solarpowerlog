/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

 Copyright (C) 2009-2014 Tobias Frost

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

/** \file CConfigHelper.h
 *
 *  \date Jul 4, 2009
 *  \author Tobias Frost (coldtobi)
 */

/** \page Config_new Handling the Configuration
 *
 * \section ConfigNew_Overview Overview
 *
 * Solarpowerlog uses a configuration file to get the settings of the individual
 * components. It even decided out of the config file which components are
 * needed ans wich objects needs to work together.
 *
 * For easier handling of the configuration, solarpowerlog provides methods to
 * gather the settings.
 *
 * First, every object gets a so-called configuaration path. This is a string,
 * describing where the configuration of the object is located. (As currently
 * libconfig is used, this is the "path" of libconfig.
 * Every object should safe this path.
 *
 * Second, a helper class, CConfigHelper exists. This class has some methods
 * allowing to extract the configuration very conveniently:
 * 	- Check for existence of a key (including error reporting)
 * 	- Check for the type of a key (including error reporting)
 * 	- Retrieve the value
 * 	- Handle "optional" values by specifying the optional value to be used
 * 	  if the configuration cannot be read.
 *
 * \section ConfigNew_Use Usage and Examples
 *
 * Without many words, here are the some examples.
 *
 * Example 1: Check for key type and existence.
 * \code
 * 	CConfigHelper hlp(configurationpath);
 * 	// Required Key: Will set fail=true if non-existent or wrong type
 *	fail |= !hlp.CheckConfig("datasource", Setting::TypeString);
 *	// Optional key: Will set fail=truel only if clearscreen is not boolean.
 *	fail |= !hlp.CheckConfig("clearscreen", Setting::TypeBoolean,
 *		true);
 * \endcode
 *
 *  Example 2: Retrieve key
 *  \code
 *  	CConfigHelper cfghlp(configurationpath);
 *	float interval;
 *	// This sets the value to 5.0 if not in the configuration:
 *	cfghlp.GetConfig("queryinterval", interval, 5.0f);
 *	// This would just set the value. if not configured, the value is left
 *	// alone.
 * 	cfghlp.GetConfig("queryinterval", interval);
 *  \endcode
 *
 *  \note C++ has strong types. So make sure, that everything matches.
 *  The above example would not compile, if you write "5.0" instead of "5.0f",
 *  as gcc would make a "double" and interval would be "float".
 *  This is also true for integers: int != unsigned int!
 *
 */

#ifndef CCONFIGHELPER_H_
#define CCONFIGHELPER_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>
#include <libconfig.h++>

#include "configuration/Registry.h"

using namespace std;

/** Encapsulates helper-functions needed for easier handling of configuration
 * issues.
 *
 * The class allows:
 * 	- easier checking of types
 * 	- easier access of values,
 * 	- easier access of optional values (by setting default values)
 */
class CConfigHelper
{
public:
	/** Constructor
	 *
	 * \param configurationpath The path to the section of the
	 * \param index (optional) to get enter a aggregate (by adding .[<index>] to the array)
	 * configuration file we want to evaluate.
	 * (Usually you got this path by the factory generated your object)
	*/
	CConfigHelper( const string& configurationpath, int index= -1);

	CConfigHelper( const string& configurationpath, const string &element,int index= -1);

	virtual ~CConfigHelper();

	/** Basic checks on configuration keys, with the support for optional parameters
	 *
	 * This function checks for the parameter associated with the
	 * configuration-key (setting) for the existence (if not an optional parameter)
	 * and for type-correctness.
	 *
	 * For mandatory options (specified by parameter optional=false) the key
	 * must exists and be of the right datatype.
	 *
	 * Optional parameters needs not to be present, but when they are the
	 * datatype must be suitable.
	 *
	 * If printerr is given as true, a found configuration error
	 * (type, and existence of mandatory options)
	 * will be logged using the mainlogger.
	 *
	 * The function checks if the configuration-key (parameter setting)
	 * Mandatory options (optional=false, default) are checked for existence first,
	 * optional options are only checked if the type if ok.
	 * If printerr is given with true (default), the type of error is logged-.
	 *
	 * \returns true if the tests passed, else false
	 *
	 * \param setting configuration-key
	 * \param type expected type
	 * \param optional is it mandatory(false) or optional(true,default),
	 * \param printerr if true, explain the error to the main logger
	 * (default:false)
	 */
	bool CheckConfig( const char *setting, libconfig::Setting::Type type,
		bool optional = false, bool printerr = true );

	/** Convenience function that checks if the setting fulfills the requirements
	 * (type, optionality...) and if it does stores the value into the given
	 * object.
	 *
	 * \returns false if the CheckConfig failed, true if CheckConfig succeeded.
	 *
	 * \note To have the default value option, just initialize your object with
	 * the initial value before calling this member. It will not be changed if
	 * an optional setting is not found.
	 */
    template<class T>
    bool CheckAndGetConfig(const char* setting, libconfig::Setting::Type type,
        T &store,bool optional = false, bool printerr = true ) {

        if (!CheckConfig(setting, type, optional, printerr)) {
            return false;
        }

        GetConfig(setting, store);
        return true;
    }

#if 0
	template<class T>
	bool CheckAndGetConfig(const string &setting, libconfig::Setting::Type type,
        T &store,bool optional = false, bool printerr = true ) {

	    if (!CheckConfig(setting.c_str(), type, optional, printerr)) {
	        return false;
	    }

	    GetConfig(setting, store);
	    return true;
	}
#endif

#if 0
    /** Specialization of the GetConfig routine for the type long.
     * This is necessary as libconfig-1.4.8 removes long support.
     * \returns false, if the default value has been used.
     */
	bool GetConfig(string const &setting, long &store,
            long defvalue)
    {
        return GetConfig(setting.c_str(), store, defvalue);
    }
#endif

    /** Specialization of the GetConfig routine for the type unsigned-long.
     * This is necessary as libconfig-1.4.8 removes long support.
     *
     * \returns false, if the default value has been used.
     */
    bool GetConfig(char const *setting, unsigned long &store,
        unsigned long defvalue)
    {
        unsigned long long tmp;

        try {
            libconfig::Setting & set =
                Registry::Instance().GetSettingsForObject(cfgpath);
            if (!set.lookupValue(setting, tmp) || tmp > ULONG_MAX ) {
                store = defvalue;
                return false;
            }
            store = tmp;
            return true;
        } catch (...) {
            store = defvalue;
            return false;
        }
    }

    /** Specialization of the GetConfig routine for the type long.
     * This is necessary as libconfig-1.4.8 removes long support.
     * \returns false, if the default value has been used.
     */
    bool GetConfig(char const *setting, long &store,
            long defvalue)
    {
        long long tmp;
        try {
            libconfig::Setting & set =
                Registry::Instance().GetSettingsForObject(cfgpath);

            if (!set.lookupValue(setting, tmp) || tmp > LONG_MAX) {
                store = defvalue;
                return false;
            }
            store = tmp;
            return true;
        } catch (...) {
            store = defvalue;
            return false;
        }
    }

#if 0
    /** Retrieves a configuration or set the config with a default value.
     *
     * \returns false, if the default value has been used.
     */
    template<class T>
    bool GetConfig( string const &setting, T &store, T defvalue )
    {
        return GetConfig(setting.c_str(), store, defvalue);
    }
#endif

    /** GetConfig when using a char-array as setting -- see ##GetConfig doc
     * \returns false, if the default value has been used.
     */
    template<class T>
    bool GetConfig(char const *setting, T &store, const T defvalue)
    {

        try {
            libconfig::Setting & set =
                Registry::Instance().GetSettingsForObject(cfgpath);
            if (!set.lookupValue(setting, store)) {
                store = defvalue;
                return false;
            }
            return true;
        } catch (...) {
            store = defvalue;
            return false;
        }
    }

    /** GetConfig for mandatory unsigned long settings, for libconfig 1.4.8
      * \returns false, if the setting could not be retrieved.
     */
    bool GetConfig(char const *setting, unsigned long &store)
    {
        unsigned long long tmp;
        try {
            libconfig::Setting & set =
                Registry::Instance().GetSettingsForObject(cfgpath);
            if (!set.lookupValue(setting, tmp) || tmp > ULONG_MAX) {
                return false;
            }
            store = tmp;
            return true;
        } catch (...) {
            return false;
        }
    }

#if 0
    /** GetConfig for mandatory settings.
      * \returns false, if the setting could not be retrieved.
     */
	template<class T>
	bool GetConfig( const string &setting, T &store )
	{
	    return GetConfig(setting.c_str(),store);
	}
#endif

    /** GetConfig for mandatory long settings, for libconfig 1.4.8
      * \returns false, if the setting could not be retrieved.
     */
    bool GetConfig(char const *setting, long &store)
    {
        try {
            long long tmp;
            libconfig::Setting & set =
                Registry::Instance().GetSettingsForObject(cfgpath);
            if (!set.lookupValue(setting, tmp) || tmp > LONG_MAX) {
                return false;
            }
            store = tmp;
            return true;
        } catch (...) {
            return false;
        }
    }

    /** GetConfig for mandatory settings, flavour "char*"
     * \returns false, if the setting could not be retrieved.
     */
    template<class T>
    bool GetConfig(char const *setting, T &store)
    {
        try {
            libconfig::Setting & set =
                Registry::Instance().GetSettingsForObject(cfgpath);
            return set.lookupValue(setting, store);
        } catch (...) {
            return false;
        }
    }

	/** Get the configuration for an array entry, identified by a index.
	 *
	 * See Libconfig's docs for how the arrays working.
	 *
	 * With this helper, you can simply query the array by its index.
	 *
	 * \param [in] setting Setting-id
	 * \param [in] index Index of the array
	 * \param [out] store place where config is placed.
	 *
	 * \return true on success, false if not an array, wrong type or index
	 * not existant.
	 *
	 */
    template<class T>
    bool GetConfigArray(const char* setting, int index, T &store)
    {
        try {
            libconfig::Setting & set =
                Registry::Instance().GetSettingsForObject(cfgpath);

            store = set[setting][index];
            return true;
#if 0
        } catch (libconfig::SettingNotFoundException &e) {
            return false;
        } catch (libconfig::SettingTypeException &e) {
            return false;
        }
#else
        } catch (...) {
            return false;
        }
#endif
	}

	/** Get the configuration for an array entry, identified by a index.
	 * (std::string specialization) -> this is needed as for libconfig it would
	 * be ambiguous for char* and std::string.
	 *
	 * See Libconfig's docs for how the arrays working.
	 *
	 * With this helper, you can simply query the array by its index.
	 *
	 * \param [in] setting Setting-id
	 * \param [in] index Index of the array
	 * \param [out] store place where config is placed.
	 *
	 * \return true on success, false if not an array, wrong type or index
	 * not existant.
	 *
	 */

    bool GetConfigArray(const char* setting, int index, string &store)
    {
        try {
            libconfig::Setting & set =
                Registry::Instance().GetSettingsForObject(cfgpath);

            store = (const char *)set[setting][index];
            return true;
#if 0
        } catch (libconfig::SettingNotFoundException &e) {
            return false;
        } catch (libconfig::SettingTypeException &e) {
            return false;
        }
#else
        } catch (...) {
            return false;
        }
#endif
    }

	/** This is a ugly helper the CHTHML Writer -- we are having a list which
	 * embeddes a array. The entries are anonymous (without a destinctive name).
	 * I currently wonder how a better access function should look like ;-O)
	 * TODO make this more elegant and reusable...
	 *
	 * so, well, this function looks up an entry in the embedded array,
	 * with param i giving the row, index the column.
	 *
	 * \returns true on success (setting found, type ok),
	 *     false else (setting not found, type error)
	 */
	template<class T>
	bool GetConfigArray( const int i, int index, T &store )
	{
		try {
		    libconfig::Setting & set
			= Registry::Instance().GetSettingsForObject(cfgpath);

			store = (const char *) set[i][index];
			return true;
#if 0
        } catch (libconfig::SettingNotFoundException &e) {
            return false;
        } catch (libconfig::SettingTypeException &e) {
            return false;
        }
#else
        } catch (...) {
            return false;
        }
#endif
	}

    /** this ugly helper is for the CHTHML Writer, for types that are strings.
     * See the other ##GetConfigArray for details.
     */
    bool GetConfigArray( const int i, int index, string &store )
    {
        try {
            libconfig::Setting & set
            = Registry::Instance().GetSettingsForObject(cfgpath);

            store = (const char *) set[i][index];
            return true;
#if 0
        } catch (libconfig::SettingNotFoundException &e) {
            cerr << e.what();
            return false;
        } catch (libconfig::SettingTypeException &e) {
            cerr << e.what();
            return false;
        }
#else
        } catch (...) {
            return false;
        }
#endif
    }

    /** Return current configurationpath. */
    const std::string & GetCfgPath(void) const
    {
        return cfgpath;
    }

    bool isExisting(void) const;

    /*** Check (type, existence) and assign value. This variant is for optional
     * parameters. This variant is necessary due to the ambiguousness of
     * char and std::string for libconfig.
     *
     * @param setting to look for
     * @param store where to store the result
     * @param defval what is the default value
     * @param logger where to log the error (if NULL, do not print the error)
     * @return true on success, false on "type error"
     */
    bool CheckAndGetConfig(const char* setting, std::string &store,
        const std::string &defval, ILogger *logger = NULL)
    {
        libconfig::Config* cfg = Registry::Instance().Configuration();
        string tmp = cfgpath + "." + setting;

        try {
            store = (const char *) cfg->lookup(cfgpath + "." + setting);
        } catch (libconfig::SettingNotFoundException &e) {
            if (logger) LOGINFO(*logger, "Setting " << setting <<
                " was not found. Using default value: \"" << defval <<"\"");
            store = defval;
        } catch (libconfig::SettingTypeException &e) {
            if (logger) LOGERROR(*logger, "Setting " << setting <<
                " is of wrong type");
            return false;
        }
        return true;
    }

    /*** Check (type, existence) and assign value. This variant is for optional
     * parameters
     *
     * @param setting to look for
     * @param store where to store the result
     * @param defval what is the default value
     * @param logger where to log the error (if NULL, do not print the error)
     * @return true on success, false on "type error"
     */
    template<class T>
    bool CheckAndGetConfig(const char* setting, T &store, const T &defval,
        ILogger *logger = NULL)
    {
        libconfig::Config* cfg = Registry::Instance().Configuration();
        string tmp = cfgpath + "." + setting;

        try {
            store = cfg->lookup(cfgpath + "." + setting);
        } catch (libconfig::SettingNotFoundException &e) {
            if (logger) LOGINFO(*logger, "Setting " << setting <<
                " was not found. Using default value: \"" << defval << "\"");
            store = defval;
        } catch (libconfig::SettingTypeException &e) {
            if (logger) LOGERROR(*logger, "Setting " << setting <<
                " is of wrong type");
            return false;
        }
        return true;
    }

    /** Convenience wrapper for CheckAndGetConfig() to take a std::string as
     * setting parameter
     *
     * @param setting to look for
     *
     * @param store where to store the result
     * @param defval what is the default value
     * @param logger where to log the error (if NULL, do not print the error)
     * @return true on success, false on "type error"
     */
    template<class T>
    bool CheckAndGetConfig(const std::string &setting, T &store, const T &defval,
        ILogger *logger = NULL)
    {
        return CheckAndGetConfig(setting.c_str(), store, defval, logger);
    }

    /*** Check (type, existence) and assign value. This variant is for mandatory
     * parameters. This variant is necessary due to the ambiguousness of
     * char and std::string for libconfig.
     *
     * This function looks up a value and store it in the destination variable,
     * if the type of the setting is convertible.
     *
     * If libconfig cannot convert to the desired datatype, it will fail,
     * If the setting is not there, the default value will be used.
     *
     * @param setting to look for
     * @param store where to store the result
     * @param logger where to log the error (if NULL, do not print the error)
     * @return true on success, false on "type error" or "not found"
     */
    bool CheckAndGetConfig(const char* setting, std::string &store,
        ILogger *logger = NULL)
    {
        libconfig::Config* cfg = Registry::Instance().Configuration();
        string tmp = cfgpath + "." + setting;

        try {
            store = (const char*) cfg->lookup(tmp);
        } catch (libconfig::SettingNotFoundException &e) {
            if (logger) LOGERROR(*logger,
                "Required setting " << setting << " was not found.");
            return false;
        } catch (libconfig::SettingTypeException &e) {
            if (logger) LOGERROR(*logger,
                "Setting " << setting << " is of wrong type.");
            return false;
        }
        return true;
    }

    /*** Check (type, existence) and assign value. This variant is for mandatory
     * parameters.
     *
     * This function looks up a value and store it in the destination variable,
     * if the type of the setting is convertible.
     *
     * If libconfig cannot convert to the desired datatype, it will fail,
     * If the setting is not there, the default value will be used.
     *
     * @param setting to look for
     * @param store where to store the result
     * @param logger where to log the error (if NULL, do not print the error)
     * @return true on success, false on "type error" or "not found"
     */
    template<class T>
    bool CheckAndGetConfig(const char* setting, T &store,
        ILogger *logger = NULL)
    {
        libconfig::Config* cfg = Registry::Instance().Configuration();
        string tmp = cfgpath + "." + setting;

        try {
            store = cfg->lookup(tmp);
        } catch (libconfig::SettingNotFoundException &e) {
            if (logger) LOGERROR(*logger,
                "Required setting " << setting << " was not found.");
            return false;
        } catch (libconfig::SettingTypeException &e) {
            if (logger) LOGERROR(*logger,
                "Setting " << setting << " is of wrong type.");
            return false;
        }
        return true;
    }

    /** Convenience wrapper for CheckAndGetConfig() to take a std::string as
     * setting parameter
     *
     * @param setting to look for
     * @param store where to store the result
     * @param logger where to log the error (if NULL, do not print the error)
     * @return true on success, false on "type error" or "not found"
     *
     */
    template<class T>
    bool CheckAndGetConfig(const std::string &setting, T &store,
        ILogger *logger = NULL)
    {
        return CheckAndGetConfig(setting.c_str(), store, logger);
    }


private:
	string cfgpath;
};

#endif /* CCONFIGHELPER_H_ */
