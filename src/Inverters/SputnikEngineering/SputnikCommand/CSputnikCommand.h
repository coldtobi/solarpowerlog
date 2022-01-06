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

/*
 * CSputnikCommand.h
 *
 *  Created on: 14.05.2012
 *      Author: tobi
 */

#ifndef CSPUTNIKCOMMAND_H_
#define CSPUTNIKCOMMAND_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>
#include <vector>
#include <stdlib.h>

#include "Inverters/interfaces/InverterBase.h"
#include "Inverters/SputnikEngineering/SputnikCommand/ISputnikCommand.h"
#include "patterns/CValue.h"
#include "Inverters/Capabilites.h"
#include "configuration/ILogger.h"

/* Defines and handles a single commmand the Sputnik Inverter.
 *
 * CSputnikCommand (CSC) will in the long term replace the handling code in
 * CSputnikInverter for an object orientated approach.
 *
 * The object will store all knowledge about an individual command, allows the
 * parsing and eventually will also take over the Capabilities handling.
 *
 * It will also allow an strategy to wisely let the command itself decide when it
 * should be issued and how often, and to probe if an actual command is indeed supported
 * by the inverter.
 *
 * Milestones to implement functionality:
 *  1- CSC will store information about
 *      - (implemented) command token, scaling information, expected telegram length,
 *      - (implemented for basic types) do the parsing and return the parsed result.
 *
 *  2- CSC will keep track if a command is supported by the Inverter and cease issuing command if not.
 *  3- (Specialisations of CSCs might handle for some commands more often than others.)
 *  4- CSC will take the Capabilities handling: creating the Capabilities, updating the values, notfiing subscribers
 *
 *  Strategy:
 *  - CSCs are kept in a list attached to the Inverters Object
 *  - The CSCS are instanciated on construction time of the Inverter's object with all the information they need to operate.
 *  5- CSC will be registered by the Inverter at Instanciation time by key parameters-
 *
 *
*/

/** Command Class template for all basic types with the support to scale.*/
template<class T>
class CSputnikCommand : public ISputnikCommand
{
public:

    CSputnikCommand(ILogger &logger, const std::string &cmd, int max_answer_len, T scale,
        IInverterBase *inv, const std::string & capname,
        ISputnikCommandBackoffStrategy *backoff = NULL)
        : ISputnikCommand(logger, cmd, max_answer_len, inv, capname, backoff), scale(
            scale) { }

    virtual ~CSputnikCommand() {};

private:
    /// convert the hex answer to a long variable.
    unsigned long converthextolong(const std::vector<std::string> & tokens) {
        return strtoul(tokens[1].c_str(), NULL, 16);
    }

    /// convert the answer of the inverter (given in tokens) to the template type
    /// and scale it
    T convert(const std::vector<std::string> & tokens) {
        unsigned long integer = this->converthextolong(tokens);
        T result = integer;
        result *= scale;
        return result;
    }

public:
    //// Parsing of the response and updating capability.
    /// Caller gives us a already tokenized response, where tokens[0] is the
    /// command (echoed by the inverter) and tokens[1] is the value is hex
    /// format. (without 0x, always integer.)
    virtual bool handle_token(const std::vector<std::string> & tokens) {
        if (tokens.size() != 2) return false;
        try {
            T temp = this->convert(tokens);
            CapabilityHandling<T>(temp);
        } catch (...) {
            return false;
        }
        this->strat->CommandAnswered();
        return true;
    }

private:
    T scale;
};

#endif /* CSPUTNIKCOMMAND_H_ */
