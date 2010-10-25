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

}

CSharedConnectionSlave::~CSharedConnectionSlave()
{
}

bool CSharedConnectionSlave::Connect(ICommand *callback)
{
	assert(master);
	return master->Connect(callback);
}

bool CSharedConnectionSlave::Disconnect(ICommand *callback)
{
	assert(master);
	return master->Disconnect(callback);
}

bool CSharedConnectionSlave::Send(const char *tosend, unsigned int len,
		ICommand *callback)
{
	assert(master);
	return master->Send(tosend, len, callback);
}

bool CSharedConnectionSlave::Send(const string& tosend, ICommand *callback)
{
	assert(master);
	return master->Send(tosend, callback);
}

bool CSharedConnectionSlave::Receive(ICommand *callback)
{
	assert(master);

	// If there is no timeout specified in the callback, derive it from
	// the configuration or default. (The master does not have access to the
	// slaves configuration, so we have to make it sure here...)
#warning configuration parameter: check in config check and documentation missing

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

	// Now, tell the master to do the job.
	// (The master will add the timestamp for timeout handling for us....)
	return master->Receive(callback);
}

bool CSharedConnectionSlave::CheckConfig(void)
{

	CConfigHelper cfg(ConfigurationPath);
	bool fail = false;
	std::string s;
	fail |= !cfg.GetConfig("useconnection", s);

	fail |= !cfg.CheckConfig("useconnection", libconfig::Setting::TypeString,
			false);

	fail |= !cfg.CheckConfig("timeout", libconfig::Setting::TypeInt, false);

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
	if (s != "sharedconnection") {
		LOGERROR(logger,"inverter " << base->GetName() <<
				" does not use a shared connection.");
		return false;
	}

	cfg.GetConfig("sharedconnectiontype", s);
	if (s != "master") {
		LOGERROR(logger,"inverter " << base->GetName() <<
				" does not use a shared master connection.");
		return false;
	}

	//
	master = (CSharedConnection*) base->getConnection();

	return true;

}

bool CSharedConnectionSlave::IsConnected(void)
{
	assert(master);
	return master->IsConnected();
}

#endif