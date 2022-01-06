/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

 Copyright (C) 2010-2012 Tobias Frost

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

/*
 * CConnectSlave.cpp
 *
 *  Created on: Sep 13, 2010
 *      Author: tobi
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "porting.h"
#endif

#ifdef HAVE_COMMS_SHAREDCONNECTION

#include "CSharedConnectionSlave.h"
#include "CSharedConnectionMaster.h"
#include "configuration/CConfigHelper.h"
#include "Inverters/interfaces/InverterBase.h"
#include "CSharedConnection.h"
#include "configuration/ILogger.h"
#include "interfaces/CMutexHelper.h"


// Slave Configuration Parameter
// useconnection = "name"  Name of the inverter having the master connection.

CSharedConnectionSlave::CSharedConnectionSlave(const string & configurationname) :
    IConnect(configurationname)
{
    master = NULL;
    current_ticket = 0; // 0 == no ticket assigned,
    slave_registered = false;
}

CSharedConnectionSlave::~CSharedConnectionSlave()
{
}

void CSharedConnectionSlave::Connect(ICommand *callback)
{
    mutex.lock(); read_buffer.clear(); mutex.unlock();
    LOGDEBUG(logger, "CSharedConnectionSlave::Connect: callback:"<<callback);
    assert(master);
    assert(callback);
    (void)HandleTickets(callback);
    master->Connect(callback, this);
}

void CSharedConnectionSlave::Disconnect(ICommand *callback)
{
    mutex.lock();
    read_buffer.clear();
    master->SubscribeSlave(this,false);
    slave_registered = false;
    mutex.unlock();
    LOGDEBUG(logger, "CSharedConnectionSlave::Disconnect: callback:"<<callback);
    assert(callback);
    (void)HandleTickets(callback);
    master->Disconnect(callback, this);
}

void CSharedConnectionSlave::Send(ICommand *callback)
{
//    LOGDEBUG(logger, __PRETTY_FUNCTION__ << ": work: "<<callback);
    assert(callback);
    assert(master);
    (void)HandleTickets(callback);
    master->Send(callback, this);
}

void CSharedConnectionSlave::Receive(ICommand *callback)
{
//    LOGDEBUG(logger, __PRETTY_FUNCTION__ << ": callback: "<<callback);
    assert(master);
    assert(callback);

    if (HandleTickets(callback)) {
        master->Receive(callback, this);
        return;
        // Receive within an atomic block.
    } else {
        CMutexAutoLock cma(mutex);
        // check if we have got already some data to return.
        // (note: to have read_buffer empty we must already in non-atomic mode
        // and registered already  with the master)
        if (read_buffer.length()) {
            callback->addData(ICONN_TOKEN_RECEIVE_STRING, read_buffer);
            callback->addData(ICMD_ERRNO, 0);
            LOGDEBUG(logger, __PRETTY_FUNCTION__ << ": buffered read available, placing callback: "<<callback);
            Registry::GetMainScheduler()->ScheduleWork(callback);
            read_buffer.clear();
            return;
        }
        // Register as "receiver".
        if (!slave_registered) {
            master->SubscribeSlave(this);
            slave_registered = true;
        }
        // Timeout handling is done by the slave.
        // clone command and redirect to own execute.
        unsigned long timeout;
        try {
            timeout = boost::any_cast<long>(
                callback->findData(ICONN_TOKEN_TIMEOUT));
        } catch (...) {
            LOGDEBUG(logger,
                "CSharedConnectionSlave::Receive(): Depreciated: Falling back to default timeout");
            timeout = SHARED_CONN_DEFAULTTIMEOUT;
        }

        // Set the due-time in the pending reading commands.
        boost::posix_time::ptime pt(
            boost::posix_time::microsec_clock::universal_time());
        pt += boost::posix_time::milliseconds(timeout);
        callback->addData(SHARED_CONN_TIMEOUTTIMESTAMP,pt);

        // Save callback in the pending reads list.
        pending_reads.push_back(callback);

        // Handle IO
        ICommand *cmd;
        // copy-construct a ICommand for the master comm object and submit work.
        // note that target and commmand will be rewritten by the master comm
        // master comm will get ownership of object, it may also delete it!
        // (note: the icommand has an implicit copy-constructor which is ok)
        cmd = new ICommand(*callback);
//        LOGTRACE(logger, __PRETTY_FUNCTION__ << ": submitting work: "<<cmd);
        master->Receive(cmd, this);

        // Handle Timeout
        // TODO needs only to be done if the new timeout is shorter than existings.
        // this will just create a little more overhead.
        cmd = new ICommand(CMD_HANDLETIMEOUTS, this);
        timespec ts;
        ts.tv_sec = timeout / 1000;
        ts.tv_nsec = (timeout % 1000) * 1000 * 1000UL;
//        LOGDEBUG(logger, __PRETTY_FUNCTION__ << ": submitting timeout work: "<< cmd);
        Registry::GetMainScheduler()->ScheduleWork(cmd, ts);
    }
}

void CSharedConnectionSlave::Accept(ICommand* callback)
{
//    LOGDEBUG(logger, __PRETTY_FUNCTION__ << ": callback: "<<callback);
    assert(callback);
    assert(master);
    (void)HandleTickets(callback);
    master->Accept(callback, this);
}

void CSharedConnectionSlave::Noop(ICommand* cmd)
{
 //   LOGDEBUG(logger, "CSharedConnectionSlave::Noop: callback:"<<cmd);
    assert(cmd);
    (void)HandleTickets(cmd);
    master->Noop(cmd, this);
}

bool CSharedConnectionSlave::CheckConfig(void)
{

    CConfigHelper cfg(ConfigurationPath);
    bool fail = false;
    std::string s;
    fail |= !cfg.GetConfig("useconnection", s);

    fail |= !cfg.CheckConfig("useconnection", libconfig::Setting::TypeString,
        false);

    if (fail) return false;

    // Retrieve the pointer to the CSharedConnectionMaster via the Inverter,
    // but do checks to ensure that the type is right.
    IInverterBase *base = Registry::Instance().GetInverter(s);

    // Check if inverter is known
    if (!base) {
        LOGERROR(logger,
            "useconnection must point to a known Inverter and this "
            "inverter must be declared first. Inverter not found: " << s;);
        return false;
    }

    // Check if the config of this inverter is a shared comm.
    CConfigHelper bcfg(base->GetConfigurationPath());
    bcfg.GetConfig("comms", s, std::string(""));
    if (s != "SharedConnection") {
        LOGERROR(logger,
            "inverter " << base->GetName() << " does not use a shared connection.");
        return false;
    }

    // Make sure it is a master.
    bcfg.GetConfig("sharedconnection_type", s);
    if (s != "master") {
        LOGERROR(logger,
            "inverter " << base->GetName() << " does not use a shared master connection");
        return false;
    }

    // The API of the SharedConnection gives us a IConnect*, promote it to
    // (CSharedConnectionMaster* (we checked the type above...)
    master =
        (CSharedConnectionMaster*)((CSharedConnection*)base->getConnection())
            ->GetConcreteSharedConnection();
    LOGDEBUG(logger,
        ConfigurationPath << " slave:" << this << " master " << master);
    return true;
}

bool CSharedConnectionSlave::IsConnected(void)
{
    assert(master);
    return master->IsConnected();
}

bool CSharedConnectionSlave::AbortAll()
{
    // We abort only our pending "receives"
    CMutexAutoLock cma(mutex);
    for (std::list<ICommand *>::iterator it = pending_reads.begin();
         it != pending_reads.end(); it++) {
        (*it)->addData(ICMD_ERRNO, -ECANCELED);
        Registry::GetMainScheduler()->ScheduleWork(*it);
    }
    pending_reads.clear();
    return true;
}

void CSharedConnectionSlave::ExecuteCommand(const ICommand* cmd)
{
    switch (cmd->getCmd()) {
        case CMD_HANDLETIMEOUTS: {
            LOGDEBUG(logger, __PRETTY_FUNCTION__ << ": CMD_HANDLETIMEOUTS ICommand " << cmd );

            // Check all pending read commands for timeouts.
            // and remove all expired reads.
            boost::posix_time::ptime pt(
                boost::posix_time::microsec_clock::universal_time());
            CMutexAutoLock cma(mutex);
            // check for timeouts in the reading commands list
            // and if so schedule work with error set to timedout.
            for (std::list<ICommand *>::iterator it = pending_reads.begin();
                it != pending_reads.end(); it++) {
                try {
                    const boost::posix_time::ptime &ppt = boost::any_cast<
                        boost::posix_time::ptime>(
                        cmd->findData(SHARED_CONN_TIMEOUTTIMESTAMP));
                    if (ppt <= pt) {
                        (*it)->addData(ICMD_ERRNO, -ETIMEDOUT);
                        Registry::GetMainScheduler()->ScheduleWork(*it);
                        it = pending_reads.erase(it);
                    }
                } catch (...) {
                    // malformed ICommand.
                    LOGDEBUG(logger, __PRETTY_FUNCTION__ <<
                        "BUG: Malformated ICommand during CMD_HANDLETIMEOUTS");
                    it = pending_reads.erase(it);
                }
            }
        }
        break;

        case CMD_HANDLEREAD: {
            LOGDEBUG(logger, __PRETTY_FUNCTION__ << ": CMD_HANDLEREAD ICommand " << cmd );
            // will be issued by the master on any reception.
            // Handling: We will answer all pending reads with this answer...

            std::string s;
            CMutexAutoLock cma(mutex);
            try {
                s = boost::any_cast<std::string>(
                    cmd->findData(ICONN_TOKEN_RECEIVE_STRING));

            } catch (...) {
            }

            if (0 == pending_reads.size()) {
                read_buffer += s;
                break;
            }

            for (std::list<ICommand *>::iterator it = pending_reads.begin();
                it != pending_reads.end(); it++) {

                // merge data and issue work
                (*it)->mergeData(*cmd);
                LOGDEBUG(logger, "Scheduling read-work " << *it << " to target " << (*it)->getTrgt());
                Registry::GetMainScheduler()->ScheduleWork(*it);
            }

            pending_reads.clear();
            break;
        }

        default: {
            LOGDEBUG(logger, __PRETTY_FUNCTION__ << ": unknown! ICommand " << cmd );
            assert(0);
            break;
        }
    }
}

bool CSharedConnectionSlave::CanAccept(void)
{
    assert(master);
    return master->CanAccept();
}

bool CSharedConnectionSlave::HandleTickets(ICommand* callback)
{
    // get if this block goes atomic or ends an atomic block.
    bool is_still_atomic = false;
    bool is_atomic = false;
    try {
        is_still_atomic = boost::any_cast<bool>(
            callback->findData(ICONN_ATOMIC_COMMS));
        is_atomic = true;
    } catch (const std::invalid_argument &e) {
        // data not contained -- it is a non-atomic block.
        is_atomic = false;
    } catch (const boost::bad_any_cast &e) {
        //"Bad cast -- Programming error."
        //throw; // abort programm.
    }

    if (!is_atomic) {
        // LOGDEBUG(logger, " Not atomic " << callback);
        return false;
    }

    // New atomic block?
    if (0 == current_ticket) {
        current_ticket = master->GetTicket();
       //LOGDEBUG(logger,   " New ticket "<< current_ticket << " requested for " << callback);
    } else {
        if (is_still_atomic) {
            //LOGDEBUG(logger,            " Existing ticket "<< current_ticket << " continued for " << callback);
        } else  {
            //LOGDEBUG(logger," Existing ticket "<< current_ticket << " ending with " << callback);
        }

    }

    callback->addData(ICONN_SHARED_TICKET, current_ticket);

    // is this the last astomic request?
    if (!is_still_atomic) current_ticket = 0;
    return true;
}

#endif
