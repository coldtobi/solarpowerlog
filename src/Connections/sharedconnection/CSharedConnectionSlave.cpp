/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

Copyright (C) 2010-2012 Tobias Frost

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
 * CConnectSlave.cpp
 *
 *  Created on: Sep 13, 2010
 *      Author: tobi
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "porting.h"
#endif

#ifdef HAVE_COMMS_SHAREDCONNECTION

#include "CSharedConnectionSlave.h"
#include "configuration/CConfigHelper.h"
#include "Inverters/interfaces/InverterBase.h"
#include "CSharedConnection.h"

// Slave Configuration Parameter
// useconnection = "name"  Name of the inverter having the master connection.
// timeout = "override for timeout if not specified for receive operations. Unit ms"


CSharedConnectionSlave::CSharedConnectionSlave(const string & configurationname) :
	IConnect(configurationname)
{
	master = NULL;
}

CSharedConnectionSlave::~CSharedConnectionSlave()
{
}

void CSharedConnectionSlave::Connect(ICommand *callback)
{
	assert(master);
	master->Connect(callback);
}

void CSharedConnectionSlave::Disconnect(ICommand *callback)
{
	// only the master will be allowed to disconnect for the time being.
	assert(master);
	callback->addData(ICMD_ERRNO, 0);
	Registry::GetMainScheduler()->ScheduleWork(callback);
}

void CSharedConnectionSlave::Send(ICommand *callback)
{
	assert(callback);
	assert(master);
	master->Send(callback);
}

void CSharedConnectionSlave::Receive(ICommand *callback)
{
	assert(master);

    // Oct-2012:
    // Timeouts are supposed inserted by the inverters logic,
    // so this logic can and will go.
#if 0
	// If there is no timeout specified in the callback, derive it from
	// the configuration or default. (The master does not have access to the
	// slaves configuration, so we have to make it sure here...)

	unsigned long timeout = 0;

	try {
		timeout
				= boost::any_cast<long>(callback->findData(ICONN_TOKEN_TIMEOUT));
	} catch (...) {
		CConfigHelper cfg(ConfigurationPath);
		cfg.GetConfig("timeout", timeout);
		if (timeout != 0) {
			callback->addData(ICONN_TOKEN_TIMEOUT, timeout);
		}
	}
#endif
	// Now, tell the master to do the job.
	// (The master will add the timestamp for timeout handling for us....)
	master->Receive(callback);
}

bool CSharedConnectionSlave::CheckConfig(void)
{

	CConfigHelper cfg(ConfigurationPath);
	bool fail = false;
	std::string s;
	fail |= !cfg.GetConfig("useconnection", s);

	fail |= !cfg.CheckConfig("useconnection", libconfig::Setting::TypeString,
			false);

#if 0
	// obsolete, see Receive()
	fail |= !cfg.CheckConfig("timeout", libconfig::Setting::TypeInt, true);
#endif

	if (fail)
		return false;

	IInverterBase *base = Registry::Instance().GetInverter(s);

	if (!base) {
		LOGERROR(logger,"useconnection must point to a known Inverter and this "
				"inverter must be declared first. Inverter: " << s;);
		return false;
	}

	CConfigHelper bcfg(base->GetConfigurationPath());
	bcfg.GetConfig("comms", s, std::string(""));
	if (s != "SharedConnection") {
		LOGERROR(logger,"inverter " << base->GetName() <<
				" does not use a shared connection.");
		return false;
	}

	bcfg.GetConfig("sharedconnection_type", s);
	if (s != "master") {
		LOGERROR(logger,"inverter " << base->GetName() <<
				" does not use a shared master connection" );
		return false;
	}

	master = (CSharedConnection*) base->getConnection();

	return true;

}

bool CSharedConnectionSlave::IsConnected(void)
{
	assert(master);
	return master->IsConnected();
}

bool CSharedConnectionSlave::AbortAll()
{
    // only the master may abort all commands at this time.
    return false;
}

#endif