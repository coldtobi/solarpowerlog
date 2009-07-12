/* ----------------------------------------------------------------------------
 solarpowerlog
 Copyright (C) 2009  Tobias Frost

 This file is part of solarpowerlog.

 Solarpowerlog is free software; However, it is dual-licenced
 as described in the file "COPYING".

 For this file (CCSVOutputFilter.cpp), the license terms are:

 You can redistribute it and/or modify it under the terms of the GNU
 General Public License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 This programm is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Library General Public
 License along with this proramm; if not, see
 <http://www.gnu.org/licenses/>.
 ----------------------------------------------------------------------------
 */

/** \file CCSVOutputFilter.cpp
 *
 *  Created on: Jun 29, 2009
 *      Author: tobi
 */
#include <assert.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <memory>

#include <boost/date_time/gregorian/gregorian.hpp>
#include "boost/date_time/local_time/local_time.hpp"

#include "configuration/Registry.h"
#include "configuration/CConfigHelper.h"
#include "interfaces/CWorkScheduler.h"

#include "Inverters/Capabilites.h"
#include "patterns/CValue.h"

#include "CCSVOutputFilter.h"

#include "Inverters/interfaces/ICapaIterator.h"

using namespace libconfig;
using namespace boost::gregorian;

CCSVOutputFilter::CCSVOutputFilter( const string & name,
	const string & configurationpath ) :
	IDataFilter(name, configurationpath)
{
	headerwritten = false;

	// Schedule the initialization and subscriptions later...
	ICommand *cmd = new ICommand(CMD_INIT, this, 0);
	Registry::GetMainScheduler()->ScheduleWork(cmd);

	// We do not anything on these capabilities, so we remove our list.
	// any cascaded filter will automatically use the parents one...
	CCapability *c = IInverterBase::GetConcreteCapability(
		CAPA_INVERTER_DATASTATE);
	CapabilityMap.erase(CAPA_INVERTER_DATASTATE);
	delete c;

	// Also we wont fiddle with the caps requiring our listeners to unsubscribe.
	// They also should get that info from our base.
	c = IInverterBase::GetConcreteCapability(CAPA_CAPAS_REMOVEALL);
	CapabilityMap.erase(CAPA_CAPAS_REMOVEALL);
	delete c;

}

CCSVOutputFilter::~CCSVOutputFilter()
{
	if (file.is_open())
		file.close();
	// TODO Auto-generated destructor stub
}

bool CCSVOutputFilter::CheckConfig()
{
	string setting;
	string str;

	bool fail = false;

	CConfigHelper hlp(configurationpath);
	fail |= !hlp.CheckConfig("datasource", Setting::TypeString);
	fail |= !hlp.CheckConfig("clearscreen", Setting::TypeBoolean, true);
	fail |= !hlp.CheckConfig("logfile", Setting::TypeString);

	if (hlp.CheckConfig("data2log", Setting::TypeString, false, false)) {
		hlp.GetConfig("data2log", setting);
		if (setting != "all") {
			cerr
				<< "Configuration Error: data2log must be \"all\" or of Type Array."
				<< endl;
			fail = true;
		}
	} else if (!hlp.CheckConfig("data2log", Setting::TypeArray)) {
		fail = true;
	}

	// FIXME: This will not work, if the inverter / logger is instanciated later.
	// in the config file. This is not nice and can be avoided.
	// (delegate the to CMD_INIT)
	hlp.GetConfig("datasource", str);
	IInverterBase *i = Registry::Instance().GetInverter(str);
	if (!i) {
		cerr << "Setting " << setting << " in " << configurationpath
			<< "." << name
			<< ": Cannot find instance of Inverter with the name "
			<< str << endl;
		fail = true;
	}
	return !fail;
}

void CCSVOutputFilter::Update( const IObserverSubject *subject )
{
	assert (subject);
	CCapability *c, *cap = (CCapability *) subject;

	// Datastate changed.
	if (cap->getDescription() == CAPA_INVERTER_DATASTATE) {
		this->datavalid = ((CValue<bool> *) cap->getValue()) ->Get();
		return;
	}

	// Unsubscribe plea -- we do not offer this Capa, our customers will
	// ask our base directly.
	if (cap->getDescription() == CAPA_CAPAS_REMOVEALL) {
		auto_ptr<ICapaIterator> it(base->GetCapaNewIterator());
		while (it->HasNext()) {
			pair<string, CCapability*> cappair = it->GetNext();
			cap = (cappair).second;
			cap->UnSubscribe(this);
		}
		return;
	}

	// propagate "caps updated"
	if (cap->getDescription() == CAPA_CAPAS_UPDATED) {
		c = GetConcreteCapability(CAPA_CAPAS_UPDATED);
		*(CValue<bool> *) c->getValue()
			= *(CValue<bool> *) cap->getValue();
		cap->Notify();
		capsupdated = true;
		return;
	}

}

void CCSVOutputFilter::ExecuteCommand( const ICommand *cmd )
{
	switch (cmd->getCmd()) {

	case CMD_INIT:
	{
		DoINITCmd(cmd);

		ICommand *ncmd = new ICommand(CMD_CYCLIC, this, 0);
		struct timespec ts;
		// Set cyclic timer to the query interval.
		ncmd = new ICommand(CMD_CYCLIC, this, 0);
		ts.tv_sec = 5;
		ts.tv_nsec = 0;

		CCapability *c = GetConcreteCapability(
			CAPA_INVERTER_QUERYINTERVAL);
		if (c && c->getValue()->GetType() == IValue::float_type) {
			CValue<float> *v = (CValue<float> *) c->getValue();
			ts.tv_sec = v->Get();
			ts.tv_nsec = ((v->Get() - ts.tv_sec) * 1e9);
		} else {
			cerr
				<< "INFO: The associated inverter does not specify the "
					"queryinterval. Defaulting to 5 seconds";
		}

		Registry::GetMainScheduler()->ScheduleWork(ncmd, ts);
		break;
	}
	case CMD_CYCLIC:
	{
		DoCYCLICmd(cmd);

		// Set cyclic timer to the query interval.
		ICommand *ncmd = new ICommand(CMD_CYCLIC, this, 0);
		struct timespec ts;
		ts.tv_sec = 5;
		ts.tv_nsec = 0;

		CCapability *c = GetConcreteCapability(
			CAPA_INVERTER_QUERYINTERVAL);
		if (c && c->getValue()->GetType() == IValue::float_type) {
			CValue<float> *v = (CValue<float> *) c->getValue();
			ts.tv_sec = v->Get();
			ts.tv_nsec = ((v->Get() - ts.tv_sec) * 1e9);
		} else {
			cerr
				<< "INFO: The associated inverter does not specify the "
					"queryinterval. Defaulting to 5 seconds";
		}

		Registry::GetMainScheduler()->ScheduleWork(ncmd, ts);
	}

	case CMD_ROTATE:
		DoINITCmd(cmd);
		break;

	}
}

void CCSVOutputFilter::DoINITCmd( const ICommand * )
{
	// Do init
	string tmp;
	CConfigHelper cfghlp(configurationpath);
	// Config is already checked (exists, type ok)
	cfghlp.GetConfig("datasource", tmp);

	base = Registry::Instance().GetInverter(tmp);
	if (base) {
		CCapability *cap = base->GetConcreteCapability(
			CAPA_CAPAS_UPDATED);
		assert(cap); // this is required to have....
		if (!cap->CheckSubscription(this))
			cap->Subscribe(this);

		cap = base->GetConcreteCapability(CAPA_CAPAS_REMOVEALL);
		assert(cap);
		if (!cap->CheckSubscription(this))
			cap->Subscribe(this);

		cap = base->GetConcreteCapability(CAPA_INVERTER_DATASTATE);
		assert(cap);
		if (!cap->CheckSubscription(this))
			cap->Subscribe(this);
	} else {
		cerr
			<< "ERROR: Could not find data source to connect. Filter: "
			<< configurationpath << "." << name << endl;
		abort();
	}

	// Try to open the file
	if (file.is_open()) {
		file.close();
	}

	cfghlp.GetConfig("logfile", tmp);
	bool rotate;
	cfghlp.GetConfig("rotate", rotate, false);
	if (rotate) {
		date today(day_clock::local_day());
		//note: the %s will be removed, so +10 is enough.
		char buf[tmp.size() + 10];
		int year = today.year();
		int month = today.month();
		int day = today.day();

		snprintf(buf, sizeof(buf) - 1, "%s%04d-%02d-%02d%s",
			tmp.substr(0, tmp.find("%s")).c_str(), year, day,
			month,
			tmp.substr(tmp.find("%s") + 2, string::npos).c_str());

		tmp = buf;
	}

	// Open the file. We use binary mode, as we want end the line ourself (LF+CR)
	// leaned on RFC4180
	file.open(tmp.c_str(), fstream::out | fstream::in | fstream::app
		| fstream::binary);

	if (file.fail()) {
		cerr << "WARNING: Failed to open file! Logger " << name
			<< " will not work. " << endl;
		file.close();
	}

	headerwritten = false;

	// Set a timer to some seconds after midnight, to enforce rotating
	boost::posix_time::ptime n =
		boost::posix_time::second_clock::local_time();

	date d = n.date() + days(1);
	boost::posix_time::ptime tomorrow(d);
	boost::posix_time::time_duration remaining = tomorrow - n;

	struct timespec ts;
	ts.tv_sec = remaining.hours() * 3600UL + remaining.minutes() * 60
		+ remaining.seconds() + 2;
	ts.tv_nsec = 0;
	ICommand *ncmd = new ICommand(CMD_ROTATE, this, 0);
	Registry::GetMainScheduler()->ScheduleWork(ncmd, ts);

}

void CCSVOutputFilter::DoCYCLICmd( const ICommand * )
{
	/* Check for data validty. */
	if (!datavalid) {
		return;
	}

	/* check if CSV-Header needs to be re-emitted.*/
	if (capsupdated) {
		capsupdated = false;
		if (CMDCyclic_CheckCapas()) {
			headerwritten = false;
		}
	}

	/* check if file is ready */
	if (!file.is_open()) {
		return;
	}

	/* output CSV Header*/
	if (!headerwritten) {
		bool first = true;
		list<string>::const_iterator it;
		for (it = CSVCapas.begin(); it != CSVCapas.end(); it++) {
			if (!first) {
				file << ",";

			}
			first = false;
			file << '\"' << *(it) << '\"';
		}
		// CSV after RFC 4180 requires CR LF
		file << 0x0d << 0x0a;
		headerwritten = true;
	}

	/* finally, output data. */

	// make timestamp
	boost::posix_time::ptime n =
		boost::posix_time::second_clock::local_time();

	file << '\"' << to_simple_string(n) << "\"";

	list<string>::const_iterator it;
	bool first = true;
	for (it = CSVCapas.begin(); it != CSVCapas.end(); it++) {
		file << ",";
#error hier gehts weiter -- wert ausgeben.
	}

}

bool CCSVOutputFilter::CMDCyclic_CheckCapas( void )
{
	// get the array
	CConfigHelper cfghlp(configurationpath);
	bool store_all = false;
	bool ret = false;
	string tmp;

	if (cfghlp.GetConfig("data2log", tmp) && tmp == "all") {
		store_all = true;
	}

	if (store_all)
		return false;

	int i = 0;
	while (!cfghlp.GetConfigArray("data2log", i++, tmp)) {
		if (search_list(tmp)) {
			continue;
		}
		CSVCapas.push_back(tmp);
		ret = true;
	}

	return ret;

}

bool CCSVOutputFilter::search_list( const string id ) const
{
	list<string>::const_iterator it;
	for (it = CSVCapas.begin(); it != CSVCapas.end(); it++) {
		if (*it == id)
			return true;
	}
	return false;
}
