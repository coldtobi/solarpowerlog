/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

 Copyright (C) 2009-2015 Tobias Frost

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

/** \file CHTMLWriter.cpp
 *
 *  Created on: Dec 20, 2009
 *      Author: tobi
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "porting.h"
#endif

#ifdef HAVE_FILTER_HTMLWRITER

#include <fstream>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include "DataFilters/HTMLWriter/CHTMLWriter.h"

#include "configuration/Registry.h"
#include "configuration/CConfigHelper.h"
#include "configuration/ConfigCentral/CConfigCentral.h"
#include "interfaces/CWorkScheduler.h"

#include "Inverters/Capabilites.h"
#include "Inverters/interfaces/ICapaIterator.h"
#include "patterns/CValue.h"
#include "configuration/ILogger.h"

#include "DataFilters/HTMLWriter/formatter/IFormater.h"
#include <string>


static const char *Description_HTMLWriter_Intro =
"HTMLWriter Logger\n"
"As the name suggests already, this logger creates HTML file which then can be "
"served by a web server. To separate presentation from program logic, the page "
"will be generated using a template, so the look and feel can be freely customized.\n"
"The template library used for this is ctemplate: "
"https://libctemplate.sourceforge.net. This site has also some documentation "
"about the features and the syntax it expects in the template files. "
"Please see the example template for an idea how to create your own. "
"The logger is designed to work with the CVSLogger, so the HTML file can "
"utilize this as well. "
"To get a HTMLWriter, \"type\" below needs to "
"be set to "  FILTER_HTMLWRITER  " (as indicated below.)";

static const char *Description_HTMLWriter_writeevery =
"Defines how often the html page should be written in seconds. If the value "
"is unset (or the default value), the timing will derived from the timing of the data source: "
"The page will then written in the same interval as the inverter queries the data.\n"
"Note: If the source does not report the update frequency and this parameter is unset/0, "
"300 seconds is used instead.";

static const char *Description_HTMLWriter_htmlfile =
"The filename where to store the generated html file. "
"The parameter may contain a \"%s\", which will be replaced by the current date "
"in the format YYYY-MM-DD, generating a new file every day.";

static const char *Description_HTMLWriter_generatetemplate =
"To assist generating your own template files, the HTML writer can generated "
"a template showing all the parameters and their names which can be used in "
"writing the template.\n"
"Set this parameter to true to turn the feature on.";

static const char *Description_HTMLWriter_templatefile =
"This parameter sets the filename where the generated template will be stored. "
"Note that this parameter only defines the base of the filename, the program will "
"append the HTMLWriter's name and \".hmtl\" to it.";

static const char *Description_HTMLWriter_generatetemplate_dir =
"This parameter sets the directory where the template generator should store "
"its output.";

static const char *Description_HTMLWriter_Formatter =
"Sometimes the raw data in the Capabilities needs to be modified to be suitable "
"for the HTMLWriter. For this several small helper exists. "
"Those helper are declared with the formatters section.\n"
"The example below shows the syntax of the array.\n"
"The first column specifies the Capability the formatters should act on,\n"
"the second column specify the formatter to be used,\n"
"the third column allows to set the result to another template variable, if unset"
"the result will be reported with the capability name in column 1 instead, "
"overwritting that information.\n"
"The other columns are parameters to the formatter, see below for details."
;

static const char *Description_HTMLWriter_Formatter_example =
"(\n"
"##[ what_capability,\tformating_operation,\twhere_to_store,\tparameters ],\n"
"# [ \"CSVDumper::Filename\", \"stripwebroot\", \"\" , \"/var/www\" ],\n"
"# [ \"CSVDumper::LoggedCaps\", \"searchcvsentry\", \"powernow\", \"Current Grid Feeding Power\" ],\n"
"# [ \"CSVDumper::LoggedCaps\", \"searchcvsentry\", \"kwhtoday\", \"Energy produced today (kWh)\" ]\n"
"#)";

static const char* Description_HTMWriter_Frmter_searchwebroot =
"The formatter searchwebroot modifies a pathname in a capability to strip any "
"prefix from this path. As the name suggest this was design to aid serving "
"a datafiles, for example a CVS file from the CVS Logger "
"This is necessary as the URL-path will be different "
"from the complete filename stored on the server.\n"
"The searchwebroot formatter takes one parameter: the component of the path "
"to be removed. It has a default value of /var/www. "
"If it cannot find this component, it will not modify the path and report an "
"error to the logs.\n."
"As an example, the rule below will reformat the path /var/www/spl/<file> "
"to the relative one, which can be served from the webserver as "
"http://<domain>/spl/<file>.\n"
"example: [ \"CSVDumper::Filename\", \"stripwebroot\", \"\" , \"/var/www\" ]";

static const char* Description_HTMLWriter_Frmter_searchCVSentry =
"The formatter searchcvsentry works in conjuntion with the CVS Logger to allow "
"the template to easier find the data it is interested in the CVS file.\n"
"It enumerates in which field the Capability is currently logged and stores this "
"information -- the column the data is in -- in the given template variable.\n"
"Usually it will always act on CSVDumper::LoggedCaps, as this Capability is "
"reported from the CVS Logger. "
"The parameter contains the name of the capability to-be-looked-for.\n"
"example:\n"
"[ \"CSVDumper::LoggedCaps\", \"searchcvsentry\", \"powernow\", \"Current Grid Feeding Power\" ]\n";

// helper, because we do not want the multi map for the formatters sorted.
#warning unused, remove after resolving the switch to another container (see warning below)
struct unsortedmultimap
{
    //bool operator ()( const std::vector<std::string>,  const std::vector<std::string>)  const {
    bool operator ()(const std::string &, const std::string &) const
    {
        return false;
    }
};

/// local helpers: Bundle the cyclic-event calculation here...
/// the following function schedules the next CMD_CYCLIC Command, out of config
/// or out of a Capability
void CHTMLWriter::ScheduleCyclicEvent(enum Commands cmd)
{
	// set all members
	ICommand *ncmd = new ICommand(cmd, this);
	CCapability *c;
	struct timespec ts;

    if (_cfg_writevery <= 0.001 ) {
        c = GetConcreteCapability(CAPA_INVERTER_QUERYINTERVAL);
        if (c && CValue<float>::IsType(c->getValue())) {
            CValue<float> *v = (CValue<float> *)c->getValue();
            ts.tv_sec = v->Get();
            ts.tv_nsec = ((v->Get() - ts.tv_sec) * 1e9);
        } else {
            LOGINFO_SA(logger, __COUNTER__,
                "INFO: The associated inverter does not specify the "
                "interval of its querie. Defaulting to 300 seconds. Consider specifying the HTMLWriter's parameter writeevery");
            ts.tv_sec = 300;
            ts.tv_nsec = 0;
        }
    } else {
        ts.tv_sec = _cfg_writevery;
        ts.tv_nsec = ((long)(_cfg_writevery - ts.tv_sec)) * 1e9;
    }
	Registry::GetMainScheduler()->ScheduleWork(ncmd, ts);
}

CHTMLWriter::CHTMLWriter(const string & name, const string & configurationpath) :
	IDataFilter(name, configurationpath), updated(false), datavalid(false)
{
    _cfg_writevery = 0;
	_cfg_generate_template = false;
	updated = 0;
	datavalid = 0;

	// Schedule the initialization and subscriptions later...
	ICommand *cmd = new ICommand(CMD_INIT, this);
	Registry::GetMainScheduler()->ScheduleWork(cmd);

	// We do not anything on these capabilities, so we remove our list.
	// any cascaded filter will automatically use the parents one...
	CCapability *c = IInverterBase::GetConcreteCapability(
			CAPA_INVERTER_DATASTATE);
	CapabilityMap.erase(CAPA_INVERTER_DATASTATE);
	delete c;

	// Also we wont fiddle with the caps requiring our listeners to unsubscribe.
	// They also should get that info from our base.
	c = IInverterBase::GetConcreteCapability(CAPA_CAPAS_REMOVEALL);
	CapabilityMap.erase(CAPA_CAPAS_REMOVEALL);
	delete c;

}

CHTMLWriter::~CHTMLWriter()
{
	// TODO Auto-generated destructor stub
}

bool CHTMLWriter::CheckConfig()
{
#warning this is incomplete! does not check the reformatter etc.

	std::auto_ptr<CConfigCentral> cc(getConfigCentralObject(NULL));

	bool fail = cc->CheckConfig(logger, configurationpath);

    if (!fail && !base) {
        LOGERROR(logger, "Cannot find datassource with the name "
            << _datasource);
        fail = true;
    }

    size_t s;
    s = _cfg_gen_template_dir.length();
    if (s  && _cfg_gen_template_dir[s - 1] != '/') {
        _cfg_gen_template_dir += '/';
    }

	return !fail;
}

void CHTMLWriter::Update(const IObserverSubject *subject)
{
	// note: the subject must be a CCapability here.
	// to avoid neverending casting we do that once.
	CCapability *parentcap = (CCapability *) subject;

	// check for the mandatory Capas now, as they might require
	// immediate actions.
	if (parentcap->getDescription() == CAPA_CAPAS_REMOVEALL) {
		CCapability *ourcap;
		// forward the notification.
		// but -- to be nice -- update the value first
		ourcap = IInverterBase::GetConcreteCapability(CAPA_CAPAS_REMOVEALL);
		assert (ourcap);
		assert (CValue<CAPA_CAPAS_REMOVEALL_TYPE>::IsType(ourcap->getValue()));
		assert (CValue<CAPA_CAPAS_REMOVEALL_TYPE>::IsType(parentcap->getValue()));

		CValue<bool> *a, *b;
		a = (CValue<bool> *) (ourcap->getValue());
		b = (CValue<bool> *) (parentcap->getValue());
		*a = *b;
		ourcap->Notify();

		CheckOrUnSubscribe(false);
		return;
	}

	if (parentcap->getDescription() == CAPA_CAPAS_UPDATED) {
		CCapability *c = IInverterBase::GetConcreteCapability(
				CAPA_CAPAS_UPDATED);
		*(CValue<bool> *) c->getValue()
				= *(CValue<bool> *) parentcap->getValue();
		c->Notify();
		// there are new caps, but currently we won't do anything with it
		// later we want to check the list if we have attach ourself to the
		// listernes.
		return;
	}

	if (parentcap->getDescription() == CAPA_INVERTER_DATASTATE) {
		datavalid = ((CValue<bool> *) parentcap->getValue())->Get();
		return;
	}

	if (!updated) {
		ICommand *cmd = new ICommand(CMD_UPDATED, this);
		Registry::GetMainScheduler()->ScheduleWork(cmd);
	}
	updated = true;

}

void CHTMLWriter::ExecuteCommand(const ICommand *cmd)
{

	switch (cmd->getCmd())
	{

	case CMD_INIT:
	{
		string tmp;
		CConfigHelper cfghlp(configurationpath);
		CCapability *c;

		// Subscribe to this->base inverter, all the required ones...
		assert(base);
        c = base->GetConcreteCapability(CAPA_CAPAS_UPDATED);
        assert(c); // this is required to have....
        c->Subscribe(this);

        c = base->GetConcreteCapability(CAPA_CAPAS_REMOVEALL);
        assert(c);
        c->Subscribe(this);

        c = base->GetConcreteCapability(CAPA_INVERTER_DATASTATE);
        assert(c);
        c->Subscribe(this);

		ScheduleCyclicEvent(CMD_CYCLIC);
	}
		break;

	case CMD_CYCLIC:
		DoCyclicCmd(cmd);
		ScheduleCyclicEvent(CMD_CYCLIC);
	break;

	case CMD_UPDATED:
		// prepared for the next version.
		// "do on update mode"
		break;
	}

}

void CHTMLWriter::DoCyclicCmd(const ICommand *)
{
	ofstream fs;

	// C-Template vars
	TMPL_varlist *tmpl_looplist = NULL; /**< list for the inverter loop */
	TMPL_loop *tmpl_loop = NULL; /**< loop */
	TMPL_varlist *tmpl_list = NULL; /**< list for generationg the out */

	if (!datavalid) {
		return;
	}

	if (_cfg_generate_template) {
		std::string s;
		s = _cfg_gen_template_dir + _cfg_name + ".html";
		fs.open(s.c_str(), fstream::out | fstream::trunc | fstream::binary);

#ifdef HAVE_WIN32_API
		if (fs.fail()) {
			fs.clear();
			fs.open(s.c_str(), fstream::out | fstream::app | fstream::binary);
		}
#endif
		if (fs.fail()) {
			LOGWARN(logger,"Template-Assistant: Failed to open file: " << s);
			fs.close();
		}

		fs << "<html><head></head><body><h1>This are the variables known to "
			"the template system: </h1>" << endl << "<table border=1>"
			"<tr><th>Name of the Attribute</th><th>Current Value</th></tr>";
	}

	// Get the configuration to the formatting specs and map them
#warning make this only once; it is also not config-checked!
#warning note, this multimap is sorted; switch to other container like vector to avoid this \
	 as the formatters should be "ordered" -- see the unused struct unsortedmultimap above

	multimap<std::string, std::vector<std::string> >
			formattermap;

	std::string s = configurationpath + ".formatters";
	std::string s1, s2;
	CConfigHelper formatters(s);

	for (int i = 0;; i++) {
//		LOG_TRACE(logger, "In formatter loop: i="<<i);
//
		if (formatters.GetConfigArray(i, 0, s1) && formatters.GetConfigArray(i,
				1, s2)) {

			// LOG_TRACE(logger,"s1="<<s1 << " s2=" << s2);

			// We store the information this way:
			// we iterate over the configuration array and make a vector out
			// of it.
			// so the vector will look like:
			// [0] formatter name
			// [1] target template variable (maybe empty)
			// [2] parameter for formatter
			// [3...] other parameters for the formatter
			std::vector<std::string> vs;
			vs.push_back(s2); // we already got the formatter's name
			int j;
			j = 2;
			while (formatters.GetConfigArray(i, j++, s2)) {
				vs.push_back(s2);
			}

			formattermap.insert(pair<std::string, std::vector<std::string> > (
					s1, vs));

			LOGTRACE(logger, "Inserting formatter spec <" << s1 << ',' << vs[0] << '>');
		} else {
			break;
		}
	}

	// reminder: this first version only attaches one inverter!
	// Step one:
	// loop over all capabilites and add it to the list

	// Add the number of the inverter to the exported vars, as the snippets
	// probably want to do something special on the first one only.
	// TODO FIXME currently, there is only one. So we make this dynamic later.

	tmpl_looplist = TMPL_add_var(tmpl_looplist, "iteration", "0", NULL);
	if (_cfg_generate_template) {
		fs << "<tr><td> iteration </td><td> " << "0" << " </td>\n";
	}

	auto_ptr<ICapaIterator> cit(GetCapaNewIterator());
	while (cit->HasNext()) {
		multimap<std::string, vector<std::string> >::iterator it;
		pair<string, CCapability*> cappair = cit->GetNext();

		// get the value of the capability
		std::string value = *(cappair.second->getValue());
		// cache the name of the capability
		std::string templatename = cappair.first.c_str();

		//	 LOG_TRACE(logger, "Capability: " << cappair.first << "\tValue: "<< value);
		// TODO Rip this part into its own function -- this way, we can also do some daisy-chain
		// formatting: Modify value x to value y, modify value y to value z ....


#if 0
		// debug code: dump the multimap find results.
		for (it = formattermap.find(cappair.first); it != formattermap.end(); it++) {

		LOGTRACE(logger, "***** " << templatename <<": found 1st=" << (*it).first << " 2nd " << (*it).second[0]);

		}
#endif

		for (it = formattermap.find(cappair.first); it != formattermap.end(); it++) {
			IFormater *frmt;

			// the multimap returns everything after the first result
			// so we have to recheck if we really want this result.
			// FIXME the multimap seems not to be best for the task, so maybe
			// code should be reworked to use another container.
			if (cappair.first != (*it).first ) break;

			string formatter_to_create = (*it).second[0];
			LOGTRACE(logger, "reformatting " << templatename << " with a " << (*it).second[0] );

			if ((frmt = IFormater::Factory(formatter_to_create))) {

				if (!frmt->Format(value, value, (*it).second)) {
					LOGERROR(logger,"Could not reformat " << cappair.first <<
							": Formatter reported error.");
				}

				// check if we should store the result to another template
				// variable not the original one.
				if ((*it).second.size() > 2 && (*it).second[1] != "") {
					// yes, store the result to the new template var.
					tmpl_looplist = TMPL_add_var(tmpl_looplist,
							(*it).second[1].c_str(), value.c_str(), NULL);

					if (_cfg_generate_template) {
						fs << "<tr><td> " << (*it).second[1].c_str() <<
								"<br/><p style=\"font-size:x-small\">reformatted from "
								<< templatename <<
								"</p></td><td> " << value
								<< " </td>\n";
					}

					// in this case, restore the original value before checking
					// out a possible next formatter.
					value = *(cappair.second->getValue());
				}

				delete frmt;
			} else {
				LOGERROR(logger,"Failed to create formatter " << formatter_to_create );
			}
		}

		tmpl_looplist = TMPL_add_var(tmpl_looplist, templatename.c_str(),
				value.c_str(), NULL);

		if (_cfg_generate_template) {
			fs << "<tr><td> " << cappair.first << " </td><td> " << value
					<< " </td>\n";
		}
	}

	// adding the loop entry to the loop varlist.
	tmpl_loop = TMPL_add_varlist(tmpl_loop, tmpl_looplist);
	tmpl_looplist = NULL;

	// (here, we would add the other inverters linked to this story)

	// now adding all the stuff to the main list.
	tmpl_list = TMPL_add_loop(tmpl_list, "inverters", tmpl_loop);

	if (_cfg_generate_template) {
		fs << "<tr><th>HTMWriter Capabilities</th>"
		      "<th>(these are outside of the loop)</th></tr>" << endl;
	}
	map<string, CCapability*>::const_iterator it;
	for (it = CapabilityMap.begin(); it != CapabilityMap.end(); it++) {
		pair<string, CCapability*> cappair = *it;
		std::string tmpstring = *(cappair.second->getValue());

		tmpl_list = TMPL_add_var(tmpl_list, cappair.first.c_str(),
				tmpstring.c_str(), NULL);

		if (_cfg_generate_template) {
			fs << "<tr><td> " << cappair.first << " </td><td> " << tmpstring
					<< " </td>\n";
		}

	}

	// we add some others here....
	tmpl_list = TMPL_add_var(tmpl_list, "spl_version", PACKAGE_VERSION, NULL);

	// template assistance is now done, so close the file.
	if (_cfg_generate_template) {
		fs << "</table></body></html>";
		fs.close();
	}

	// okay, now open a file in memory to catch template errors.
	// (if supported at least)
	FILE *errfile = NULL;
	FILE *out = NULL;
#ifdef HAVE_OPEN_MEMSTREAM
	char *ptr = NULL;
	size_t size = 0;
#endif

#ifdef HAVE_OPEN_MEMSTREAM
	errfile = open_memstream(&ptr, &size);
#else
#warning Note: opem_memstream not available on this platform. Will not be able to tell you template errors
#endif
	// generate filename of the output file
	if (_cfg_html_file.find("%s") != std::string::npos) {
		boost::gregorian::date today(boost::gregorian::day_clock::local_day());
		char buf[_cfg_html_file.size() + 10]; //note: the %s will be removed, so +10 is enough.
		int year = today.year();
		int month = today.month();
		int day = today.day();

        snprintf(buf, sizeof(buf) - 1, "%s%04d-%02d-%02d%s",
            _cfg_html_file.substr(0, _cfg_html_file.find("%s")).c_str(), year,
            month, day, _cfg_html_file.substr(_cfg_html_file.find("%s") + 2,
            string::npos).c_str());

		out = fopen(buf, "w+");
		if (out == 0) {
			LOGERROR(logger, "Could not open filename "<< buf);
		}

	} else {
		out = fopen(_cfg_html_file.c_str(), "w+");
		if (out == NULL) {
			LOGERROR(logger, "Could not open filename "<< _cfg_html_file);
		}
	}

	if (out && -1 == TMPL_write(_cfg_template_file.c_str(), NULL, NULL, tmpl_list,
			out, errfile)) {
		// error while writing. lets examine errfile.
		// we need first to close the file to update ptr and size.
#ifdef HAVE_OPEN_MEMSTREAM
		fclose(errfile);
		errfile = NULL; // avoid closing a 2nd time later.
		LOGERROR(logger, "Error while writing html file."
				" The template library reported this: " << ptr);
#else
		// esp. for the cygwin 1.5 port, we cannot tell the reason why it happened.
		// patches are welcome to open a temporary file on disk instead and then
		// (when cygwin 1.7 comes out, this is no longer an issue: They implemented
		// the GNU extended syscall)
		LOGERROR(logger, "Error while writing html file (template error)");
#endif

	} else if (out) {
		LOGTRACE(logger, "Done writing HTML File. Wrote " << ftell(out) << " Bytes");
	}

	// cleanup./
	if (tmpl_list)
		TMPL_free_varlist(tmpl_list);
	if (out)
		fclose(out);
#ifdef HAVE_OPEN_MEMSTREAM
	if (errfile)
		fclose(errfile);
	if (ptr)
		free(ptr);
#endif
}

void CHTMLWriter::CheckOrUnSubscribe(bool subscribe)
{
    assert(base);
	CCapability *cap = base->GetConcreteCapability(CAPA_INVERTER_DATASTATE);
	if (cap)
		cap->SetSubscription(this, subscribe);
}

CConfigCentral* CHTMLWriter::getConfigCentralObject(CConfigCentral *parent)
{
    if (!parent) parent = new CConfigCentral;

    (*parent)
    (NULL, Description_HTMLWriter_Intro);

    parent = IDataFilter::getConfigCentralObject(parent);
    assert(parent);

    (*parent)
    ("writevery", Description_HTMLWriter_writeevery,
        _cfg_writevery, 0.0F , 0.0F ,FLT_MAX)
    ("htmlfile", Description_HTMLWriter_htmlfile, _cfg_html_file)
    ("generate_template", Description_HTMLWriter_generatetemplate,
        _cfg_generate_template, false)
    ("templatefile", Description_HTMLWriter_templatefile,
        _cfg_template_file, std::string("htmltemplate"))
    ("generate_template_dir", Description_HTMLWriter_generatetemplate_dir,
        _cfg_gen_template_dir, std::string("/tmp/"))
     ;

    // The following entries cannot be auto-checked, as too complex for
    // ConfigCentral.
    (*parent)
    // formatters
    ("formatters", Description_HTMLWriter_Formatter,
        Description_HTMLWriter_Formatter_example)
    // and some docs for the formatter types and their parameters
    ("searchwebroot formatter", Description_HTMWriter_Frmter_searchwebroot)
    ("searchcvsentry", Description_HTMLWriter_Frmter_searchCVSentry);

    parent->SetExample("type", std::string(FILTER_HTMLWRITER), false);

    return parent;
}


#endif
