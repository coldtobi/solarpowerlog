/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

Copyright (C) 2011-2012 Tobias Frost

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


/** \file CDebugHelper.cpp
 *
 *  Created on: 30.12.2011
 *      Author: tobi
 */

#include "CDebugHelper.h"
#include "configuration/Registry.h"

CDebugHelperCollection::~CDebugHelperCollection()
{
    if (registered) Registry::Instance().RemoveDebugCollection(this);

    std::list<IDebugObject*>::iterator it = this->dobjlist.begin();
    while (it != dobjlist.end()) {
        // IDebugObject *p =
        delete (*it);
        it++;
    }
}

void CDebugHelperCollection::Register(IDebugObject *dobj) {
    dobjlist.push_back(dobj);
    if (!registered) {
        Registry::Instance().AddDebugCollection(this);
        registered = true;
    }
}

void CDebugHelperCollection::Unregister(IDebugObject *dobj) {
    dobjlist.remove(dobj);
}

void CDebugHelperCollection::Dump() {
    std::cerr << "CONTEXT " << context << std::endl;
    std::list<IDebugObject*>::iterator it = this->dobjlist.begin();
    while (it != dobjlist.end()) {
        // IDebugObject *p =
        std::cerr << "\t"; (*it)->Dump();
        it++;
    }
}

IDebugObject::~IDebugObject()
{
}



