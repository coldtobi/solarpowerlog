/* ----------------------------------------------------------------------------
   solarpowerlog
   Copyright (C) 2009  Tobias Frost

   This file is part of solarpowerlog.

   Solarpowerlog is free software; However, it is dual-licenced
   as described in the file "COPYING".

   For this file (solarpowerlog.cpp), the license terms are:

   You can redistribute it and/or modify it under the terms of the GNU
   General Public License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This programm is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this proramm; if not, see
   <http://www.gnu.org/licenses/>.
   ----------------------------------------------------------------------------
*/

#include <vector>
#include <string>
#include <iostream>
#include "configuration/Registry.h"

using namespace std;


// Debug Code dumpster....


/** Just dump the read config to cout.... (without values, as for these one must know the type  forehand )

 Use with:
 [code]
	libconfig::Setting &set = Registry::Configuration()->getRoot();
	if (set.getName()) cout << set.getName(); else cout << "anonymous";
	cout << " has the Path \"" << set.getPath() << "\" and Type " <<
		set.getType() << " and has a Lenght of " <<  set.getLength() << endl;

	DumpSettings(set);
 [/code]
*/
void DumpSettings(libconfig::Setting &set)
{

	if (set.getPath() != "" ) cout << set.getPath() ;

	if (!set.getName()) {
		 cout << "(anonymous) " ;
	}

	if (set.isAggregate()) {cout << "\t is aggregate" ;   };
	if (set.isArray()) { cout << "\t is array ";      }
	if (set.isGroup()) { cout << "\t is group ";  }
	if (set.isList()) { cout << "\t is list ";    }
	//if (set.getPath() != "" ) cout << set.getPath() << "." ;
	if (set.isNumber()) cout << " \t is number ";
	if (set.isScalar())cout << " \t is scalar ";

	cout <<  endl;

	for ( int i=0; i < set.getLength() ; i ++) {

			libconfig::Setting &s2 = set[i];
			DumpSettings(s2);
		}
}


static const char *required_sections[] =
{
		"application",
		"inverter",
		"logger"
};


int main() {

	bool error_detected = false;

	/** Loading configuration file */
	cout << "Generating Registry" << endl;
	// TODO avoid hardcoded filename. Get it from parameters.
	if (! Registry::Instance().LoadConfig("solarpowerlog.conf"))	{
		cerr << "Could not load configuration " << endl;
		return 1;
	}

	// Note: As a limitation of libconfig, one cannot create the configs
	// structure.
	// Therefore we check here for the basic required sections and abort,
	// if one is not existing.
	libconfig::Config *cfg = Registry::Configuration();
	libconfig::Setting &rt = cfg->getRoot();

	for ( unsigned int i = 0; i < sizeof(required_sections) / sizeof (char*) ; i++ ) {
		if (! rt.exists(required_sections[i])) {
			cerr << " Configuration Check: Required Section " <<
				required_sections[i] << " missing" << endl;
			error_detected = true;
		}
	}

	if(error_detected) return(1);






	return 0;
}
