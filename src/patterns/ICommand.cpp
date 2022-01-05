/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

Copyright (C) 2009-2012 Tobias Frost

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


/** \file ICommand.cpp
 *
 *  Created on: May 17, 2009
 *      Author: tobi
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "patterns/ICommand.h"
#include "patterns/ICommandTarget.h"

ICommand::ICommand(int command, ICommandTarget *target, std::map<std::string,
		boost::any> dat)
{

	this->cmd = command;
	this->trgt = target;
	this->dat = dat;
}

ICommand::ICommand(int command, ICommandTarget *target)
{
	cmd = command;
	trgt = target;
}

/** Destructor, even for no need for destruction */
ICommand::~ICommand()
{
}

/** Delegate the command to the one that should do the work */
void ICommand::execute()
{
    // Allow also "fire-and-forget" commmands which will be done but never a
    // callback issued.
	if (trgt) trgt->ExecuteCommand(this);
}

/** Getter for the private cmd field (field is for Commandees use) */
int ICommand::getCmd() const
{
	return cmd;
}

const boost::any ICommand::findData(const std::string &key) const
{
	std::map<std::string, boost::any>::const_iterator it = dat.find(key);
	if (it != dat.end()) {
		return (*it).second;
	}
	throw(std::invalid_argument(key));
}

void ICommand::mergeData(const ICommand &other)
{
	// first delete all duplicate data, but only if the containers have data.
    // (this is needed as std::map insert won't change the content if a key is
    // alredy there)
    // FIXME: missed optimization: iterate only over the shorter one of both containers.
    if (dat.size() && other.dat.size()) {
        std::map<std::string, boost::any>::const_iterator it;
        for(it = dat.begin(); it != dat.end(); it++)
        {
            if(other.dat.count(it->first)) dat.erase(it->first);
        }
    }
	// and then merge the others data into the map
	dat.insert(other.dat.begin(), other.dat.end());
}
