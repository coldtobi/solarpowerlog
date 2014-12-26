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
/** \file ILogger.h
 *
 *  Created on: Jul 20, 2009
 *      Author: tobi
 *
 *  \section LoggingLevels Loggging Levels
 *
 *  Here some guideline to choose the right logging level to keep consistent
 *  throughout the program.
 *
 *  - TRACE: Very verbatim (debug purpose) infos, like protocol, telegramms,
 *    low level data .... This level should be used to gather every information
 *    that might be needed to debug problems... Milestones during execution
 *    also counts to this. The goal is to find out whats happening if debugging
 *    some rare problems.
 *  - DEBUG: "Regular" Debug infos, like tracepoints, unusual program flow
 *    detection, etc. Detected problems that are likely a programming problem....
 *  - INFO: Verbatim information targeted to the user, showing details of
 *    the program flow, but not too much details.
 *    (showing when talking to a inverter, ...)
 *  - WARN:  This level indicates a minor problems, caused by external events.
 *    Usually some functions might be temporary not available.
 *  - ERROR: The program cannot function under this circumstances. The feature
 *    imposed will not be available until the reason is fixed and the program
 *    restarted.
 *    The program can usually continue to execute, but with the limitations.
 *  - FATAL: The program detected a problem which makes it impossible to
 *    continue. The program will usually call abort() after FATAL.
 *
 * \section LoggingStateAware State-aware logging
 *
 * This new feature allows to keep track of already issued log messages, and
 * suppress duplicate messages. This feature is not used by default, there
 * are dedicated macros: They have the suffix _SA for state aware.
 *
 * Those macros take an additional parameter, which is supposed to be some hash
 * value to distinguish the logging location.
 * The hash value can be generated by e.g. __COUNTER__ or also from strings
 * using  the macro LOG_SA_HASH("somestring"). The nice thing about this macro
 * is that the hash is computed at compile time, so there is no runtime overhead
 * However, (to make that work, optimization needs to be turned on, for the gcc
 * -O1 seems to be sufficient.
 * Due to the compile-time-impact, __COUNTER__ should be preferred whenever
 * possible.
 *
 * NOTE: The compile-time hash must be constant. (They are compile-time anyway;
 * but the API doesn't enforce this -- if you bring a new constant every time
 * you call the macro, you'll have a nice memory leak as noone is cleaning
 * up after you.
 *
 * When a code location using the same hash emits identical log messages, the
 * log messages will be not printed but suppressed, when:
 * - the log message is identical
 * - the log message has not been repeated xx times (where xx is by default
 *   LOG_STATEWARE_REPEAT) This can be set per-Logger via
 *   setSaMaxSuppressRepeattions()
 * - the log message has not been xx seconds (xx per default
 *   LOG_STATEAWARE_TIME) -- set-able via setSaMaxSupressTime()
 *
 * The state-awareness is bound to the ILogger object and is also thread-safe.
 *
 * Implementation details: As already indicated, a hash is used to correlate
 * logging location with the suppression feature. To keep track of logging content,
 * a 32-bit hash is calculated from the string and stored, using sdbm algorithm,
 * The information about seen logging info is stored in a stdlib container:
 * Currently map, later maybe unordered_map, as this is faster in looking up
 * the hashes (but needs C++11)
 *
 * As this is only logging, hash-collisions are neglected, but probability is
 * low anyway: If the compile-time hash collides, only suppression will
 * most likely be not working, as *additionally* the hashes on the
 * strings would need to collide.
 *
 * A word on the compile-time: The hash-calculation has been configured for
 * strings up to 96 bytes. Larger strings will be truncated and if the truncated
 * string is equal, they will collide. This limitation is necessary as the
 * compile-time hash works with macro expansion and the needed time raises
 * exponentially with the limit. For example, while benchmarking, the
 * compile time for ILogger.cpp raised from 2 secs -> 4 secs -> 7 secs ->
 * 12 secs -> 90 secs for "no hash" -> 64 bytes -> 96 byts -> 128 bytes limit;
 * calculating 10 hashes of "ILogger::ILogger()" each.
 *
 *
 * Examples:
 *      LOG_WARN_SA(logger, __COUNTER__, "I do not want to repeat myself!");
 *
 *
 *      if (!xy_available) {
 *          LOG_WARN_SA(logger, LOG_SA_HASH("on_xy_avail", "XY is not available.");
 *          // (...)
 *      }
 *
 *      // 1000's line of code later or the second if even in another part of the class
 *      if (!xy_available) {
 *          LOG_WARN_SA(logger, LOG_SA_HASH("on_xy_avail", "XY is available.");
 *          // (...)
 *          }
 *
 *
 *      // use of __COUNTER__ to keep track
 *      const int sa_zz_hash = __COUNTER__;
 *      if (!zz_available) {
 *          LOG_WARN_SA(logger, sa_zz_hash, "ZZ is not available.");
 *          // 1000's line of code
 *      } else {
 *          LOG_WARN_SA(logger, sa_zz_hash, "ZZ is available.");
 *      }
 *
 *      // oneliner -- will log only if x changes.
 *      LOG_WARN_SA(logger, __COUNTER__, "The value of x is " << x);
 *
 */

#ifndef ILOGGER_H_
#define ILOGGER_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>
#include <ostream>
#include <stdint.h>

// unordered maps are not available without c++11
#if __cplusplus < 201103L
#include <map>
#else
#include <unordered_map>
#endif

#ifdef HAVE_LIBLOG4CXX
#include <log4cxx/logger.h>
#endif

#ifdef HAVE_LIBLOG4CXX

#include <boost/thread/mutex.hpp>

/// Define for default number of repeations before a message is repeated
#define LOG_STATEWARE_REPEAT  (100)
/// Define for default time before a already issued message is repeated
#define LOG_STATEWARE_TIME    (3600)

// Compile-Time-Hashing function for state-aware logging
// See http://www.chrissavoie.com/articles/15-research/14-hashing

// Eclipse is getting very slow with the recursive macros, so I excluded to
// index the macros.. This "hack" configures some replacement..
// (Eclipse sets HAVE_CONIG_H to "42", while configure won't)
#if (HAVE_CONFIG_H != 42)
#include "ILogger_hashmacro.h"
#else
#error eclipse hack! should not compile
#define LOG_SA_HASH(string) (__LINE__)
#endif

// the state-aware logging macros are only available for levels below ERROR.
#define LOGWARN_SA(logger, hash, message)   {\
        if ((logger).IsEnabled(ILogger::LL_WARN)) { \
            std::stringstream ss;\
            ss << message;\
            logger.Log_sa(hash,ss); \
        }\
    } while(0)


#define LOGINFO_SA(logger, hash, message)   {\
        if ((logger).IsEnabled(ILogger::LL_INFO)) { \
            std::stringstream ss;\
            ss << message;\
            logger.Log_sa(hash,ss); \
        }\
    } while(0)


#define LOGDEBUG_SA(logger, hash, message)   {\
        if ((logger).IsEnabled(ILogger::LL_DEBUG)) { \
            std::stringstream ss;\
            ss << message;\
            logger.Log_sa(hash,ss); \
        }\
    } while(0)


#define LOGTRACE_SA(logger, hash, message)    {\
        if ((logger).IsEnabled(ILogger::LL_TRACE)) { \
            std::stringstream ss;\
            ss << message;\
            logger.Log_sa(hash,ss); \
        }\
    } while(0)


#define LOGALL_SA(logger, hash, message)    {\
        if ((logger).IsEnabled(ILogger::LL_ALL)) { \
            std::stringstream ss;\
            ss << message;\
            logger.Log_sa(hash,ss); \
        }\
    } while(0)



#define LOGFATAL(logger, message)  do \
	{\
		if ((logger).IsEnabled(ILogger::LL_FATAL)) { \
			std::stringstream ss;\
			ss << message;\
			logger << ss;\
		}\
	} while(0)

#define LOGERROR(logger, message)  do \
	{\
		if ((logger).IsEnabled(ILogger::LL_ERROR)) { \
			std::stringstream ss;\
			ss << message;\
			logger << ss;\
		}\
	} while(0)

#define LOGWARN(logger, message)   do \
	{\
		if ((logger).IsEnabled(ILogger::LL_WARN)) { \
			std::stringstream ss;\
			ss << message;\
			logger << ss;\
		}\
	} while(0)

#define LOGINFO(logger, message)   do \
	{\
		if ((logger).IsEnabled(ILogger::LL_INFO)) { \
			std::stringstream ss;\
			ss << message;\
			logger << ss;\
		}\
	} while(0)

#define LOGDEBUG(logger, message)   do \
	{\
		if ((logger).IsEnabled(ILogger::LL_DEBUG)) { \
			std::stringstream ss;\
			ss << message;\
			logger << ss;\
		}\
	} while(0)

#define LOGTRACE(logger, message)   do \
	{\
		if ((logger).IsEnabled(ILogger::LL_TRACE)) { \
			std::stringstream ss;\
			ss << message;\
			logger << ss;\
		}\
	} while(0)

#define LOGALL(logger, message)   do \
	{\
		if ((logger).IsEnabled(ILogger::LL_ALL)) { \
			std::stringstream ss;\
			ss << message;\
			logger << ss;\
		}\
	} while(0)

#else

#define LOGWARN_SA(logger, hash, message)

#define LOGINFO_SA(logger, hash, message)

#define LOGDEBUG_SA(logger, hash, message)

#define LOGTRACE_SA(logger, hash, message)

#define LOGALL_SA(logger, hash, message)



#define LOGFATAL(logger, message) do { \
	cerr << message << endl;\
} while(0)

#define LOGERROR(logger, message) do { \
	cerr << message << endl;\
} while(0)

#define LOGWARN(logger, message)

#define LOGINFO(logger, message)

#define LOGDEBUG(logger, message)

#define LOGTRACE(logger, message)

#define LOGALL(logger, message)

#endif


#ifdef HAVE_LIBLOG4CXX

/// struct to keep state for stateaware logging .
struct log_stateaware_info {
    uint32_t hash;
    int supressed_cnt;
    time_t last_seen;
};

/** Interface for logging services
 *
 * This class is the interface to the underlying logging class.
 * (planned: log4cxx)
 *
 * Loggers can be attached to every object, and log4cxx allows to structure
 * them into a hierarchy.
 *
 * This class is responsible to give every object access to its own logger,
 * and extract the logger's configuration out of the configuration file.
 * (allowing to configure the log for each component individually)
 *
 * The class is intended to use a composition or by inheritance.
 *
 */
class ILogger : protected boost::mutex
{
public:

	enum level
	{
		LL_OFF = log4cxx::Level::OFF_INT,
		LL_FATAL = log4cxx::Level::FATAL_INT,
		LL_ERROR = log4cxx::Level::ERROR_INT,
		LL_WARN = log4cxx::Level::WARN_INT,
		LL_INFO = log4cxx::Level::INFO_INT,
		LL_DEBUG = log4cxx::Level::DEBUG_INT,
		LL_TRACE = log4cxx::Level::TRACE_INT,
		LL_ALL = log4cxx::Level::ALL_INT
	};

	/** Configure the logger with a name (to identify) ,
	 * the configuration string (for retrieving logger config)
	 * and a section (under what hierarchy to place the logger)
	 *
	 * \param name of the logger
	 * \param configurationpath where to retrieve the config
	 * \param section where to place the logger
	 *
	 */
	void Setup( const std::string &name, const std::string &configuration,
		const std::string& section );

	/** Adding a logger in a lower hierarchy level (below a parent object)
	 * by just specifying the parent.
	 *
	 * This logger will inheritate all settings by its parent, or, the XML
	 * file might configure it.
	 * */
	void Setup( const std::string &parent,
		const std::string &specialization );

	/** the default constructor set up logging with the root logger. */
	ILogger();

	virtual ~ILogger();

	/// Getter for loggername
	inline std::string getLoggername() const
	{
		return loggername_;
	}

	/// Modify the logger level (at runtime)
	void SetLoggerLevel(log4cxx::LevelPtr level);

	/** Check if a logging statement would go through and
	 * if so setup logging level.
	 *
	 * This function should be used before using the << operator, as
	 * this function will avoid calling all the ostream-operators,
	 * when indeed the logging level is below that one configured
	 * for the logger,
	 *
	 * The LOG_xxx macros will do that for you.
	 *
	 * Example:
	 * \code
	 * if (logger->IsEnabled(FATAL)) logger << "Fatal Error occured" <<endl;
	 * \endcode*/
	inline bool IsEnabled( int loglevel )
	{
		if (loglevel >= currentloggerlevel_) {
			currentlevel = loglevel;
			return true;
		} else
			return false;
	}

	/// set the next loglevel (the level which the app will log with the next time)
	/// usually this is not needed, as IsEnabled and LOG_xxx() do that for you
	///
	/// \param loglevel the level for subsequent logging.
	///
	/// \note: Please use LOG_xxx whenever possible.
	inline void SetLogLevel( int loglevel )
	{
		currentlevel = loglevel;
	}

	/// Log a string with the level.
	///
	/// \param loglevel level to log with
	/// \param log string to log
	/// \note when logging a static string, this might be more performant.
	/// than LOG_xxx.
	inline void Log( int loglevel, const std::string &log )
	{
		if (IsEnabled(loglevel))
			loggerptr_->log(log4cxx::Level::toLevel(loglevel),log);
	}

	/** provides the << operator for convenient logging (std::string version)
	 *
	 * \note the loglevel of the message has to be setup prior logging
	 *
	 * \note Use LOG_xxx() macros whenever possible.
	*/
	std::string & operator <<( std::string &os )
	{
		if (currentlevel >= currentloggerlevel_) {
			loggerptr_->forcedLog(log4cxx::Level::toLevel(
				currentlevel), os);
		}
		return os;
	}

	/** provides the << operator for convenient logging (stringstream version)
	 *
	 * \note the loglevel of the message has to be setup prior logging
	 *
	 * \note Use LOG_xxx() macros whenever possible.
	*/
	std::stringstream & operator <<( std::stringstream &os )
	{
		if (currentlevel >= currentloggerlevel_) {
			std::string s = os.str();
			loggerptr_->forcedLog(log4cxx::Level::toLevel(
				currentlevel), s);
		}
		return os;
	}

	/** State-aware logging
	 *
	 * This logging function will keep track on state-aware logging messages,
	 * and suppress those messages when the content has not been changed, or
	 * the message has either been repeated LOG_STATEWARE_REPEAT or not issued
	 * since LOG_STATEAWARE seconds.
	 * The
	*/
	void Log_sa(const int32_t hash, std::stringstream &ss);

	/// Get the max suppressions
    int getSaMaxSuppressRepetitions() const
    {
        return sa_max_suppress_repetitions_;
    }

    /** Set the maximum repetitions to suppress identical logs
     *
     *  @param saMaxTimeSuppress maximum time to suppress identical messages.
     * "0" disables the repeating due to repetitions.
     */
    void setSaMaxSuppressRepetitions(int saMaxSuppressRepetitions)
    {
        sa_max_suppress_repetitions_ = saMaxSuppressRepetitions;
    }

    /// Get the maximum time to suppress identical logs
    time_t getSaMaxSuppressTime() const
    {
        return sa_max_time_suppress_;
    }

    /** Set the maximum time suppressing identical logs
     *
     *  @param saMaxTimeSuppress maximum time to suppress identical messages.
     * "0" disables repeating due to time-constraints
     */
    void setSaMaxSuppressTime(time_t saMaxTimeSuppress)
    {
        sa_max_time_suppress_ = saMaxTimeSuppress;
    }

    /// Forget the history for one stateaware log entry
    /// (the next one will be issued anyway)
    bool sa_forgethistory(int32_t hash);

    /// Forget the history for all stateaware log entries
    /// (that means complete reset)
    void sa_forgethistory();

private:
	/// cache for the configuration string.
	std::string config_;

	/// cache for the loggers name.
	std::string loggername_;

	/// The logger of liblog4cxx...
	log4cxx::LoggerPtr loggerptr_;

	/// the last set log level from the application ("I want to log at this level next")
	int currentlevel;

	/// stores the logging level of the underlaying logger (cache)
	/// FIXME: Think about removing that one...
	/// should be queried from the logger, or at least updated when setting the level.
	int currentloggerlevel_;

	/// stateaware: repeat every this times when duplicates have found
	int sa_max_suppress_repetitions_;

    /// stateaware: repeat even when a duplicate after this time
    time_t sa_max_time_suppress_;

	/// Contains ths state-aware-logging data
	/// TODO with C++11 unordered_map would be faster, while theres the ifdef its untested.
#if __cplusplus < 201103L
	std::map<uint32_t,struct log_stateaware_info> sa_info;
#else
	std::unordered_map<int32_t,struct log_stateaware_info> sa_info;
#endif
};

#endif // HAVE_LIBLOG4CXX
#ifndef HAVE_LIBLOG4CXX
/** Interface for logging services
 *
 * This class is the interface to the underlying logging class.
 * (planned: log4cxx)
 *
 * Loggers can be attached to every object, and log4cxx allows to structure
 * them into a hierarchy.
 *
 * This class is responsible to give every object access to its own logger,
 * and extract the logger's configuration out of the configuration file.
 * (allowing to configure the log for each component individually)
 *
 * The class is intended to use a composition or by inheritance.
 *
 */

class ILogger /*: public std::ostream*/
{
public:

	enum level
	{
		LL_OFF = 0,
		LL_FATAL ,
		LL_ERROR ,
		LL_WARN ,
		LL_INFO ,
		LL_DEBUG ,
		LL_TRACE ,
		LL_ALL
	};
	/** Configure the logger with a name (to identify) ,
	 * the configuration string (for retrieving logger config)
	 * and a section (under what hierarchy to place the logger)
	 *
	 * \param name of the logger
	 * \param configurationpath where to retrieve the config
	 * \param sectin where to place the logger
	 *
	 */
	void Setup( const std::string &, const std::string &,
		const std::string&  ) { };

	/** Adding a logger in a lower hierarchy level (below a parent object)
	 * by just specifying the parent.
	 *
	 * This logger will inheritate all settings by its parent, or, the XML
	 * file might configure it.
	 * */
	void Setup( const std::string &,
			const std::string & ) {};

	/** the default constructor set up logging with the root logger. */
	ILogger() {};

	virtual ~ILogger() {};

	std::string getLoggername() const
	{
		return "";
	}

	/** Check if a logging statement would go through and
	 * if so setup logging level.
	 *
	 * This function should be used before using the << operator, as
	 * this function will avoid calling all the ostream-operators.
	 *
	 * Example:
	 * \code
	 * if (logger->IsEnabled(FATAL)) logger << "Fatal Error occured" <<endl;
	 * \endcode*/
	inline bool IsEnabled( int )
	{
		return false;
	}

	inline void SetLogLevel( int )
	{
	}

	inline void Log( int , std::string )
	{
	}

	std::string & operator <<( std::string &os )
	{
		return os;
	}

	std::stringstream & operator <<( std::stringstream &os )
	{
		return os;
	}

};
#endif // !HAVE_LIBLOG4CXX



#endif /* ILOGGER_H_ */
