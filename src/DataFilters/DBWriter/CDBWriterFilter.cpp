/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

Copyright (C) 2009-2014 Tobias Frost

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
/** \file CDBWriterFilter.cpp
 *
 *  Created on: Jun 28, 2014
 *      Author: tobi
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "porting.h"
#endif

#ifdef  HAVE_FILTER_DBWRITER

#include "DataFilters/DBWriter/CDBWriterFilter.h"

#include "configuration/CConfigHelper.h"
#include "interfaces/CWorkScheduler.h"

#include "Inverters/Capabilites.h"
#include "patterns/CValue.h"

#include <Inverters/interfaces/ICapaIterator.h>

#include "configuration/ConfigCentral/CConfigCentral.h"

static const char *Description_DBWriter_Intro =
"This logger allows logging to a SQL database. For the moment, please see the "
"examples for configuration hints, as this documentation is incomplete.\n"
"\n"
"As this logger uses the cppdb database abstraction library, it can be "
"configured with many SQL systems. Please note that the required parameters "
"are much depending on the exact SQL system to be used, see the configuration "
"options for default.\n"
"Note that an empty configuration entry usually means that the libraries "
"default is used."
;

static const char *Description_DBWriter_db_type =
"Selects the database type/backend to be used.\n"
"Solarpowerlog uses libcppdb as database abstraction library, therefore all "
"SQL engines supported by this library should work"
"This parameter selects which backend should be configured, as the parameters "
" are dependent on this selection.\n"
"You can see information about "
"the library and its supported backends here: "
"http://cppcms.com/sql/cppdb/backendref.html\n"
"The following db_types are currently understood:\n"
"mysql, postgresql, sqlite3, odbc.\n"
"Another type, \"custom\" can be used for advanced use. In this case "
"db_cppdb_options will be soley passed to the library as connection string.";

static const char *Description_DBWriter_db_host =
"The hostname of the database server.\n"
"Supported by the mysql and postgresql backends. "
"For MySQL, do not specify both and db_hostname db_unixsocket."
;

static const char *Description_DBWriter_db_port =
"The port to connect to the database server.\n"
"Supported by the mysql and postgresql backends. "
"If not set or set to an empty value, the default value for the backend is used."
;


static const char *Description_DBWriter_db_unixsocket =
"For MySQL, this specifies the path to the unix socket to be used for the "
"connection.\n"
"Supported by the mysql backend. "
"Do not specify both db_hostname and db_unixsocket"
;

static const char *Description_DBWriter_db_mode =
"For SQLite3 databases, this parameter set the mode in which the database should "
"be openend. Sensible values are \"create\" and \"readwrite\".\n"
"The difference between \"readwrite\" and \"create\" "
"that if the database does not exist the connection fails.\n"
"Supported for SQLite and "
"defaults for SQLite to create.";

static const char *Description_DBWriter_db_user =
"The username to be used to log on the database server.\n "
"Supported by the mysql and postgresql backends, "
"also mandatory for those.\n";

static const char *Description_DBWriter_db_passwd =
"The password needed to connect to the server.\n"
"Supported by the mysql and postgresql backends, "
"also mandatory for those.\n";

static const char *Description_DBWriter_db_database =
"The database to be used.\n"
"Supported by the sqlite3, mysql and postgresql backends, "
"and mandatory for those.\n"
"For SQLite3 this specify the path and filename of the database file.\n";

static const char *Description_DBWriter_db_cppdb_options =
"Option string which will be appended to the connection string.\n"
"See your backend config http://cppcms.com/sql/cppdb/backendref.html "
"and http://cppcms.com/sql/cppdb/connstr.html for cppdb options.\n"
"Only use if you know what you are doing. There'll be dragons.\n"
"Mandatory for the ODBC backend, as this is the only way to specify "
"its connections settings."
"If you use the custom mode, you also need to specify the driver name"
"as documented in the cppdb documentation.\n";

using namespace libconfig;

// small config checker helper
static void _missing_req_parameter(ILogger &logger, std::string type, std::string parameter)
{
    LOGERROR(logger, "Database backend " << type << " requires parameter " << parameter);
}

static void _wrong_parameter(ILogger &logger, std::string type, std::string parameter)
{
    LOGERROR(logger, "Database backend " << type << " does not support parameter " << parameter);
}



CDBWriterFilter::CDBWriterFilter( const std::string & name,
	const std::string & configurationpath ) :
	IDataFilter(name, configurationpath)
{
    _datavalid = false;

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

	// Also we do not update capas on our own..
    c = IInverterBase::GetConcreteCapability(CAPA_CAPAS_UPDATED);
    CapabilityMap.erase(CAPA_CAPAS_UPDATED);
    delete c;


    Registry::GetMainScheduler()->RegisterBroadcasts(this);
}

CDBWriterFilter::~CDBWriterFilter()
{
    std::vector<CDBWriterHelper*>::iterator it;
    for (it = _dbwriterhelpers.begin(); it != _dbwriterhelpers.end(); it++) {
        delete *it;
    }
}

bool CDBWriterFilter::CheckConfig()
{
    auto_ptr<CConfigCentral> cc(getConfigCentralObject(NULL));

    bool fail = !cc->CheckConfig(logger,configurationpath);

    // checking logical combinations for each backend.
    // mysql: required: user, password, database, host or socket, not port and socket
    if (_cfg_cache_db_type == "mysql") {

        if (_cfg_cache_db_user.empty()) {
            _missing_req_parameter(logger, _cfg_cache_db_type, "db_user");
            fail = true;
        }
        if (_cfg_cache_db_passwd.empty()) {
            _missing_req_parameter(logger, _cfg_cache_db_type, "db_password");
            fail = true;
        }
        if (_cfg_cache_db_database.empty()) {
            _missing_req_parameter(logger, _cfg_cache_db_type, "db_database");
            fail = true;
        }

        if (!_cfg_cache_db_mode.empty()) {
            _wrong_parameter(logger, _cfg_cache_db_type, "db_mode");
            fail = true;
        }

        if (_cfg_cache_db_host.empty() && _cfg_cache_db_unixsocket.empty())
        {
            LOGERROR(logger, "Either db_host or db_unixsocket must be specified for MySQL");
            fail = true;
        }

        if (!_cfg_cache_db_port.empty() && ! _cfg_cache_db_unixsocket.empty()) {
             fail = true;
             LOGERROR(logger, "both db_port and db_unixsocket cannot be used at the same time for MySQL.");
        }

   }
    else if (_cfg_cache_db_type == "postgresql") {

        if (_cfg_cache_db_host.empty()) {
             _missing_req_parameter(logger, _cfg_cache_db_type, "db_host");
             fail = true;
        }

        if (_cfg_cache_db_user.empty()) {
            _missing_req_parameter(logger, _cfg_cache_db_type, "db_user");
            fail = true;
        }

        if (_cfg_cache_db_passwd.empty()) {
            _missing_req_parameter(logger, _cfg_cache_db_type, "db_password");
            fail = true;
        }
        if (_cfg_cache_db_database.empty()) {
            _missing_req_parameter(logger, _cfg_cache_db_type, "db_database");
            fail = true;
        }

        if (!_cfg_cache_db_mode.empty()) {
            _wrong_parameter(logger, _cfg_cache_db_type, "db_mode");
            fail = true;
        }

        if (!_cfg_cache_db_unixsocket.empty()) {
            _wrong_parameter(logger, _cfg_cache_db_type, "db_unixsocket");
            fail = true;
        }
    }
    else if (_cfg_cache_db_type == "sqlite3") {
        // mode is optional, defaults to create when not given
        // database is required
        // all other not supported.

        if (!_cfg_cache_db_host.empty()) {
            _wrong_parameter(logger, _cfg_cache_db_type, "db_host");
            fail = true;
        }

        if (!_cfg_cache_db_port.empty()) {
            _wrong_parameter(logger, _cfg_cache_db_type, "db_port");
            fail = true;
        }

        if (!_cfg_cache_db_user.empty()) {
            _wrong_parameter(logger, _cfg_cache_db_type, "db_user");
            fail = true;
        }

        if (!_cfg_cache_db_passwd.empty()) {
            _wrong_parameter(logger, _cfg_cache_db_type, "db_password");
            fail = true;
        }

        if (_cfg_cache_db_database.empty()) {
            _missing_req_parameter(logger, _cfg_cache_db_type, "db_database");
            fail = true;
        }

        if (!_cfg_cache_db_unixsocket.empty()) {
            _wrong_parameter(logger, _cfg_cache_db_type, "db_unixsocket");
            fail = true;
        }
    }
    else if (_cfg_cache_db_type == "_cfg_cache_odbc" || _cfg_cache_db_type == "custom") {
        if (!_cfg_cache_db_host.empty() || !_cfg_cache_db_user.empty()
            || !_cfg_cache_db_database.empty() || !_cfg_cache_db_mode.empty()
            || !_cfg_cache_db_port.empty() || !_cfg_cache_db_unixsocket.empty()
            || !_cfg_cache_db_passwd.empty()
            || _cfg_cache_db_cppdb_options.empty()) {
            LOGERROR(logger,
                _cfg_cache_db_type << " please use db_cppdb_options to specify the connection string.");
            fail = true;
        }
    } else {
        LOGERROR(logger, "unknown database backend type " << _cfg_cache_db_type);
        fail =  true;
    }

    if (!base) {
        LOGERROR(logger, "Cannot find datassource with the name " << _datasource);
        fail = true;
    }

    if (fail) return false;

    bool add_semicolon = false;
    std::string dbgstring;

    if (_cfg_cache_db_type != "custom") {
        _connectionstring = _cfg_cache_db_type + ":";
    };

    // check: mysql, postgresql, (sqlite3) ok
    if (!_cfg_cache_db_host.empty()) {
        _connectionstring += "host=\'" + _cfg_cache_db_host + "\'";
        add_semicolon = true;
    }

    // check: mysql, postgresql, (sqlite3) ok
    if (!_cfg_cache_db_user.empty()) {
        if (add_semicolon) _connectionstring += ";";
        _connectionstring += "user=\'" + _cfg_cache_db_user + "\'";
        add_semicolon = true;
    }

    // check: mysql, postgresql, (sqlite3) ok
    if (!_cfg_cache_db_database.empty()) {
        std::string db;
        if (_cfg_cache_db_type == "sqlite3") db = "db=";
        if (_cfg_cache_db_type == "mysql") db = "database=";
        if (_cfg_cache_db_type == "postgresql") db = "dbname=";
        if (add_semicolon) _connectionstring += ";";
        _connectionstring += db + _cfg_cache_db_database + "\'";
        add_semicolon = true;
    }

    // check: mysql, postgresql, (sqlite3) ok
    if (!_cfg_cache_db_port.empty()) {
        if (add_semicolon) _connectionstring += ";";
        _connectionstring += "port=\'" + _cfg_cache_db_port + "\'";
        add_semicolon = true;
    }

    // check: mysql, (postgresql), (sqlite3) ok
    if (!_cfg_cache_db_unixsocket.empty()) {
        if (add_semicolon) _connectionstring += ";";
        _connectionstring += "unix_socket=\'" + _cfg_cache_db_unixsocket + "\'";
        add_semicolon = true;
    }

    dbgstring = _connectionstring;
    // check: mysql, (postgresql), (sqlite3) ok
    if (!_cfg_cache_db_passwd.empty()) {
        if (add_semicolon) _connectionstring += ";";
        dbgstring += "password=\'***\'";
        _connectionstring += "password=\'" + _cfg_cache_db_passwd + "\'";
        add_semicolon = true;
    }

    // check: (mysql), (postgresql), sqlite3 ok
    if (!_cfg_cache_db_mode.empty()) {
         if (add_semicolon) _connectionstring += ";";
         _connectionstring += "mode=\'" + _cfg_cache_db_mode + "\'";
         add_semicolon = true;
     }

    // check: mysql, (postgresql), (sqlite3) ok
    if (!_cfg_cache_db_cppdb_options.empty()) {
        std::string tmp;
        if (add_semicolon) tmp = ";";
        tmp += _cfg_cache_db_cppdb_options;
        _connectionstring += tmp;
        dbgstring += tmp;
        add_semicolon = true;
    }

    LOGTRACE(logger, "DBWriter: connectionstring for CppDB is: " << dbgstring);

    CConfigHelper hlp(configurationpath);

    // Checking config for the tables aggregate
    int i=0;
    CDBWriterHelper *dbwh = NULL;

    do {
        std::string table, mode, createmode;
        bool logchangedonly;
        float logevery;
        logchangedonly = false;
        logevery = 0;
        dbwh = NULL;

        hlp = CConfigHelper(configurationpath, "db_jobs", i);

        if (!hlp.isExisting()) {
            if (i == 0) {
                LOGFATAL(logger, "db_jobs missing in configuration.");
            }
            break;
        }

        if (!hlp.CheckAndGetConfig("db_table", Setting::TypeString, table)) {
            LOGFATAL(logger, "db_table missing in entry number "
                << i << " of the tables list.");
            fail = true;
            i++; continue;
        }

        LOGINFO(logger, "Parsing configuration for table " << table);

        if (!hlp.CheckAndGetConfig("db_create_table", Setting::TypeString, createmode, true)) {
            LOGFATAL(logger, "db_create_table is of wrong type for table " << table);
            fail = true;
        } else if (!createmode.empty()) {
            if (createmode == "YES") {
                LOGWARN(logger, "db_create_table is YES -- will create this table later: " << table);
            } else
            if (createmode == "YES-WIPE-MY-DATA") {
                LOGWARN(logger, "db_create_table is YES-WIPE-MY-DATA -- will *DESTROY* and create this table later: " << table);
            } else if (createmode == "print-sql-statement") {
                LOGWARN(logger, "Will print creation statement for " << table);
            }
            else if (createmode == "no") {
                createmode = "";
            }
            else {
                LOGWARN(logger, "db_create_table is neither YES, YES-WIPE-MY-DATA, print-sql-statement or no. Ignored");
                createmode = "";
            }
        }

        if (!hlp.GetConfig("db_operation_mode", mode)) {
            fail = true;
        } else {
            if (!(mode == "continuous" || mode == "single"
                || mode == "cumulative")) {
                LOGFATAL(logger,
                    "Setting operation_mode for table " << table <<
                    " must be either continuous, single or cumulative, not " << mode);
                fail = true;
            } else {
                LOGDEBUG(logger,
                    "Setting operation_mode for table " << table <<
                    " is " << mode);
            }
        }

        if (!hlp.CheckConfig("db_logchangedonly", Setting::TypeBoolean, true)) {
            fail = true;
        } else {
            hlp.GetConfig("db_logchangedonly", logchangedonly, false);
        }

        bool allow_sparse;
        if (!hlp.CheckConfig("db_allowsparse", Setting::TypeBoolean, true)) {
            fail = true;
        } else {
            hlp.GetConfig("db_allowsparse", allow_sparse, false);
        }

#warning FIXME  logevery is optional and should be derived from the inverter if not specified.
#warning THIS IS A RELEASE GOAL FOR 0.26
        if (!hlp.CheckConfig("db_logevery", Setting::TypeFloat)) {
            fail = true;
        } else {
            hlp.GetConfig("db_logevery", logevery);
        }

        hlp = CConfigHelper(hlp.GetCfgPath(),"db_layout");
        if (!hlp.isExisting()) {
            LOGFATAL(logger, "Configuration error: Entry db_layout missing for table " << table);
            i++; continue;
        }

        // note, on misconfiguration (no valid data source!) base *can* be NULL here!
        if (base) {
            dbwh = new CDBWriterHelper(base, logger, table, mode, createmode,
                logchangedonly, logevery, allow_sparse);
        } else {
            continue;
        }

        int j=0;
        bool k = true;
        // Iterate over all datasets to be logged.
        do {
            std::string capa,column;
            capa.clear(); column.clear();
            CConfigHelper hlp2(hlp.GetCfgPath(),j);
            k = hlp2.isExisting();

            if (!k) break;

            if (!(hlp.GetConfigArray(j, 0, capa) && hlp.GetConfigArray(j, 1, column))) {
                LOGFATAL(logger, "Config Error in entry " << j
                    << " of " << table << "::" << "db_layout");
                fail = true;
                j++; continue;
            }
            LOGDEBUG(logger, "j=" << j << " capa=" << capa <<
                " column=" << column);

            if (capa.empty()) {
                LOGFATAL(logger, "Capability in entry " << j
                    << " of " << table << "::" << "db_layout must not be empty");
               fail = true;
               j++; continue;
            }
            if (column.empty()) {
                LOGFATAL(logger, "Column in entry " << j
                    << " of " << table << "::" << "db_layout must not be empty");
               fail = true;
               j++; continue;
            }

            if (!dbwh->AddDataToLog(capa,column)) {fail = true;}
            j++;
        } while(k);

        if (!j) {
            LOGFATAL(logger, "no layout for table " << table);
            fail = true;
        }
        _dbwriterhelpers.push_back(dbwh);

        i++;
    }
    while(true);

    return !fail;
}

void CDBWriterFilter::Update( const IObserverSubject *subject )
{
    // TODO check if we can completely empty this member function
    // NOTE: The Observer-Pattern-Handling is delegated to the helper claa
    // CDBWriterHelper

    assert(subject);
    CCapability *cap = (CCapability *)subject;

    // Datastate changed.
    if (cap->getDescription() == CAPA_INVERTER_DATASTATE) {
        _datavalid = ((CValue<bool> *)cap->getValue())->Get();
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
}

void CDBWriterFilter::ScheduleCyclicWork(void)
{
    ICommand *ncmd = NULL;
    struct timespec ts;
    std::vector<CDBWriterHelper*>::iterator it;

    for (it = _dbwriterhelpers.begin(); it != _dbwriterhelpers.end(); it++) {
        ncmd = new ICommand(CMD_CYCLIC, this);
        ncmd->addData("DB_WORK", *it);
        float logevery = (*it)->getLogevery();
        ts.tv_sec = logevery;
        ts.tv_nsec = (logevery - ts.tv_sec) * 1e9;
        Registry::GetMainScheduler()->ScheduleWork(ncmd, ts);
    }
}

void CDBWriterFilter::ExecuteCommand( const ICommand *cmd )
{

//    LOGERROR(logger, __PRETTY_FUNCTION__ << " Warning not finished / implemented");

	switch (cmd->getCmd()) {

        case CMD_INIT: {
            DoINITCmd(cmd);
            ScheduleCyclicWork();
        }
        break;

        case CMD_CYCLIC: {
            DoCYCLICmd(cmd);

            CDBWriterHelper* helper = NULL;

            try {
                helper = boost::any_cast<CDBWriterHelper*>(
                    cmd->findData("DB_WORK"));

            } catch (...) {
                LOGDEBUG(logger,
                    __PRETTY_FUNCTION__ << "Unexpected exception while getting "
                        "object out of cmd");
            }

            assert(helper);

            LOGTRACE(logger, "now handling: " << helper->GetTable());

            if (!_sqlsession.is_open()) {
                LOGDEBUG(logger, "Trying to open database.");
                try {
                    _sqlsession.open(_connectionstring);
                } catch (std::exception const &e) {
                    LOGWARN(logger, "Exception while opening database: "
                        << e.what());
                }
            }

            if (_sqlsession.is_open()) {
                try {
                    // The helper will pass exceptions by the cppdb library
                    // to give access to the error message.
                    helper->ExecuteQuery(_sqlsession);
                } catch (const std::exception &e) {
                    LOGWARN(logger, "Exception while handling database access:" << e.what() );
                    LOGWARN(logger, "Closing DB connection to try error recovery.");
                    _sqlsession.close();
                }
            }

            ICommand *ncmd = new ICommand(CMD_CYCLIC, this);
            struct timespec ts;
            ncmd->addData("DB_WORK", helper);
            float logevery = helper->getLogevery();
            ts.tv_sec = logevery;
            ts.tv_nsec = (logevery - ts.tv_sec) * 1e9;
            Registry::GetMainScheduler()->ScheduleWork(ncmd, ts);
        }
        break;

        case CMD_BRC_SHUTDOWN:
            // shutdown requested, we will terminate soon.
            try {
                if (_sqlsession.is_open()) _sqlsession.close();
            } catch (std::exception const &e) {
                LOGWARN(logger, " Exception while closing sqlsession. "
                    << e.what());

            }
        break;
    }
}

void CDBWriterFilter::DoINITCmd( const ICommand * )
{

    assert(base);
    CCapability *cap;
    cap = base -> GetConcreteCapability(CAPA_CAPAS_UPDATED);
    assert(cap);
    cap->Subscribe(this);

    cap = base->GetConcreteCapability(CAPA_CAPAS_REMOVEALL);
    assert(cap);
    cap->Subscribe(this);

    cap = base->GetConcreteCapability(CAPA_INVERTER_DATASTATE);
    assert(cap);
    cap->Subscribe(this);

    return;
}

void CDBWriterFilter::DoCYCLICmd( const ICommand * )
{
 //   LOGERROR(logger, __PRETTY_FUNCTION__ << " not implemented");
    return;

}

CConfigCentral* CDBWriterFilter::getConfigCentralObject(CConfigCentral* parent) {

    if (!parent) parent = new CConfigCentral;

    (*parent)(NULL, Description_DBWriter_Intro);

    parent = IDataFilter::getConfigCentralObject(parent);

    (*parent)
        ("db_type", Description_DBWriter_db_type, _cfg_cache_db_type)
        ("db_host", Description_DBWriter_db_host, _cfg_cache_db_host,
            std::string(""))
        ("db_port", Description_DBWriter_db_port, _cfg_cache_db_port,
            std::string(""))
        ("db_unixsocket", Description_DBWriter_db_unixsocket,
            _cfg_cache_db_unixsocket, std::string(""))
        ("db_mode", Description_DBWriter_db_mode,
             _cfg_cache_db_mode, std::string(""))
        ("db_user", Description_DBWriter_db_user,
            _cfg_cache_db_user, std::string(""))
        ("db_password", Description_DBWriter_db_passwd,
            _cfg_cache_db_passwd, std::string(""))
        ("db_database", Description_DBWriter_db_database,
            _cfg_cache_db_database, std::string(""))
        ("db_cppdb_options", Description_DBWriter_db_cppdb_options,
             _cfg_cache_db_cppdb_options, std::string(""))
        ;

    parent->SetExample("type", std::string(FILTER_DBWRITER), false);

    return parent;
}


#endif
