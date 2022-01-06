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

/** \file IValue.h
 *
 *  \date May 13, 2009
 *  \Author: Tobias Frost (coldtobi)
 */

#ifndef IVALUE_H_
#define IVALUE_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>

#include <boost/date_time/posix_time/posix_time.hpp>

/** IValue is the interface to arbitrary value storage.
 *
 * It is supposed to be derived, and the derived class is responsible for
 * type-correct storage.
 *
 * \ingroup factories
 */
class IValue
{
public:

protected:
    template<class T>
    friend class CValue;

public:
    int GetInternalType(void) const
    {
        return _type;
    }

    /** Interface method for easier transfer to strings. */
    virtual operator std::string() = 0;

    /// Serves as a virtual copy constructor.
    virtual IValue* clone() = 0;

    virtual bool operator==(IValue &v) = 0;
    virtual bool operator!=(IValue &v) = 0;

    virtual IValue& operator=(const IValue &v) = 0;

    /** Determine if the data is valid
     *
     * \note the data will be set to valid automatically on
     * every call to Set or using the = operator.
     *
     * @return true if valid, false if invalid
     */
    virtual bool IsValid(void) const
    {
        return _valid;
    }

    /** Invalidate datastate
     *
     * record data as invalid.
     */
    virtual void Invalidate(void)
    {
        _valid = false;
    }

    /*** Get the timestamp associated with the last set
     *
     * @return timestamps. If never set, boost::posix_time::not_a_date_time
     */
    virtual boost::posix_time::ptime GetTimestamp(void) const {
        return _timestamp;
    }

    /*** Set the timestamp to be associated with the data (usually the last set)*
     *
     * @param newtime boost::posix_time::ptime representing the timestamp
     */
    virtual void SetTimestamp(const boost::posix_time::ptime &newtime)
    {
        _timestamp = newtime;
    }

protected:
    /// Constructor
    IValue(int type) :
        _type(type), _valid(false),
        _timestamp(boost::posix_time::min_date_time)
    { }

    /** protected setter to explicitly set validity. */
    virtual void SetValid(bool v = true) {
        _valid = v;
    }

public:
    virtual ~IValue() { }

private:
    /// Private to avoid accidental creation with default constructor.
    IValue() :
        _type(0), _valid(false), _timestamp(boost::posix_time::min_date_time)
    { }

    /// Internal typeid
    int _type;

    /// Data validty mark. (Set automatically with every set. use Invalidate() to
    /// void data
    bool _valid;

    /// Timestamp of last update.
    boost::posix_time::ptime _timestamp;
};

#endif /* IVALUE_H_ */
