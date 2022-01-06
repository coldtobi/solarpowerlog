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

/** \file Capabilites.h
 *
 *  This file declares the cpabilites, a inverter might have.
 *  (Of course also the ones a inverter "have to have")
 *  The names defined are to be used for generation and lookup.
 *
 *  The strings are for "human information"
 *
 *
 *
 *  Created on: May 22, 2009
 *      Author: Tobias Frost
 *
 *      Contributors:
 *         E.A.Neonakis <eaneonakis@freemail.gr>
 */

#ifndef CAPABILITES_H_
#define CAPABILITES_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/** The Capabilites list has been updated.
 * This "pseudo" capabilites can be used to tell observers, that the
 * data provider might have detected new capabilites and therefore
 * check the presence of new interesting data.
 *
 * A common use is, if the InverterBase-client can do autodetection
 * of capabilities during runtime, or as a result of auto-detecting the
 * excact model of a Inverter Family.
 *
 * Every Observer should subscribe to this one.
 *
 *\note The associated concrete value is unused.
 *
 * \sa The pseudo cap "force unsubscribe" is related.
 *
 * THIS CAPABILITY IS REQUIRED -- EVERY INVERTER HAS THIS ONE.  */
#define CAPA_CAPAS_UPDATED  	 "CapabilityList Updated"
#define CAPA_CAPAS_UPDATED_TYPE  bool


/** Some Capabilites are now void and the observers have to unsubscribe.
 *
 * This "pseudo" capabilites can be used to tell observers, that the
 * data provider detected a situation, where the Capbailities List have
 * to be recreated.
 *
 * For this to happen safely, every Subscriber must remove itself from the
 * subscription list on every capability -- except on the remove and add pseudo
 * capability.
 *
 *\note The associated concrete value is unused.
 *
 * Common situation swhere this will be used:
 * 	- Configuration reload
 *  - Program termination
 *
 * In case of a reload, a "CAPA_CAPAS_UPDATED" will follow.
 *
 * \sa The pseudo cap "force unsubsribe" is related.
 *
 * THIS CAPABILITY IS REQUIRED -- EVERY INVERTER HAS THIS ONE.*/
#define CAPA_CAPAS_REMOVEALL 		"CapabilityList Please Unsubscribe"
#define CAPA_CAPAS_REMOVEALL_TYPE 	bool


/** Is data the provided by the inverter valid?
 * If the  inverter is "power down" (like the Sputnik at night), this
 * is the master switch telling that all data is now invalid.
 * (if the associated value is false)
 *
 * THIS IS A MUST CAPABILITY -- EVERY INVERTER HAS THIS ONE.  */
#define CAPA_INVERTER_DATASTATE  "Data Validity"
#define CAPA_INVERTER_DATASTATE_TYPE  bool

/** How often are the datas queried
 *
 * How often are the datas queried, if done cyclic. Unit is seconds.
 *
 * Type: float
 *
 * optional
 */
#define CAPA_INVERTER_QUERYINTERVAL  "Data Query Interval"
#define CAPA_INVERTER_QUERYINTERVAL_TYPE  float



/** Basic information for the user -- these information are usually not
 * updated. But, as a exception to this, a inverter class might do runtime
 * detection of these parameters, if the inverter supports them.
 *
 * This one is the "human readable" manufacturer of the Inverter.
 *
 * \depreciated this capa should no longer be used. Use
 *  CAPA_INVERTER_MANUFACTURER_NAME instead
 */
#define CAPA_INVERTER_MANUFACTOR_NAME "Inverter Manufactor"
#define CAPA_INVERTER_MANUFACTOR_TYPE std::string

/** Basic information for the user -- these information are usually not
 * updated. But, as a exception to this, a inverter class might do runtime
 * detection of these parameters, if the inverter supports them.
 *
 * This one is the "human readable" manufacturer of the Inverter.
 */

#define CAPA_INVERTER_MANUFACTURER_NAME "Inverter Manufacturer"
#define CAPA_INVERTER_MANUFACTURER_TYPE std::string

/** Basic information for the user -- these information are usually not
 * updated. But, as a exception to this, a inverter class might do runtime
 * detection of these parameters, if the inverter supports them.
 *
 * This one is the "human readable" model of the Inverter.
 */

#define CAPA_INVERTER_MODEL "Inverter Model"
#define CAPA_INVERTER_MODEL_TYPE std::string


/** Basic information again -- how is the inverter named in the config file
 * Note: This has to be added by the concrete inverter class. */
#define CAPA_INVERTER_CONFIGNAME "Inverter Name"
#define CAPA_INVERTER_CONFIGNAME_TYPE std::string



/** Firmware version information.
 *
 * If available, can contain a human-readable info about the
 * inverters firmware
 *
 * type: string
 *
 * Optional.
 *
*/

#define CAPA_INVERTER_FIRMWARE "Firmware Version"
#define CAPA_INVERTER_FIRMWARE_TYPE std::string


/** Total power feeding AC,DC
 *
 * On inverters which feeds more than one phase, this is the
 * sum of all phases.
 *
 * Type: float
 *
 * Recommended for every inverter, but still optional
*/
#define CAPA_INVERTER_ACPOWER_TOTAL "Current Grid Feeding Power"
#define CAPA_INVERTER_ACPOWER_TOTAL_TYPE float
#define CAPA_INVERTER_DCPOWER_TOTAL "DC Power"
#define CAPA_INVERTER_DCPOWER_TOTAL_TYPE float

/** Power On Hours
 *
 * Counts the hours a inverter was powered.
 *
 * Type: float
 *
 * Optional.
*/
#define CAPA_INVERTER_PON_HOURS "Inverter Power On Hours"
#define CAPA_INVERTER_PON_HOURS_TYPE float

/** Total Inverter Startups
 *
 * Counts inverter startups
 *
 * Type: Integer
 *
 * Optional.
*/
#define CAPA_INVERTER_STARTUPS "Inverter Startups"
#define CAPA_INVERTER_STARTUPS_TYPE long


/** Feeded Energy Y2D
 *
 * This year the inverter has produced this amount of energy. (kWh)
 *
 * Type: float
 *
 * Optional.
*/
#define CAPA_INVERTER_KWH_Y2D "Energy produced this year (kWh)"
#define CAPA_INVERTER_KWH_Y2D_TYPE float


/** Feeded Energy M2D
 *
 * This month the inverter has produced this amount of energy. (kWh)
 *
 * Type: float
 *
 * Optional.
*/
#define CAPA_INVERTER_KWH_M2D "Energy produced this month (kWh)"
#define CAPA_INVERTER_KWH_M2D_TYPE float

/** Feeded Energy Today,Yesterday
 *
 * Today the inverter has produced this amount of energy. (kWh)
 *
 * Type: float
 *
 * Optional.
*/
#define CAPA_INVERTER_KWH_2D "Energy produced today (kWh)"
#define CAPA_INVERTER_KWH_2D_TYPE float
#define CAPA_INVERTER_KWH_YD "Energy produced yesterday (kWh)"
#define CAPA_INVERTER_KWH_YD_TYPE float

/** Feeded Energy Total
 *
 * Today the inverter has produced this amount of energy. (kWh)
 *
 * Type: float
 *
 * Optional.
*/
#define CAPA_INVERTER_KWH_TOTAL_NAME "Energy produced accumulated all time (kWh)"
#define CAPA_INVERTER_KWH_TOTAL_TYPE float

/** Installed Power
 *
 * How much power has beein installed?
 *
 * At least on the Sputnik, this can be configured and read by a query.
 *
 * Others can set this by e.g configuration options.
 *
 * Type: Float
 *
 * Optional.
 *
 * */
#define CAPA_INVERTER_INSTALLEDPOWER_NAME "Installed solar power (Wp)"
#define CAPA_INVERTER_INSTALLEDPOWER_TYPE float

/** Current AC Power Frequency
 *
 *
 * Type: Float
 *
 * Optional.
 *
 * */
#define CAPA_INVERTER_NET_FREQUENCY_NAME "Net frequency (Hz)"
#define CAPA_INVERTER_NET_FREQUENCY_TYPE float

/** relative power
 *
 * The Sputnik offers what is called "relative Power".
 * TODO: Check whats relative about that.
 *
 * (However, this reading is somwhat vague, so it is probably
 * better calculated by PAC and PIN or PDC*IDC and PIN)
 *
 * Type: Float
 *
 * Optional.
 *
 * */
#define CAPA_INVERTER_RELPOWER_NAME "relative Power (%)"
#define CAPA_INVERTER_RELPOWER_TYPE float

/** DC Input Voltage
 *
 * Input voltage from the generator.
 *
 * Type: Float
 *
 * Optional.
 *
 * */
#define CAPA_INVERTER_INPUT_DC_VOLTAGE_NAME "DC voltage in (V)"
#define CAPA_INVERTER_INPUT_DC_VOLTAGE_TYPE float

#define CAPA_INVERTER_INPUT_DC_VOLTAGE_T1_NAME "DC voltage Tracker 1 in (V)"
#define CAPA_INVERTER_INPUT_DC_VOLTAGE_T1_TYPE float

#define CAPA_INVERTER_INPUT_DC_VOLTAGE_T2_NAME "DC voltage Tracker 2 in (V)"
#define CAPA_INVERTER_INPUT_DC_VOLTAGE_T2_TYPE float

#define CAPA_INVERTER_INPUT_DC_VOLTAGE_T3_NAME "DC voltage Tracker 3 in (V)"
#define CAPA_INVERTER_INPUT_DC_VOLTAGE_T3_TYPE float

/** DC Input Current
 *
 * Input voltage from the generator.
 *
 * Type: Float
 *
 * Optional.
 *
 * */
#define CAPA_INVERTER_INPUT_DC_CURRENT_NAME "DC current in (A)"
#define CAPA_INVERTER_INPUT_DC_CURRENT_TYPE float

#define CAPA_INVERTER_INPUT_DC_CURRENT_T1_NAME "DC current Tracker 1 in (A)"
#define CAPA_INVERTER_INPUT_DC_CURRENT_T1_TYPE float

#define CAPA_INVERTER_INPUT_DC_CURRENT_T2_NAME "DC current Tracker 2 in (A)"
#define CAPA_INVERTER_INPUT_DC_CURRENT_T2_TYPE float

#define CAPA_INVERTER_INPUT_DC_CURRENT_T3_NAME "DC current Tracker 3 in (A)"
#define CAPA_INVERTER_INPUT_DC_CURRENT_T3_TYPE float

/** Power feeding DC per Tracker
 *
 * Type: float
 *
 * Optional.
 *
 * */
#define CAPA_INVERTER_DCPOWER_T1_NAME "DC Power Tracker 1"
#define CAPA_INVERTER_DCPOWER_T1_TYPE float

#define CAPA_INVERTER_DCPOWER_T2_NAME "DC Power Tracker 2"
#define CAPA_INVERTER_DCPOWER_T2_TYPE float

#define CAPA_INVERTER_DCPOWER_T3_NAME "DC Power Tracker 3"
#define CAPA_INVERTER_DCPOWER_T3_TYPE float

/** AC Grid Voltage
 *
 * For single-phase inverters: Grid voltage of the (single) phase.
 *
 * Type: Float
 *
 * Optional.
 *
 * */
#define CAPA_INVERTER_GRID_AC_VOLTAGE_NAME "AC grid voltage (V)"
#define CAPA_INVERTER_GRID_AC_VOLTAGE_TYPE float

#define CAPA_INVERTER_GRID_AC_VOLTAGE_PHASE2_NAME "AC grid L2 voltage (V)"
#define CAPA_INVERTER_GRID_AC_VOLTAGE_PHASE2_TYPE float

#define CAPA_INVERTER_GRID_AC_VOLTAGE_PHASE3_NAME "AC grid L3 voltage (V)"
#define CAPA_INVERTER_GRID_AC_VOLTAGE_PHASE3_TYPE float

/** AC Grid Current
 *
 * For single-phase inverters: Grid voltage of the (single) phase.
 *
 * Type: Float
 *
 * Optional.
 *
 * */
#define CAPA_INVERTER_GRID_AC_CURRENT_NAME "AC grid feeding current (A)"
#define CAPA_INVERTER_GRID_AC_CURRENT_TYPE float

#define CAPA_INVERTER_GRID_AC_CURRENT_PHASE2_NAME "AC grid L2 feeding current (A)"
#define CAPA_INVERTER_GRID_AC_CURRENT_PHASE2_TYPE float

#define CAPA_INVERTER_GRID_AC_CURRENT_PHASE3_NAME "AC grid L3 feeding current (A)"
#define CAPA_INVERTER_GRID_AC_CURRENT_PHASE3_TYPE float

/** Inverter internal temperature
 *
 * if supported, this shows the temp of the inverter.
 * (and one some models, if the fan is on.)
 *
 * (Note: If you prefer °F, you can program a filter to
 * transform it...)
 *
 * Type: Float
 *
 * Optional.
 *
 * */
#define CAPA_INVERTER_TEMPERATURE_NAME "Inverter Temperature (C)"
#define CAPA_INVERTER_TEMPERATURE_TYPE float

#define CAPA_INVERTER_TEMPERATURE_PHASE2_NAME "Inverter Temperature 2 (C)"
#define CAPA_INVERTER_TEMPERATURE_PHASE2_TYPE float

#define CAPA_INVERTER_TEMPERATURE_PHASE3_NAME "Inverter Temperature 3 (C)"
#define CAPA_INVERTER_TEMPERATURE_PHASE3_TYPE float


// IEE IEA IED

/** Error Currents Ground Voltage
 *
 * Type: Float
 *
 * Optional.
 *
 * */
#define CAPA_INVERTER_ERROR_CURRENT_NAME "Error current in (mA)"
#define CAPA_INVERTER_ERROR_CURRENT_TYPE float
#define CAPA_INVERTER_DC_ERROR_CURRENT_NAME "DC Error current in (mA)"
#define CAPA_INVERTER_DC_ERROR_CURRENT_TYPE float
#define CAPA_INVERTER_AC_ERROR_CURRENT_NAME "AC Error current in (mA)"
#define CAPA_INVERTER_AC_ERROR_CURRENT_TYPE float
#define CAPA_INVERTER_GROUND_VOLTAGE_NAME "Voltage to Ground (V)"
#define CAPA_INVERTER_GROUND_VOLTAGE_TYPE float

/** Inverter Status Codes
 *
 */
enum InverterStatusCodes
{
	/**  offline   -- the inverter is not responsing to queries (e.g night)
	  That status will be set automatically by the Inverter-Class logic,
	  if the capability has been registered and the connection is lost
	  to the inverter.
	*/
	OFFLINE,

	/** status unavailable -- whatever reason. Could be that we just don't
	  know that status code
	*/
	STATUS_UNAVAILABLE,

	/** warning -- a non-fatal situation, like "solar radiance too low" */
	NOT_FEEDING_OK,

	/** not feeding -- because some external event prevents feeding
 	 (like grid power loss, frequency too low....) */
	NOT_FEEDING_EXTEVENT,

	/**< error -- the inverter is inoperable, as it have deteced some error */
	NOT_FEEDING_ERROR,

	/** warning -- user action required or limited operation
      (the inverter sensed a problem, but can operate, but maybe at a
      lower power settings) */
	FEEDING_WARNING,

	/** operating -- basically green, but not in the optimun (yet) */
	FEEDING,

	/** perfect 	-- everything fine. Operating in MPP*/
	FEEDING_MPP,

	/** the inveter is feeding at its limit.
	 * Note: If this is due a problem, use FEEDING_WARNING*/
	FEEDING_MAXPOWER,
};

/** Inverter Overall status
 *
 * for the values, see the enum InverterStatusCodes.
*/

#define CAPA_INVERTER_STATUS_NAME "Inverter Overall Status (int)"
#define CAPA_INVERTER_STATUS_TYPE long

/** Inverter Overall status -- human readable version
 *
 * This contains the status of the inverter, but parsed for humans.
 * This also should contain information, why the inverter is in that state,
 * if available.
 *
 * If available, one might give also the manufacturer's statuscode
 *
 * Examples:
 * NOT FEEDING -- Solar radiation insufficient
 * NOT FEEDING -- Inverter Starting up
 * WARNING -- FAN MALFUNCTION DETECTED. USER ATTENTION REQUIRED. LIMITED FEEDING!
 * FEEDING -- Searching MPP
 * FEEDING -- at MPP
 * FEEDING -- MAXIMUM POWER
 *
*/

#define CAPA_INVERTER_STATUS_READABLE_NAME "Inverter Overall Status"
#define CAPA_INVERTER_STATUS_READABLE_TYPE std::string

// Filter "CSVDumper" provides the current logging filename in this value.
// Present only if CSV Dumper is in the chain.
// Empty, if the file could not be opened.
#define CAPA_CSVDUMPER_FILENAME "CSVDumper::Filename"
#define CAPA_CSVDUMPER_FILENAME_TYPE std::string

// Filer "CSVDumper" logges these capabilites.
// Note: This lis might change over runtime, as only in "log everything" mode
// the CSV is expanded dynamically. Be prepared that might be not enough data
// in your CSV!
// The Capabilites are "Comma seperated", with no blank in between.
#define CAPA_CSVDUMPER_LOGGEDCAPABILITES "CSVDumper::LoggedCaps"
#define CAPA_CSVDUMPER_LOGGEDCAPABILITES_TYPE std::string


#endif /* CAPABILITES_H_ */
