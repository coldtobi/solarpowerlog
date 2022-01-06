/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

Copyright (C) 2010-2012 Tobias Frost

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
 * CFormaterWebRootStrip.h
 *
 *  Created on: Jan 4, 2010
 *      Author: tobi
 */

#ifndef CFORMATERWEBROOTSTRIP_H_
#define CFORMATERWEBROOTSTRIP_H_

#include "DataFilters/HTMLWriter/formatter/IFormater.h"

/// Formater for: Strip a webroot-path from the front of a capability
class CFormaterWebRootStrip: public IFormater
{

public:
	virtual ~CFormaterWebRootStrip();

	virtual bool Format(const std::string &what, std::string &store,
			const std::vector<std::string> &parameters);

protected:
	// make the factory class my friend to instantiate the object.
	friend class IFormater;
	CFormaterWebRootStrip();

};

#endif /* CFORMATERWEBROOTSTRIP_H_ */
