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

/** \file CConnectSerialAsio.cpp
 *
 * This interface class abstracts the Comms-Interface (IConnect) to the
 * boost::asio class
 *
 * Boost::asio can act either as a serial class (like RS233, Rs485)
 * or as TCP class. so copying would mean lots of duplicated code
 *
 *  Created on: May 16, 2010
 *      Author: tobi
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "porting.h"
#endif

#ifdef HAVE_COMMS_ASIOSERIAL

#define DEBUG_SERIALASIO

#include "interfaces/IConnect.h"
#include "Connections/CConnectSerialAsio.h"

#include <iostream>
#include <string>

#include "configuration/CConfigHelper.h"

#include <boost/asio/write.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>

#include <boost/algorithm/string.hpp>

#include "interfaces/CMutexHelper.h"

#include <errno.h>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/date_time/posix_time/posix_time_config.hpp>
#include <boost/bind.hpp>

using namespace boost::posix_time;

using namespace std;
using namespace boost::asio;
using namespace boost;
using namespace libconfig;

struct asyncASIOCompletionHandler
{
    asyncASIOCompletionHandler(size_t *b, boost::system::error_code *ec)
    {
        bytes = b;
        this->ec = ec;
    }

    void operator()(const boost::system::error_code& e,
        std::size_t bytes_transferred)
    {
        *bytes = bytes_transferred;
        *ec = e;
    }

    // note, we need pointer as boost seems to make a copy of our handler...
    size_t *bytes;
    boost::system::error_code *ec;
};

/// Helping function for timeout and receive, will be called by boosts' asio.
/// this handler just will set the int store with the value value.
static void boosthelper_set_result(int* store, int value)
{
    if (store) *store = value;
}

CConnectSerialAsio::CConnectSerialAsio(const string &configurationname) :
    IConnect(configurationname),_cfg_characterlen('8'), _cfg_baudrate(9600)
{
    ioservice = new io_service;
    port = new boost::asio::serial_port(*ioservice);
    sem_init(&cmdsemaphore, 0, 0);
}

CConnectSerialAsio::~CConnectSerialAsio()
{
    SetThreadTermRequest();
    // Try to shutdown cleanly...
    // (most likely abortall() has been previously called anyway)
    boost::system::error_code ec;
    mutex.lock();
    cmds.clear();
    ioservice->stop();
    if (port->is_open()) {
        port->cancel(ec);
        port->close(ec);
    }
    mutex.unlock();

    // need to post to semaphore to interrupt worker thread to terminate.
    sem_post(&cmdsemaphore);

    LOGDEBUG(logger, "Waiting for thread to join");
    workerthread.join();
    LOGDEBUG(logger, "Joined.");

    delete port;
    delete ioservice;

    sem_destroy(&cmdsemaphore);
}

void CConnectSerialAsio::Accept(ICommand *callback) {
    // Accept is the same as Connect for this class
    Connect(callback);
}

void CConnectSerialAsio::Connect(ICommand *callback)
{
    CAsyncCommand *commando = new CAsyncCommand(CAsyncCommand::CONNECT,
        callback);
    PushWork(commando);
}

/* Disconnect
 *
 * The disconnection is done by the async task.
 *
 * \note starting with 0.25 the interface is async only!
 */
void CConnectSerialAsio::Disconnect(ICommand *callback)
{
    assert(callback);

    CAsyncCommand *commando = new CAsyncCommand(CAsyncCommand::DISCONNECT,
        callback);
    PushWork(commando);
}

void CConnectSerialAsio::Send(ICommand *callback)
{
    assert(callback);
    CAsyncCommand *commando = new CAsyncCommand(CAsyncCommand::SEND, callback);
    PushWork(commando);
}

/** Receive bytes from the stream -- asynced.
 *
 * As with all other methods, will be done by the worker thread, even if
 * synchronous operation is requested. In this case, we'll just wait for
 * completion.*/
void CConnectSerialAsio::Receive(ICommand *callback)
{
    CAsyncCommand *commando = new CAsyncCommand(CAsyncCommand::RECEIVE,
        callback);
    PushWork(commando);
}

bool CConnectSerialAsio::IsConnected(void)
{
    if (!port) return false;
    mutex.lock();
    bool ret = port->is_open();
    mutex.unlock();
    return ret;
}

bool CConnectSerialAsio::CheckConfig(void)
{
    string setting;
    bool fail = false;
    bool portsetting_parseerr = false;

    CConfigHelper cfghelper(ConfigurationPath);

    fail |= !cfghelper.CheckConfig("serial_serialportname",
        libconfig::Setting::TypeString,
        false);

    fail |= !cfghelper.CheckConfig("serial_baudrate",
        libconfig::Setting::TypeInt,
        false);

    fail |= !cfghelper.CheckConfig("serial_portparameters",
        libconfig::Setting::TypeString,
        true);
    fail |= !cfghelper.CheckConfig("serial_flowcontrol",
        libconfig::Setting::TypeString,
        true);
    fail |= !cfghelper.CheckConfig("serial_timeout",
        libconfig::Setting::TypeInt,
        true);

    if (cfghelper.CheckConfig("serial_timeout", libconfig::Setting::TypeInt)) {
        LOGWARN(logger, " Parameter serial_timeout is depreciated.");
    }

    fail |= !cfghelper.CheckConfig("serial_interbytetimeout",
        libconfig::Setting::TypeInt,
        true);

    cfghelper.GetConfig("", setting, std::string("none"));
    boost::algorithm::to_lower(setting);
    if (setting == "none") {
        _cfg_flowctrl = boost::asio::serial_port_base::flow_control(
            boost::asio::serial_port_base::flow_control::none);
    } else if (setting == "hardware") {
        _cfg_flowctrl = boost::asio::serial_port_base::flow_control(
            boost::asio::serial_port_base::flow_control::hardware);
    } else if (setting == "software") {
        _cfg_flowctrl = boost::asio::serial_port_base::flow_control(
            boost::asio::serial_port_base::flow_control::software);
    } else {
        fail = true;
        LOGERROR(
            logger,
            "serial_flowcontrol must be \"none\", \"hardware\" or \"software\".");
    }

    cfghelper.GetConfig("serial_portparameters", setting, std::string("8N1"));

    if (setting.size() != 3) {
        LOGERROR(logger, "serial_portparameters: Must be exactly three "
        "characters");
    } else {

        if (setting[0] >= '5' || setting[0] <= '9') {
            _cfg_characterlen = setting[0] - '0';
        } else {
            fail = true;
            portsetting_parseerr = true;
            LOGERROR(logger, "serial_portparameters: Number of bits per "
            "symbol must be between 5 and 9");
        }

        if (_cfg_characterlen > 8) {
            fail = true;
            LOGERROR(logger, "serial_portparameters: 9 bits per symbol currently not supported by solarpowelog");
        }

        switch (setting[1])
        {
            case 'E':
                case 'e':
                _cfg_parity = boost::asio::serial_port_base::parity(
                    boost::asio::serial_port_base::parity::even);
            break;

            case 'O':
                case 'o':
                _cfg_parity = boost::asio::serial_port_base::parity(
                    boost::asio::serial_port_base::parity::odd);
            break;

            case 'N':
                case 'n':
                _cfg_parity = boost::asio::serial_port_base::parity(
                    boost::asio::serial_port_base::parity::none);
            break;

            default:
                portsetting_parseerr = true;
                LOGERROR(logger, "serial_portparameter: Invalid parity. Your "
                "choices are 'E'ven ,'O'dd or 'N'one");
                fail = true;
            break;
        }

        // If you are bored, you could implement 1.5 stop bits ;-)
        if (setting[2] == '1') {
            _cfg_stopbits = boost::asio::serial_port_base::stop_bits(
                boost::asio::serial_port_base::stop_bits::one);
        } else if (setting[2] == '2') {
            _cfg_stopbits = boost::asio::serial_port_base::stop_bits(
                boost::asio::serial_port_base::stop_bits::two);
        } else {
            portsetting_parseerr = true;
            fail = true;
            LOGERROR(logger,
                "serial_portparameter: Invalid number of stop bits.");
        }
    }

    if (portsetting_parseerr) {
        // print some explanations...
        LOGERROR(logger, "serial_portparameter must be of the format like"
        " \"8N1\" to specify symbol length(e.g  for example , parity "
        "and number of stopbits.)");
    }

    if (fail) return false;

    // Cache baudrate.
    cfghelper.GetConfig("serial_baudrate", _cfg_baudrate);

    StartWorkerThread();
    return true;
}

void CConnectSerialAsio::_main(void)
{
    LOGTRACE(logger, "Starting helper thread");

    while (!IsTermRequested()) {
        int syscallret;

        // wait for work or signals.
        syscallret = sem_wait(&cmdsemaphore);
        if (syscallret == -1) {
            continue;
        }

        // semaphore had work for us. process it.
        // safety check: really some work?
        mutex.lock();
        if (cmds.empty()) {
            mutex.unlock();
            continue;
        }

        CAsyncCommand *donow = cmds.front();
        cmds.pop_front();
        // reset the ioservice for the next commmand.
        // (must be done during holding the mutex to avoid a race
        // with the AbortAll() call.
        ioservice->reset();
        mutex.unlock();

        switch (donow->c)
        {
            case CAsyncCommand::CONNECT:
                HandleConnect(donow);
            break;

            case CAsyncCommand::DISCONNECT:
                HandleDisconnect(donow);
            break;

            case CAsyncCommand::RECEIVE:
                HandleReceive(donow);
            break;

            case CAsyncCommand::SEND:
                HandleSend(donow);
            break;

            default:
                LOGDEBUG_SA(logger, __COUNTER__, "Unknown command "
                    << donow->c <<" received.");
            break;
        }
        delete donow;
    }
    IConnect::_main();
}

bool CConnectSerialAsio::PushWork(CAsyncCommand *cmd)
{
    mutex.lock();
    cmds.push_back(cmd);
    mutex.unlock();
    sem_post(&cmdsemaphore);
    workerthread.interrupt();
    return true;
}

void CConnectSerialAsio::HandleConnect(CAsyncCommand *cmd)
{
    string portname;
    boost::system::error_code ec;

    // if connected, ignore the commmand, pretend success.
    if (IsConnected()) {
        cmd->callback->addData(ICMD_ERRNO, 0);
        cmd->HandleCompletion();
        return;
    }

    CConfigHelper cfghelper(ConfigurationPath);
    cfghelper.GetConfig("serial_serialportname", portname);

    // we need first to open the port, before applying the settings to it.
    ec = port->open(portname, ec);

    if (ec) {
        // retrieve error code out of ec object.
        // Boost doc won't tell if it is negative, so we better make sure
        int eint = ec.value();
        if (eint > 0) eint = -eint;
        cmd->callback->addData(ICMD_ERRNO, eint);
        if (!ec.message().empty()) {
            cmd->callback->addData(ICMD_ERRNO_STR, ec.message());
        }
        cmd->HandleCompletion();
        return;
    }

    try {
        port->set_option(
            boost::asio::serial_port_base::baud_rate(_cfg_baudrate));
        port->set_option(
            boost::asio::serial_port_base::character_size(_cfg_characterlen));
        port->set_option(_cfg_stopbits);
        port->set_option(_cfg_parity);
        port->set_option(_cfg_flowctrl);
    } catch (boost::system::system_error &e) {
        LOGERROR(logger, "Setting serial port options failed: " << e.what(););
        cmd->callback->addData(ICMD_ERRNO_STR, e.what());
        cmd->callback->addData(ICMD_ERRNO, -EIO);
        port->close(ec);
        cmd->HandleCompletion();
        return;
    }

    LOGDEBUG(logger, "Opened " << portname);
    cmd->callback->addData(ICMD_ERRNO, 0);
    cmd->HandleCompletion();
    return;
}

void CConnectSerialAsio::HandleDisconnect(CAsyncCommand *cmd)
{
    boost::system::error_code ec, ec2;
    std::string message;
    int error = 0;

    if (!IsConnected()) {
        cmd->callback->addData(ICMD_ERRNO, 0);
        cmd->HandleCompletion();
        return;
    }

    ec = port->cancel(ec);
    ec2 = port->close(ec2);

    if (ec) {
        error = -EIO;
        message = ec.message();
    }

    if (ec2) {
        error = -EIO;
        if (!message.empty())
        message = message + " ";
        message = message + ec2.message();
    }

    cmd->callback->addData(ICMD_ERRNO, error);
    if (!message.empty()) {
        cmd->callback->addData(ICMD_ERRNO_STR, ec.message());
    }

    cmd->HandleCompletion();
    return;
}

/** Handle Receive -- asynchronous read from the asio socket with timeout.
 *
 * Strategy:
 * -- get timeout config from caller or configuration (depreciated)
 *    -- spawn timer with timeout setting in background
 * -- setup async read operation of one byte (to detect incoming communication)
 * -- check if we got a byte or timeout
 * -- if got the byte, try to read all available bytes (as the serial ASIO class
 *    cannot tell us the remaining bytes, we need to poll them
 * -- to detect the end of the message, spawn a "inter byte timeout" timer to
 *    abort reading if no new bytes are arriving. This is derived from the
 *    configuration or from the baidrate, where a compile-time
 *    minimum is enforced.
 */
void CConnectSerialAsio::HandleReceive(CAsyncCommand *cmd)
{
    boost::system::error_code ec, handlerec;

    volatile int result_timer = 0;
    size_t bytes = 0;
    unsigned long timeout;
    char buf[2] = {0,0};
    struct asyncASIOCompletionHandler read_handler(&bytes, &handlerec);

    // avoid that the io_service runs out of work as it auto-stops then.
    boost::asio::io_service::work work(*ioservice);

    // timeout setup
    try {
        timeout = boost::any_cast<long>(cmd->callback->findData(
            ICONN_TOKEN_TIMEOUT));
    } catch (std::invalid_argument &e) {
        CConfigHelper cfghelper(ConfigurationPath);
        cfghelper.GetConfig("serial_timeout", timeout,
            SERIAL_ASIO_DEFAULT_TIMEOUT);
    } catch (boost::bad_any_cast &e) {
        LOGDEBUG(logger,
            "Unexpected exception in HandleReceive: Bad cast" << e.what());
        timeout = SERIAL_ASIO_DEFAULT_TIMEOUT;
    }

    LOGDEBUG(logger, "timeout  "<< timeout);

    deadline_timer timer(*(this->ioservice));
    boost::posix_time::time_duration td = boost::posix_time::millisec(timeout);
    timer.expires_from_now(td);
    timer.async_wait(boost::bind(&boosthelper_set_result, (int*)&result_timer,
        1));

    // socket preparation
    //  async_read. However, boost:asio seems not to allow auto-buffers,
    // so we will just read one byte and when this is available, we'll
    // check for if there are some others left

    // same as with the TCP/IP class: Sometimes the ioservice instantly returns
    // without any actual work done. (read_handler & timeout handler not called)
    // Lets try to catch that with the same approach as in the TCP class...
    // Should file a bug report at booot...
    size_t num = 0;
    int maxsleep = 5; // we limit the hack to 5 times.
    do {
        bool sleep = false;
        if (!sleep) {
            boost::posix_time::time_duration td2 = boost::posix_time::millisec(
                25);
            deadline_timer timer(*ioservice, td2);
            try {
                timer.wait();
                maxsleep--;
            } catch (...) {
            }
        }
        sleep = true;
        port->async_read_some(boost::asio::buffer(&buf, 1), read_handler);
        num = ioservice->run_one(ec);
    } while (maxsleep != 0 && !result_timer && 0 == *(read_handler.bytes));

    LOGTRACE(logger, "hack needed " << 5-maxsleep << "times.");

    // ioservice error or timeout
    if (num == 0 || result_timer) {
        if (result_timer) {
            LOGTRACE(logger, "Read timeout");
            cmd->callback->addData(ICMD_ERRNO_STR, std::string("Read timeout"));
            cmd->callback->addData(ICMD_ERRNO, -ETIMEDOUT);
        } else {
            LOGTRACE(logger, "IO Service Error: " << ec.message());
            cmd->callback->addData(ICMD_ERRNO_STR, std::string("IO-service error " + ec.message()));
            cmd->callback->addData(ICMD_ERRNO, -EIO);
        }
        cmd->HandleCompletion();
        timer.cancel(ec);
        port->cancel(ec);
        ioservice->poll();
        return;
    }
    timer.cancel();
    ioservice->poll(ec);
    if (*read_handler.ec) {
        if (*read_handler.ec != boost::asio::error::eof) {
            LOGDEBUG(logger, "Async read failed with ec=" << *read_handler.ec
                << " msg="<< read_handler.ec->message());
            cmd->callback->addData(ICMD_ERRNO, -EIO);
            cmd->callback->addData(ICMD_ERRNO_STR, read_handler.ec->message());

        } else {
            cmd->callback->addData(ICMD_ERRNO, -ENOTCONN);
            LOGDEBUG(logger, "Received eof on socket read");
        }
        cmd->HandleCompletion();
        return;
    }

    /* now one byte is read -- we apply the byte-timeout here to read the
     * remaining bytes. By default deducted from the baudarate, but in this
     * case a minimum is enforced. */
    CConfigHelper cfghelper(ConfigurationPath);
    cfghelper.GetConfig("serial_interbytetimeout", timeout, 0UL);
    if (timeout == 0) {
        // default interbyte timeout is 10 times the time for one byte.
        // (we allow the inaccuracy and assume 10 bits per byte, which is
        // valid for 8N1)
        // however, we ensure a minimum time of 50 ms.
        // (which is still tough as our OS might idle around for even longer)
        timeout = (1000 * 10 * 10) / _cfg_baudrate;
        if (timeout <= SERIAL_ASIO_DEFAULT_INTERBYTETIMEOUT)
        timeout = SERIAL_ASIO_DEFAULT_INTERBYTETIMEOUT;
    }

    std::string received;
    received = buf[0];

    while (1) {
        // clear previous states.
        bytes = 0;
        result_timer = 0;
        ec.clear();
        handlerec.clear();
        // prepare timer
        td = boost::posix_time::millisec(timeout);
        timer.expires_from_now(td);
        timer.async_wait(
            boost::bind(&boosthelper_set_result, (int*)&result_timer,
                1));

        // read a byte
        port->async_read_some(boost::asio::buffer(&buf, 1), read_handler);

        size_t num = ioservice->run_one(ec);

        // ioservice error or timeout
        if (num == 0 || result_timer) {
            if (result_timer) LOGTRACE(logger, "Timeout while reading");
            if (0 == num) LOGTRACE(logger, "IO-Service error: " << ec.message());
            timer.cancel(ec);
            port->cancel(ec);
            ioservice->poll();
            LOGTRACE(logger, "break 1");
            break;
        }

        timer.cancel();
        ioservice->poll(ec);

        // asio async read completed -- check for read error
        if (*read_handler.ec) {
            if (*read_handler.ec != boost::asio::error::eof) {
                // read error occured, which is not timeout.
                LOGDEBUG(logger,
                    "Async read failed with ec=" << *read_handler.ec
                    << " msg="<< read_handler.ec->message());
                cmd->callback->addData(ICMD_ERRNO, -EIO);
                cmd->callback->addData(ICMD_ERRNO_STR,
                    read_handler.ec->message());
                cmd->HandleCompletion();
                return;
            } else {
                // other end closed connection -- treat that as timeout
                break;
            }
        }

        // append read byte to string.
        received.push_back(buf[0]);
    }

    // we got at least one byte -- assemble answer.
    LOGTRACE(logger, "Serial read " << received.length() << " bytes");
    cmd->callback->addData(ICONN_TOKEN_RECEIVE_STRING, received);
    cmd->callback->addData(ICMD_ERRNO, 0);
    cmd->HandleCompletion();
    return;
}

bool CConnectSerialAsio::AbortAll()
{
    // obtain mutex
    LOGINFO(logger, "AbortAll()");
    mutex.lock();
    // abort all pending commands.
    std::list<CAsyncCommand *>::iterator it = cmds.begin();
    for (it = cmds.begin(); it != cmds.end(); it++) {
        CAsyncCommand *c = *it;
        c->callback->addData(ICMD_ERRNO, -ECANCELED);
        c->HandleCompletion();
    }
    // stop any run ioservices.
    ioservice->stop();
    mutex.unlock();
    LOGINFO(logger, "AbortAll() end");
    return true;
}

bool CConnectSerialAsio::CanAccept()
{
    // for the serial, accept is the same as connect
    return true;
}

/** handles async sending */
void CConnectSerialAsio::HandleSend(CAsyncCommand *cmd)
{

    boost::system::error_code ec, handlerec;
    volatile int result_timer = 0;
    size_t bytes;
    unsigned long timeout;
    struct asyncASIOCompletionHandler write_handler(&bytes, &handlerec);
    // timeout setup
    std::string s;

    try {
        s = boost::any_cast<std::string>(cmd->callback->findData(
            ICONN_TOKEN_SEND_STRING));
    }
    #ifdef DEBUG_SERIALASIO
    catch (std::invalid_argument &e) {
        LOGDEBUG(logger,
            "BUG: required " << ICONN_TOKEN_SEND_STRING << " argument not set");

    } catch (boost::bad_any_cast &e) {
        LOGDEBUG(logger,
            "Unexpected exception in HandleSend: Bad cast" << e.what());
    }
#else
    catch (...);
#endif

    try {
        timeout = boost::any_cast<long>(cmd->callback->findData(
            ICONN_TOKEN_TIMEOUT));
    }
    #ifdef DEBUG_SERIALASIO
    catch (std::invalid_argument &e) {
        CConfigHelper cfghelper(ConfigurationPath);
        cfghelper.GetConfig("serial_timeout", timeout,
            SERIAL_ASIO_DEFAULT_TIMEOUT);
    } catch (boost::bad_any_cast &e) {
        LOGDEBUG(logger,
            "Unexpected exception in HandleSend: Bad cast" << e.what());
        timeout = SERIAL_ASIO_DEFAULT_TIMEOUT;
    }
#else
    catch (...) {
        CConfigHelper cfghelper(ConfigurationPath);
        cfghelper.GetConfig("tcptimeout", timeout, SERIAL_ASIO_DEFAULT_TIMEOUT);
    }
#endif

    deadline_timer timer(*(this->ioservice));
    boost::posix_time::time_duration td = boost::posix_time::millisec(timeout);
    timer.expires_from_now(td);
    timer.async_wait(boost::bind(&boosthelper_set_result, (int*)&result_timer,
        1));

    // socket preparation
    //  async_write the whole std::string
    boost::asio::async_write(*port, boost::asio::buffer(s), write_handler);

    // run one of the scheduled services
    size_t num = ioservice->run_one(ec);

    // ioservice error or timeout
    if (num == 0 || result_timer) {
        timer.cancel(ec);
        port->cancel(ec);
        LOGTRACE(logger, "Async write timeout");
        cmd->callback->addData(ICMD_ERRNO, -ETIMEDOUT);
        cmd->HandleCompletion();
        ioservice->poll();
        return;
    }

    // cancel the timer, and catch the completion handler
    timer.cancel();
    ioservice->poll(ec);

    if (*write_handler.ec) {
        if (*write_handler.ec != boost::asio::error::eof) {
            LOGDEBUG(logger, "Async write failed with ec=" << *write_handler.ec
                << " msg="<< write_handler.ec->message());
            cmd->callback->addData(ICMD_ERRNO, -EIO);
            cmd->callback->addData(ICMD_ERRNO_STR, write_handler.ec->message());
        } else {
            cmd->callback->addData(ICMD_ERRNO, -ENOTCONN);
            LOGTRACE(logger, "Received eof on socket write");
        }
        cmd->HandleCompletion();
        return;
    }

    if (s.length() != *write_handler.bytes) {
        LOGDEBUG(logger, "Sent "
            << *write_handler.bytes << " but expected "<< s.length());
        cmd->callback->addData(ICMD_ERRNO, -EIO);
        cmd->HandleCompletion();
        return;
    }

    cmd->callback->addData(ICMD_ERRNO, 0);
    cmd->HandleCompletion();
    return;
}

#endif /* HAVE_COMMS_ASIOSERIAL */
