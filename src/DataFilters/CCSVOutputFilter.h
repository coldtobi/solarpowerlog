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

/** \file CCSVOutputFilter.h
 * \date Jul 1, 2009
 * \author Tobias Frost
 *
 * \page CVSDataLogger [LOGGER] CVSDataLogger: Logging to CSV Files
 *
 * \section DLCSV_Description Overview
 * The CSV Data Logger takes some or all data and writes it to a regular comma-
 * sepearated-file.
 *
 * The data to be logged can be selected by specifying the identifiers or
 * all data.
 *
 * Solarpowerlog honors the RFC 4180. However, some feature, like the "log all"
 * feature causes that for example the number of columns changes during runtime.
 * (However, patches will be accepted to fix this: For example, on a new feature,
 * one could just reparse the file and add the missing datas.
 *
 * If some data is unavailable, it will be logged with an empty value.
 *
 * To increase the use of the logfile, a (ISO 8601)-like timestamp
 * will be inserted as the first column.
 *
 * \section DLCSV_Configuration Configuration
 *
 * As every logger, the CSV Logger is configured using the Loggers Section.
 *
 * <TABLE border=1 summary="CSV Logger Configuration Options" >
 * <tr>
 * 	<td> <b>Option</b> </td>
 * 	<td> <b>Type</b> </td>
 * 	<td> <b>Mandatory</b> </td>
 * 	<td> <b>Default Value</b></td>
 *      <td> <b>Description </b></td>
 *</tr>
 * <tr>
 * 	<td> name </td>
 * 	<td> string </td>
 * 	<td> yes </th>
 * 	<td> &nbsp; </td>
 *      <td> Names the Logger. Used to identify the logger. </td>
 *
 *</tr>
 * <tr>
 * 	<td> type </td>
 * 	<td> string </td>
 * 	<td> yes </th>
 *  	<td> &nbsp; </td>
 *      <td> Selects the LoggerType. To get a CSVLogger, this
 *           <b>must</b> be CSVLogger. </td>
 *</tr>
 *  <tr>
 * 	<td> datasource </td>
 * 	<td> string </td>
 * 	<td> yes </th>
 *  	<td> &nbsp; </td>
 *      <td> Name of the datasource. Must be either a name of a Inverter
 *           (Inverter Section) or a name of another DataFilter/logger. </td>
 * </tr>
 * <tr>
 * 	<td> logfile  </td>
 * 	<td> string  </td>
 * 	<td> yes </th>
 *  	<td> &nbsp;  </td>
 *      <td> Defines the target file for this CSV file.
 *      	See below for additional information. </td>
 * </tr>
 * <tr>
 * 	<td> rotate  </td>
 * 	<td> bool  </td>
 * 	<td> &nbsp;</th>
 *  	<td> false  </td>
 *      <td> Create a new logfile at midnight. Also see the notes on logfile below.
 *      </td>
 * </tr>
 * <tr>
 * 	<td> compact_csv  </td>
 * 	<td> bool  </td>
 * 	<td> &nbsp;</th>
 *  	<td> false  </td>
 *      <td> tries to keep the files compact if the data is not changing.
 *      Done by not writing lines with the exact same content.
 *      </td>
 * </tr>
 * <tr>
 * 	<td> flush_file_buffer_immediatly </td>
 * 	<td> bool  </td>
 * 	<td> &nbsp;</th>
 *  	<td> false  </td>
 *      <td> if true, do not cache information but immediately write to disk.
 *      If you are "only logging" this might be fine to set to false, if you do
 *      some kind of real-time data processing, make this false, as it might
 *      take some times for the data to enter the disk.
 *      One use of this option disabled is if you log to flash memory or if
 *      you want to avoid spinning up disks.
 *      \note Solarpowerlog only hints the operating system to flush the file
 *       buffers. The OS or hardware (harddisk) still might use some caching.
 *      \note Even if this is setting is false, the operating system will still
 *      take care that the data is written to the disk usually after a few
 *      seconds.)
 *      \note Up to version 0.21 including this setting was default set to true.
 *      </td>
 * </tr>
 * <tr>
 * 	<td> format_timestamp </td>
 * 	<td> string  </td>
 * 	<td> &nbsp;</th>
 *  	<td> "%Y-%m-%d %T" </td>
 *      <td> How should the timestamp be rendered? You can use the options as
 *      described here:
 *      http://www.boost.org/doc/libs/1_37_0/doc/html/date_time/date_time_io.html#date_time.format_flags
 *      <br/>
 *      However, the default set the date in the ISO 8601 format, for example
 *      2009-12-20 13:34:56.
 *      </td>
 * </tr>
 * <tr>
 * 	<td> data2log  </td>
 * 	<td> string or array  </td>
 * 	<td> &nbsp;</th>
 *  	<td> all  </td>
 *      <td> If the string reads "all", everything is logged. If an array is
 *      given, the data identified by the array will be logged.
 *      See below for details.
 *      </td>
 * </tr>
 * </table>
 *
 * <b>logfile:</b>
 * With lofile the filename to log to will be specified.
 * If "rotate" is enabled, this logger will
 * begin a new logfile at midnight . To avoid overwriting the old logfile it will
 * add the current date to the filename, using by default an ISO 8601 format: YYYY-MM-DD.
 * To specify *where* the timestamp should be placed, use "%s".
 * If %s is not given, it will be appended *at the end* of the filename.
 * For example
 * \code
 * 	logfile="Inverter_1_%s.csv"
 * \endcode
 * will create a logfile like "Inverter_1_2009-07-04.csv"
 *
 * To set the format of the timestamp see the option <b>format_timestamp</b>.
 *
 * <b> data2log:</b>
 *
 * To specify which data should be logged, their identifiers have to be listed in a
 * array.
 *
 * The identifiers supported can be retrieved by either the inverter's
 * documentation, in the documentation of the intermediate datafilters.
 *
 * One can also use the DumbDumper logger or run the CVS Logger in the "all" mode
 * to obtain a list.
 *
 * \note The identifiers are case sensitive!
 *
 * Array-Example:
 * \code
 *  data2log = [
 *  	"Current Grid Feeding Power",
 *  	"Energy produced today (kWh)",
 *  	"Energy produced this month (kWh)",
 *      "DC voltage in (V)"
 * ];
 * \endcode
 *
 * If you want to use the "log all" feature, just say "all" or do not specify the data to log
 * (as "all" is the default)
 *
 * \note When selecting "all features" and new features are detected at runtime,
 * the CSV-header will be written again with the new data added to a new column.
 * If you want to avoid having ever-changing tables, please configure
 * all the data you want to see in the log file. Data which is not present all
 * the time will then still gets its placeholder in the output file.
 */

#ifndef CCSVOUTPUTFILTER_H_
#define CCSVOUTPUTFILTER_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_FILTER_CSVDUMP

#include <fstream>
#include <list>
#include "boost/date_time/local_time/local_time.hpp"

#include "DataFilters/interfaces/IDataFilter.h"
#include "Inverters/BasicCommands.h"


/** This class implements a logger to write the data to a CSV File
 *
 * Please see \ref DLCSV_Description for configuration, etc.
 */

class CCSVOutputFilter : public IDataFilter
{

protected:
	friend class IDataFilterFactory;
	CCSVOutputFilter( const string &name, const string & configurationpath );

public:
	virtual ~CCSVOutputFilter();

	virtual bool CheckConfig();

	virtual void  Update( const IObserverSubject *subject );

	virtual void ExecuteCommand( const ICommand *cmd );

    virtual CConfigCentral* getConfigCentralObject(CConfigCentral *parent);

private:
	fstream file;

	/** Do the initialization of the module
	 *
	 * - subscribe to basic Capabilities
	 * - open logfile (and create filename)
	 * - schedule rotation of the filename at 00:00:05
	 * - schedule cyclic working
	 *
	 * Also will do the rotating of the logfile*/
	void DoINITCmd (const ICommand *);

	/** does the actual work:
	 *
	 * - maintain the list of columns to be written
	 * (the Capa map is sorted, but we need to make sure that
	 * we won't switch columns)
	 * - write the heade if necessary
	 * - check for data validty.
	 *  */
	void DoCYCLICmd(const ICommand *);

    enum Commands
    {
        CMD_BRC_SHUTDOWN = BasicCommands::CMD_BRC_SHUTDOWN,
        CMD_INIT = BasicCommands::CMD_USER_MIN,
        CMD_CYCLIC,
        CMD_ROTATE ///<Rotate logfile
    };

	/** has the header been outputted to the file */
	bool headerwritten;

	/** is the data supplied by the inverter valid?*/
	bool datavalid;

	/** are there some updated capas? */
	bool capsupdated;

	/** list of Capabilities in the CSV */
	list<string> CSVCapas;

	// Helpers to shrink some functions...
	/** Check if any capas are now available which were not before
	 * (but should be tracked)
	 * Read from configuration which capabilities to be logged and
	 * and assemble the std::list containing everything we want to log.
	 * In case of dynamic logging ("the all feature") check for new features
	 * and append them to the list to be logged (at the end of the list)
	 *
	 * The function will be called whenever the Capa-Updated event
	 * is set via this->Update()
	 *
	 * \return true, if a new Capability was detected and a new CSV Header
	 * should be generated. Otherwise false.
	 *
	*/
	bool CMDCyclic_CheckCapas(void);

	/**  search the CSVCapas for a named capa
	 *
	 * \returns true, if in list, else false. */
	bool search_list(const string id) const;

	/// cache: last emitted string without timestamp
	std::string last_line;

	/** configuration cache: filename of the CVS log */
	std::string _cfg_cache_filename;

    /** configuration cache: how to format the timestamp */
    std::string _cfg_cache_formattimestap;

	/** configuration cache rotate the logfile at midnight */
	bool _cfg_cache_rotate;

	/** configuration cache: should repeated lines be suppressed? */
	bool _cfg_cache_compactcsv;

	/** configuration cache: should we "flush" after every write
	 * TODO: Maybe change this parameter / add another to specify file buffer
	 * length via std::streambuf::pubsetbuf */
	bool _cfg_cache_flushfb;

	/** configuration cache: is data2log="all"? */
	bool _cfg_cache_data2log_all;

	/** cache, if we created the CapasList already.*/
	bool _cache_found_all_capas;

};

#endif

#endif /* CCSVOUTPUTFILTER_H_ */
