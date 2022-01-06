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

/** \file IDataFilter.cpp
 *  \date Jun 1, 2009
 *  \author Tobias Frost (coldtobi)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "DataFilters/interfaces/IDataFilter.h"
#include "Inverters/interfaces/CNestedCapaIterator.h"
#include "configuration/CConfigHelper.h"
#include "configuration/ConfigCentral/CConfigCentral.h"

#define DESCRIPTION_DATAFILTER_INTRO \
"Like inverters, loggers and datafilters needs some basic configuration parameter:\n" \
"\"name\", \"type\" and \"datasource\""

#define DESCRIPTION_DATAFILTER_NAME \
"This parameter names the logger. The name are used internally to identify " \
"the logger and thus needs to be unique."

#define DESCRIPTION_DATAFILTER_DATASOURCE \
"This parameter needs to state the name another logger or inverter -- " \
"the one which will supply data to this logger."

#define DESCRIPTION_DATAFILTER_TYPE \
"Tells solarpowerlog the type of the logger to be created. This version of " \
"solarpowerlog supports the following loggers (and datafilters): " \
FILTER_DUMBDUMPER " " \
FILTER_CSVWRITER " " \
FILTER_HTMLWRITER " " \
FILTER_DBWRITER " "

IDataFilter::IDataFilter(const string &name, const string & configurationpath) :
    IInverterBase(name, configurationpath, "datafilter"), base(0)
{
    // try to setup base to be more error-robust.
    // this way, Datafilter have already setup their base early on.
    // The child can always override later, e.g after CMD_INIT.
    CConfigHelper hlp(configurationpath);
    std::string str;

    if (hlp.CheckAndGetConfig("datasource", libconfig::Setting::TypeString, str)) {
        this->base = Registry::Instance().GetInverter(str);
    }
}


IDataFilter::~IDataFilter()
{
	// TODO Auto-generated destructor stub
}

ICapaIterator *IDataFilter::GetCapaNewIterator()
{
	return new CNestedCapaIterator(this, base);
}

CCapability *IDataFilter::GetConcreteCapability( const string & identifier )
{
	CCapability *c;
	if ((c = IInverterBase::GetConcreteCapability(identifier))) {
		// TODO cleanup debug code
		// cout << "DEBUG: found " << identifier << " in " << GetName()
		//	<< endl;
		return c;
	} else {
		// TODO cleanup debug code
		// cout << "DEBUG: searching " << identifier << " in base class "
		//	<< base->GetName() << endl;
		return base->GetConcreteCapability(identifier);
	}
}

// datasource is config from the baseclass..
CConfigCentral* IDataFilter::getConfigCentralObject(CConfigCentral *parent)
{
    static std::string dummy_string;

    if (!parent) parent = new CConfigCentral;

    (*parent)(NULL, DESCRIPTION_DATAFILTER_INTRO)
        ("name", DESCRIPTION_DATAFILTER_NAME, dummy_string)
        ("type", DESCRIPTION_DATAFILTER_TYPE, dummy_string)
        ("datasource", DESCRIPTION_DATAFILTER_DATASOURCE, _datasource);

    // Override optional setting and set a sane example.
    parent->SetExample("name", std::string("<name>"), false);
    parent->SetExample("datasource", std::string("<name of the source>"), false);
    parent->SetExample("type", std::string("<DataFilter-type>"), false);

    return parent;
}
