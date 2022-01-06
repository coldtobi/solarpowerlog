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

/** \file CDumpOutputFilter.h
 *  \date Jun 1, 2009
 *  \author Tobias Frost
 *
 */

/**
 *  \page CDumpOutputFilter [LOGGER] DumpDumper: A Simple Data Logger
 *
 * This Logger writes the data to standard oputput.
 *
 * This is filter can be seen as a reference implementation to see how things
 * work.
 *
 * \section DLDUMP_Description Overview
 *
 * This Logger simply queries all Capbilities available and dumps their value
 * periodically to cout.
 *
 * The filter automatically determines how often the values updates (via the
 * Inverters period) and adapts to this value.
 * If this value is not available, it defaults to 5 seconds.
 *
 * \section DLDUMP_Configuration Configuration
 *
 * As every logger, this  Logger is configured using the Loggers Section.
 * <TABLE border=1 summary="DumbDumper Logger Configuration Options" >
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
 * 	<td> x </th>
 * 	<td> -</td>
 *      <td> Names the Logger. </td>
 *
 *</tr>
 * <tr>
 * 	<td> type </td>
 * 	<td> string </td>
 * 	<td> "DumbDumper" </th>
 *  	<td>-/td>
 *      <td> Selects the LoggerType. To get a DumpOutput Logger, this
 *           <b>must</b> be DumbDumper . </td>
 *</tr>
 *  <tr>
 * 	<td> datasource </td>
 * 	<td> string </td>
 * 	<td> x </th>
 *  	<td> -</td>
 *      <td> Name of the datasource. Must be eighter a name of a Inverter
 *           (Inverter Section)
 *      or a name of another DataFilter. </td>
 * </tr>
 * <tr>
 * 	<td> clearscreen </td>
 * 	<td> boolean </td>
 * 	<td> "CSVLogger" </th>
 *  	<td> false </td>
 *      <td> Specify, if the logger should clean the terminal before starting
 *      to write. </td>
 * </tr>
 * </table>
 *
 * \subsection DLDUMPCONF_example Configuration Example
 *
 * \code
 * 	loggers = (
 {
 # This dumper is known as (required)
 name = "Simple Dumper 1";
 # It is of type
 type = "DumbDumper";
 # And gets its data from
 datasource = "Inverter_1";
 # Yes, it should clean the screen before dumping
 # (optional. Defaults to false (off)
 # use true to enable it.
 clearscreen = true;
 };
 *
 *
 * \endcode
 *
 */

#ifndef CDUMPOUTPUTFILTER_H_
#define CDUMPOUTPUTFILTER_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_FILTER_DUMBDUMP

/** \fixme COMMENT ME
 *
 *
 * TODO DOCUMENT ME!
 */
#include "DataFilters/interfaces/IDataFilter.h"
#include "Inverters/interfaces/CNestedCapaIterator.h"
#include "Inverters/BasicCommands.h"

class CDumpOutputFilter : public IDataFilter
{
protected:
    friend class IDataFilterFactory;
    CDumpOutputFilter(const string &name, const string & configurationpath);

public:
    virtual ~CDumpOutputFilter();

    virtual bool CheckConfig();

    virtual void Update(const IObserverSubject *subject);

    /** This DataFilter uses the CWorkScheduler, so it needs to implement
     * this function. \sa ICommandTarget::ExecuteCommand */
    virtual void ExecuteCommand(const ICommand *cmd);

#warning implement me!
    virtual CConfigCentral* getConfigCentralObject(void) { return NULL; }

private:
    void CheckOrUnSubscribe(bool subscribe = true);

    void DoCyclicWork(void);

    enum Commands
    {
        CMD_INIT = BasicCommands::CMD_USER_MIN,
        CMD_CYCLIC,
        CMD_UNSUBSCRIBE,
        CMD_ADDED_CAPAS
    };

    bool AddedCaps;

    bool clearscreen;
};

#endif

#endif /* CDUMPOUTPUTFILTER_H_ */
