/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

Copyright (C) 2009-2012 Tobias Frost

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

/** \file ICommand.h
 *
 *  Created on: May 17, 2009
 *      Author: tobi
 */

#ifndef ICOMMAND_H_
#define ICOMMAND_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "porting.h"
#endif

/// #define ICMD_WITH_DUMP_MEMBER

#ifdef HAVE_INV_DUMMY
    /// Debug-Helper, define and you have the DumpData Member to dump at runtime
    /// A Icommand,
    /// (Will be automatically enabled if you have the dummy inverter enabled...)
#define ICMD_WITH_DUMP_MEMBER
#endif


#include <string>
#include <map>
#include <stdexcept>

#include <boost/any.hpp>

#include "configuration/ILogger.h"
#include "Inverters/BasicCommands.h"
#include <assert.h>

// Tokens for ICommands (general meanings)

/// Error indicator. Same as errno, integer. 0 for no error.
#define ICMD_ERRNO "ICMD_ERRNO"

/// Error indicator.
/// Optional, but if exists it contains human readable error message
#define ICMD_ERRNO_STR "ICMD_ERRMSG"

class ICommandTarget;

/** Encapsulates a command
 *
 * See the command pattern for details....
 *
 * Commands are used, for example, to schedule work,
 * to check for completion, etc...
 *
 * NOTE: The one that calls execute should delete the object afterwards!
 * (like CWorkScheduler does...)
 *
 * \note New from Oct 2012: "Fire-and-forget"-ICommands. This are commands whose
 * ICommandTarget is NULL. They will be be answered, just deleted...
 * Purpose is to allow calls to subsystems which takes ICommands as callbacks
 * (like the IConnect ones) when the actual callback is unimportant.
 *
 */
class ICommand
{
public:
	ICommand(int command, ICommandTarget *target, std::map<std::string,
			boost::any> dat);

	ICommand(int command, ICommandTarget *target);

	virtual ~ICommand();

	/// excecute the command
	void execute();

	/// Getter for the command
	int getCmd() const;

	/** Find Data in Command
	 *
	 * Returns the data associated in the map to the given key
	 *
	 * \in param key to search for
	 * \returns boost::any object with the data
	 *
	 * \throw std::invalid_argument { throws this if data is not existant. The
	 * data of the invalid_argument is the key which was not found }
	 */
	const boost::any findData(const std::string & key) const;

	///  Setter for Command
	void setCmd(int cmd)
	{
		this->cmd = cmd;
	}

	/// Setter for the target of the command
	void setTrgt(ICommandTarget *trgt)
	{
		this->trgt = trgt;
	}


    /// Getter for the target
    ICommandTarget* getTrgt() const {
        return trgt;
    }

	/** Remove Data from Command
	 *
	 * Removes the named key from the data list of the command.
	 * \note: As the underlaying storage is a std::map,
	 * the associated boost::any object will be deleted.
	 */
	inline void RemoveData(const std::string & key)
	{
		dat.erase(key);
	}

    /** Remove all data from command.
     *
     * \note: As the underlaying storage is a std::map,
     * the associated boost::any objects will be deleted.
     */
    inline void RemoveData()
    {
        dat.clear();
    }

	/** Add/Replace Data from the Command
	 *
	 * Add new data, or if the data is already existing, replace
	 * the data with the new one.
	 *
	 */
	void addData(const std::string &key, const boost::any &data)
	{
		if (dat.count(key)) {
			dat.erase(key);
		}
		dat.insert(std::pair<std::string, boost::any>(key, data));
	}

	/// Merge data from other ICommand into this one.
	void mergeData(const ICommand &other);

#ifdef ICMD_WITH_DUMP_MEMBER
    /// Debug-Helper to dump all data which is stored in the ICommand.
    void DumpData(ILogger& logger) const {
        std::map<std::string, boost::any>::const_iterator it;
        LOGDEBUG(logger, "ICommand::DumpData() with command " << this->cmd);
        if (this->cmd < BasicCommands::CMD_BROADCAST_MAX) {
            LOGDEBUG(logger, "(BROADCAST COMMAND) " << this->cmd);
        } else {
            assert(this->trgt);
            LOGDEBUG(logger, "Target: " << trgt);
        }
        LOGDEBUG(logger, "dumping available data: ");
        for (it = dat.begin(); it != dat.end(); it++) {
            LOGDEBUG(logger, it->first);
        }
        LOGDEBUG(logger, "dumping done.");
    }
#endif

private:
	int cmd;
	ICommandTarget *trgt;

	std::map<std::string, boost::any> dat;
};

#endif /* ICOMMAND_H_ */
