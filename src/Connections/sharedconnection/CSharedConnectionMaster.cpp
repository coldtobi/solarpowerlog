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
 * CSharedConnectionMaster.cpp
 *
 *  Created on: Sep 13, 2010
 *      Author: tobi
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "porting.h"
#endif

#ifdef HAVE_COMMS_SHAREDCONNECTION

#include "configuration/Registry.h"
#include "CSharedConnectionMaster.h"
#include "Connections/factories/IConnectFactory.h"
#include "configuration/Registry.h"

#include <boost/date_time.hpp>
#include "configuration/CConfigHelper.h"
#include "interfaces/CMutexHelper.h"
#include "CSharedConnection.h"
#include "patterns/ICommand.h"

#define STATUS_CONNECTED (1<<0)

#define ICONN_SHAREDCOMMS_APIID "ICONN_CSC_APIID"

enum
{
    CMD_HANDLEENDOFBLOCK = BasicCommands::CMD_USER_MIN,
    CMD_NONATOMIC_HANDLEREADCOMPLETION,
    CMD_NONATOMIC_HANDLETIMEOUT
};

CSharedConnectionMaster::CSharedConnectionMaster(
    const string & configurationname) :
    IConnect(configurationname)
{
    connection = NULL;
    last_atomic_cmd = NULL;
    ticket_cnt = active_ticket = 0;
    ownslave = new CSharedConnectionSlave(configurationname);
    ownslave->setMaster(this);
    non_atomic_mode = false;
    _nam_interrupted = false;

}

CSharedConnectionMaster::~CSharedConnectionMaster()
{
    if (connection) delete connection;
    delete ownslave;
}

void CSharedConnectionMaster::SetupLogger(const string& parentlogger,
    const string &specalization)
{
    IConnect::SetupLogger(parentlogger, specalization);
    ownslave->SetupLogger(logger.getLoggername(),"ownslave");
}

void CSharedConnectionMaster::ExecuteCommand(const ICommand *Command)
{
//    LOGDEBUG(logger, __PRETTY_FUNCTION__ << " now handling: " << Command << " (" << Command->getCmd() << "");

    // handling end of atomic blocks and issuing pending commands.
    switch (Command->getCmd()) {
        case CMD_HANDLEENDOFBLOCK: {
            // ok, we've got signalled that the last atomic block command has
            // just been finished.
            // redirect the answer and clean up our stats.
            CMutexAutoLock m(mutex); // mutex is needed....
//            LOGDEBUG(logger, "Ending Ticket " << active_ticket );
            assert(last_atomic_cmd);
            last_atomic_cmd->mergeData(*Command);
//            LOGDEBUG(logger, __PRETTY_FUNCTION__ << " scheduling " << last_atomic_cmd);
            Registry::GetMainScheduler()->ScheduleWork(last_atomic_cmd);
            last_atomic_cmd = NULL;
            active_ticket = 0;

            // answer redirected, now lets check for pending work.
            // first the "non-atomic-ones"
            // they can just be queued to the comms object

//            LOGDEBUG(logger,"non-atomic-backlog:" << non_atomic_icommands_pending.size());

            while (non_atomic_icommands_pending.size()) {
                ICommand *cmd = non_atomic_icommands_pending.front();
                non_atomic_icommands_pending.pop();
//                LOGDEBUG(logger, "Dispatching1 " << cmd);
                ICommandDispatcher(cmd);
             }

 //           LOGDEBUG(logger,"atomic-backlog (blocks):" << atomic_icommands_pending.size());

            // after giving the non-atomic priority, resume atomic operation.
            // we can also queue the next atomic block completely, but we need
            // again to catch the command completing the block.
            // as std::maps are ordered, we can just "pop" the first item to
            // get the next due ticket
            if (atomic_icommands_pending.size()) {
                ICommand *cmd;
                std::map<long, std::queue<ICommand*> >::iterator it =
                    atomic_icommands_pending.begin();
                active_ticket = it->first;
                std::queue<ICommand *> *q = &(it->second); // Convenience only :)
                while (q->size() > 1) {
                    cmd = q->front();
                    q->pop();
//                    LOGDEBUG(logger, "Dispatching2 " << cmd);
                    ICommandDispatcher(cmd);
                }

                // now we have one left in the queue, but this could be one
                // finishing the block (then we need to intercept the result)
                // or it just one block in the middle of the atomic block
                // in this case we can just submit the command to the comms.
                assert(q->size() == 1);
                cmd = q->front();
                bool end_of_block = !boost::any_cast<bool>(
                    cmd->findData(ICONN_ATOMIC_COMMS));

                if (end_of_block) {
                    ICommand *newcmd = new ICommand(CMD_HANDLEENDOFBLOCK, this);
                    newcmd->mergeData(*cmd);
                    last_atomic_cmd = cmd;
                    cmd->RemoveData(); // free up some memory, data not needed.
//                    LOGDEBUG(logger, "Dispatching3 " << cmd);
                    ICommandDispatcher(newcmd);
                } else {
 //                   LOGDEBUG(logger, "Dispatching4 " << cmd);
                    ICommandDispatcher(cmd);
                }

 //               LOGDEBUG(logger, "New Ticket " << active_ticket );

                // we can delete the map entry for this ticket, as subsequent
                // commands will be handled directly.
                atomic_icommands_pending.erase(it);
            }
            break;
        }
        case CMD_NONATOMIC_HANDLEREADCOMPLETION: {
            CMutexAutoLock cma(mutex);
            _nam_interrupted = false;
            // will be executed if a non-atomic read completes or read
            // was interrupted with another request.

            // retrieve error information about this read.
            int err = 0;
            try {
                err = boost::any_cast<int>(Command->findData(ICMD_ERRNO));
            } catch (...) {
            }

             // Submit everything to the slaves except timeout (handled by slaves)
            // and the aborted calls.
            if (err != -ECANCELED && err != -ETIMEDOUT) {
                // call was not canceled, tha means transmit to all slaves.
                if (err < 0 ) {
                    LOGDEBUG(logger, "NOT Timeout and NOT CANCELED "<< err);
                }
                for (std::list<CSharedConnectionSlave *>::iterator it =
                    _reading_slaves.begin(); it != _reading_slaves.end();
                    it++) {
                    ICommand *c = new ICommand(
                        CSharedConnectionSlave::CMD_HANDLEREAD, *it);
                    c->mergeData(*Command);
                    //LOGDEBUG(logger, __PRETTY_FUNCTION__ << "Dispatching receive work to slave comms. " << c << " (slave:" << *it << ")");
                    Registry::GetMainScheduler()->ScheduleWork(c);
                }

                // All other errors than timeouts ends the current read requests.
                readtimeout = boost::posix_time::not_a_date_time;
                LOGDEBUG(logger, "not rescheduling read due to err "<< err);
                return;
            }

            if (_reading_slaves.empty()) {
                // if there are no longer slaves listening, we can cease
                // reading.
                readtimeout = boost::posix_time::not_a_date_time;
                LOGDEBUG(logger, "Not rescheduling read as no reading slaves");
                return;
            }

            // Restart receive if there is still time for reading.
            boost::posix_time::ptime pt(
                boost::posix_time::microsec_clock::universal_time());
            if (!readtimeout.is_special() && pt < readtimeout) {
                boost::posix_time::time_duration d = readtimeout - pt;
                ICommand *c = new ICommand(CMD_NONATOMIC_HANDLEREADCOMPLETION,
                    this);
                long timeout = d.total_milliseconds();
                if (timeout >= 1000) timeout = 1000;
                c->addData(ICONN_TOKEN_TIMEOUT, timeout);
                LOGDEBUG(logger, __PRETTY_FUNCTION__ << " rescheduling read: " << c << " remaining timeout:" << d.total_milliseconds());
                connection->Receive(c);
            } else {
                LOGDEBUG(logger, __PRETTY_FUNCTION__ << " not rescheduling read");
                readtimeout = boost::posix_time::not_a_date_time;
            }
        }
        break;
    }
}

void CSharedConnectionMaster::Connect(ICommand *callback)
{
    ownslave->Connect(callback);
}

void CSharedConnectionMaster::Disconnect(ICommand *callback)
{
    ownslave->Disconnect(callback);
}

void CSharedConnectionMaster::Send(ICommand *callback)
{

    ownslave->Send(callback);
}

void CSharedConnectionMaster::Receive(ICommand *callback)
{
    ownslave->Receive(callback);
}

void CSharedConnectionMaster::Accept(ICommand* callback)
{
    ownslave->Accept(callback);
}

void CSharedConnectionMaster::Noop(ICommand* callback)
{
    ownslave->Noop(callback);
}

bool CSharedConnectionMaster::CheckConfig(void)
{
    // Get real configuration path to extract target comms config.
    string commsconfig = this->ConfigurationPath + ".realcomms";
    string s;
    CConfigHelper h(commsconfig);

    if (!h.GetConfig("comms", s)) {
        LOGERROR(logger, "realcomms section: comms missing");
        return false;
    }

    connection = IConnectFactory::Factory(commsconfig);

    // connection always valid -- the factory returns a dummy
    // object if it does not know the comms.
    if (connection) {
        connection->SetupLogger(logger.getLoggername());
        return connection->CheckConfig();
    }
    LOGFATAL(logger,"Could not create real communication object");
    return false;
}

bool CSharedConnectionMaster::IsConnected(void)
{
    assert(connection);
    return connection->IsConnected();
}

bool CSharedConnectionMaster::AbortAll()
{
    assert(connection);
    CMutexAutoLock cma(mutex);
    readtimeout = boost::posix_time::not_a_date_time;
    return connection->AbortAll();
}

long CSharedConnectionMaster::GetTicket()
{
    CMutexAutoLock lock(mutex);
    ++ticket_cnt;
    // Overflow protection -- the zero is reserved for "no ticket"
    if (0 == ticket_cnt) ticket_cnt++;
    //LOGDEBUG(logger," New ticket requested:" << ticket_cnt);
    return ticket_cnt;
}

void CSharedConnectionMaster::Connect(ICommand* callback,
    CSharedConnectionSlave* s)
{
    assert(callback);
    assert(s);
    _HandleNonAtomicReceiveInterrupts();
    callback = HandleAtomicBlock(callback, API_CONNECT);
    if (callback) connection->Connect(callback);
}

void CSharedConnectionMaster::Disconnect(ICommand* callback,
    CSharedConnectionSlave* s)
{
    assert(callback);
    assert(s);
    // at the moment only the master can disconnect.
    // and if the master disconnects, we'll disconnect regardless of other
    // slaves present.
    // so if someone else disconnects, we'll make a NOOP instead
    if (s == ownslave) {
        _HandleNonAtomicReceiveInterrupts();
        callback = HandleAtomicBlock(callback, API_DISCONNECT);
        if (callback) connection->Disconnect(callback);
    } else {
        _HandleNonAtomicReceiveInterrupts();
        callback->addData(ICMD_ERRNO,0);
        callback = HandleAtomicBlock(callback, API_NOOP);
        if (callback) connection->Noop(callback);
    }
}

void CSharedConnectionMaster::Send(ICommand* callback,
    CSharedConnectionSlave* s)
{

    assert(callback);
    assert(s);
    _HandleNonAtomicReceiveInterrupts();
    callback = HandleAtomicBlock(callback, API_SEND);
    LOGDEBUG(logger, "CSharedConnectionMaster::Send() ICmd: " << callback);
    if (callback) connection->Send(callback);
}

void CSharedConnectionMaster::Receive(ICommand* callback,
    CSharedConnectionSlave* s)
{
    LOGDEBUG(logger, __PRETTY_FUNCTION__<< " callback:" << callback);

    bool is_atomic;
    assert(callback);
    assert(s);
    callback = HandleAtomicBlock(callback, API_RECEIVE, &is_atomic);

    if(is_atomic) {
        LOGDEBUG(logger, __PRETTY_FUNCTION__<< " sending atomic callback : " << callback);
        if (callback) connection->Receive(callback);
        return;
    }

    // non-atomic read.
    if (!non_atomic_mode) {
        non_atomic_mode = true;
        // LOGDEBUG(logger,"SharedComms switched to Non-Atomic read mode.");
    }

    long timeout;
    try {
        timeout = boost::any_cast<long>(callback->findData(
                ICONN_TOKEN_TIMEOUT));
    } catch (...) {
        LOGDEBUG(logger,"CSharedConnectionMaster::Receive(): Depreciated: Falling back to default timeoout");
        timeout = SHARED_CONN_DEFAULTTIMEOUT;
    }

    CMutexAutoLock m(mutex);
    boost::posix_time::ptime ptimetmp =
        boost::posix_time::microsec_clock::universal_time()
            + boost::posix_time::milliseconds(timeout);

    // check if we got already an read request pending.
    if (readtimeout == boost::posix_time::not_a_date_time) {
 //       LOGDEBUG(logger, "We are not yet reading!");
        // not a pending read.
        // If no read is pending, directly issue a read commmand to the connection,
        // but divert result to our class for later distribution.
        // In the Execute member our master will then inform every listening slave
        // about the incoming comms.
        // We will need to track timeouts to determine when we can stop
        // listening.
        // The slave kept a copy of the read request, so we can use the one
        // we have been supplied with for our purpose.

        readtimeout = ptimetmp;

        callback->setTrgt(this);
        callback->setCmd(CMD_NONATOMIC_HANDLEREADCOMPLETION);
        callback->addData(ICONN_TOKEN_TIMEOUT, (long)1000);
//        LOGDEBUG(logger, __PRETTY_FUNCTION__ << " sending non-atomic to own Execute(): " << callback);
        connection->Receive(callback);
    } else {
        LOGDEBUG(logger, "We are ALREADY reading");
        // we've got already a read pending, just check if we need to update
        // the timeout value if necesary.
        if( readtimeout < ptimetmp) {
            // Just record the value, the completion handler will do the rest.
            readtimeout = ptimetmp;
        }
        // in this case we do not need this callback anymore
        // as we own it, we delete it.
//        LOGDEBUG(logger, "CSharedConnectionMaster::Receive() deleting: " << callback);
        delete callback;
    }
}

void CSharedConnectionMaster::Noop(ICommand* callback,
    CSharedConnectionSlave* s)
{
    assert(callback);
    assert(s);
    _HandleNonAtomicReceiveInterrupts();
    callback = HandleAtomicBlock(callback, API_NOOP);
    if (callback) connection->Noop(callback);

}

void CSharedConnectionMaster::Accept(ICommand* callback,
    CSharedConnectionSlave* s)
{
    assert(callback);
    assert(s);
    _HandleNonAtomicReceiveInterrupts();
    callback = HandleAtomicBlock(callback, API_ACCEPT);
    if (callback) connection->Accept(callback);
}

ICommand* CSharedConnectionMaster::HandleAtomicBlock(ICommand* cmd,
    enum api_id id, bool *isatomic)
{
    long ticket = 0;
    try {
        ticket = boost::any_cast<long>(cmd->findData(ICONN_SHARED_TICKET));
    } catch (...) {
    }

    if (isatomic) {
        *isatomic = (0 != ticket);
    }

    bool end_of_block = false;
    try {
        end_of_block = !boost::any_cast<bool>(
            cmd->findData(ICONN_ATOMIC_COMMS));
    } catch (...) {
        // assert if we could not retrieve this in a signaled atomic block.
        assert(!ticket);
    }

    CMutexAutoLock m(mutex);
    //LOGDEBUG(logger,"Ticket for this command is: "<< ticket << " (current ticket is "    << active_ticket << ")");

    // check if this received work is part of an atomic block
    if (ticket) {
        // -> yes: check if the active one
        // check if we currently got an atomic-block
        if (!active_ticket) {
            // nope, this will start it.
            active_ticket = ticket;
            //LOGDEBUG(logger, "New ticket: "<< ticket);
        }

        if (active_ticket == ticket) {
            //-> yes: check if the current block ends the atomic block
            if (end_of_block) {
                //-> yes: this ends the atomic block.
                // So save Icommand and create a new one to redirect to own
                // for completion handling in own ICommandTrgt. Send new one to comms.
                //LOGDEBUG(logger, "Ticket: "<< ticket << " ends soon");
                ICommand *newcmd = new ICommand(CMD_HANDLEENDOFBLOCK, this);
                newcmd->mergeData(*cmd);
                assert(!last_atomic_cmd);
                last_atomic_cmd = cmd;
                cmd->RemoveData(); // free up some memory, this copy wont need
                // the data.
                return newcmd;
            } else {
                //-> no: just send to comms directly,
                // the communication will handle it with its fifo.
                //LOGDEBUG(logger, "Ticket: "<< ticket << " continues");
                return cmd;
            }
        } else {
            // -> no, not part of active atomic block, queue for later.
            // (as currently the interface is occupied)
            cmd->addData(ICONN_SHAREDCOMMS_APIID, id);
            // check if this sets a completly new atomic block
            if (!atomic_icommands_pending.count(ticket)) {
                //LOGDEBUG(logger, "Ticket: "<< ticket << " new backlog entry");

                // Yes, we need a new map-entry.
                std::pair<long, std::queue<ICommand*> > p;
                p.first = ticket;
                p.second.push(cmd);
                atomic_icommands_pending.insert(p);
            } else {
                // No, just append to queue.
                //LOGDEBUG(logger, "Ticket: "<< ticket << " new backlog entry to queue");
                std::map<long, std::queue<ICommand*> >::iterator it =
                    atomic_icommands_pending.find(ticket);
                it->second.push(cmd);
            }
            return NULL;
        }
    } else {
        // -> no, not atomic.  check if currently an atomic block is locking the comms
        if (active_ticket) {
            // -> yes, queue for later
            cmd->addData(ICONN_SHAREDCOMMS_APIID, id);
            non_atomic_icommands_pending.push(cmd);
            return NULL;
        } else {
            // -> no, send directly to comms
            return cmd;
        }
    }
}

void CSharedConnectionMaster::SubscribeSlave(CSharedConnectionSlave* slave, bool subscribe )
{

    if ( subscribe) {
        _reading_slaves.push_back(slave);
    } else {
        _reading_slaves.remove(slave);
    }
}

void CSharedConnectionMaster::ICommandDispatcher(ICommand* cmd)
{
    api_id api = boost::any_cast<api_id>(
        cmd->findData(ICONN_SHAREDCOMMS_APIID));

    // forward the api calls to the real comms object.
    switch (api) {
        case API_CONNECT:
            connection->Connect(cmd);
        break;

        case API_DISCONNECT:
            connection->Disconnect(cmd);
        break;

        case API_RECEIVE:
            connection->Receive(cmd);

        break;

        case API_SEND:
            connection->Send(cmd);
        break;

        case API_ACCEPT:
            connection->Accept(cmd);
        break;

        case API_NOOP:
            connection->Noop(cmd);
        break;

        default:
            assert(0);
        break;
    }
}


bool CSharedConnectionMaster::CanAccept(void)
{
    assert(connection);
    return connection->CanAccept();
}

void CSharedConnectionMaster::_HandleNonAtomicReceiveInterrupts(void)
{
    return;
    // due to the bug in boost::asio this method  is curently empty.
#if 0
    // if non-atomic mode, every command during a receive() will interrupt said
    // read, but this interrupt must only happen once until.
    CMutexAutoLock cma(&mutex);

    // only work in non_atomic_mode and if not already interrupted
    if(!non_atomic_mode || _nam_interrupted) return;

    _nam_interrupted = true;
    // ok, we AbortAll() now, this will cancel the receive.
    connection->AbortAll();
#endif
}


#endif
