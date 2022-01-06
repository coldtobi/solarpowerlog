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

    template<typename T>
    static int magic_number_for() {
        static int result(next_magic_number());
        return result;
    }

    template<typename T>
    friend class CValue;

};

/** Generalized storage for data.
 * A CValue stores one information, regardless of the type.*/
template<typename T>
class CValue : public IValue
{
public:
    CValue() : IValue(MagicNumbers::magic_number_for<T>()) {
        value = T();
    }

    CValue(const T &set) : IValue(MagicNumbers::magic_number_for<T>()) {
        value = set;
        SetTimestamp(boost::posix_time::second_clock::local_time());
        SetValid();
    }

    virtual ~CValue() {};

    /// Serves as a virtual copy constructor.
    virtual CValue<T>* clone() {
        return new CValue<T>(*this);
    }

    void Set(T value, boost::posix_time::ptime timestamp = boost::posix_time::second_clock::local_time()) {
        this->value = value;
        SetTimestamp(timestamp);
        SetValid();
    }

    T Get(void) const {
        return value;
    }

    virtual void operator=(const T& val) {
        value = val;
        SetTimestamp(boost::posix_time::second_clock::local_time());
        SetValid();
    }

    virtual void operator=(const CValue<T> &val) {
        value = val.Get();
        SetTimestamp(val.GetTimestamp());
        SetValid(val.IsValid());
    }

    virtual operator std::string() {
        std::stringstream ss;
        ss << value;
        return ss.str();
    }

    virtual bool operator==(IValue &v) {
        if (GetInternalType() == v.GetInternalType()) {
            CValue<T> &realv = (CValue<T>&)v;
            return (realv.Get() == Get());
        }

        throw std::bad_cast();
        return false;
    }

    virtual bool operator!=(IValue &v) {
         if (GetInternalType() == v.GetInternalType()) {
             CValue<T> &realv = (CValue<T>&)v;
             return (realv.Get() != Get());
         }
         throw std::bad_cast();
         return false;
     }

    virtual IValue& operator=(const IValue &v) {
        if (&v == this) return *this;

        if (this->IsType(&v)) {
            CValue<T> *rv = (CValue<T>*)&v;
            this->value = rv->value;
            SetTimestamp(rv->GetTimestamp());
            SetValid(rv->IsValid());
            return *this;
        }
        throw std::bad_cast();
        return *this;
    }

    /** Static interface function to determine at runtime the type of the CValue
     * object.
     * Usage example:
     * CValue<int> cv_int;
     * IValue *iv1 = &cv_int;
     * cout << CValue<int>::IsType(iv1);
     */
    static bool IsType(const IValue *totest) {
        if (MagicNumbers::magic_number_for<T>() == totest->GetInternalType()) {
            return true;
        }
        return false;
    }

private:
    T value;
};

// TODO check if factory really needed or substituted already by some other
// pattern
class CValueFactory
{
public:
    template <typename T>
    static IValue* Factory() {
        return new CValue<T>;
    }
};

#endif /* CVALUEX_H_ */
