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

/** \file CValue.h
 *
 *  Created on: May 14, 2009
 *      Author: tobi
 *
 * Template-Class for the concrete Values.
 *
 * CValue is used to story any type as data.
 * It also offers type check to ensure that the cast will be ok.
 *
 *
 */

#ifndef CVALUEX_H_
#define CVALUEX_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "IValue.h"

#include <iostream>
#include <sstream>
#include <boost/date_time/posix_time/posix_time.hpp>

/** This helper class allows type-identificatino without RTTI.
 * See http://ciaranm.wordpress.com/2010/05/24/runtime-type-checking-in-c-without-rtti/
 * for the idea.
 * Basically, MagicNumbers is a singleton to supply unique numbers and
 * the templated function will keep the number same for each type. */
class MagicNumbers
{

protected:
    static int next_magic_number() {
        static int magic = 0;
        return magic++;
    }

    template<typename T_>
    static int magic_number_for() {
        static int result(next_magic_number());
        return result;
    }

    template<class T>
    friend class CValue;

};

/** Generalized storage for data.
 * A CValue stores one information, regardless of the type.*/
template<class T>
class CValue : public IValue
{
public:

    CValue() {
        IValue::type_ = MagicNumbers::magic_number_for<T>();
    }

    CValue(const T &set) {
        IValue::type_ = MagicNumbers::magic_number_for<T>();
        value = set;
        timestamp = boost::posix_time::second_clock::local_time();
    }

    /// Serves as a virtual copy constructor.
    virtual CValue<T>* clone() {
        return new CValue<T>(*this);
    }


    void Set(T value, boost::posix_time::ptime timestamp = boost::posix_time::second_clock::local_time()) {
        this->timestamp = timestamp;
        this->value = value;
    }

    T Get(void) const {
        return value;
    }

    virtual void operator=(const T& val) {
        timestamp = boost::posix_time::second_clock::local_time();
        value = val;
    }

    virtual void operator=(const CValue<T> &val) {
        timestamp = val.GetTimestamp();
        value = val.Get();
    }

    virtual boost::posix_time::ptime GetTimestamp(void) const {
        return timestamp;
    }

    virtual operator std::string() {
        std::stringstream ss;
        ss << value;
        return ss.str();
    }

    /** Static interface function to determine at runtime the type of the CValue
     * object.
     * Usage example:
     * CValue<int> cv_int;
     * IValue *iv1 = &cv_int;
     * cout << CValue<int>::IsType(iv1);*/
    static bool IsType(IValue *totest) {
        if (MagicNumbers::magic_number_for<T>() == totest->GetInternalType()) {
            return true;
        }
        return false;
    }

    template <typename U>
    static IValue* Factory() {
        return new CValue<U>;
    }

private:
    T value;
    boost::posix_time::ptime timestamp;

};

class CValueFactory
{
public:
    template <typename T>
    static IValue* Factory() {
        return new CValue<T>;
    }
};



#endif /* CVALUEX_H_ */
