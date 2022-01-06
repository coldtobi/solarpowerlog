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

/**
 * \file ISputnikCommandBackoffStrategy.h
 *
 * The backoff strategies decided how often a command will be issued
 * due to some compile-time selected algorithms.
 *
 * Also (they will) determine if a command is at all supported and if
 * necessary will cease this command completely. (
 *
 * This class defines the interface for the algorithms.
 *
 * Note that Backoff-Stragetgies can be staggered (similar to the
 * decorator Pattern), therefore derived classes needs always to call
 * the interface method.
 *
 *  Created on: 28.05.2012
 *      Author: tobi
 */

#ifndef ISPUTNIKCOMMANDBACKOFFSTRATEGY_H_
#define ISPUTNIKCOMMANDBACKOFFSTRATEGY_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stddef.h>

#include "configuration/ILogger.h"

/**
 * Interface class for the decorator-pattern-influenced Backoff strategies.
 */
class ISputnikCommandBackoffStrategy
{
public:

protected:
    /**
     * protected constructor for BackoffStrategies
     *
     * As BO's needs to be specialized, this is protected.
     *
     * \param [in] botype pretty-print typename of the BO Algorithm (used for logging)
     * \param [in] next next object for the decorator-pattern.
     */
    ISputnikCommandBackoffStrategy(const std::string &botype, ISputnikCommandBackoffStrategy *next = NULL);

public:
    /**
     *  Setting up the logger for the BO Algorithm and emit a DEBUG-level
     *  (which helps to log the configured BO-decorator-chain)
     *
     * @param parent parent logger to hook up to
     * @param specialisation specialization to extend the logger with (usually the com
     */
    virtual void SetLogger(const std::string &parent) {
        _logger.Setup(parent, _botype);
        if (next) next->SetLogger(parent);

        LOGINFO(_logger,"back off strategy " << _botype);
    }

    virtual ~ISputnikCommandBackoffStrategy();

    /// Should the command be considered?
    /// return false if not, true if yes.
    /// Call Interface first. If Interface returns "false", also return false.
    virtual bool ConsiderCommand();

    /// The command has been issued
    /// Note: If "Command Issued" is followed by "Command Answered" or
    /// a "CommandNotAnswered".
    virtual void CommandIssued() ;

    /// The command has been answered.
    virtual void CommandAnswered();

    /// The command has not been answered.
    virtual void CommandNotAnswered();

    /// Inverter disconnected, reset state.
    virtual void Reset();

protected:
    /// Next backoff strategy in the chain (decorator pattern)
    ISputnikCommandBackoffStrategy *next;
    /// Logger  for this strategy object
    ILogger _logger;
    /// Caching the type -- needed for pretty print logging
    std::string _botype;
};

#endif /* ISPUTNIKCOMMANDBACKOFFSTRATEGY_H_ */
