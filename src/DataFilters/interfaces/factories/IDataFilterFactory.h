/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

 Copyright (C) 2009-2015 Tobias Frost

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

/** \file IDataFilterFactory.h
 *
 *  \date Jun 1, 2009
 *  \author Tobias Frost (coldtobi)
 */

#ifndef IDATAFILTERFACTORY_H_
#define IDATAFILTERFACTORY_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>

#ifdef HAVE_FILTER_DUMBDUMP
#define FILTER_DUMBDUMPER "DumpDumper"
#else
#define FILTER_DUMBDUMPER
#endif

#ifdef HAVE_FILTER_CSVDUMP
#define FILTER_CSVWRITER "CVSWriter"
#else
#define FILTER_CSVWRITER
#endif

#ifdef HAVE_FILTER_HTMLWRITER
#define FILTER_HTMLWRITER "HTMLWriter"
#else
#define FILTER_HTMLWRITER
#endif

#ifdef HAVE_FILTER_DBWRITER
#define FILTER_DBWRITER "DBWriter"
#else
#define FILTER_DBWRITER
#endif

class IDataFilter;

/** Factory for Data-Filters
 *
 *\ingroup factories
 */
class IDataFilterFactory
{
public:
    virtual IDataFilter *FactoryByName(const std::string &type,
        const std::string &name, const std::string &configurationpath);

	virtual IDataFilter* Factory( const std::string &configurationpath );


	IDataFilterFactory() {};
	virtual ~IDataFilterFactory() {};
};

#endif /* IDATAFILTERFACTORY_H_ */
