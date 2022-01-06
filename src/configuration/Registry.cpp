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
 * Registry.cpp
 *
 *  Created on: May 9, 2009
 *      Author: tobi
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <libgen.h>

#include "configuration/Registry.h"
#include "interfaces/CWorkScheduler.h"

#include "Inverters/interfaces/InverterBase.h"

using namespace std;

/** constructor for the registry. */
Registry::Registry()
{
	Config = NULL;
	mainscheduler = NULL;
}

bool Registry::LoadConfig( std::string name )
{
	if (Config)
		delete Config;
	Config = new libconfig::Config;

	// set include dir the directory, if name has a complete path.
	// this allows using @include with relative paths to the main config file.
	char *tmp = strdup(name.c_str());
	char *tmp2 = tmp;
	tmp = dirname(tmp);
    if (tmp && strcmp(tmp,".")) Registry::Configuration()->setIncludeDir(tmp);
    free(tmp2);

    try {
		Config->readFile(name.c_str());
	} catch (libconfig::ParseException &ex) {
		std::cerr << "Error parsing configuration file " << name
			<< " at Line " << ex.getLine() << ". ("
			<< ex.getError() << ")" << std::endl;
		delete Config;
		Config = 0;
		return false;
	} catch (libconfig::FileIOException &ex) {
		std::cerr << "Error parsing configuration file " << name
			<< ". IO Exception " << std::endl;
		delete Config;
		Config = 0;
		return false;
	}

	// Be more sloppy on datatypes -> automatically convert if possible.
	Config->setAutoConvert(true);
	return true;
}

void Registry::FakeConfig(void)
{
    static const std::string defaultconfig =
        "application: { \ndbglevel = \"OFF\"\n}\n"
        "inverters: "
        "{ "
            "inverters = ("
                "{ }"
            ")"
        "}"
        "loggers: "
        "{ "
            "inverters = ("
                "{ }"
            ")"
        "}";

    if (Config)
        delete Config;
    Config = new libconfig::Config;

    Config->readString(defaultconfig);
}

libconfig::Setting & Registry::GetSettingsForObject( std::string section,
	std::string objname )
{

	assert(Config);

	libconfig::Setting &s = Config->lookup(section);

	if (objname == "")
		return s;

	for (int i = 0; i < s.getLength(); i++) {

		std::string tmp = s[i]["name"];
		if (tmp == objname)
			return s[i];
	}

	// note: we cannot deliver a object here ... we simply do not have one!
	// As libconfig::SettingNotFoundException is private only, we also cannot
	// throw here. So, as convention, we return the root of the configuration here....
	// We "BUG" here, as it is the responsibility of the caller to ensure the
	// objects existence.
	// (Only Objects with a valid name should query their configuration)
	std::cerr << "BUG: " << __FILE__ << ":" << __LINE__
		<< " --> Queried for unknown Object " << objname
		<< " in section " << section << std::endl;

	assert(false);

	return Config->getRoot();
}

IInverterBase *Registry::GetInverter( const string & name ) const
{

	list<IInverterBase*>::const_iterator iter;
	for (iter = inverters.begin(); iter != inverters.end(); iter++) {
		if ((*iter)->GetName() == name)
			return (*iter);
	}

	return 0;
}

void Registry::AddInverter( const IInverterBase *inverter )
{
	assert (inverter);
	inverters.push_back((IInverterBase*) inverter);
}

/** destructor */
Registry::~Registry()
{
	if (Config)
		delete Config;
	Config = NULL;
	if (mainscheduler)
		delete mainscheduler;
}

void Registry::Shutdown(void) {
    // Clear datafilters and inverters.
    std::list<IInverterBase*>::iterator it;
    for (it = inverters.begin(); it != inverters.end(); it++) {
        delete *(it);
    }
    inverters.clear();

    // shutdown mainscheduler.
    delete mainscheduler;
    mainscheduler = NULL;

    // delete config.
    delete Config;
    Config = NULL;


}
