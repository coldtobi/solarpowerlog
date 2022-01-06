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


/** \defgroup DataFilter Loggers and DataFilters: Description and Configuration
 *
 * This page documents the available loggers / datafilters.
 *
 * \ref CVSDataLogger
 *
 * \ref CDumpOutputFilter
 *
 * */

/** \file IDataFilter.h
 *
 *  Created on: Jun 1, 2009
 *      Author: tobi
 *
 *   Interface for all filters, data enhancers and finally all data-sinks
 *   (which do not implement the ObserverSubject pattern)
 *
 *   How it works:
 *   - A filter, when instanciatesd, gets its configuration its own name
 *   - Using that configuration, it connects to the data source (type IInverterBase)
 *     and subscribes at least to the CAPA Notifications of the data source
 *   - the filter is aware, that most likely the capabilites will change over
 *     runtime. It will only serve the
 *   - If a cdownsteram filter queries for something which is not
 *     modified or created, the request is simply forwarded upstream
 */

#ifndef IDATAFILTER_H_
#define IDATAFILTER_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/** \file IDatafilter.h
 *
 * A Datafilter is a processor for inverter datas, or a logger which will sinks
 * the data to some file or database, ....
 *
 * For details describing how inverters, capabilites and datafilter work
 * together, please see \ref IInverterBase "this page".
 *
 * TODO DOCUMENT ME!
 */
#include "patterns/IObserverObserver.h"
#include "patterns/IObserverSubject.h"
#include "Inverters/interfaces/InverterBase.h"
#include "patterns/ICommand.h"
#include "DataFilters/interfaces/factories/IDataFilterFactory.h"


class IDataFilter : public IObserverObserver ,
	public IInverterBase // The inverter, though the capabilites, also provides the
// interface for being a subject (observer pattern)
{
protected:
	IDataFilter( const string &name, const string & configurationpath );

public:
	virtual ~IDataFilter();

	/** Filters need not to use the CommandQueue facility, so defaulting to
	 * doing nothing
	 * */
	virtual void ExecuteCommand( const ICommand *)
	{ }

	// Just a reminder: You need to override that.
	virtual bool CheckConfig() = 0;

	// Just another reminder. Update is called by the subject,
	// when new data arrives. So you defintly want to override this.
	virtual void Update( const IObserverSubject *subject ) = 0;

	/** When iterating over all the Capabilities, also transparently iterate
	 * over all parent objects as well. See INestedCapaIterator for details.
	 *
	 * \returns A "nested" CapaIterator, (CNestetCapaIterator), which
	 *  traverses thourh all layers.
	*/
	virtual ICapaIterator* GetCapaNewIterator();

	/** Check if the current DataFilter has the capability, if not ask its
	 * parent.
	 *
	 * Overriden from IInverterbase, this function checks first its own
	 * list and then asks its parent for the Capability
	 *
	 * \param identifier Capability to be looked for.
	 *  */
	virtual CCapability *GetConcreteCapability( const string &identifier );

	// datasource is config from the baseclass..
	virtual CConfigCentral* getConfigCentralObject(CConfigCentral *parent);

protected:
	/// Inverter to connect to. Can also be a another DataFilter
	/// (as data are exchanged over the IInverterBase Interface,
	/// this does not matter.
	/// Note: The child has to set this to the proper value.
	/// If the child wants to reveive data from multiple / different sources,
	/// it also might use its own implementation
	IInverterBase *base;

protected:
	std::string _datasource;

};

#endif /* IDATAFILTER_H_ */
