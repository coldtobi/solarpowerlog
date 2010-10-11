/*
 * CSharedConnectionMaster.h
 *
 *  Created on: Sep 13, 2010
 *      Author: tobi
 */

#ifndef CSHAREDCONNECTIONMASTER_H_
#define CSHAREDCONNECTIONMASTER_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "porting.h"
#endif

#ifdef HAVE_COMMS_SHAREDCONNECTION

#include <list>
#include <semaphore.h>

#include "interfaces/IConnect.h"
#include "Connections/CAsyncCommand.h"
#include "patterns/ICommandTarget.h"
#include "patterns/ICommand.h"
#include "CSharedConnectionSlave.h"


// Token inserted by this or the slave class to specify individual timeouts.
// At this timestamp, the command can be considered timed-out.
#define ICONNECT_TOKEN_TIMEOUTTIMESTAMP "CSharedConnection_Timeout"

#define ICONNECT_TOKEN_PRV_ORIGINALCOMMAND "CSharedConnection_OrgiginalWork"

#define SHARED_CONN_MASTER_DEFAULTTIMEOUT (3000UL)

class CSharedConnectionMaster: public IConnect, ICommandTarget
{

protected:
	friend class CSharedConnection;
	friend class CSharedConnectionSlave;
	CSharedConnectionMaster(const string & configurationname);

public:
	virtual ~CSharedConnectionMaster();

	void ExecuteCommand(const ICommand *Command);

protected:

	virtual bool Connect(ICommand *callback);

	virtual bool Disconnect(ICommand *callback);

	virtual void SetupLogger(const string& parentlogger, const string & = "");

	virtual bool Send(const char *tosend, unsigned int len, ICommand *callback =
			NULL);

	virtual bool Send(const string& tosend, ICommand *callback = NULL);

	virtual bool Receive(ICommand *callback);

	virtual bool CheckConfig(void);

	virtual bool IsConnected(void);

private:
	IConnect *connection;

	list<ICommand*> readcommands;

	// When is the current receive scheduled to timeout?
	boost::posix_time::ptime readtimeout;

};

#endif

#endif /* CSHAREDCONNECTIONMASTER_H_ */
