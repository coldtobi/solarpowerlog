/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

 Copyright (C) 2012-2015 Tobias Frost

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

/** \file CInverterSputnikSSeriesSimulator.cpp
 *
 *  Created on: June 07, 2012
 *
 *  Author: Tobias Frost
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_INV_SPUTNIKSIMULATOR

#include "configuration/Registry.h"
#include "configuration/CConfigHelper.h"

#include "CInverterSputnikSSeriesSimulator.h"

#include "patterns/ICommand.h"

#include "interfaces/CWorkScheduler.h"

#include "Inverters/Capabilites.h"
#include "patterns/CValue.h"

#include "configuration/ILogger.h"

#include "Inverters/SputnikEngineering/SputnikCommand/CSputnikCommand.h"
#include "Inverters/SputnikEngineering/SputnikCommand/CSputnikCommandSoftwareVersion.h"
#include "Inverters/SputnikEngineering/SputnikCommand/CSputnikCommandSYS.h"
#include "Inverters/SputnikEngineering/SputnikCommand/CSputnikCommandTYP.h"
#include "Inverters/SputnikEngineering/SputnikCommand/BackoffStrategies/CSputnikCmdBOOnce.h"
#include "Inverters/SputnikEngineering/SputnikCommand/BackoffStrategies/CSputnikCmdBOTimed.h"
#include "Inverters/SputnikEngineering/SputnikCommand/BackoffStrategies/CSputnikCmdBOIfSupported.h"
#include "Connections/factories/IConnectFactory.h"

#include <boost/algorithm/string.hpp>

#include <boost/random.hpp>

struct CInverterSputnikSSeriesSimulator::simulator_commands simcommands[] = {
        { "ADR", 1, new CValue<int>(1), 0, NULL, false },
        { "PAC", 0.5, new CValue<float>(42), 0, NULL, false },
        { "PDC", 0.5, new CValue<float>(42), 0, NULL, false },
        { "KHR", 1.0, new CValue<float>(42), 0, NULL, false },
        { "CAC", 1.0, new CValue<long>(42), 0, NULL, false },
        { "SYS", 1.0, new CValue<int>(20004), 1, new CValue<int>(0), false },
        { "TYP", 1.0, new CValue<int>(65534), 0, NULL, false },
        { "BDN", 1.0, new CValue<int>(24), 0, NULL, false },
        { "SWV", 1.0, new CValue<int>(1), 0, NULL, false },
        { "KYR", 1.0, new CValue<float>(42), 0, NULL, false },
        { "KMT", 1.0, new CValue<float>(42), 0, NULL, false },
        { "KDY", 0.1, new CValue<float>(42), 0, NULL, false },
        { "KLD", 0.1, new CValue<float>(42), 0, NULL, false },
        { "KT0", 1.0, new CValue<float>(42), 0, NULL, false },
        { "PIN", 0.5, new CValue<float>(42), 0, NULL, false },
        { "TNF", 0.01, new CValue<float>(50), 0, NULL, false },
        { "PRL", 1.0, new CValue<float>(42), 0, NULL, false },
        { "PRL", 1.0, new CValue<float>(42), 0, NULL, false },
        { "UDC", 0.1, new CValue<float>(200), 0, NULL, false },
        { "UL1", 0.1, new CValue<float>(230), 0, NULL, false },
        { "UL2", 0.1, new CValue<float>(200), 0, NULL, false },
        { "UL3", 0.1, new CValue<float>(230), 0, NULL, false },
        { "IDC", 0.01, new CValue<float>(2), 0, NULL, false },
        { "IL1", 0.01, new CValue<float>(2), 0, NULL, false },
        { "IL2", 0.01, new CValue<float>(2), 0, NULL, false },
        { "IL3", 0.01, new CValue<float>(2), 0, NULL, false },
        { "TKK", 1.0, new CValue<float>(42), 0, NULL, false },
        { "TK2", 1.0, new CValue<float>(42), 0, NULL, false },
        { "TK3", 1.0, new CValue<float>(42), 0, NULL, false },
        { "IEE", 0.1, new CValue<float>(1), 0, NULL, false },
        { "IED", 0.1, new CValue<float>(1), 0, NULL, false },
        { "IEA", 0.1, new CValue<float>(1), 0, NULL, false },
        { "UGD", 0.1, new CValue<float>(25), 0, NULL, false },
        { "SAL", 1, new CValue<int>(0), 0, NULL, false },

        { "UD01", 0.1, new CValue<float>(200), 0, NULL, false },
        { "UD02", 0.1, new CValue<float>(200), 0, NULL, false },
        { "UD03", 0.1, new CValue<float>(200), 0, NULL, false },
        { "ID01", 0.01, new CValue<float>(2), 0, NULL, false },
        { "ID02", 0.01, new CValue<float>(2), 0, NULL, false },
        { "ID03", 0.01, new CValue<float>(2), 0, NULL, false },
        { "PD01", 0.5, new CValue<float>(400), 0, NULL, false },
        { "PD02", 0.5, new CValue<float>(400), 0, NULL, false },
        { "PD03", 0.5, new CValue<float>(400), 0, NULL, false },

        // Inverter Solarmax 20C uses those to signal stati...
        // (not implemented in the inverter class!)
        { "SE1", 1, new CValue<int>(0), 0, NULL, false },
        { "SE2", 1, new CValue<int>(0), 0, NULL, false },
        { "SPR", 1, new CValue<int>(0), 0, NULL, false },
        { "SCD", 1, new CValue<int>(0), 0, NULL, false },

        // some other commands to evaluate... Seems to be general settings
        // for the inverter / grid configuration.
        // take with care, values mostly guessed.
        // not implemented in the inverter.
        { "ULH", 0.1, new CValue<float>(220), 0, NULL, false },  // Uac max
        { "ULL", 0.1, new CValue<float>(240), 0, NULL, false },  // Uac min
        { "TNH", 0.01, new CValue<float>(49.7), 0, NULL, false }, // freq max
        { "TNL", 0.01, new CValue<float>(50.2), 0, NULL, false }, // freq min
        { "TND", 0.01, new CValue<float>(0.1), 0, NULL, false },  // df/dt max
        { "ISL", 1, new CValue<bool>(0), 0, NULL, false }, // "ISL"anding detection
        { "IEM", .1, new CValue<float>(.5), 0, NULL, false }, // max error current
        { "IAA", 0.01, new CValue<float>(16), 0, NULL, false }, // max Iac mean
        { "ILM", 0.01, new CValue<float>(17), 0, NULL, false }, // max Iac
        { "UMX", 0.1, new CValue<float>(240), 0, NULL, false }, // 10 min Uax max.
        { "RSD", 1, new CValue<float>(30), 0, NULL, false }, // Restart delay
        { "CYC", 1, new CValue<float>(0), 0, NULL, false }, // ???
        { "PWF", 1, new CValue<float>(0), 0, NULL, false }, // ???
        { "SYM", 1, new CValue<float>(0), 0, NULL, false }, // Symmetry Settings?
        { "PAM", 1, new CValue<float>(4000), 0, NULL, false }, // Maximum power to feed.

        { "PWM", 1, new CValue<float>(25), 0, NULL, false }, // ??? some modulation
        { "PWT", 1, new CValue<float>(26), 0, NULL, false }, // ??? some modulation

        { NULL , 0  , NULL, 0, NULL, false}
};


/** helper to convert a string to a CValue
 * \param ivalue will be updated.
 * \param value stringvalue
 * \param scale applied to the decoded value, if sputnikmode=true
 * \param sputnikmode if true, expect hexvalue (raw value in telegramm).
 * \return false if string could not be converted.
 */
static bool converttovalue(IValue *ivalue, const std::string &value,
    float scale = 1.0, bool sputnikmode = false)
{
    float tmp;
    if (!sputnikmode) {
        if (1 != sscanf(value.c_str(), "%f", &tmp)) return false;
    } else {
        long ltmp;
        if (1 != sscanf(value.c_str(), "%ld", &ltmp)) return false;
        tmp = ltmp * scale;
    }

    if (CValue<float>::IsType(ivalue)) {
        ((CValue<float>*)ivalue)->Set(tmp);
    } else if (CValue<long>::IsType(ivalue)) {
        ((CValue<long>*)ivalue)->Set(tmp + 0.5);
    } else if (CValue<int>::IsType(ivalue)) {
        ((CValue<int>*)ivalue)->Set(tmp + 0.5);
    } else if (CValue<bool>::IsType(ivalue)) {
        ((CValue<bool>*)ivalue)->Set(tmp > 0.5 ? true : false);
    } else {
        LOGERROR(Registry::GetMainLogger(),
            "converttovalue -- not implemented  CValue<type>");
        return false;
    }

    //LOGTRACE(Registry::GetMainLogger(),"value now " << std::string(*ivalue));
    return true;
}

/** modify the value a little bit
 * \param ivalue will be updated.
 */
static void modifyvalue(IValue *ivalue)
{
    static boost::random::mt19937 rng;
    // allow changes from -20 to +20%, in 0.1% steps
    boost::random::uniform_int_distribution<> change(-200,200);
    float tmp = (float) change(rng) * 0.001;

    // only modify with a certain hardcoded probability.
    int shouldwe = change(rng) + 200;

    if (shouldwe < 350) return;

    cout << endl << "modifying with tmp=" << tmp << endl;

    if (CValue<float>::IsType(ivalue)) {
        CValue<float> &v = *((CValue<float>*)ivalue);
        float tmp2 = v.Get();
        tmp2 = tmp2 + tmp*tmp2;
        v.Set(tmp2);
    } else if (CValue<long>::IsType(ivalue)) {
        CValue<long> &v = *((CValue<long>*)ivalue);
        float tmp2 = v.Get();
        tmp2 = tmp2 + tmp*tmp2 + 0.5;
        v.Set(tmp2);
    } else if (CValue<int>::IsType(ivalue)) {
        CValue<int> &v = *((CValue<int>*)ivalue);
        float tmp2 = v.Get();
        tmp2 = tmp2 + tmp*tmp2 + 0.5;
        v.Set(tmp2);
    } else if (CValue<bool>::IsType(ivalue)) {
        ((CValue<bool>*)ivalue)->Set(tmp >= 0.0 ? true : false);
    } else {
        LOGERROR(Registry::GetMainLogger(),
            "converttovalue -- not implemented  CValue<type>");
    }
}


/// helper to convert the values to suitable strings
/// Implemented for float, long and int
static std::string convert2sputnikhex(IValue *value, float scale)
{
    char buf[32];
    unsigned long tmp;
    if (CValue<float>::IsType(value)) {
        float ftmp = ((CValue<float>*)value)->Get() / scale + 0.5;
        tmp = (long)ftmp;
    } else if (CValue<long>::IsType(value)) {
        float ftmp = ((CValue<long>*)value)->Get() / scale + 0.5;
        tmp = (long)ftmp;
    } else if (CValue<int>::IsType(value)) {
        float ftmp = ((CValue<int>*)value)->Get() / scale + 0.5;
        tmp = (long)ftmp;
    } else if (CValue<bool>::IsType(value)) {
        bool btmp = ((CValue<bool>*)value)->Get();
        tmp = (long)btmp;
    } else {
        LOGERROR(Registry::GetMainLogger(),
            "convert2sputnikhex -- not implemented CValue<type>");
        return "";
    }

    snprintf(buf, sizeof(buf), "%lX", tmp);
    return buf;
}

CInverterSputnikSSeriesSimulator::CInverterSputnikSSeriesSimulator(
    const string &name, const string & configurationpath) :
    IInverterBase::IInverterBase(name, configurationpath, "inverter")
{
    commadr = 0x01;
    _disconnect = false;
    _offline = false;
    _shutdown_requested = false;
    _isconnected = false;
    // will be initialized later.
    ctrlserver = NULL;
    _inject_chksum_err = false;
    _modify_values = false;

    // Add the capabilites that this inverter has
    // Note: The "must-have" ones CAPA_CAPAS_REMOVEALL and CAPA_CAPAS_UPDATED are already instanciated by the base class constructor.
    // Note2: You also can add capabilites as soon you know them (runtime detection)

    string s;
    IValue *v;
    CCapability *c;

#warning remove this depreciated cruft (and spell correctly manufacturer)
    s = CAPA_INVERTER_MANUFACTOR_NAME;
    v = CValueFactory::Factory<CAPA_INVERTER_MANUFACTOR_TYPE>();
    ((CValue<string>*)v)->Set("Solarpowerlog");
    c = new CCapability(s, v, this);
    AddCapability(c);

    s = CAPA_INVERTER_MANUFACTURER_NAME;
    v = CValueFactory::Factory<CAPA_INVERTER_MANUFACTURER_TYPE>();
    ((CValue<string>*) v)->Set("Sputnik Engineering");
    c = new CCapability(s, v, this);
    AddCapability(c);

    // Add the request to initialize as soon as the system is up.
    ICommand *cmd = new ICommand(CMD_INIT, this);
    Registry::GetMainScheduler()->ScheduleWork(cmd);

    CConfigHelper cfghlp(configurationpath);

    // Query settings needed and default all optional settings.
    cfghlp.GetConfig("commadr", commadr, 0x01u);

    s = CAPA_INVERTER_CONFIGNAME;
    v = CValueFactory::Factory<CAPA_INVERTER_CONFIGNAME_TYPE>();
    ((CValue<std::string>*)v)->Set(name);
    c = new CCapability(s, v, this);
    AddCapability(c);

    LOGDEBUG(logger, "Inverter configuration:");
    LOGDEBUG(logger, "class CInverterSputnikSSeriesSimulator ");
    LOGDEBUG(logger, "Commadr: " << commadr);
    cfghlp.GetConfig("comms", s, (string)"unset");
    LOGDEBUG(logger, "Communication: " << s);

    this->scommands = new struct simulator_commands[sizeof(simcommands)
        / sizeof(struct simulator_commands)];

    // copy the lookup table to have an working copy available (to be able to
    // modify the values with the control server)
    int i = 0;
    do {
        scommands[i].token = simcommands[i].token;
        scommands[i].scale1 = simcommands[i].scale1;
        scommands[i].scale2 = simcommands[i].scale2;
        scommands[i].value = NULL;
        scommands[i].value2 = NULL;
        if (simcommands[i].value) scommands[i].value = simcommands[i].value
            ->clone();
        if (simcommands[i].value2) scommands[i].value2 = simcommands[i].value2
            ->clone();
        scommands[i].killbit = false;
    } while (simcommands[i++].token);

    // Register for broadcast events
    Registry::GetMainScheduler()->RegisterBroadcasts(this);
}

CInverterSputnikSSeriesSimulator::~CInverterSputnikSSeriesSimulator()
{
    int i = 0;
    do {
        if (scommands[i].value) delete scommands[i].value;
        if (scommands[i].value2) delete scommands[i].value2;
    } while (scommands[++i].token);
    delete[] scommands;

    if (ctrlserver) delete ctrlserver;
}

bool CInverterSputnikSSeriesSimulator::CheckConfig()
{
    string setting;
    string str;

    bool fail = false;

    CConfigHelper hlp(configurationpath);
    fail |= (true != hlp.CheckConfig("comms", libconfig::Setting::TypeString));
    // Note: Queryinterval is optional. But CConfigHelper handle also opt.
    // parameters and checks for type.
    fail |=
        (true
            != hlp.CheckConfig("queryinterval", libconfig::Setting::TypeFloat,
                true));
    fail |= (true != hlp.CheckConfig("commadr", libconfig::Setting::TypeInt));

    // Check config of the connection component
    fail |= (true != connection->CheckConfig());

    if (!connection->CanAccept()) {
        LOGFATAL(logger,
            "Configuration Error: Communication method supports not "
            "server-mode or is not configured accordingly.");
        fail = true;
    }

    std::string ctrl_cfg = configurationpath + ".ctrl_comms";
    CConfigHelper ch(ctrl_cfg);
    if (ch.GetConfig("comms", setting)) {
        ctrlserver = IConnectFactory::Factory(ctrl_cfg);
        ctrlserver->SetupLogger(configurationpath, "ctrl-server");
        if (!ctrlserver->CheckConfig()) {
            LOGFATAL(logger,
                "Ctrl-Server communication method configuration error");
            return false;
        }
        if (!ctrlserver->CanAccept()) {
            LOGFATAL(logger,
                "Ctrl-Server communication method cannot act as a server");
            return false;
        }
    } else {
        LOGINFO(logger,
            "Simulator: control server disabled as config not found.");
    }

    int type;
    int cadr;
    hlp.GetConfig("TYP", type, -1);
    hlp.GetConfig("commadr", cadr);
    if (-1 != type) {
        int i;
        for (i = 0; scommands[i].token; i++) {
            if (0 == strcmp(scommands[i].token, "TYP")) {
                CValue<int>* v = (CValue<int>*)scommands[i].value;
                v->Set(type);
            } else if (0 == strcmp(scommands[i].token, "ADR")) {
                CValue<int>* v = (CValue<int>*)scommands[i].value;
                v->Set(cadr);
            }
        }
    }

    LOGTRACE(logger, "Check Configuration result: " << !fail);
    return !fail;
}

/** Calculate the telegram checksum and return it.
 *
 * The sputnik protects it telegrams with a checksum.
 * The checksum is a sum of all bytes of a telegram,
 * starting after the { and ending at the | just before
 * the checksum.
 *
 * This routine assumes, that you give it the complete
 * telegram, and only the checksum and the } is missing.
 */
unsigned int CInverterSputnikSSeriesSimulator::CalcChecksum(const char *str,
    int len)
{
    unsigned int chksum = 0;
    str++;
    do {
        chksum += *str++;
    } while (--len);

    return chksum;
}

void CInverterSputnikSSeriesSimulator::ExecuteCommand(const ICommand *Command)
{
    string commstring = "";
    string reccomm = "";
    ICommand *cmd;

    switch ((Commands)Command->getCmd()) {

        case CMD_INIT: {
            LOGDEBUG(logger, "new state: CMD_INIT");

            cmd = new ICommand(CMD_SIM_INIT, this);
            Registry::GetMainScheduler()->ScheduleWork(cmd);

            if (ctrlserver) {
                cmd = new ICommand(CMD_CTRL_INIT, this);
                Registry::GetMainScheduler()->ScheduleWork(cmd);
            }
            break;
        }

        case CMD_SIM_INIT: // Wait for incoming connections.
        {
            // only if no shutdown have been requested.
            if (_shutdown_requested) break;

            LOGDEBUG(logger, "new state: CMD_SIM_INIT");

            if (connection->IsConnected()) {
                cmd = new ICommand(CMD_SIM_WAITDISCONNECT, this);
                connection->Disconnect(cmd);
                _isconnected = false;
                break;
            }
        }
            // fall-through ok
        case CMD_SIM_DISCONNECTED:
            LOGDEBUG(logger, "new state: CMD_SIM_DISCONNECTED");
            LOGINFO(logger,
                "Disconnecting because we are in disconnected mode");
            // we will wait here until we were allowed to connect again.
            // in this case the ctrl server will issue a work with
            // CMD_SIM_DISCONNECTED as target and let fall through here.

            // avoid race with ctrl server.
            if (_disconnect) break;

            // if we are still connected, do not accept -- otherwise we are
            // "double" accepting.
            if (_isconnected) break;

        case CMD_SIM_WAITDISCONNECT: {
            LOGDEBUG(logger, "new state: CMD_SIM_WAITDISCONNECT");
            if (!_disconnect) {
                cmd = new ICommand(CMD_SIM_CONNECTED, this);
                connection->Accept(cmd);
                break;
            }
            // else wait here until reactivation.
            LOGDEBUG(logger, "waiting for _disconnect getting false");
            break;
        }

        case CMD_SIM_CONNECTED: // Wait for
        {
            LOGDEBUG(logger, "new state: CMD_SIM_CONNECTED");
            int err = -1;
            // CMD_CONNECTED: Accept succeeded of failure.
            try {
                err = boost::any_cast<int>(Command->findData(ICMD_ERRNO));
            } catch (...) {
                LOGDEBUG(logger,
                    "CMD_SIM_CONNECTED: unexpected exception while "
                    "trying to get the errorcode.");
                err = -1;
            }

            if (err < 0) {
                try {
                    LOGERROR(logger,
                        "Error while connecting: " << boost::any_cast<string>(Command->findData(ICMD_ERRNO_STR)));
                } catch (...) {
                    LOGERROR(logger,
                        "Unknown error " << err << " while connecting.");
                }

                cmd = new ICommand(CMD_SIM_INIT, this);
                Registry::GetMainScheduler()->ScheduleWork(cmd);
                break;

            }
            if (!_disconnect) {
                _isconnected = true;
                cmd = new ICommand(CMD_SIM_EVALUATE_RECEIVE, this);
                cmd->addData(ICONN_TOKEN_TIMEOUT, (long)3600 * 1000);
                connection->Receive(cmd);
                LOGINFO(logger, "Simulator connected.");
            } else {
                // tear down connection again after we've got instructed to
                // be in disconnected mode.
                connection->Disconnect(
                    new ICommand(CMD_SIM_DISCONNECTED, this));
                LOGINFO(logger,
                    "Simulator connected, but we shall disconnect.");
            }
        }
        break;

        case CMD_SIM_EVALUATE_RECEIVE: {
            LOGDEBUG(logger, "new state: CMD_SIM_EVALUATE_RECEIVE");

            int err;
            std::string s;
            try {
                err = boost::any_cast<int>(Command->findData(ICMD_ERRNO));
            } catch (...) {
                LOGDEBUG(logger, "BUG: Unexpected exception.");
                err = -1;
            }

            if (err < 0) {
                // we do not differentiate between errors.
                cmd = new ICommand(CMD_SIM_INIT, this);
                Registry::GetMainScheduler()->ScheduleWork(cmd);
                try {
                    s = boost::any_cast<std::string>(
                        Command->findData(ICMD_ERRNO_STR));
                    LOGERROR(logger, "Receive Error: " << s);
                } catch (...) {
                    LOGERROR(logger, "Receive Error: " << strerror(-err));
                }
            }

            try {
                s = boost::any_cast<std::string>(
                    Command->findData(ICONN_TOKEN_RECEIVE_STRING));
            } catch (...) {
                LOGDEBUG(logger, "Unexpected Exception");
                err = -EINVAL;
                break;
            }

            if (err < 0) {
                cmd = new ICommand(CMD_SIM_INIT, this);
                Registry::GetMainScheduler()->ScheduleWork(cmd);
                break;
            }

            LOGTRACE(logger, "Received: " << s << " len: " << s.size());
            if (logger.IsEnabled(ILogger::LL_TRACE)) {
                string st;
                char buf[32];
                for (unsigned int i = 0; i < s.size(); i++) {
                    sprintf(buf, "%02x", (unsigned char)s[i]);
                    st = st + buf;
                    if (i && i % 16 == 0) st = st + "\n";
                    else st = st + ' ';
                }
                LOGTRACE(logger, "Received in hex: "<< st);
            }

            if (!_offline) {	// make answer and send it.
                s = parsereceivedstring(s);
            } else {
                s.clear();
                LOGINFO(logger, "Not answering because we are in offline mode");
            }

            if (_disconnect) {
                // ctrl server tells not to connect... so we disconnect now and do not
                // send the answer
                cmd = new ICommand(CMD_SIM_DISCONNECTED, this);
                connection->Disconnect(cmd);
                _isconnected = false;
            }

            // only response if parsing was successful.
            if (s.size()) {
                LOGTRACE(logger, "Response :" << s << " len: " << s.size());
                cmd = new ICommand(CMD_SIM_WAIT_SENT, this);
                cmd->addData(ICONN_TOKEN_TIMEOUT, (long)3000);
                cmd->addData(ICONN_TOKEN_SEND_STRING, s);
                connection->Send(cmd);
            } else {
                cmd = new ICommand(CMD_SIM_EVALUATE_RECEIVE, this);
                cmd->addData(ICONN_TOKEN_TIMEOUT, (long)3600 * 1000);
                connection->Receive(cmd);
            }
        }
        break;

        case CMD_SIM_WAIT_SENT: {
            LOGDEBUG(logger, "new state: CMD_SIM_WAIT_SENT");
            int err;
            try {
                err = boost::any_cast<int>(Command->findData(ICMD_ERRNO));
            } catch (...) {
                LOGDEBUG(logger, "BUG: Unexpected exception.");
                err = -1;
            }

            if (err < 0) {
                LOGERROR(logger, "Error while sending");
                cmd = new ICommand(CMD_SIM_INIT, this);
                Registry::GetMainScheduler()->ScheduleWork(cmd);
                break;
            }

            cmd = new ICommand(CMD_SIM_EVALUATE_RECEIVE, this);
            cmd->addData(ICONN_TOKEN_TIMEOUT, (long)3600 * 1000);
            connection->Receive(cmd);
            break;
        }

            // ##### CONTROL SEVER #####
        case CMD_CTRL_INIT: {
            // only if no shutdown have been requested.
            if (_shutdown_requested) break;
            // the commandserver is known to be non-NULL, otherwise CMD_INIT
            // would not have scheduled this work.

            LOGDEBUG(logger, "new state: CMD_CTRL_INIT");

            LOGINFO(logger, "Sputnik-Simulator control server ready.");

            if (ctrlserver->IsConnected()) {
                cmd = new ICommand(CMD_CTRL_WAITDISCONNECT, this);
                ctrlserver->Disconnect(cmd);
                break;
            }
        }
            // fall through ok

        case CMD_CTRL_WAITDISCONNECT: {
            cmd = new ICommand(CMD_CTRL_CONNECTED, this);
            ctrlserver->Accept(cmd);
            break;
        }

        case CMD_CTRL_CONNECTED: {

            LOGDEBUG(logger, "new state: CMD_CTRL_CONNECTED");
            LOGINFO(logger, "Ctrl-Server connected");

            int err = -1;
            try {
                err = boost::any_cast<int>(Command->findData(ICMD_ERRNO));
            } catch (...) {
                LOGDEBUG(logger,
                    "CMD_CTRL_CONNECTED: unexpected exception while "
                    "trying to get the errorcode.");
                err = -1;
            }

            if (err < 0) {
                try {
                    LOGERROR(logger,
                        "Error while connecting (ctrl-server): " << boost::any_cast<string>(Command->findData(ICMD_ERRNO_STR)));
                } catch (...) {
                    LOGERROR(logger,
                        "Unknown error " << err << " while connecting.");
                }

                cmd = new ICommand(CMD_CTRL_INIT, this);
                timespec ts;
                ts.tv_sec = 15;
                ts.tv_nsec = 0;
                Registry::GetMainScheduler()->ScheduleWork(cmd, ts);
                break;
            } else {
                cmd = new ICommand(CMD_CTRL_PARSERECEIVE, this);
                cmd->addData(ICONN_TOKEN_TIMEOUT, (long)3600 * 1000);
                ctrlserver->Receive(cmd);
                break;
            }
            break;
        }

        case CMD_CTRL_PARSERECEIVE: {
            LOGDEBUG(logger, "new state: CMD_CTRL_PARSERECEIVE");
            int err;
            std::string s;
            try {
                err = boost::any_cast<int>(Command->findData(ICMD_ERRNO));
            } catch (...) {
                LOGDEBUG(logger, "BUG: Unexpected exception.");
                err = -1;
            }

            if (err < 0) {
                // we do not differentiate the error here, an error is an error....
                cmd = new ICommand(CMD_CTRL_INIT, this);
                Registry::GetMainScheduler()->ScheduleWork(cmd);
                try {
                    s = boost::any_cast<std::string>(
                        Command->findData(ICMD_ERRNO_STR));
                    LOGERROR(logger, "Receive Error (ctrl-server): " << s);
                } catch (...) {
                    LOGERROR(logger,
                        "Receive Error (ctrl-server): " << strerror(-err));
                }
                break;
            }

            try {
                s = boost::any_cast<std::string>(
                    Command->findData(ICONN_TOKEN_RECEIVE_STRING));
            } catch (...) {
                LOGDEBUG(logger, "Unexpected Exception");
                break;
            }

            LOGTRACE(logger,
                "Received (ctrl-server): " << s << " len: " << s.size());
            if (logger.IsEnabled(ILogger::LL_TRACE)) {
                string st;
                char buf[32];
                for (unsigned int i = 0; i < s.size(); i++) {
                    sprintf(buf, "%02x", (unsigned char)s[i]);
                    st = st + buf;
                    if (i && i % 16 == 0) st = st + "\n";
                    else st = st + ' ';
                }
                LOGTRACE(logger, "Received in hex: " << st);
            }

            bool disconnect_old = _disconnect;
            // make answer and send it.
            s = parsereceivedstring_ctrlserver(s);
            LOGTRACE(logger,
                "Response (ctrl-server):" << s << "len: " << s.size());

            cmd = new ICommand(CMD_CTRL_WAIT_SENT, this);
            cmd->addData(ICONN_TOKEN_TIMEOUT, ((long)3000));
            cmd->addData(ICONN_TOKEN_SEND_STRING, s);
            if (s == "BYE!\n") {
                cmd->setCmd(CMD_CTRL_INIT);
            }
            ctrlserver->Send(cmd);

            // detect if we re-enable the connection of the simulator
            if (disconnect_old && !_disconnect) {
                LOGDEBUG(logger, "Re-enabling connection");
                Registry::GetMainScheduler()->ScheduleWork(
                    new ICommand(CMD_SIM_DISCONNECTED, this));
            }
            break;
        }

        case CMD_CTRL_WAIT_SENT: {
            LOGDEBUG(logger, "new state: CMD_CTRL_WAIT_SENT");
            int err;
            try {
                err = boost::any_cast<int>(Command->findData(ICMD_ERRNO));
            } catch (...) {
                LOGDEBUG(logger, "BUG: Unexpected exception.");
                err = -1;
            }

            if (err < 0) {
                LOGERROR(logger, "Error while sending (ctrl-server)");
                cmd = new ICommand(CMD_CTRL_INIT, this);
                Registry::GetMainScheduler()->ScheduleWork(cmd);
                break;
            }

            cmd = new ICommand(CMD_CTRL_PARSERECEIVE, this);
            cmd->addData(ICONN_TOKEN_TIMEOUT, (long)3600 * 1000);
            ctrlserver->Receive(cmd);
            break;
        }

            // Broadcast events
        case CMD_BRC_SHUTDOWN:
            // stop all pending I/Os, as we will exit soon.
            connection->AbortAll();
            ctrlserver->AbortAll();
            _shutdown_requested = true;
        break;

        default:
            if (Command->getCmd() <= BasicCommands::CMD_BROADCAST_MAX) {
                // broadcast event
                LOGDEBUG(logger,
                    "Unhandled broadcast event received " << Command->getCmd());
                break;
            }
            LOGERROR(logger, "Unknown CMD received: "<< Command->getCmd());
        break;

    }
}

// parsing of the reception -- simulator.
std::string CInverterSputnikSSeriesSimulator::parsereceivedstring(
    const string & s)
{
    unsigned int i;
    unsigned int sender_adr;

    // check for basic constraints...
    if (s[0] != '{' || s[s.length() - 1] != '}') return "";
    // tokenizer (taken from
    // http://oopweb.com/CPP/Documents/CPPHOWTO/Volume/C++Programming-HOWTO-7.html
    // but  modified.

    // we take the received string and "split" it into smaller pieces.
    // the pieces ("tokens") then assemble one single information to be
    // for this, we split by:
    // ";" "|" ":" (and "{}")

    vector<string> tokens;
    {
        char delimiters[] = "{;|:}";
        tokenizer(delimiters, s, tokens);
    }

    // Minimum is {<from>;<to>;<len>|<port>: .... |<chksum>} - 5 tokens
    if (tokens.size() <= 5) return "";

    // Debug: Print all received tokens
#if defined DEBUG_TOKENIZER
    DEBUG_tokenprinter(this->logger, tokens);
#endif

    unsigned int tmp;
    if (1 != sscanf(tokens.back().c_str(), "%x", &tmp)) {
        LOGDEBUG(logger, "could not parse checksum. Token was:");
        return "";
    }

    if (tmp != CalcChecksum(s.c_str(), s.length() - 6)) {
        LOGDEBUG(logger, "Checksum error on received telegram");
        return "";
    }

    if (1 != sscanf(tokens[0].c_str(), "%x", &sender_adr)) {
        LOGDEBUG(logger, " could not parse from address");
        return "";
    }

    if (1 != sscanf(tokens[1].c_str(), "%x", &tmp)) {
        LOGDEBUG(logger, "could not parse to-address");
        return "";
    }

    if (tmp != commadr) {
        LOGDEBUG(logger, "Received string is not for us: Wrong receiver");
        return "";
    }

    if (1 != sscanf(tokens[2].c_str(), "%x", &tmp)) {
        LOGDEBUG(logger, "could not parse telegram length");
        return "";
    }

    if (tmp != s.length()) {
        LOGDEBUG(logger, "wrong telegram length ");
        return "";
    }

    if (tokens[3] != "64") {
        LOGDEBUG(logger, "Simulator only handling port 100");
        return "";
    }

    std::string ret;
    std::string tmps;
    bool found;
    int j = 0;

    for (i = 4; i < tokens.size() - 1; i++) {
        // tokens[i] contains the commands to be answered.
        //LOGDEBUG(logger,"token=" << tokens[i]);
        found = false;
        for (j = 0; scommands[j].token; j++) {
            if (tokens[i] == scommands[j].token) {

                // check if command was disabled via the ctrl server
                if (scommands[j].killbit) {
                    found = true;
                    LOGINFO(logger,
                        "Token " << tokens[i] << " disabled and ignored.");
                    break;
                }
                if (scommands[j].value) {
                    found = true;
                    // LOGTRACE(logger, tokens[i] << " found");
                    tmps = convert2sputnikhex(scommands[j].value,
                        scommands[j].scale1);

                    if (_modify_values) {
                        modifyvalue(scommands[j].value);
                    }

                    if (!tmps.empty()) {
                        if (!ret.empty()) {
                            ret += ";";
                        }
                        ret += tokens[i] + "=" + tmps;
                    } else {
                        continue;
                    }
                }
                if (scommands[j].value2) {
                    ret += ","
                        + convert2sputnikhex(scommands[j].value2,
                            scommands[j].scale2);
                }
                break; // token handled. continue with the next
            }
        }
        // token not in the list
        if (!found) LOGINFO(logger,
            "Token " << tokens[i] << " unknown and not answered");
    }

    // ret contains token answer, but without framing.
    // size of answer = header {12;12;12|64:<ret>|1234}
    //                         1234567890123     456789
    char buf[32];
    unsigned int telsize = ret.length() + 19;
    snprintf(buf, sizeof(buf), "{%02X;%02X;%02X|64:", commadr, sender_adr,
        telsize);
    ret = buf + ret + "|";
    unsigned int checksum = CalcChecksum(ret.c_str(), ret.length());

    if (_inject_chksum_err) {
        LOGINFO(logger, "Injecting checksum error");
        checksum = 0xdead;
        _inject_chksum_err = false;
    }

    snprintf(buf, sizeof(buf), "%04X}", checksum);
    ret += buf;
    return ret;

}

std::string CInverterSputnikSSeriesSimulator::parsereceivedstring_ctrlserver(
    std::string s)
{
    bool sputnikmode = false;
    // this parser accepts two versions of input
    // - complete telegrams in sputnik format,
    //   where all parts except the data is ignored
    // - <token>=<value> or <token>=<value1>,<value2>
    //
    // as the complete telegramm is only a special case of the seconds one,
    // parsing is still straight forward.

    // strip off the sputnik protocol path, if existant.
    // from the front: {xx;xx;xx|64:
    // from the end: |xxxx}

    // remove whitespaces.
    boost::algorithm::trim(s);

    // special commmands .. those commands must be the only command supplied on
    // the the line.
    if (s == "offline") {
        _offline = true;
        LOGDEBUG(logger, "offline");
        return ("DONE\n");
    } else if (s == "online") {
        _offline = false;
        LOGDEBUG(logger, "online");
        return ("DONE\n");
    }

    if (s == "disconnect") {
        _disconnect = true;
        return ("DONE\n");
    } else if (s == "connect") {
        _disconnect = false;
        return ("DONE\n");
    } else if (s == "quit") {
        return ("BYE!\n");
    } else if (s == "version") {
        return PACKAGE_STRING + std::string("\n");
    } else if (s == "inject_chksum") {
        _inject_chksum_err = true;
        return ("DONE\n");
    } else if (s == "modify_on") {
        _modify_values = true;
        return ("DONE\n");
    } else if (s == "modify_off") {
        _modify_values = false;
        return ("DONE\n");
    }

    size_t first = s.find_first_of(':');
    if (first != std::string::npos && s.length() > first) {
        first++;
    } else {
        first = std::string::npos;
    }
    size_t last = s.find_last_of('|');

    if (first != std::string::npos && last != std::string::npos) {
        s = s.substr(first, last - first);
        sputnikmode = true;
    }

    // now should have just the data portion.
    // token1=value1;=value2;token3=value3,value4
    // so seperated by ";"
    vector<string> tokens;
    tokenizer(";", s, tokens);

    s.clear();

    // now go through the tokens.
    vector<string>::iterator it;
    vector<string> subtokens;
    for (it = tokens.begin(); it != tokens.end(); it++) {
        subtokens.clear();
        boost::algorithm::trim(*it);
        tokenizer("=,", *it, subtokens);
        if (subtokens.size() < 2 || subtokens.size() > 3) {
            LOGERROR(logger, "ctrl-server parse: Parse error.");
            s += "ERR: Parse error on " + *it + ". ";
            continue;

        }
        int i = 0;
        for (i = 0; scommands[i].token; i++) {
            if (scommands[i].token == subtokens[0]) {
                bool ret1 = true, ret2 = true;
                LOGTRACE(logger, "parsing " << *it);
                if (boost::algorithm::to_lower_copy(subtokens[1]) == "off") {
                    LOGTRACE(logger, "Disabing " << subtokens[0]);
                    scommands[i].killbit = true;
                    break;
                }
                if (boost::algorithm::to_lower_copy(subtokens[1]) == "on") {
                    LOGTRACE(logger, "Enabling " << subtokens[0]);
                    scommands[i].killbit = false;
                    break;
                }
                if (scommands[i].killbit) {
                    break;
                }
                if (scommands[i].value) {
                    ret1 = converttovalue(scommands[i].value, subtokens[1],
                        scommands[i].scale1, sputnikmode);
                }
                if (scommands[i].value2 && subtokens.size() == 3) {
                    ret2 = converttovalue(scommands[i].value2, subtokens[2],
                        scommands[i].scale2, sputnikmode);
                }
                if (!ret1 || !ret2) {
                    LOGDEBUG(logger, "Parse error on token "<<*it);
                    s += "ERR: Parse error on " + *it + ". ";
                }
                break;
            }
        }
        if (!scommands[i].token) {
            s += "ERR: " + *it + " not found.\n";
        }

    }
    s = s + "DONE\n";
    return s;
}

// tokenizer.
void CInverterSputnikSSeriesSimulator::tokenizer(const char *delimiters,
    const string& s, vector<string> &tokens)
{
    unsigned int i;

    string::size_type lastPos = 0;
    string::size_type pos = 0;

    i = 0;
    // Skip tokens at the start of the string
    do {
        if (s[lastPos] == delimiters[i]) {
            lastPos++;
            i = 0;
        }
    } while (++i < strlen(delimiters));

    pos = lastPos;

    // get the first substring by finding the "second" delimiter
    i = lastPos;

    do {
        unsigned int tmp;
        tmp = s.find_first_of(delimiters[i], lastPos);
        if (tmp < pos) pos = tmp;
    } while (++i < strlen(delimiters));

    while (s.length() > pos && s.length() > lastPos) {
        unsigned int tmp, tmp2;

        if (pos - lastPos) {
            tokens.push_back(s.substr(lastPos, pos - lastPos));
        }
        lastPos = pos;

        // Skip delimiters.
        i = 0;
        do {
            if (s[lastPos] == delimiters[i]) {
                lastPos++;
                i = 0;
            }

        } while (++i < strlen(delimiters));

        // Find next "delimiter"
        i = 0;
        tmp2 = -1;
        do {
            tmp = s.find_first_of(delimiters[i], lastPos);
            if (tmp < tmp2) tmp2 = tmp;
        } while (++i < strlen(delimiters));
        pos = tmp2;
    }

    // Check if we have an "end-token" (not seperated)
    if (lastPos != s.length()) {
        tokens.push_back(s.substr(lastPos, s.length() - lastPos));
    }
}
#endif
