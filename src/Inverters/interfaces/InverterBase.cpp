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
/*
 * InverterBase.cpp
 *
 *  Created on: May 9, 2009
 *      Author: tobi
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <map>
#include <assert.h>

#include "Inverters/interfaces/InverterBase.h"
#include "Connections/factories/IConnectFactory.h"
#include "interfaces/CCapability.h"

#include "Inverters/Capabilites.h"
#include "patterns/IValue.h"
#include "patterns/CValue.h"

#include "Inverters/interfaces/ICapaIterator.h"

#include "configuration/ConfigCentral/CConfigCentral.h"

using namespace std;

IInverterBase::IInverterBase( const string& name,
	const string & configurationpath, const string& role )
{

	// Setup the logger
	logger.Setup(name,configurationpath, role);

	this->name = name;
	this->configurationpath = configurationpath;
	connection = IConnectFactory::Factory(configurationpath);
	connection->SetupLogger(logger.getLoggername());

	pair<map<string, CCapability*>::iterator, bool> b;

	string s;
	CCapability *c;

    // Add the "must have" capabilites.
    c = new CCapability(CAPA_CAPAS_UPDATED, new CValue<CAPA_CAPAS_UPDATED_TYPE>,
        this);
    b = CapabilityMap.insert(pair<string, CCapability*>(CAPA_CAPAS_UPDATED, c));
    assert( b.second);

    c = new CCapability(CAPA_CAPAS_REMOVEALL,
        new CValue<CAPA_CAPAS_REMOVEALL_TYPE>, this);
    b = CapabilityMap.insert(
        pair<string, CCapability*>(CAPA_CAPAS_REMOVEALL, c));
    assert( b.second);

    c = new CCapability(CAPA_INVERTER_DATASTATE,
        new CValue<CAPA_INVERTER_DATASTATE_TYPE>, this);
    b = CapabilityMap.insert(
        pair<string, CCapability*>(CAPA_INVERTER_DATASTATE, c));
    assert( b.second);
}

IInverterBase::~IInverterBase()
{
	if (connection)
		delete connection;
	connection = NULL;

	// TODO auto destruct all elements in the capability map.
	map<string, CCapability*>::iterator it = GetCapabilityIterator();
	for (; it != GetCapabilityLastIterator(); it++)
		delete (*it).second;
	CapabilityMap.clear();

}

const std::string& IInverterBase::GetName( void ) const
{
	return name;
}

ICapaIterator *IInverterBase::GetCapaNewIterator()
{
	return new ICapaIterator(this);
}

/** returns a iterator of the Capabilties. The iterator is inizialized at the begin of the map.*/
map<string, CCapability*>::iterator IInverterBase::GetCapabilityIterator()
{
	map<string, CCapability*>::iterator it = CapabilityMap.begin();
	return it;
}

/** return a iterator of the Capabilites. The iterator is placed at the end of the map
 * This allows a end-of-list check */
map<string, CCapability*>::iterator IInverterBase::GetCapabilityLastIterator()
{
	map<string, CCapability*>::iterator it = CapabilityMap.end();
	return it;
}

CCapability *IInverterBase::GetConcreteCapability( const string &identifier )
{
	map<string, CCapability*>::iterator it = CapabilityMap.find(identifier);
	if (it == CapabilityMap.end())
		return 0;
	return it->second;
}

/// Add a Capability for the inverter.
void IInverterBase::AddCapability(CCapability* capa)
{
	CapabilityMap.insert(pair<string, CCapability*> (capa->getDescription(), capa));
	LOGDEBUG(logger, "Added new Capability to " << name << ": " << capa->getDescription());
}
