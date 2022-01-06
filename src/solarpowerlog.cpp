/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

 Copyright (C) 2009-2015 Tobias Frost

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

/** \defgroup Concepts Developer: Basic Concepts
 *
 * This section describes some basics concepts used in deveoping the software.
 */

/**
 * \file solarpowerlog.cpp
 *
 * \mainpage
 * <h1> Welcome to solarpowerlog's developer documentation. </h1>
 *
 * This documentation purpose is to understand the internals and conecpts used,
 * so that the software can easily be enhanced.
 *
 * However, the best way to understand the program is jumping into cold water:
 * Fire up your debugger and look how the program executes.
 *
 * \sa \ref mainBasicConcepts "Basic Concepts"
 *
 * \page mainBasicConcepts Basic Concepts
 *
 * To get an overview how solarpowerlog is designed, lets take a look at some
 * basic concepts.
 *
 * \section mbcinterfaces "Code against interfaces, not implementations"
 *
 * All base classes are designed as interfaces. This allows loosly coupled
 * objects and also allows to easier code reuse.
 *
 * For example, the connection classes are defined through IConnect. When using
 * IConnects, the inverter simply does not need to know which is its
 * communication method, it will just use the interface and will be fine.
 *
 * \section mbcfactories Design Pattern: Factories
 *
 * Usually object generation is done by factories. The Factories gets an
 * identifier (usually a string) and return the created object.
 *
 * Factories allows that new specializations of interfaces can be added to
 * the program without the need to change any of the other classes.
 *
 * For example, if you add a fancy bluetooth class, you just create a IConnect
 * based CConnectionBluetooth, implement it and add its id-string to the
 * IConnectFactory.
 * Now, all the inverters can instanciate (via their comms settings) a
 * CConnectionBluetooth without knowing actually knowing the details.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined HAVE_LIBLOG4CXX
#include <log4cxx/logger.h>
#include <log4cxx/simplelayout.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/xml/domconfigurator.h>
#endif

#ifdef HAVE_OPENLOG
#include <syslog.h>
#endif

#include "configuration/ILogger.h"
#include "configuration/ConfigCentral/CConfigCentral.h"

#ifdef HAVE_CMDLINEOPTS
#include <boost/program_options.hpp>
#endif

#include "configuration/Registry.h"
#include "interfaces/CWorkScheduler.h"
#include "patterns/ICommand.h"
#include "patterns/ICommandTarget.h"

#include "Inverters/factories/IInverterFactory.h"
#include "Inverters/factories/InverterFactoryFactory.h"
#include "Inverters/interfaces/InverterBase.h"

#include "DataFilters/interfaces/factories/IDataFilterFactory.h"
#include "DataFilters/interfaces/IDataFilter.h"
#include "configuration/CConfigHelper.h"

#include "interfaces/CDebugHelper.h"

#include "daemon.h"

using namespace std;

#ifdef HAVE_LIBLOG4CXX
using namespace log4cxx;
#endif

/** this array of string specifies which sections int the config file must be present.
 * The program will abort if any of these is missing.
 */
static const char *required_sections[] = { "application", "inverter",
		"inverter.inverters", "logger", "logger.loggers" };


/** Just dump the read config to cout.... (the values are automatically promoted to a string...)

 Use with:
 [code]
 libconfig::Setting &set = Registry::Configuration()->getRoot(); if (set.getName()) cout << set.getName(); else cout << "anonymous";
 cout << " has the Path \"" << set.getPath() << "\" and Type " << set.getType() << " and has a Lenght of " <<  set.getLength() << endl;

 DumpSettings(set);
 [/code]
 */
void DumpSettings(libconfig::Setting &set)
{
	cout << "line " << set.getSourceLine() << " ";

	if (set.getPath() != "")
		cout << set.getPath() << " ";

	if (!set.getName()) {
		cout << "(anonymous) ";
	}

	try {
		std::string s = set;
		cout << "= " << s << "\t is";
	} catch (...) {

	}

	try {
		float s = set;
		cout << "= " << s << "\t is";
	} catch (...) {

	}

	try {
		bool s = set;
		cout << "= " << s << "\t is";
	} catch (...) {

	}


	if (set.isAggregate()) {
		cout << " aggregate";
	};
	if (set.isArray()) {
		cout << " array";
	}
	if (set.isGroup()) {
		cout << " group";
	}
	if (set.isList()) {
		cout << " list";
	}
	//if (set.getPath() != "" ) cout << set.getPath() << "." ;
	if (set.isNumber())
		cout << " number";
	if (set.isScalar())
		cout << " scalar";

	cout << endl;

	for (int i = 0; i < set.getLength(); i++) {

		libconfig::Setting &s2 = set[i];
		DumpSettings(s2);
	}
}

int main(int argc, char* argv[])
{
    CDebugHelperCollection dhc("main section");
	bool error_detected = false;
	bool dumpconfig = false;
	string configfile = "solarpowerlog.conf";
	long autoterminate = 0;
	string printsnippets;

	progname = argv[0];
    dhc.Register(new CDebugObject<char*>("progname", progname));
    dhc.Register(new CDebugObject<int>("argc", argc));
    for (int i=0; i<argc; i++) {
        char buf[10];
        snprintf(buf,sizeof buf-1,"argv[%d]",i);
        dhc.Register(new CDebugObject<char*>(buf, argv[i]));
    }


#ifdef  HAVE_CMDLINEOPTS
	using namespace boost::program_options;

	options_description desc("Program Options");
	desc.add_options()
	    ("help", "this message")
	    ("conf,c", value<string>(&configfile), "specify configuration file")
	    ("version,v", "display solarpowerlog version")
	    ("background,b", value<bool>(&background)->zero_tokens(),
	        "run in background.")
	    ("dumpcfg", value<bool>(&dumpconfig)->zero_tokens(),
	        "Dump configuration structure, then exit")
	    ("chdir", value<string>(&rundir), "working directory for daemon "
	        " (only used when running as a daemon). Defaults to /")
	    ("stdout",value<string>(&daemon_stdout), "redirect stdout to this file "
	        "(only used when running as a daemon). Defaults to /dev/null")
	    ("stderr", value<string>(&daemon_stderr), "redirect stderr to this "
	        "file (only used when running as a daemon). Defaults to /dev/null")
	    ("pidfile", value<string>(&pidfile),
			"create a pidfile after the daemon has been started. "
			"(only used when running as a daemon.) Default: no pid file")
	    ("autoterminate", value<long>(&autoterminate),
	        "terminate after performing this amount of work. "
	        "This feature is intended for debugging, e.g valgrind runs.")
        ("printsnippet", value<string>(&printsnippets),
            "print configuration snippets to stdout. "
            "the parameter specifies which snippet should be generated and is "
            "of the format <category>[::<subcategory>]::[<target>.] "
            "category is either inverter, filter or communication. "
            "subcategroy is for inverters specifing the manufacturer and "
            "target is the final target to get the snippet from. Note that "
            "this function is intended as development aid to keep snippets and code in sync. "
            " solarpowerlog will exit after the snippet has been dumped."
            );

	variables_map vm;
	try {
		store(parse_command_line(argc, argv, desc), vm);
		notify(vm);
	} catch (exception &e) {
		cerr << "commandlinge options problem:" << desc << "\n" << e.what() << "\n";
		return 1;
	}

	if (vm.count("help")) {
		cout << desc << "\n";
		return 0;
	}

	if (vm.count("version")) {
		cout << PACKAGE_STRING << endl;
		return 0;
	}

#else
	if (argc > 1) {
		(void) argv; // remove warning about unused parameter.
		cerr << "This version does not support command line options." << endl;
		exit(1);
	}
#endif

	if (!printsnippets.empty()) {
	    // minimum bootstraping: Fake config and logging set to OFF.
	    Registry::Instance().FakeConfig();

	    BasicConfigurator::configure();
        LoggerPtr l = Logger::getRootLogger();
        l->setLevel(Level::toLevel(Level::OFF_INT));
        Registry::Instance().GetMainLogger().SetLoggerLevel(Level::toLevel(Level::OFF_INT));

	    std::string cat, subcat, target;
	    try {
	        size_t s = printsnippets.find("::");
	        cat = printsnippets.substr(0,s);
	        printsnippets = printsnippets.substr(s+2);

	        if (cat == "inverter") {
	            s = printsnippets.find("::");
	            cat = printsnippets.substr(0,s);
                printsnippets = printsnippets.substr(s+2);

                std::auto_ptr<IInverterFactory>
                    factory(InverterFactoryFactory::createInverterFactory(cat));

                if (!factory.get()) {
                    cerr << "printsnippet: Could not create inverter factory." << endl;
                    return 1;
                }

                std::auto_ptr<IInverterBase>
                    inverter(factory->Factory(printsnippets,"name","empty"));

                if (!inverter.get()) {
                    cerr << "printsnippet: Could not create inverter object." << endl;
                    return 1;
                }
                std::auto_ptr<CConfigCentral> cfg(inverter->getConfigCentralObject(NULL));
                if (!cfg.get()) {
                    cerr << "printsnippet: Could not create inverter's configuration information." << endl;
                    return 1;
                }

                cout << cfg->GetConfigSnippet();
                return 0;

	        } else if ( cat == "communication") {

            } else if ( cat == "datafilter") {
                s = printsnippets.find("::");
                cat = printsnippets.substr(0,s);
                IDataFilterFactory dffactory;
                std::auto_ptr<IDataFilter> datafilter(
                    dffactory.FactoryByName(cat, "name", "empty"));

                if (!datafilter.get()) {
                    cerr << "printsnippet: Could not create datafilter object." << endl;
                    return 1;
                }

                std::auto_ptr<CConfigCentral> cfg(datafilter->getConfigCentralObject(NULL));
                if (!cfg.get()) {
                    cerr << "printsnippet: Could not create inverter's configuration information." << endl;
                    return 1;
                }

                cout << cfg->GetConfigSnippet();
                return 0;

            } else {
                cerr << "printsnippets: unkown category." << endl;
                return 1;
            }
	    }
	    catch (...) {
	        cerr << "Error while handling printsnippets" << endl;
	        return 1;
	    }
	    return 0;
	}

	if (!Registry::Instance().LoadConfig(configfile)) {
		cerr << "Could not load configuration " << configfile << endl;
		exit(1);
	}

	if (dumpconfig) {
		cout << "Dumping structure of " << configfile << endl;
		Registry::Configuration()->setAutoConvert(true);
		DumpSettings(Registry::Configuration()->getRoot());
		exit(0);
	}

	// Note: As a limitation of libconfig, one cannot create the configs
	// structure.
	// Therefore we check here for the basic required sections and abort
	// if they are missing
	{
		libconfig::Config *cfg = Registry::Configuration();
		libconfig::Setting &rt = cfg->getRoot();

		for (unsigned int i = 0; i < sizeof(required_sections) / sizeof(char*); i++) {
			if (!rt.exists(required_sections[i])) {
				cerr << " Configuration Check: Required Section "
						<< required_sections[i] << " missing" << endl;
				error_detected = true;
			}
		}
	}

	if (error_detected) {
		cerr << "Error detected" << endl;
		exit(1);
	}

#if defined HAVE_LIBLOG4CXX

#ifdef HAVE_OPENLOG
	// prepare the syslog, needed if we gonna log to it
	// (if the user configures this, as the liblog4cxx supports syslog as well)
	openlog(progname, LOG_PID, LOG_USER);
#endif

	// Activate Logging framework
	{
		string tmp;
		LoggerPtr l = Logger::getRootLogger();

		CConfigHelper global("application");
		global.GetConfig("dbglevel", tmp, (std::string) "ERROR");
		l->setLevel(Level::toLevel(tmp));
		// Set the mainlogger priority to this prio as well.
		Registry::Instance().GetMainLogger().SetLoggerLevel(Level::toLevel(tmp));

		try {
			// Choose your poison .. aem .. config file format
			if (global.GetConfig("logconfig", tmp)) {
				if (tmp.substr(tmp.length() - 4, string::npos) == ".xml") {
					xml::DOMConfigurator::configure(tmp);
				} else {
					PropertyConfigurator::configure(tmp);
				}

			} else {
				BasicConfigurator::configure();
			}
		} catch (...) {
			cerr << "WARNING: Could not configure logging." << endl;
		}

		LOG4CXX_INFO(l,"Logging set up.");
	}
#endif

	// fork to background if demanded
	if (background)
		daemonize();

	SetupSignalHandler();

	/** bootstraping the system */
	ILogger mainlogger;

	LOGINFO(mainlogger, "Instanciating Inverter objects");

	/** create the inverters via its factories. */
	{
		string section = "inverter.inverters";
		libconfig::Setting &rt = Registry::Configuration()->lookup(section);

		for (int i = 0; i < rt.getLength(); i++) {
			std::string name;
			std::string manufacturer;
			std::string model;

			// alias for manufacturer is manufactor -- old
            try {
                manufacturer = (const char *)rt[i]["manufacturer"];
            } catch (libconfig::SettingNotFoundException &e1) {
                try {
                    manufacturer = (const char *)rt[i]["manufactor"];
                    LOGWARN(mainlogger, "NOTE: \"manufactor\" is depreciated. "
                        "Please update to \"manufacturer\".");
                } catch (libconfig::SettingNotFoundException &e2) {
                    LOGFATAL(mainlogger,
                        "Configuration Error: Required Setting was not found: \""
                        << e1.getPath() << '\"');
                    cleanup();
                    exit(1);
                }
            }

			try {
				name = (const char *) rt[i]["name"];
				model = (const char *) rt[i]["model"];
				LOGINFO(mainlogger,
						"Setting up inverter " << name << " (" << manufacturer << ")");
			} catch (libconfig::SettingNotFoundException &e) {
				LOGFATAL(mainlogger,
						"Configuration Error: Required Setting was not found in \""
						<< e.getPath() << '\"');
				cleanup();
				exit(1);
			}

			if (Registry::Instance().GetInverter(name)) {
				LOGFATAL(mainlogger, "Inverter " << name
						<< " declared more than once");
				cleanup();
				exit(1);
			}

			IInverterFactory *factory =
					InverterFactoryFactory::createInverterFactory(manufacturer);
			if (!factory) {
				LOGFATAL(mainlogger,
						"Unknown inverter manufacturer \""
						<< manufacturer << '\"');
				cleanup();
				exit(1);
			}

			IInverterBase *inverter = factory->Factory(model, name,
					rt[i].getPath());

			if (!inverter) {
				LOGFATAL(mainlogger,
						"Cannot create inverter model "
						<< model << " for manufacturer \""
						<< manufacturer << '\"');

				LOGFATAL(mainlogger,
						"Supported models are: "
						<< factory->GetSupportedModels());
				cleanup();
				exit(1);
			}

			if (!inverter->CheckConfig()) {
				LOGFATAL(mainlogger,
						"Inverter " << name << " ( "
						<< manufacturer << ", " << model
						<< ") reported configuration error");
				cleanup();
				exit(1);
			}

			Registry::Instance().AddInverter(inverter);
			// destroy the (used) factory.
			delete factory;
		}
	}

	LOGINFO(mainlogger, "Instantiating data filter objects");

	{
		IDataFilterFactory factory;

		/* create the data filters via its factories. */
		string section = "logger.loggers";
		libconfig::Setting & rt = Registry::Configuration()->lookup(section);

		for (int i = 0; i < rt.getLength(); i++) {
			std::string name;
			std::string previousfilter;
			std::string type;

			try {
				name = (const char *) rt[i]["name"];
				previousfilter = (const char *) rt[i]["datasource"];
				type = (const char *) rt[i]["type"];

				LOGINFO(mainlogger,
						"Datafilter " << name << " ("
						<< type << ") connects to "
						<< previousfilter <<
						" with Config-path " << rt[i].getPath());

			} catch (libconfig::SettingNotFoundException &e) {
				LOGFATAL(mainlogger,
						"Configuration Error: Required Setting was not found in \""
						<< e.getPath() << '\"' );
				cleanup();
				exit(1);
			}

			// TODO Also check for duplicate DataFilters.
			if (Registry::Instance().GetInverter(name)) {
				LOGFATAL(mainlogger,
						"CONFIG ERROR: Inverter or Logger Nameclash: "
						<< name << " declared more than once"
				);
				cleanup();
				exit(1);
			}

			IDataFilter *filter = factory.Factory(rt[i].getPath());

			if (!filter) {
				LOGFATAL(mainlogger,
						"Couldn't create DataFilter " << name
						<< "(" << type << ") connecting to "
						<< previousfilter << " Config-path " << rt[i].getPath()
				);
				cleanup();
				exit(1);
			}

			if (!filter->CheckConfig()) {
				LOGFATAL(mainlogger,
						"DataFilter " << name << "(" << type
						<< ") reported config error"
						<< previousfilter );
				cleanup();
				exit(1);
			}

			Registry::Instance().AddInverter(filter);
			// Filter is ready.
		}
	}

	while (!killsignal) {

		if (sigusr1) {
			if (background ) {
				logreopen(true);
				LOGINFO(mainlogger, "Logfiles rotated");
			}
			sigusr1= false;
		}

		Registry::GetMainScheduler()->DoWork(true);

        if (autoterminate) {
            LOGERROR(Registry::GetMainLogger(),
                " !!!!! AUTOTERMINATE ENABLED. !!!!! left=" << autoterminate);

            if (0 == --autoterminate) {
                LOGDEBUG(Registry::GetMainLogger(), "Raising SIGTERM");
                raise(SIGTERM);
            }
        }
    }

	// Ensure that the "terminator thread" (if spawn) is finished before
	// handling the remaining events (or the broadcast shutdown event
	// might get lost)
	terminator_thread.join();

	// Termination requested -- execute all pending events.
	while(Registry::GetMainScheduler()->DoWork(false));

	LOGINFO(Registry::GetMainLogger(), "Terminating.");

	Registry::Instance().Shutdown();
	cleanup();
	return 0;
}
