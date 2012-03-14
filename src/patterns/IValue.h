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

/** \file IValue.h
 *
 *  \date May 13, 2009
 *   \Author: Tobias Frost (coldtobi)
 */

#ifndef ICAPABILITY_H_
#define ICAPABILITY_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>

/** IValue is the interface to arbitrary value storage.
 *
 * It is supposed to be derived, and the derived class is responsible for
 * type-correct storage.
 *
 * In this interface, also the factory is embedded to create the concrete
 * values.
 *
 * \ingroup factories
 */
class IValue
{
public:

	/** This enumeration specifies the type of the storage.
	 * It can be used as parameter for the factory. */
	enum factory_types
	{
		bool_type, ///< boolean storage
		int_type, ///< integer (signed)
		float_type, ///< float storage
		string_type
	///< string type
	};

	/** Factory method to generate desired concrete Value */
	static IValue* Factory( const factory_types typedescriptor );

	/** Interface method to check the type of the value */
	virtual factory_types GetType( void ) const;

	// virtual std::string string GetValueAsString();

public:
	/** Inteface method for easier transfer to strings. */
	virtual operator std::string() = 0;

protected:
	IValue();
public:
	virtual ~IValue();

protected:
	factory_types type;

};

#endif /* ICAPABILITY_H_ */
