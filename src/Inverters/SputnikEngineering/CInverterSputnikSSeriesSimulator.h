/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

 Copyright (C) 2012-2015 Tobias Frost

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

/** \file CInverterSputnikSSeriesSimulator.h
 *
 *  Created on: June 07, 2012
 *      Author: Tobias Frost
 *
 */

#ifndef CINVERTERSPUTNIKSSERIESSIMULATOR_H_
#define CINVERTERSPUTNIKSSERIESSIMULATOR_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined HAVE_INV_SPUTNIKSIMULATOR

#include "Inverters/interfaces/InverterBase.h"
#include "Inverters/BasicCommands.h"

#include "Inverters/SputnikEngineering/SputnikCommand/ISputnikCommand.h"

/** Implements a (simple) simulator for the Sputnik S Series
 *
 * The Sputnik S-Series are an inverter family by Sputnik Engineering
 * Please see the manufacturer's homepage for details.
 */
class CInverterSputnikSSeriesSimulator : public IInverterBase
{
public:
    struct simulator_commands
    {
        const char *token;
        float scale1;
        IValue *value;
        float scale2;
        IValue *value2;
        bool killbit;
    };

    CInverterSputnikSSeriesSimulator(const string & name,
        const string & configurationpath);
    virtual ~CInverterSputnikSSeriesSimulator();

    virtual bool CheckConfig();

    /** implements the ICommandTarget interface */
    virtual void ExecuteCommand(const ICommand *Command);

#warning implement me!
    virtual CConfigCentral* getConfigCentralObject(void) { return NULL; }

protected:
    /** calculate the checksum for the telegram stored in str */
    static unsigned int CalcChecksum(const char* str, int len);

private:

    /// Commands for the Workscheduler
    enum Commands
    {
        // broadcast event.
        CMD_BRC_SHUTDOWN = BasicCommands::CMD_BRC_SHUTDOWN,

        CMD_INIT = BasicCommands::CMD_USER_MIN,
        // Simulator commands
        CMD_SIM_INIT, ///< Wait for incoming connections.
        CMD_SIM_DISCONNECTED, ///< ctrl server forced us "disconnected"
        CMD_SIM_WAITDISCONNECT, ///< if connected, wait for disconnection.
        CMD_SIM_CONNECTED, ///< Wait for incoming data
        CMD_SIM_EVALUATE_RECEIVE, ///< Parse incoming data and send response
        CMD_SIM_WAIT_SENT, ///< Wait till data sent
        // Control-Server commmands.
        CMD_CTRL_INIT, ///< Wait for incoming connections (cmd-server).
        CMD_CTRL_WAITDISCONNECT, ///< if connected, wait for disconnection.
        CMD_CTRL_CONNECTED, ///< Wait for incoming data (cmd-server).
        CMD_CTRL_PARSERECEIVE, ///< Parse incoming data (cmd-server) and send response.
        CMD_CTRL_WAIT_SENT  ///< Wait till response sent.
    };

    /// Dataports of the sputnik inverters.
    enum Ports
    {
        QUERY = 100, COMMAND = 200, ALARM = 300, INTERFACE = 1000
    };

    /// parse the answer of the inverter.
    std::string parsereceivedstring(const string& s);

    /// parser for the control server.
    std::string parsereceivedstring_ctrlserver(std::string s);

    /// Adress to use as "our" adress for communication
    /// This can be set by the conffile and the parameter ownadr
    /// defaults to 0xFB
    /// unsigned int ownadr;
    void tokenizer(const char *delimiters, const string& s,
        vector<string> &tokens);

    /// cache for inverters comm adr.
    unsigned int commadr;

    struct simulator_commands *scommands;

    /// Command Server.
    IConnect *ctrlserver;

    /// set to true if the shutdown request has been received via broadcast
    /// event.
    bool _shutdown_requested;

    /// if queries should be answered at all or if the inverter should be simulated as offline
    /// note: in offline mode it will still accept connections, just not answer them.
    bool _offline;

    /// the inverter should not accept() connections anymore.
    bool _disconnect;

    /// tracking if we are already connected and thus if we need to
    /// accept again when ctrl server allows us again to connect.
    /// \sa _disconnect
    bool _isconnected;

    /// inject the next time an checksum error (will force an reconnect from the
    /// client)
    bool _inject_chksum_err;

    /// modify values automatically to simulate changes
    bool _modify_values;

};

#endif

#endif /*CINVERTERSPUTNIKSSERIESSIMULATOR_H_*/
