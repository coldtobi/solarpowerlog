/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

Copyright (C) 2009-2014 Tobias Frost

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

/*
 * \file CDBWriterHelper.hpp
 *
 *  Created on: 05.07.2014
 *      Author: tobi
 */

#ifndef DDBWRITER_HELPER_HPP_
#define DDBWRITER_HELPER_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "porting.h"
#endif

#ifdef  HAVE_FILTER_DBWRITER

#include <string>
#include <map>

#include <cppdb/frontend.h>

#include "configuration/ILogger.h"
#include "patterns/IObserverObserver.h"
#include "Inverters/interfaces/InverterBase.h"

#include "CdbInfo.h"

class CConfigCentral;

/** Handling class for one db job (one table)
 *
 *  This class maintains one table, retrieving data from the inverters,
 *  validty checking and SQL statement issuing when conditions are met.
 */
class CDBWriterHelper : public IObserverObserver
{

public:
    CDBWriterHelper(IInverterBase *base, const ILogger &parent,
        const std::string &table, const std::string &mode,
        const std::string &createmode, bool logchangedonly, float logevery,
        bool allow_sparse, std::string configurationpath ="");

    virtual ~CDBWriterHelper();

    // The SQL Magic....
    virtual void ExecuteQuery(cppdb::session &session);

    /// Add the tuple Capability, Column to the "should be logged information"
    /// Returns "FALSE" if the combination of Capabilty and Column is alreaedy there.
    virtual bool AddDataToLog(const std::string &Capability, const std::string &Column);

    virtual void Update(const class IObserverSubject * subject);

    virtual const std::string &GetTable(void) {
        return _table;
    }

    /** Getter for logevery */
    virtual float getLogevery() const
    {
        return _logevery;
    }

    /** Setter for logevery */
    virtual void setLogevery(float logevery)
    {
        _logevery = logevery;
    }

    virtual bool CheckConfig(void);

    virtual CConfigCentral* getConfigCentralObject(CConfigCentral *parent);

private:
    /** Assemble a value-string from the dbinfos
     * the generated form is:
     * [col1]=?, [col2]=?, ... [colx]=?
     * The "?" are for the values to be bound
     * \returns the generated value string
     */
    std::string _GetValStringForUpdate(void);

    /** Assemble a column-value-string from the dbinfos
     *
     * the generated form is:
     * (col1,col2,col3) VALUES (?,?,?)
     * The "?" are for the values to be bound
     *
     * \returns the generated value string
     */
    std::string _GetValStringForInsert(bool with_selector = false);

    /** Bind all "?" in the (previously calculated) value string
     *
     * \param s statement for the cppdb interaction
     * \param with_selector should the selectors skipped (false) or also bound.
     *
     * \returns true on sucess, false on error.
     */
    bool _BindValues(cppdb::statement &s, bool with_selector = false);

    /** Bind a single value to the CppDB statement
     *
     *
     * \param stat statement for the cppdb interaction
     * \param with_selector should the selectors skipped (false) or also bound.
     *
     * \returns true on sucess, false on error.
     */
    bool _BindSingleValue(cppdb::statement &stat, Cdbinfo &info);

    /** Check if a string is save to avoid SQL injections. *
     *
     * \returns false if a "forbidden" character is encountered.
     */
    bool issane(const std::string s);

    // Definitions.

    /*** enum for the basic operational modes. */
    enum omode
    {
        continuous, /**< "continuous" mode */
        single, /**< "single" mode */
        cumulative /**< "cumulative" mode */
    };

    /** enum for the "create the table" mode */
    enum cmode
    {
        cmode_no, /**< do not create table */
        cmode_yes, /**< try create table */
        cmode_yes_and_drop, /**< destroy and recreate table */
        cmode_print_statment /**< do not create table, just print the statement to create the table */
    };

    // Class state data / internal objects.

    /** The DB-Writer's parent inverter (or datafilter) */
    IInverterBase *_base;

    /** The logger instance */
    ILogger logger;

    /** mutex to protect concurrent access to this class data */
    boost::mutex mutex;

    /** Caching the information if the table name is sane */
    bool _table_sanizited;

    /** Caching the information if the inverter's data is marked valid */
    bool _datavalid;

    /** Caching the config for create table and also state variable:
     * will be set to cmode_no once create table succeed. */
    cmode _createtable_mode;

    /// Storage for the individual data sets to be stored (one per column)
    std::multimap<std::string, class Cdbinfo*> _dbinfo;

    /// when the last logging took place (to determine changes in the dataset)
    boost::posix_time::ptime _lastlogged;

    /** Cache for "regular" insert sql statements -- we don't need to recalculate
     * them all over
     * (TODO check if we should also cache the cppdb::statement object)
     */
    std::string _insert_cache;

    // Configuration cache etc.

    /** The table to act on */
    std::string _table;

    /** Caching the operational mode */
    omode _mode;

    /** Caching the config if we are allow sparse logging */
    bool _allow_sparse;

    /** Caching the config if we should only log on changed data. */
    bool _logchangedonly;

    /** Configuration cache: How often to log. */
    float _logevery;


#warning FIXME new config cache ... currently not hooked up, needs refactoring later.
    std::string _cfg_cache_db_table;
    std::string _cfg_cache_db_create_table;
    std::string _cfg_cache_db_operation_mode;

    bool _cfg_cache_db_logchangedonly;
    bool _cfg_cache_db_allowsparse;

    float _cfg_cache_db_logevery;


};

#endif

#endif /* DDBWRITER_HELPDER_HPP_ */
