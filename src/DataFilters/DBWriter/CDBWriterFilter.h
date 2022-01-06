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

/** \file CDBWriterFilter.h
 * \date Jul 28, 2014
 * \author Tobias Frost
 *
 * TODO Dokumentation is missing!!!!!
 */

#ifndef CDBWriterFilter_H_
#define CDBWriterFilter_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_FILTER_DBWRITER

#include "DataFilters/interfaces/IDataFilter.h"

#include "CDBWriterHelper.h"

#include <cppdb/frontend.h>

/** This class implements a logger to write the data to a CSV File
 *
 * Please see \ref DLCSV_Description for configuration, etc.
 */

class CDBWriterFilter : public IDataFilter
{

protected:
	friend class IDataFilterFactory;
	CDBWriterFilter( const std::string &name, const std::string & configurationpath );

public:
	virtual ~CDBWriterFilter();

	virtual bool CheckConfig();

	virtual void  Update( const IObserverSubject *subject );

	virtual void ExecuteCommand( const ICommand *cmd );

private:

	/** Do the initialization of the module
	*/
	void DoINITCmd (const ICommand *);

	/** does the actual work:
	 */
	void DoCYCLICmd(const ICommand *);

	void ScheduleCyclicWork(void);

    virtual CConfigCentral* getConfigCentralObject(CConfigCentral* parent);

    enum Commands
    {

        CMD_BRC_SHUTDOWN = BasicCommands::CMD_BRC_SHUTDOWN,
        CMD_INIT = BasicCommands::CMD_USER_MIN,
        CMD_CYCLIC
    };

    std::string _connectionstring;
    std::string _table;

    std::vector<CDBWriterHelper*> _dbwriterhelpers;

    bool _datavalid;

    cppdb::session _sqlsession;

    // Configuration cache
    std::string _cfg_cache_db_type;
    std::string _cfg_cache_db_host;
    std::string _cfg_cache_db_port;
    std::string _cfg_cache_db_unixsocket;
    std::string _cfg_cache_db_mode;
    std::string _cfg_cache_db_user;
    std::string _cfg_cache_db_passwd;
    std::string _cfg_cache_db_database;
    std::string _cfg_cache_db_cppdb_options;

};

#endif

#endif /* CDBWriterFilter_H_ */
