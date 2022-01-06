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

/*
 * CHTMLWriter.h
 *
 *  Created on: Dec 20, 2009
 *      Author: tobi
 */

#ifndef CHTMLWRITER_H_
#define CHTMLWRITER_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_FILTER_HTMLWRITER

#include "DataFilters/interfaces/IDataFilter.h"

extern "C" {
#include "ctemplate/ctemplate.h"
}

#include "Inverters/BasicCommands.h"

/** Writes data prepared by other plugins / inverters
 * to a template, which can be HTML, for example
 *
 * This plugin does load a template, and replaces magic
 * values with the ones from Capabilites.
 *
 * It also can combine them from seveal inverters.
 * (planned)
 *  */

class CHTMLWriter: public IDataFilter
{
public:
	CHTMLWriter(const string &name, const string & configurationpath);

	virtual ~CHTMLWriter();

	virtual bool CheckConfig();

	virtual void Update(const IObserverSubject *subject);

	virtual void ExecuteCommand(const ICommand *cmd);

	enum Commands
	{
		CMD_INIT = BasicCommands::CMD_USER_MIN,
		CMD_UPDATED,
		CMD_CYCLIC,
	};

    virtual CConfigCentral* getConfigCentralObject(CConfigCentral *parent);

private:

	void CheckOrUnSubscribe( bool subscribe = true );

	// Configuration cache
	float _cfg_writevery;
	bool _cfg_generate_template;
	std::string _cfg_gen_template_dir;
	std::string _cfg_html_file;
	std::string _cfg_template_file;
	std::string _cfg_name;

	bool updated;
	bool datavalid;

	/// Helper: Generated the cyclic ICommand cmd and attach it.
	void ScheduleCyclicEvent(enum CHTMLWriter::Commands cmd);

	/// DoCyclicCmd -- makes the page.
	void DoCyclicCmd(const ICommand *);

};

#endif

#endif /* CHTMLWRITER_H_ */
