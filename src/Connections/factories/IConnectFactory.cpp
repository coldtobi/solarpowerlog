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

/** \file IConnectFactory.cpp
 *
 *  \date Created on: May 16, 2009
 *  \author: Tobias Frost (coldtobi)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "porting.h"
#endif

#include "configuration/Registry.h"
#include "configuration/CConfigHelper.h"
#include "Connections/factories/IConnectFactory.h"
#include "Connections/CConnectDummy.h"

#include <libconfig.h++>

#ifdef HAVE_COMMS_ASIOTCPIO
#include "Connections/CConnectTCPAsio.h"
#endif

#ifdef HAVE_COMMS_ASIOSERIAL
#include "Connections/CConnectSerialAsio.h"
#endif

#ifdef HAVE_COMMS_SHAREDCONNECTION
#include "Connections/sharedconnection/CSharedConnection.h"
#endif

using namespace std;

/** Facortry for generation of connection methods.
 * Give it the configurationpath, and out of the config, it will
 * generate the right class.
 *
 * If the class is not known, it will return a dummy connection class.
 * So you can also create inverters or derived classes without comms. */
IConnect * IConnectFactory::Factory( const string &configurationpath )
{
	string type = "";
	CConfigHelper cfghelper(configurationpath);
	cfghelper.GetConfig("comms",type);

#ifdef HAVE_COMMS_ASIOTCPIO
	if (type == COMMS_ASIOTCP_ID) {
		return new CConnectTCPAsio(configurationpath);
	}
#endif
#ifdef HAVE_COMMS_ASIOSERIAL
	if (type == COMMS_ASIOSERIAL_ID ) {
		return new CConnectSerialAsio(configurationpath);
	}
#endif
#ifdef HAVE_COMMS_SHAREDCONNECTION
	if (type == COMMS_SHARED_ID) {
		return new CSharedConnection(configurationpath);
	}
#endif

	return new CConnectDummy(configurationpath);
}

