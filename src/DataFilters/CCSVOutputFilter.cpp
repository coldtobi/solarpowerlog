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

/** \file CCSVOutputFilter.cpp
 *
 *  Created on: Jun 29, 2009
 *      Author: tobi
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "porting.h"
#endif

#ifdef  HAVE_FILTER_CSVDUMP

#include <assert.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <memory>

#include <time.h>
#include <stdio.h>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include "configuration/Registry.h"
#include "configuration/CConfigHelper.h"
#include "configuration/ConfigCentral/CConfigCentral.h"
#include "interfaces/CWorkScheduler.h"

#include "Inverters/Capabilites.h"
#include "patterns/CValue.h"

#include "CCSVOutputFilter.h"

#include "Inverters/interfaces/ICapaIterator.h"

#define DESCRIPTION_CSVWRITER_INTRO \
"Logger CSVWriter\n" \
"The CSV Data Logger takes some or all data and writes it to a CSV " \
"(comma-separated-values) file as specified in the RFC 4180.\n" \
"The data to be logged can be selected, either by specifying the identifiers " \
"or just logging \"all\" data. " \
"However, \"log all\" will causes that the number of columns can change " \
"during logging, violating the RFC. \n" \
"When some data is unavailable, empty " \
"values will be logged instead. \n" \
"Also, a (ISO 8601)-like timestamp " \
"will be inserted as the first column. \n" \
"To get a CSVWriter, \"type\" below needs to " \
"be set to " \
FILTER_CSVWRITER \
" (as indicated below.)"

#define DESCRIPTION_CSVWRITER_FILENAME \
"Defines the target file for this CSV file.\n This setting is dependent on " \
"other parameters:\n" \
"If \"rotate\" is enabled, the logger will start a new logfile at midnight. " \
"To avoid overwriting the old logfile it will add the current date to the " \
"filename, using by default the ISO 8601 format YYYY-MM-DD. " \
"If the parameter contains a \"%s\", the timestamp will be inserted at this " \
"position; if not specified, it will be appended at the end.\n" \
"For example\n" \
"logfile=\"Inverter_1_%s.csv\"\n" \
"will create a logfile like \"Inverter_1_2009-07-04.csv\"\n" \
"To set the format of the timestamp see the option format_timestamp."

#define DESCRIPTION_CSVWRITER_ROTATE \
"Rotate: Create a new logfile at midnight."

#define DESCRIPTION_CSVWRITER_COMPACTCSV \
"Tries to keep the files compact by suppressing logs when all data is " \
"unchanged.\n" \
"In other words: This option will eliminate lines in the CSV file which are " \
"identical to the previous line, if everything to be logged (except date/time) " \
"has not changed."

#define DESCRIPTION_CSVWRITER_FLUSHFILEBUFFER \
"If true, writes to the CSV file are immediate, if false, use the cache provided " \
"by the operating system.\n" \
"If you are \"just logging\" this might be fine to set to false, if you do " \
"some kind of real-time data processing, use false, as it might " \
"take some times for the data to enter the disk. " \
"One use of this option disabled is if you log to flash memory or if " \
"you want to avoid spinning up disks. " \
"Note: Solarpowerlog only hints the operating system to flush the file " \
"buffers. The operating system or hardware (harddisk) still might use some " \
"caching.\n" \
"Note: Up to solarpowerlog 0.21 this setting was by default set to true."

#define DESCRIPTION_CSVWRITER_FORMATTIMESTAMP \
"You can customize the timestamp format with this setting. As solarpowerlog is " \
"using boost, please refer to this list for all valid options: " \
"http://www.boost.org/doc/libs/1_57_0/doc/html/date_time/date_time_io.html#date_time.format_flags\n" \
"The default set the date in the ISO 8601 format, e.g.: 2009-12-20 13:34:56."

#define DESCRIPTION_CSVWRITER_DATA2LOG \
"This parameter specifies tha data to be logged. There are two modes: " \
"Logging everything by specifing \"all\" here or selective logging by specifing " \
" excplictly the capabilities to be logged in an array." \
"Hint: To retrieve all the capabilites names supported, first use \"all\" and then" \
"examine the created CSV file to select the ones you really want.\n" \
"This setting is mandatory."

#define EXAMPLE_CSVWRITER_DATA2LOG \
"data2log= \"all\";\n" \
"data2log= [ \n" \
"\t\"AC grid feeding current (A)\",\n" \
"\t\"AC grid voltage (V)\",\n" \
"\t\"Current Grid Feeding Power\",\n" \
"\t\"DC current in (A)\",\n" \
"\t\"DC voltage in (V)\",\n" \
"\t\"Data Query Interval\",\n" \
"\t\"Data Validity\",\n" \
"\t\"Energy produced cumulated all time (kWh)\",\n" \
"\t\"Energy produced this month (kWh)\",\n" \
"\t\"Energy produced this year (kWh)\",\n" \
"\t\"Energy produced today (kWh)\",\n" \
"\t\"Inverter Overall Status\",\n" \
"\t\"Inverter Power On Hours\",\n" \
"\t\"Inverter Temperature (C)\",\n" \
"\t\"Net frequency (Hz)\"\n" \
"]"


using namespace std;
using namespace libconfig;
using namespace boost::gregorian;

CCSVOutputFilter::CCSVOutputFilter( const string & name,
	const string & configurationpath ) :
	IDataFilter(name, configurationpath), datavalid(false), capsupdated(false)
{
	headerwritten = false;
	_cfg_cache_data2log_all = false;
	_cache_found_all_capas = false;

	// Schedule the initialization and subscriptions later...
	ICommand *cmd = new ICommand(CMD_INIT, this);
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

	// However, to help following plugins, we will publish some data here:
	// (this enables other plugins to use our files as data source)
    c = new CCapability(CAPA_CSVDUMPER_FILENAME,
        new CValue<CAPA_CSVDUMPER_FILENAME_TYPE>, this);
    AddCapability(c);

	// A comma-seperated list of parameters which are currently logged.
	// note: This list might grow over time, so when parsing the CSV File,
	// be prepared that there might be not all given from the beginning of the
	// file
    c = new CCapability(CAPA_CSVDUMPER_LOGGEDCAPABILITES,
        new CValue<CAPA_CSVDUMPER_LOGGEDCAPABILITES_TYPE>, this);
    AddCapability(c);

    Registry::GetMainScheduler()->RegisterBroadcasts(this);
}

CCSVOutputFilter::~CCSVOutputFilter()
{
	if (file.is_open())
		file.close();
}

bool CCSVOutputFilter::CheckConfig()
{
    std::auto_ptr<CConfigCentral> cfg(getConfigCentralObject(NULL));
	bool fail = !cfg->CheckConfig(logger,configurationpath);

	// check datasource: The type and existance is already checked, but
	// we need to see if it really exists -- this is done already in the
	// baseclass' constructor.
    if (!fail && !base) {
        LOGERROR(logger, "Cannot find datassource with the name "
            << _datasource);
        fail = true;
    }

	// Datatolog -- can be "all" or an array.
    // ConfigCentral cannot check this complexity...
    CConfigHelper hlp(configurationpath);
    bool data2log_fail = false;
    if (hlp.CheckConfig("data2log", Setting::TypeString, false, false)) {
        std::string setting;
        hlp.GetConfig("data2log", setting);
        if (setting == "all") {
            _cfg_cache_data2log_all = true;
        } else {
            data2log_fail = true;
        }
    } else if (!hlp.CheckConfig("data2log", Setting::TypeArray)) {
        data2log_fail = true;
    }
    if (data2log_fail) {
        LOGERROR(logger, "Configuration Error: data2log must be "
            "\"all\" or of the type \"Array\".");
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
		this->datavalid = ((CValue<bool> *) cap->getValue())->Get();
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
		c = IInverterBase::GetConcreteCapability(CAPA_CAPAS_UPDATED);
		*(CValue<bool> *) c->getValue()
			= *(CValue<bool> *) cap->getValue();
		c->Notify();
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

		ICommand *ncmd = new ICommand(CMD_CYCLIC, this);
		struct timespec ts;
		// Set cyclic timer to the query interval.
		ts.tv_sec = 5;
		ts.tv_nsec = 0;

		CCapability *c = GetConcreteCapability(
			CAPA_INVERTER_QUERYINTERVAL);
		if (c && CValue<float>::IsType(c->getValue())) {
			CValue<float> *v = (CValue<float> *) c->getValue();
			ts.tv_sec = v->Get();
			ts.tv_nsec = ((v->Get() - ts.tv_sec) * 1e9);
		} else {
			LOGINFO(logger,
				"INFO: The associated inverter does not specify the "
				"queryinterval. Defaulting to 5 seconds");
		}

		Registry::GetMainScheduler()->ScheduleWork(ncmd, ts);

	}
		break;

	case CMD_CYCLIC:
	{
		DoCYCLICmd(cmd);

		// Set cyclic timer to the query interval.
		ICommand *ncmd = new ICommand(CMD_CYCLIC, this);
		struct timespec ts;
		ts.tv_sec = 5;
		ts.tv_nsec = 0;

		CCapability *c = GetConcreteCapability(
			CAPA_INVERTER_QUERYINTERVAL);
		if (c && CValue<float>::IsType(c->getValue())) {
			CValue<float> *v = (CValue<float> *) c->getValue();
			ts.tv_sec = v->Get();
			ts.tv_nsec = ((v->Get() - ts.tv_sec) * 1e9);
		}

		Registry::GetMainScheduler()->ScheduleWork(ncmd, ts);

	}
		break;

	case CMD_ROTATE:
		DoINITCmd(cmd);
		break;


	case CMD_BRC_SHUTDOWN:
            // shutdown requested, we will terminate soon.
            // So flush filesystem buffers.
            if (file.is_open()) {
                file.flush();
            }
        break;
    }
}

void CCSVOutputFilter::DoINITCmd( const ICommand * )
{
	std::string filename;
	CCapability *cap;

    assert(base);
    cap = base->GetConcreteCapability(CAPA_CAPAS_UPDATED);
    assert(cap); // this cap is required to have.
    if (!cap->CheckSubscription(this)) cap->Subscribe(this);

    cap = base->GetConcreteCapability(CAPA_CAPAS_REMOVEALL);
    assert(cap);
    if (!cap->CheckSubscription(this)) cap->Subscribe(this);

    cap = base->GetConcreteCapability(CAPA_INVERTER_DATASTATE);
    assert(cap);
    if (!cap->CheckSubscription(this)) cap->Subscribe(this);

	// Try to open the file
	if (file.is_open()) {
		file.close();
	}

	if (_cfg_cache_rotate) {
		date today(day_clock::local_day());
		//note: the %s will be removed, so +10 is enough.
		char buf[_cfg_cache_filename.size() + 10];
		int year = today.year();
		int month = today.month();
		int day = today.day();

		snprintf(buf, sizeof(buf) - 1, "%s%04d-%02d-%02d%s",
		    _cfg_cache_filename.substr(0, _cfg_cache_filename.find("%s")).c_str(), year, month,
			day,
			_cfg_cache_filename.substr(_cfg_cache_filename.find("%s") + 2, string::npos).c_str());

		filename = buf;
	}

	// Open the file. We use binary mode, as we want end the line ourself (LF+CR)
	// leaned on RFC4180
	file.clear(); // clear errorstates of fstream.
	file.open(filename.c_str(), fstream::out | fstream::in | fstream::app
		| fstream::binary);

#ifdef HAVE_WIN32_API
	if (file.fail()) {
		file.clear();
		file.open(tmp.c_str(), fstream::out | fstream::app | fstream::binary);
	}
#endif
	if (file.fail()) {
		LOGWARN(logger,"Failed to open file " << filename <<". Logger " << name
			<< " will not work. " );
		file.close();
		filename = "";
	}

	// Update the filename. If empty, the subsequent plugin knows that there
	// was a problem.
    cap = this->GetConcreteCapability(CAPA_CSVDUMPER_FILENAME);
    ((CValue<std::string> *) cap->getValue())->Set(filename);
	cap->Notify();

	// a new file needs a new header
	headerwritten = false;
    // Technically seen, the file is now empty and the we-are-logging-this
	// capability CAPA_CSVDUMPER_LOGGEDCAPABILITES is wrong.
	// But in some seconds, we probably write the same as the last day,
	// so we set the changes later.
	// (In other words: I told you, that the file needs not to contain all the
    // datas we claim to be there here...)

	// Set a timer to some seconds after midnight, to enforce rotating with correct date
	boost::posix_time::ptime n =
		boost::posix_time::second_clock::local_time();

	date d = n.date() + days(1);
	boost::posix_time::ptime tomorrow(d);
	boost::posix_time::time_duration remaining = tomorrow - n;

	struct timespec ts;
	ts.tv_sec = remaining.hours() * 3600UL + remaining.minutes() * 60
		+ remaining.seconds() + 10;
	ts.tv_nsec = 0;
	ICommand *ncmd = new ICommand(CMD_ROTATE, this);
	Registry::GetMainScheduler()->ScheduleWork(ncmd, ts);
}

void CCSVOutputFilter::DoCYCLICmd( const ICommand * )
{
//	bool compact_file, flush_after_write;
//	std::string format;

//	CConfigHelper cfg(configurationpath);
//	cfg.GetConfig("format_timestamp", format, std::string("%Y-%m-%d %T"));
//	cfg.GetConfig("compact_csv", compact_file, false);
//	cfg.GetConfig("flush_file_buffer_immediatly", flush_after_write, false);

	/* Check for data validity. */
	if (!datavalid) {
		return;
	}

	/* check if CSV-Header needs to be re-emitted.*/
	if (capsupdated || !headerwritten) {
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
	    std::stringstream ss_header;
		last_line.clear();
		bool first = true;
		list<string>::const_iterator it;
		for (it = CSVCapas.begin(); it != CSVCapas.end(); it++) {
			if (!first) {
			    ss_header << ",";
			} else {
			    ss_header << "Timestamp,";
			}
			first = false;
			ss_header << *(it);
		}
		// CSV after RFC 4180 requires CR LF
		file << ss_header.str() << (char) 0x0d << (char) 0x0a;
		CCapability *cap = GetConcreteCapability(CAPA_CSVDUMPER_LOGGEDCAPABILITES);
		assert(cap);
		((CValue<std::string> *)cap->getValue())->Set(ss_header.str());
		cap->Notify();
		headerwritten = true;
	}

	/* finally, output data. */

	// make timestamp
	boost::posix_time::ptime n =
		boost::posix_time::second_clock::local_time();

	// assign facet only to a temporary stringstream.
	// this avoids having a persistent object.
	/// time_facet for the formating of the string

	// note: do not delete the facet. This is done by the locale.
	// See: http://rhubbarb.wordpress.com/2009/10/17/boost-datetime-locales-and-facets/
	// (the locale will delete the object, so there is no leak. If we would
	// delete, this crashes.)

    std::stringstream ss;
	list<string>::const_iterator it;
	CCapability *c;
	IValue *v;
	for (it = CSVCapas.begin(); it != CSVCapas.end(); it++) {
		ss << ",";
		c = base->GetConcreteCapability(*it);
		if (c) {
			v = c->getValue();
			string tmp = (string) *v;

			if (string::npos != tmp.find('"')) {
				string t2 = tmp;
				size_t t;
				while (string::npos != (t = t2.find('"'))) {
					tmp = t2.substr(0, t);
					tmp += '"';
					t2 = t2.substr(t, string::npos);
				}
				tmp += t2;
			}

			if (string::npos != tmp.find(',') || string::npos
				!= tmp.find("\x0d\x0a")) {
				ss << '"' << tmp << '"';
			} else {
				ss << tmp;
			}

		} else {
			// file << ' ';
		}
	}

	if ( !_cfg_cache_compactcsv ||  ss.str() != last_line) {
        last_line = ss.str();
        std::stringstream timestamp;
        boost::posix_time::time_facet *facet =
            new boost::posix_time::time_facet(_cfg_cache_formattimestap.c_str());
        timestamp.imbue(std::locale(ss.getloc(), facet));
        timestamp << n;
		file << timestamp.str() << ss.str() << (char) 0x0d << (char) 0x0a;
		if (_cfg_cache_flushfb)
			file << flush;
	}
}

bool CCSVOutputFilter::CMDCyclic_CheckCapas( void )
{
	bool ret = false;

	if (!_cfg_cache_data2log_all && !_cache_found_all_capas ) {
	    CConfigHelper cfghlp(configurationpath);
	    string tmp;
		int i = 0;
		while (cfghlp.GetConfigArray("data2log", i++, tmp)) {
			if (search_list(tmp)) {
				continue;
			}
			CSVCapas.push_back(tmp);
			ret = true;
		}
		_cache_found_all_capas = true;
		return ret;
	}

	/** check for new capabilites not already in the list.
	 * Add the new ones to the end of the list. */
	auto_ptr<ICapaIterator> it(base->GetCapaNewIterator());
	pair<string, CCapability*> pair;
	while (it->HasNext()) {
		pair = it->GetNext();
		if (search_list(pair.first)) {
			continue;
		}
		CSVCapas.push_back(pair.first);
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


CConfigCentral* CCSVOutputFilter::getConfigCentralObject(CConfigCentral *parent)
{

    if (!parent) parent = new CConfigCentral;

    (*parent)
    (NULL, DESCRIPTION_CSVWRITER_INTRO);

    parent = IDataFilter::getConfigCentralObject(parent);

    (*parent)
    ("logfile", DESCRIPTION_CSVWRITER_FILENAME, _cfg_cache_filename)
    ("rotate", DESCRIPTION_CSVWRITER_ROTATE, _cfg_cache_rotate, false)
    ("compact_csv", DESCRIPTION_CSVWRITER_COMPACTCSV, _cfg_cache_compactcsv, false)

    ("flush_file_buffer_immediatly", DESCRIPTION_CSVWRITER_FLUSHFILEBUFFER,
            _cfg_cache_flushfb, false)
    ("format_timestamp", DESCRIPTION_CSVWRITER_FORMATTIMESTAMP,
            _cfg_cache_formattimestap, std::string("%Y-%m-%d %T"))
    ("data2log", DESCRIPTION_CSVWRITER_DATA2LOG, EXAMPLE_CSVWRITER_DATA2LOG)
    ;

    parent->SetExample("type", std::string(FILTER_CSVWRITER), false);

    return parent;
}

#endif
