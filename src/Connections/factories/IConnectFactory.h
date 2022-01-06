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

/** \file IConnectFactory.h
 *
 *  \date Created on: May 16, 2009
 *  \author: Tobias Frost (coldtobi)
 */

#ifndef CONNECTIONFACTORY_H_
#define CONNECTIONFACTORY_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_COMMS_ASIOTCPIO
#define COMMS_ASIOTCP_ID "TCP/IP"
#else
#define COMMS_ASIOTCP_ID
#endif

#ifdef HAVE_COMMS_ASIOSERIAL
#define COMMS_ASIOSERIAL_ID "RS2xx"
#else
#define COMMS_ASIOSERIAL_ID
#endif

#ifdef HAVE_COMMS_SHAREDCONNECTION
#define COMMS_SHARED_ID "SharedConnection"
#else
#define COMMS_SHARED_ID
#endif


#include "Connections/interfaces/IConnect.h"

using namespace std;

/** Factory for IConnection-Classes
 *
 *\ingroup factories
 */
class IConnectFactory
{
public:
	/** Factory for generation of connection methods.
	 * Give it the configurationpath, and out of the config, it will
	 * generate the right class.
	 *
	 * If the class is not known, it will return a dummy connection class.
	 * So you can also create inverters or derived classes without commms.
	 *
	 * \parameter configurationpath Where to extract the type of the
	 * requested comms?
	 *  */
	static IConnect* Factory( const string &configurationpath );

protected:
	IConnectFactory()
	{};
public:
	virtual ~IConnectFactory()
	{};

};

#endif /* CONNECTIONFACTORY_H_ */
