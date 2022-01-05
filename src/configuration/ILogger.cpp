/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

 Copyright (C) 2009-2015 Tobias Frost

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

/** \file ILogger.cpp
 *
 *  Created on: Jul 20, 2009
 *      Author: tobi
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "configuration/ILogger.h"

#ifdef HAVE_LIBLOG4CXX

#include "configuration/CConfigHelper.h"
#include "interfaces/CMutexHelper.h"

#include <iostream>

using namespace std;


/// sbdm hash function -- public domain
/// used to
static uint32_t runtime_hash(const char *str)
{
    uint32_t hash = 0;
    int c;

    while ((c = *str++))
        hash = c + (hash << 6) + (hash << 16) - hash;

    return hash;
}

#define MYLINE(x) #x
#define MYLINE2(x) MYLINE(x)
#define MYLINE3  MYLINE2(__LINE__)


/** Construct a logger object with default settings.
 *
 * When using this constructor, the logger will attach to the root
 * logger by default. This can be overridden by a subsequent call Setup() */
ILogger::ILogger()
{
    // if not overridden by setup, always log to the root logger.
    loggerptr_ = log4cxx::Logger::getRootLogger();
    currentlevel = currentloggerlevel_ = loggerptr_->getLevel()->toInt();
    sa_max_suppress_repetitions_ = LOG_STATEWARE_REPEAT;
    sa_max_time_suppress_ = LOG_STATEWARE_TIME;
}

/** Setup logger in the logger, attaching to a parent.
 *
 * The parent logger and the specialization forms the path of the new logger
 * for easier identification of the source of the message. The new logger is
 * put one level deeper than its parent.
 *
 * For example, Inverter_1 adds a Comm which specalizsation of "Comms_TCP_ASIO",
 * the resulting logger is Inveter_1.Comms_TCP_ASIO
 *
 * The logging level will be deducted from the parent, (default
 * from log4cxx).
 *
 * \param parent logger to attach from. Must not be empty.
 * \param specialization for the logger. (the name of the new one)
 */
void ILogger::Setup(const std::string & parent,
    const std::string & specialization)
{
    if (specialization.empty()) {
        loggername_ = parent;
    } else {
        loggername_ = parent + "." + specialization;
    }
    log4cxx::LoggerPtr logger(log4cxx::Logger::getLogger(loggername_));
    log4cxx::LevelPtr ptr = logger->getEffectiveLevel();
    currentloggerlevel_ = ptr->toInt();
    loggerptr_ = logger;
}

/** Setup a logger. Do not associate to a parent
 *
 * (of course, the root logger is always parent)
 *
 * This variant creates a logger with the path <section>.<name>
 * <section> is the name of the section in the configuration file,
 * <name> the name of the object,
 *
 * The Settings for the logger are extracted from the configuration
 * file and with this precedence:
 * - Specification in XML file (liblog4cxx config)
 * - <section>.<name> enry dbglevel in the solarpowerlog.conf
 * - application.dbglevel in the solarpowerlog.conf
 * - "ERROR" if nothing is given,
 *
 * \param name of the new logger
 * \param configuration path where to obtain te objects config.
 *
 */
void ILogger::Setup(const string & name, const string & configuration,
    const string& section)
{
    config_ = configuration;

    loggername_ = section + "." + name;

    log4cxx::LoggerPtr logger(log4cxx::Logger::getLogger(loggername_));
    loggerptr_ = logger;

    // check if the logger has magically already a level.
    // if so, it must be from XML.
    log4cxx::LevelPtr ptr = logger->getLevel();
    if (!ptr) {
        // no, no level set.
        // first try to retrieve specific setting, then global setting and
        // if both fails, just keep using the root loggers setup (which is setup
        // in the constructor)

        string level;
        bool success;

        CConfigHelper hlp(configuration);
        success = hlp.GetConfig("dbglevel", level);

        if (!success) {
            CConfigHelper global("application");
            success = global.GetConfig("dbglevel", level, (std::string)"ERROR");
        }

        if (success) {
            logger->setLevel(log4cxx::Level::toLevel(level));
            currentloggerlevel_ = logger->getLevel()->toInt();
        }
    }
}

void ILogger::SetLoggerLevel(log4cxx::LevelPtr level)
{
    loggerptr_->setLevel(level);
    currentloggerlevel_ = level->toInt();
}

void ILogger::Log_sa(const int32_t hash, std::stringstream &ss)
{
    bool needlog = false;
    uint reason = 0;
    time_t now = time(NULL);
    uint32_t strhash = runtime_hash(ss.str().c_str());

    // LL_ALL disables this feature.
    if (this->IsEnabled(ILogger::LL_ALL)) {
        (*this) << ss;
        return;
    }

    CMutexAutoLock cma(*this);
#if __cplusplus < 201103L
    std::map<uint32_t, struct log_stateaware_info>::iterator it;
#else
	std::unordered_map<uint32_t, struct log_stateaware_info>::iterator it;
#endif
    it = sa_info.find(hash);
    if (it != sa_info.end()) {
        // check for suppression criteria.
        struct log_stateaware_info &info = (*it).second;
        do {
            if (strhash != info.hash) {
                needlog = true;
                reason = 1;
                break;
            }
            if (sa_max_suppress_repetitions_
                && (info.supressed_cnt >= sa_max_suppress_repetitions_)) {
                needlog = true;
                reason = 2;
                break;
            }
            if (sa_max_time_suppress_
                && ((info.last_seen + sa_max_time_suppress_) <= now)) {
                needlog = true;
                reason = 3;
                break;
            }
        } while (0);

        if (!needlog) {
            // supress message.
            info.supressed_cnt++;
            return;
        }

        // print message and update info
        std::stringstream ss2;
        if (reason == 1) {
            if (!info.supressed_cnt) {
                reason = 0;
            } else {
                ss2 << "note: " << info.supressed_cnt
                    << " messages in this context have been suppressed.";
            }
        }
        if (reason == 2) {
            ss2 << info.supressed_cnt
                << " duplicate messages have been suppressed";
        }
        if (reason == 3 && info.supressed_cnt) {
            ss2 << "repeating after " << info.supressed_cnt
                << " duplicate messages have already been suppressed for "
                << now - info.last_seen << " seconds";
        } else {
            reason = 0;
        }

        info.hash = strhash;
        info.supressed_cnt = 0;
        info.last_seen = now;

        cma.unlock();
        (*this) << ss;
        if (reason) (*this) << ss2;
        return;
    }

    // data not in map.
    // means we need to print it and create an entry.
    struct log_stateaware_info newinfo;
    newinfo.hash = strhash;
    newinfo.supressed_cnt = 0;
    newinfo.last_seen = now;
    sa_info[hash] = newinfo;
    cma.unlock();
    (*this) << ss;
    return;
}

/// Forget the history for one stateaware log entry
/// (the next one will be issued anyway)
bool ILogger::sa_forgethistory(int32_t hash) {
    CMutexAutoLock cma(*this);
    return sa_info.erase(hash);
}

/// Forget the history for all stateaware log entries
/// (that means complete reset)
void ILogger::sa_forgethistory() {
     CMutexAutoLock cma(*this);
     sa_info.clear();
 }


ILogger::~ILogger()
{
    // TODO Auto-generated destructor stub
}

#endif
