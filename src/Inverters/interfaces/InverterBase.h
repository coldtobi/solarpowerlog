/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

 Copyright (C) 2009-2015 Tobias Frost

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

/** \defgroup Inverters Description and Configration of Inverter Models */

/** \file InverterBase.h
 *
 *  \date   May 9, 2009
 *  \author Tobias Frost
 *
 * This is the interface for inverters.
 *
 * \page IInverterBase IInverterBase: Interface for Inverters
 * \ingroup Concepts
 *
 * IIinverterBase is the interface which should be used for all inverters.
 * It is also the base class for all DataFilters, (IDataFilter), as they
 * also need most of the infracstructure for their duties.
 *
 * It is ought to be derived, and the concrete implemenation implements the
 * data aquisition.
 *
 * To help avoiding doublicated code, the base class have contains already some
 * code for the following funcions:
 *	- <b>\ref IIBConnection "Comms"</b>
 *		- Generate the concrete connection object (IConnect
 *		  derived) The type of connection is extracted automatically
 *		  from the configuration. On Datafilters, a default "Dummy"
 *		  Connection will be created, if the specific configuratoin
 *		  entries are missing.
 *	- <b>\ref IIBCapabilites "Capabilites"</b>
 *		- Generating the required CCapabilites (see
 *		  #Inverters/Capabilites.h)
 *		- Infrastructure around Capabilites: Adding, Iterator, Finding
 *	          an concrete Capability...
 *	 - <b> \ref IIBWorkScheduler "Target for a WorkScheduler" </b>
 *	 	- Inverters should use solarpowerlog's Work Scheduler for
 *	 	  execution planning. To be able to receive ICommands they are
 *	 	  derived from ICommandTarget.
 *	        - By using the default Scheduler (see #Registry), concurrency
 *	          is avoided, so there is no need for locking methods for common
 *	          accessed datas.
 *
 * \section IIBConnection Connection: Connecting to the word.
 *
 * As stated before, the base class creates the communication object.
 * To determine which connectiion type is to be used, it will use the factory
 * IConnectionFactory. This factory looks in the configuration-path -- supplied
 * by the bootstrap code -- for the entry "commtype" and create the associated
 * comms object.
 *
 * BTW: The connection classes will also retrieve their configuration from the
 * config file automatically.
 *
 * The comms object will <b>not</b> automatically connect. This should be done
 * later while running the initializtion commmand (see workqueue below)
 *
 * \image html inverter_comm.png "Connection Services"
 *
 * The image above shows the services currently exists for the comm classes.
 *
 * Currently, the connection classed are mixed synchron and asynchronous. But
 * this will change, the connection class should use asynchronous
 * methods to avoid blocking the main task while waiting for completion.
 *
 * This also implies, that the inverters have to implement some kind of time-out
 * handling by itself. (Well its on the TODO list: \todo implement some kind of
 * generic timeout handling, like schedule timeout, cancel timeout.)
 *
 * \sa #IInverterBase::connection
 *
 * \section IIBCapabilites How Capabilites transport datas
 *
 * As described \ref CapaConcept "here", Capabilies store abstraced measurement
 * values as well as some meta-datas controlling the Capabilites itself.
 *
 * Simplified, the values are generated at the inverter's object and passed
 * down to Datafilters (or loggers).
 *
 * \image html FilterSequence.png "Capabilitiy passed through two filters (simplified)"
 *
 * The data is passed along the Filters using the Observer Design pattern
 * inheritated by the capability: Upon updating it, the Inverter calls the
 * Notify function and the DataFilter gets notified.
 *
 * As illustrated,
 * Datafilters can also be chained. With this abitrary data
 * manipulations can be achieved: The datafilter can calculate additional data
 * sets (example: Efficiency out of Pin and Pout), or do some other arithmetics
 * on them (mean value...)
 *
 * Datafilters can also be parallel to e.g enable parallel serving of data to
 * different logging systems: One would log to disk, the other servin a web
 * page.
 *
 * By the way, the Inverter manages a list of capablities in the map
 * (IInverterBase::CapabilityMap). (The map is organized in pairs of
 * <string,CCapability*> containing the description of the Capability and a
 * pointer to the Capability.) However, there is an API to access the Capability
 * in IInverteBase and an enhanced one in IDataFilter.
 *
 * ( \todo More details are planned to appear on the IDataFilter description.)
 *
 * The image below shows all the classes involved in the dataflow from the
 * Inverter to the DataFilter:
 *
 * \image html dataflow_classes.png "Classes needed to let the data flow"
 *
 * This image shows how a typical update of a Capbility, and how the Data-Filter
 * receives the information.
 *
 * \image html dataflow_sequence.png "Dataflow Sequence Diagram"
 *
 * \subsection IIBCSnip Code-Snippets around Capabilites.
 *
 * <b> Unconditionally Register a new Capability </b>(e.g in the constructor)
 * \code
 * 	c = new CCapability(CAPA_CAPAS_UPDATED, CAPA_CAPAS_UPDATED_TYPE, this);
 *	AddCapability(CAPA_CAPAS_UPDATED, c);
 * * \endcode
 *
 * <b> Update a Capability's Value, create if not existance. Check for type </b>
 *\code
 *
 * CCapability *cap = GetConcreteCapability(CAPA_INVERTER_TEMPERATURE_NAME);
 *
 * if (!cap) {
 *	string s;
 *	IValue *v;
 *	CCapability *c;
 *	s = CAPA_INVERTER_TEMPERATURE_NAME;
 *	v = IValue::Factory(CAPA_INVERTER_TEMPERATURE_TYPE);
 *	((CValue<float>*) v)->Set(f);
 *	c = new CCapability(s, v, this);
 *	AddCapability(s, c);
 *
 *	cap = GetConcreteCapability(CAPA_CAPAS_UPDATED);
 *	cap->Notify();
 * }
 *
 * // Capa already in the list. Check if we need to update it.
 * else if (cap->getValue()->GetType() == CAPA_INVERTER_TEMPERATURE_TYPE) {
 * 	CValue<float> *val = (CValue<float>*) cap->getValue();
 *
 *	if (val -> Get() != f) {
 *		val->Set(f);
 *		cap->Notify();
 *	}
 * } else {
 *	cerr << "BUG: " << CAPA_INVERTER_TEMPERATURE_NAME*
 *	<< " not a float ";
 * }
 *
 * \endcode
 *
 *
 * \section IIBWorkScheduler "Work Scheduler"
 *
 * IInverterbase is derived from ICOmmandTarget. This class can reiceive
 * commands from CWorkSchedule objects. One CWorkSchedule object can be received
 * by the #Registry, which is open for all objects to use. (This Scheduler is
 * operated by solarpowerlogs internals, so no extra code is required to have
 * an operating schedule.)
 *
 * For Details, for example, how to use a CWorkSchedule and how to schedule
 * some Work in a specific amount of time, please see the \ref PageCWorkSchedule
 * "WorkScheduler overview" Page.
 *
 * \todo that Workscheduler-Page is not yet written,

 */

#ifndef INVERTERBASE_H_
#define INVERTERBASE_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>
#include <map>

#include "Connections/interfaces/IConnect.h"
#include "Connections/factories/IConnectFactory.h"
#include "Inverters/factories/InverterFactoryFactory.h"
#include "interfaces/CCapability.h"
#include "patterns/ICommandTarget.h"
#include "configuration/ILogger.h"

class ICapaIterator;
class CConfigCentral;

using namespace std;

/// connection_timeout: Wait this long for the connection attempt to time out
/// note: currently FIXME (not implemented in ASIO TCP)
/// note: This tweak is optional. The Connection method needs not to honor
/// this parameter (especially if the success/failure is immediately known)
#define CONFIG_TWEAK_CONNECTION_TIMEOUT "option_connectiontimeout"
#define CONFIG_TWEAK_CONNECTION_TIMEOUT_DEFAULT (15000)


#define IBASE_DESCRIPTION_INTRO \
"Inverter definition\n" \
"These basic parameters are necessary so that solarpowerlog has knowldege " \
"about the inverter to be configured:\n" \
"\"name\", \"manufacturer\" and \"model\""

#define IBASE_DESCRIPTION_NAME \
"This configuration names the inverter. The name are used internally to identify " \
"the inverter and thus needs to be unique."

#define IBASE_DESCRIPTION_MANUFACTURER \
"Specifies the manufacturer of the inverter.\n" \
"Possible values are:\n" \
INV_MANU_SPUTNIK " " \
INV_MANU_DUMMY

#define IBASE_DESCRIPTION_MODEL \
"Specifies the model of the inverter."

#define IBASE_DESCRIPTION_COMMS \
"Specifies the communication method to be used.\n" \
"Possible values are:\n" \
COMMS_ASIOTCP_ID " " \
COMMS_ASIOSERIAL_ID " " \
COMMS_SHARED_ID " " \
"\nNote: Every communication method has its own configuration parameter, " \
"please consult its documentation and sample configuration files."

/** Inverter Interface .... */
// TODO: This class renamed, as it also fits for the "Filters" (Data source, data computing/enhancing, ...)
// Inverters will be only a special interface, derived from this base class
class IInverterBase: public ICommandTarget
{

public:
	friend class ICapaIterator;

	/** Constructor for the Inverter Interface.
	 * The baseclass does this already for their concrete Implementations:
	 * 	- Create the connection (out of config, if not needed a dummy
	 * 	  will be created
	 * 	- Create the "must have" Capabilites
	 * 		- CAPA_CAPAS_UPDATED
	 * 		- CAPA_CAPAS_REMOVEALL
	 * 		- CAPA_INVERTER_DATASTATE
	 * 	  \note: DataFilters might want to delete one or more of them
	 *        (out of their instance!), if they are sure that they don't
	 *        need them. For example, a DataFilter has usually no influence
	 *        over data validity. If it deletes its Capability, its client
	 *        will automatically get the one of the inverter)
	 *
	 * \param name of the Inverter (name as in the config)
	 * \param configurationpath is the path to access this inverters config
	 * \param role specifies the role in which the inverter is used. Currently this
	 *  is only used to setup the correct logging path for the associated logger.
	 *  use "inverter" for inverters and "datafilter" for filters.
	 *
	 */
	IInverterBase(const string &name, const string & configurationpath,
			const string & role);

	virtual ~IInverterBase();

	/** Getter for the name property. (Inverter Name equals to the one given in the config) */
	virtual const std::string& GetName(void) const;

	/** check for a specific capability and return the pointer to it
	 * returns NULL if it is not registered. */
	// TODO Move to c++ file
	virtual CCapability *GetConcreteCapability(const string &identifier);

	/// Get a Iterator over all Capability.
	/// Filters may overload it to provide a CCapaNestedIterator.
	/// \warning you are responsible deleting the iterator! Use auto_ptr !
	virtual ICapaIterator* GetCapaNewIterator();

	/// Check Configuration
	/// \returns false on error, true on success.
	virtual bool CheckConfig() = 0;

	virtual const std::string& GetConfigurationPath() const
	{
		return configurationpath;
	}

	virtual IConnect * getConnection(void) const {
		return connection;
	}

    /** Create & Get a CCOnfigCentral object for this instance.
     *
     * @return generated object. Ownership is waived to caller. might return NULL.
     *
     * \note in overriden version, call the base's implementation.
     * The base *might* already have added properties to check aloing with
     * their descriptions. It also can opt to just return a NULL pointer though.
     */
    virtual CConfigCentral* getConfigCentralObject(CConfigCentral *parent)
    {
        return parent;
    }

protected:
	/// Add a Capability for the inverter.
	friend class ISputnikCommand;
	virtual void AddCapability(CCapability* capa);

	/** returns a iterator of the Capabilties. The iterator is inizialized at the begin of the map.*/
	virtual map<string, CCapability*>::iterator GetCapabilityIterator(void);

	/** return a iterator of the Capabilites. The iterator is placed at the end of the map
	 * This allows a end-of-list check */
	virtual map<string, CCapability*>::iterator GetCapabilityLastIterator(void);

	/** Configuration path as determined on start -- for easier fetching the config.*/
	std::string configurationpath;
	/** Inverter Name -- as in config */
	std::string name;

	// Class for handling the connectivity.
	// Is a strategy design pattern interface. So different ways of connection can be handled by the same interface
	// (In other words: Our inverter does need nor want to know, if it is RS485 or TCP/IP, or even, both.)
	// NOTE: The Connection is to be made by a factory!
	// NOTE2: Beware: Connections can die any time! Make sure to handle this.
	IConnect *connection;

protected:

	// This maps contains all the Caps by the Inverter.
	// Caps implements the Subject in the Observer-Pattern.
	// This keeps all informed!
	// Class for handling capabilities.
	// (Inverters can add capabilities at runtime, also can enhancement filters.
	// (A Capability bundles reading of a inverter with a tag describing the information

	map<string, CCapability*> CapabilityMap;

	// Allow sub-objects of the inverter accessing the logger.
public:
	/// The Logger Class for Debugging, Error reporting etc...
	ILogger logger;
};

#endif /* INVERTERBASE_H_ */

// debug code to keep...

// Dumping the map.
//		map<string, CCapability*>::iterator it1 = CapabilityMap.begin();
//		cerr << "DUMP Capabilites: " << this->GetName() <<  endl;
//		for (int i=0; it1 != CapabilityMap.end(); it1++,i++)
//		{
//			cerr << i << " " << (*it1).first << endl;
//		}
