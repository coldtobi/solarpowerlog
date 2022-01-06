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

/**
 * \file Registry.h
 *
 * \page The class Registry: Configuration, Scheduler and Object Database
 *
 *  \date Created on: May 9, 2009
 *  \author Tobias Frost (coldtobi)
 *
 * \section LibConfig The Configuration Concept
 *
 * \sa \ref Registry
 *
 * Solarpowerlog uses libconfig as configuration backend.
 *
 * On startup it loads a file and extracts the configuration for the individual
 * components.
 *
 * The settings are structured to the indivdual sections, like general settings,
 * settings for inverters, settings for the loggers.
 *
 * The program parses the basic layout of configuration.
 *
 * Out of the infos stored in configuration, the program generates a number of
 * objects.
 *
 * The generated object will be told its own configuration path (in libconfig
 * notation) to allow it to extract its own configuration. (This makes us the
 * the live easier, as we do not need to care which configuration items a object
 * requires: It knows best.)
 *
 * \section Registry Registry: The Object Register
 *
 * \sa Registry
 *
 * The class Registry also maintains a list of all objects it has created during
 * startup. At the moment these are Inverters and Loggers.
 *
 * This allows other components to search for other components by its
 * name.
 *
 * Solarpowerlog will make sure, that no objects will exist with identical name.
 *
 * \section TWS The Work Scheduler
 *
 * \sa CWorkScheduler
 *
 * The Registry also maintains one CWorkScheduler object which is can be used
 * by all objects.
 *
 * \note By sharing one scheduler for all objects, the objects does not need to
 * care about concurrency, as all the work is done sequencially. This is also
 * true for timed work.
 *
 * \sa CWorkScheduler, ICommand, ICommandTarget
 *
 * \subsection TWS_snippet Default Scheduler example code
 *
 * This example code shows how to schedule work using the main scheduler:
 * \code
 * 		// Schedule the initialization and subscriptions later...
 *		ICommand *cmd = new ICommand(CMD_INIT, this, 0);
 *		Registry::GetMainScheduler()->ScheduleWork(cmd);
 * \endcode
 *
 * This example shows the usage with Timed Work. (1 second)
 * \code
 *		timespec ts = { 1, 0 };
 *		ICommand *cmd = new ICommand(CMD_INIT, this, 0);
 *		Registry::GetMainScheduler()->ScheduleWork(cmd, ts);
 * \endcode
 */

#ifndef REGISTRY_H_
#define REGISTRY_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <list>
#include <utility>
#include <string>
#include <libconfig.h++>

#include <boost/noncopyable.hpp>
#include "interfaces/CWorkScheduler.h"

#include "configuration/ILogger.h"

#include "interfaces/CDebugHelper.h"

class IInverterBase;

using namespace std;

/** The Registry stores some information needed by several program parts.
 * It also provides a global WorkScheduler.
 *
 * The Registry is a Singleton.
 *
 * \sa \ref LibConfig "The Configuration Concept"
 * \sa \ref Registry "The Object Register"
 * */
class Registry : boost::noncopyable
{
public:

	friend class CConfigHelper;

	/** return the static, global object of the registry. */
	static Registry& Instance()
	{
		static Registry Instance;
		return Instance;
	}

	// C O N F I G U R A T I O N

	/** Shortcut to get the configuration.
	 *
	 * Please note, that it must be config_loaded beforehand.
	 * (but if coding for solarpowerlog, this will be done early
	 * in the startup phase, so one can expect a valid object here. */
	inline static libconfig::Config* Configuration()
	{
		return Registry::Instance().Config;
	}

	inline static ILogger & GetMainLogger(void) {
		return Registry::Instance().l;
	};

	 /** (re)load configuration file
	 *
	 * Just brings it in memory... The old one will be deleted.
	 *
	 * \param [in] name Filename to load
	 *
	 * \returns false on error, true on success.
	 */
	bool LoadConfig( std::string name );

	 /** Fake a configuration
	  *
	  * (Well, this is a hack to make the snippets printing work without
	  * mayor rework throughout the program)
	  *
	  * This function will initilalize an absolute minimum configuration set
	  * tp have the libconfig objects available */
	 void FakeConfig(void);


	/* NOTE: This is obsolete! Use an Object of CConfigHelper to extract config!
	 *
	 *  Extract the settings-subset for a specific object,
	 * identified by section (like inverters) and name (like inverter_solarmax_1)
	 *
	 * ex:
	 *
	 * inverters = (
	 * 					{ name = "Inverter_1";
	 * 					  type = "Solarmax_XYZ";
	 * 					  driver = "Sputnik_TCP";
	 * 					  # (...)
	 * 					},
	 * 					{ name = "Inverter_2";
	 * 					  type = "Solarmax_XYZ";
	 * 					  driver = "Sputnik_TCP";
	 * 					  # (...)
	 * 					}
	 * 				);
	 *
	 * and  GetSettingsForObject("inverters", "Inverter_1") will return the Settings object
	 * for the group "inverters.[0]".
	 *
	 * Please note, that libconfig throws some exceptions: Especially, if the section is not found.
	 * This is not handled here, as the Factories should check if the configuration is complete on
	 * constructing them. (They also can add their own settings (default values)...
	 *
	 * [code]
	 *
	 *	libconfig::Setting &set = Registry::Instance().GetSettingsForObject("inverters", "Inverter_1");
	 *	libconfig::Setting &new = set.add("NewPropertyNotSet",libconfig::Setting::TypeString);
	 *	new = "New Default Setting";
	 *
	 * [/code]
	 *
	 * Snippet to retrieve Settings:
	 * [code]
	 * 		libconfig::Setting &set = Registry::Instance().GetSettingsForObject("inverters", "Inverter_1");
	 * [/code]
	 *
	 * \warning This function is not intended to check if a setting exists!
	 */

private:
	/** lookup a specific setting.
	 *
	 * \param section Configurationpath of the object.
	 * \param objname Additional path
	 *
	 * \warning libconfig might throw exceptions if the setting is not
	 * found.
	 * However, if using the configurationpath (supplied by the bootstrap
	 * code) and having objname empty, the path has been validated. */
	libconfig::Setting & GetSettingsForObject( std::string section,
		std::string objname = "" );
public:
	/** search for an inverter/logger object with the object name
	 *
	 * \param name of the object
	 *
	 * \returns Pointer to the object or NULL if not found.
	*/
	IInverterBase* GetInverter( const string& name ) const;

	/** Add the a new created inveter/logger to the object database
	 *
	 * \note: This function is only for the bootstrap process!*/
	void AddInverter( const IInverterBase *inverter );

	/** Get the main scheduler to schedule work...
	 *
	 * \returns pointer to the scheduler */
	static CWorkScheduler *GetMainScheduler( void )
	{
		if(! Registry::Instance().mainscheduler) {
			Registry::Instance().mainscheduler = new CWorkScheduler;
		}
		return Registry::Instance().mainscheduler;
	}

	void AddDebugCollection(CDebugHelperCollection *obj) {
	    debughelpercollection.push_back(obj);
	}

	void RemoveDebugCollection(CDebugHelperCollection *obj) {
        debughelpercollection.remove(obj);
    }

	void DumpDebugCollection() {
	    std::list<CDebugHelperCollection*>::iterator it =
	        debughelpercollection.begin();

        std::cerr << "--- cut here -----------------------------------------------"<< std::endl;
	    std::cerr << "Starting DUMP "<< std::endl;
	    while(it != debughelpercollection.end()) {
	        (*it)->Dump();
	        it++;
	    }
        std::cerr << "Done DUMP"<< std::endl;
        std::cerr << "--- cut here -----------------------------------------------"<< std::endl;

	}

	/** Destroy all inverter and datafilter objects.
	 * Called before termination. */
	void Shutdown(void);

private:
	/// keeps the object for the main scheduler
	/// GetMainScheduler will create the object. (singleton)
	/// note: cpptest detects a memory leak, which is not true. Check the
	/// destructor in Registry.cpp
	CWorkScheduler *mainscheduler;

protected:
	/** singleton: no object generation */
	Registry();
	virtual ~Registry();

private:
	libconfig::Config *Config;

	// TODO generalize this interface, as we might also store
	// other types of objects here.
	std::list<IInverterBase*> inverters;

	ILogger l;

	std::list<CDebugHelperCollection *> debughelpercollection;

};

#endif /* REGISTRY_H_ */
