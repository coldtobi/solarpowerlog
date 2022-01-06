/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

Copyright (C) 2009-2012 Tobias Frost

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


/** \file CConnectionTCPAsio.cpp
 *
 *  Created on: May 21, 2009
 *      Author: tobi
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "porting.h"
#endif

#ifdef HAVE_COMMS_ASIOTCPIO

#define DEBUG_TCPASIO

#include "interfaces/IConnect.h"
#include "Connections/CConnectTCPAsio.h"

#include <iostream>
#include <string>

#include "configuration/CConfigHelper.h"

#include <boost/asio/write.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>

#include <boost/scoped_ptr.hpp>

#include "interfaces/CMutexHelper.h"

#include <errno.h>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/date_time/posix_time/posix_time_config.hpp>
#include <boost/bind.hpp>
#include <memory>

using namespace boost::posix_time;

using namespace std;
using namespace boost::asio;
using namespace boost;
using namespace libconfig;

struct asyncASIOCompletionHandler
{
	asyncASIOCompletionHandler( size_t *b, boost::system::error_code *ec )
	    : bytes(b), ec(ec)
	{ }

	void operator()( const boost::system::error_code& e,
			std::size_t bytes_transferred )
	{
		*bytes = bytes_transferred;
		*ec = e;
	}

	void operator() (const boost::system::error_code& e)
	{
	    *ec = e;
	}

private:
	// note, we need a pointer as boost seems to make a copy of our handler...
	size_t *bytes;
	boost::system::error_code *ec;
};

/** Helping function for timeout and receive, will be called by boost::asio.
 *  this handler just will set the int store with the value value.
*/
static void boosthelper_set_result( int* store, int value )
{
    if (store)
        *store = value;
}

CConnectTCPAsio::CConnectTCPAsio( const string &configurationname ) :
	IConnect(configurationname)
{
	// Generate our own asio ioservice
	// TODO check if one central would do that too...
    configured_as_server = false;
    _connected = false;
	ioservice = new io_service;
	sockt = new ip::tcp::socket(*ioservice);
	sem_init(&cmdsemaphore, 0, 0);
}

CConnectTCPAsio::~CConnectTCPAsio()
{
    // Request thread to exit.
    SetThreadTermRequest();
    // Try a clean shutdown
    mutex.lock();
    cmds.clear();

    ioservice->stop();
    if (_connected) {
        boost::system::error_code ec;
        sockt->cancel(ec);
        sockt->close(ec);
    }
    mutex.unlock();

    // need to post to semaphore to interrupt worker thread to terminate.
    sem_post(&cmdsemaphore);

    LOGDEBUG(logger, "Waiting for thread to join");
    workerthread.join();
    LOGDEBUG(logger, "Joined.");

    delete sockt;
    delete ioservice;

    sem_destroy(&cmdsemaphore);
}

void CConnectTCPAsio::Connect( ICommand *callback )
{
	assert(callback);
	CAsyncCommand *co = new CAsyncCommand(CAsyncCommand::CONNECT, callback);
	PushWork(co);
}

void CConnectTCPAsio::Disconnect( ICommand *callback )
{
    assert(callback);
	CAsyncCommand *co = new CAsyncCommand(CAsyncCommand::DISCONNECT, callback);
	PushWork(co);
}

void  CConnectTCPAsio::Send( ICommand *callback)
{
	assert(callback);
	CAsyncCommand *commando = new CAsyncCommand(CAsyncCommand::SEND, callback);
	PushWork(commando);
}

void CConnectTCPAsio::Receive( ICommand *callback )
{
	assert(callback);
	CAsyncCommand *commando = new CAsyncCommand(CAsyncCommand::RECEIVE, callback);
	PushWork(commando);
}

void CConnectTCPAsio::Accept(ICommand *callback)
{
    assert(callback);
    CAsyncCommand *commando = new CAsyncCommand(CAsyncCommand::ACCEPT,
            callback);
    PushWork(commando);
}

bool CConnectTCPAsio::IsConnected( void )
{
	return _connected;
}

bool CConnectTCPAsio::CheckConfig( void )
{
	string setting;
	bool fail = false;
	CConfigHelper cfghelper(ConfigurationPath);

	if (cfghelper.GetConfig("tcpmode",setting)) {
	    if (setting == "server") {
	        fail |= !cfghelper.CheckConfig("tcpadr", libconfig::Setting::TypeString,true);
	        fail |= !cfghelper.CheckConfig("tcpport", libconfig::Setting::TypeInt);
	        if (!fail) this->configured_as_server = true;
	    }
	}

	if (!configured_as_server) {
        fail |= !cfghelper.CheckConfig("tcpadr", libconfig::Setting::TypeString);
        fail |= !cfghelper.CheckConfig("tcpport", libconfig::Setting::TypeString);
        fail |= !cfghelper.CheckConfig("tcptimeout", libconfig::Setting::TypeInt,
                true);

        if (cfghelper.CheckConfig("tcptimeout", libconfig::Setting::TypeInt,
                true,false)) {
            LOGWARN(logger,"Tcptimeout configuration parameter has changed and might not work anymore! It will be removed soon.");
            LOGWARN(logger,"Timeouts are now configured in the inverter class, see documentation.");
        }
	}

	if (!fail) {
		StartWorkerThread();
		return true;
	}

	return false;
}

void CConnectTCPAsio::_main( void )
{
	LOGTRACE(logger, "Starting helper thread");

    std::auto_ptr<boost::asio::io_service::work> work(new boost::asio::io_service::work(*ioservice));

    while (!IsTermRequested()) {
        int syscallret;

        syscallret = sem_wait(&cmdsemaphore);
        if (syscallret == -1) {
            continue;
        }

        mutex.lock();
        // Safety check if there's really work.
        if (cmds.empty()) {
            mutex.unlock();
            continue;
        }

        CAsyncCommand *donow = cmds.front();
        cmds.pop_front();

        // if the ioservice has been stopped (e.g abortall call)
        // reset it and spawn a new boost::asio::io_service::work if the ioservice has been stopped.
        // must be done with mutex held due to a possible race with abortall.
        if (ioservice->stopped()) {
            LOGDEBUG(logger, "ioservice stopped");
            work.reset(new boost::asio::io_service::work(*ioservice));
            ioservice->reset();
        }

        mutex.unlock();

        switch (donow->c) {
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

            case CAsyncCommand::ACCEPT:
                HandleAccept(donow);
            break;

            default:
                LOGDEBUG_SA(logger, __COUNTER__,
                    "BUG: Unknown command " << donow->c << " received.");
            break;
        }

        delete donow;

	}
    LOGDEBUG(logger, "Thread terminating");
	IConnect::_main();
}

bool CConnectTCPAsio::PushWork( CAsyncCommand *cmd )
{
	mutex.lock();
	cmds.push_back(cmd);
	mutex.unlock();
	sem_post(&cmdsemaphore);
	workerthread.interrupt();
	return true;
}

void CConnectTCPAsio::HandleConnect( CAsyncCommand *cmd )
{
	string strhost, port;
    volatile int result_timer = 0;
    boost::system::error_code handler_ec;
    struct asyncASIOCompletionHandler connect_handler(NULL, &handler_ec);

	// if already connected, ignore the command, pretend success.
	if (IsConnected()) {
	    LOGDEBUG_SA(logger, __COUNTER__, __PRETTY_FUNCTION__ << " Already connected");
		cmd->callback->addData(ICMD_ERRNO, 0);
		cmd->HandleCompletion();
        return;
	}

	// fail, if we are configured for inbound connections.
    if (configured_as_server) {
        LOGDEBUG_SA(logger, __COUNTER__,
            "BUG: Configured as server! Unexpeced connect.");
        cmd->callback->addData(ICMD_ERRNO, -EPERM);
        cmd->callback->addData(ICMD_ERRNO_STR,
            std::string("TCP/IP Comms configured as server. Cannot use connect method!"));
        cmd->HandleCompletion();
        return;
    }

	CConfigHelper cfghelper(ConfigurationPath);

	unsigned long timeout = -1;
    try {
        timeout = boost::any_cast<long>(
            cmd->callback->findData(ICONN_TOKEN_TIMEOUT));
    } catch (std::invalid_argument &e) {
        cfghelper.GetConfig("tcptimeout", timeout, TCP_ASIO_DEFAULT_TIMEOUT);
        LOGDEBUG_SA(logger, __COUNTER__, "Depreciated fall back to tcptimeout");
    } catch (boost::bad_any_cast &e) {
        LOGDEBUG(logger, "BUG: Handling Connect: Bad cast for "
                 << ICONN_TOKEN_TIMEOUT);
        timeout = TCP_ASIO_DEFAULT_TIMEOUT;
    }

	cfghelper.GetConfig("tcpadr", strhost);
	cfghelper.GetConfig("tcpport", port);

	boost::system::error_code ec;
	ip::tcp::resolver resolver(*ioservice);
	ip::tcp::resolver::query query(strhost.c_str(), port);

	// returns on error a default constructed iterator ...
	ip::tcp::resolver::iterator iter = resolver.resolve(query, ec);
	ip::tcp::resolver::iterator end; // ... which is a "End marker" itself.

    boost::asio::deadline_timer timer(*ioservice);
    boost::posix_time::time_duration td = boost::posix_time::millisec(timeout);
    timer.expires_from_now(td);
    timer.async_wait(
            boost::bind(&boosthelper_set_result, (int*) &result_timer, 1));

    while (iter != end) {
        ip::tcp::endpoint endpoint = *iter++;
        LOGDEBUG_SA(logger, __COUNTER__, "Connecting to " << endpoint);
        handler_ec.clear();
        sockt->async_connect(endpoint, connect_handler);
        size_t num = ioservice->run_one(ec);
        if (num == 0) {
            LOGDEBUG(logger, __PRETTY_FUNCTION__ << " WTF: no service run!!!");
        }
        if (result_timer) {
            LOGINFO_SA(logger, LOG_SA_HASH("Connection-error-reason"),
                "Connection timeout");
            cmd->callback->addData(ICMD_ERRNO, -ETIMEDOUT);
            cmd->callback->addData(ICMD_ERRNO_STR,
                                   std::string("Connection timeout"));
            cmd->HandleCompletion();
            sockt->cancel(ec);
            ioservice->poll();
            return;
        }
        if (ioservice->stopped()) {
            LOGINFO_SA(logger, LOG_SA_HASH("Connection-error-reason"),
                "Connection aborted");
            cmd->callback->addData(ICMD_ERRNO, -ECANCELED);
            cmd->callback->addData(ICMD_ERRNO_STR,
                                   std::string("Connection aborted"));
            cmd->HandleCompletion();
            ioservice->poll();
            return;
        }

        // not timer, so it must be the connect that returned.
        if (handler_ec.value() == 0) {
            break;
        }
    }

    timer.cancel(ec);
    ioservice->poll(ec);

    if (handler_ec) {
        cmd->callback->addData(ICMD_ERRNO, -ECONNREFUSED);
        if (!handler_ec.message().empty()) {
            LOGDEBUG_SA(logger, __COUNTER__, "Connection error: "
                << handler_ec.message());

            cmd->callback->addData(ICMD_ERRNO_STR, handler_ec.message());
        }
        cmd->HandleCompletion();
        return;
    }

    LOGINFO(logger, "Connected to " << strhost);
    _connected = true;
    cmd->callback->addData(ICMD_ERRNO, 0);
    cmd->HandleCompletion();
    return;
}

void CConnectTCPAsio::HandleDisconnect( CAsyncCommand *cmd )
{
	boost::system::error_code ec;
	std::string message;
	int error = 0;

	if (!IsConnected()) {
		cmd->callback->addData(ICMD_ERRNO, 0);
		cmd->HandleCompletion();
		return ;
	}

	// according boost:asio documentation, one should call shutdown before
	// close
	// (http://www.boost.org/doc/libs/1_36_0/doc/html/boost_asio/reference/basic_stream_socket/close/overload2.html)
	sockt->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

	if (ec) {
		error = -EIO;
		message = ec.message();
		LOGDEBUG_SA(logger,LOG_SA_HASH("diconnect_err"),
		    "Disconnect error while shutdown: " << ec.message());
	}

	ec.clear();
    sockt->close(ec);

	if (ec) {
		error = -EIO;
		if (!message.empty())
			message = message + " ";
		message = message + ec.message();
        LOGDEBUG_SA(logger,LOG_SA_HASH("diconnect_err2"),
            "Disconnect error while close: " << ec.message());
	}

	_connected = false;
	cmd->callback->addData(ICMD_ERRNO, error);
	if (!message.empty()) {
		cmd->callback->addData(ICMD_ERRNO_STR, message);
	}
	cmd->HandleCompletion();
    return;
}

/** Handle Receive -- asynchronous read from the asio socket with timeout.
 *
 * Strategy:
 * -- get timeout config from caller or configuration (depreciated)
 *    -- spawn timer with timeout setting in background
 *    -- run timer in background (async)
 * -- setup async read operation of one byte (to detect incoming communication)
 * -- check if we got a byte or timeout
 * -- if got the byte, try to read all available bytes (ASIO tells us how many
 *    are pending)
 * -- if got the timer, cancel socket read and return error.
*/
void CConnectTCPAsio::HandleReceive( CAsyncCommand *cmd )
{
	boost::system::error_code ec;
	boost::system::error_code read_handlerec;
	volatile int result_timer = 0;
	size_t read_bytes = 0;
	long timeout = 0;
	char buf[2];
	struct asyncASIOCompletionHandler read_handler(&read_bytes, &read_handlerec);

    // when running out of work.
	// on deletion of this object the ioserver might get stopped, if not
	// externally AbortAll has been called.

	// timeout setup
	try {
		timeout = boost::any_cast<long>(
				cmd->callback->findData(ICONN_TOKEN_TIMEOUT));
	} catch (std::invalid_argument &e) {
		CConfigHelper cfghelper(ConfigurationPath);
		cfghelper.GetConfig("tcptimeout", timeout, TCP_ASIO_DEFAULT_TIMEOUT);
        LOGDEBUG_SA(logger, __COUNTER__, "Depreciated fallback to tcptimeout");
	} catch (boost::bad_any_cast &e) {
		LOGDEBUG_SA(logger, __COUNTER__,
				"BUG: Unexpected exception in HandleReceive: Bad cast "
		        << e.what());
		timeout = TCP_ASIO_DEFAULT_TIMEOUT;
	}

	deadline_timer timer(*(this->ioservice));
	boost::posix_time::time_duration td = boost::posix_time::millisec(timeout);
	timer.expires_from_now(td);
	timer.async_wait(
			boost::bind(&boosthelper_set_result, (int*) &result_timer, 1));

	// socket preparation
	// async_read: boost:asio seems not to allow auto-buffers,
	// so we will just read one byte and when this is available, we'll
	// check for if there are some others left

	// seen when connecting to localhost: ioservice returns, with ec = eof,
	// but still connected. So if retry to async_read_some as long as timeout
	// not expired. (but we will limit to a few times in case of a real eof and
	// a long timeout)
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
		sockt->async_read_some(boost::asio::buffer(&buf, 1), read_handler);
		num = ioservice->run_one(ec);
	} while (maxsleep != 0 && !result_timer && 0 == read_bytes);

	// ioservice error or timeout
    if (num == 0 || result_timer) {
        if (result_timer) {
            LOGDEBUG(logger, "Read timeout");
            cmd->callback->addData(ICMD_ERRNO_STR, std::string("Read timeout"));
            cmd->callback->addData(ICMD_ERRNO, -ETIMEDOUT);
        } else if (ioservice->stopped()){
            LOGDEBUG(logger, "ioservice stopped (1)");
            cmd->callback->addData(ICMD_ERRNO_STR, std::string("Aborted 1"));
            cmd->callback->addData(ICMD_ERRNO, -ECANCELED);
       } else {
            LOGDEBUG(logger, "IO Service error: " << ec.message());
            cmd->callback->addData(ICMD_ERRNO_STR,
                std::string("IO-service error " + ec.message()));
            cmd->callback->addData(ICMD_ERRNO, -EIO);
        }
        cmd->HandleCompletion();
        timer.cancel(ec);
        sockt->cancel(ec);
        ioservice->poll(ec);
        return;
    }

	timer.cancel();
	ioservice->poll(ec);

    if (read_handlerec || ioservice->stopped()) {
        if (read_handlerec != boost::asio::error::eof) {
            LOGDEBUG(logger, "Async read failed with ec="
                << read_handlerec << " msg="<< read_handlerec.message());
            cmd->callback->addData(ICMD_ERRNO, -EIO);
            cmd->callback->addData(ICMD_ERRNO_STR, read_handlerec.message());
        } else if (ioservice->stopped()) {
            cmd->callback->addData(ICMD_ERRNO_STR, std::string("Aborted 2"));
            cmd->callback->addData(ICMD_ERRNO, -ECANCELED);
            LOGDEBUG(logger, "ioservice stopped (2)");
        } else {
            cmd->callback->addData(ICMD_ERRNO, -ENOTCONN);
            LOGDEBUG(logger, "Received eof on socket read");
        }
        cmd->HandleCompletion();
        return;
	}

	if (1 != read_bytes) {
		LOGDEBUG(logger,"Received "
				<< read_bytes << " but expected only 1 byte");
		cmd->callback->addData(ICMD_ERRNO, -EIO);
		cmd->HandleCompletion();
		return ;
	}

	size_t avail = sockt->available();
	size_t tmp;
	size_t numrecvd = 1;
	std::string receivestr;

	LOGTRACE(logger, "There are " << avail << " bytes ready to read");
	char recved[256];
	receivestr.append(buf,1);
	while (avail > 0) {
	    size_t readmax = avail > 256 ? avail : 256;

		tmp = sockt->read_some(asio::buffer(recved, readmax), ec);
        if (ioservice->stopped()) {
            cmd->callback->addData(ICMD_ERRNO_STR, std::string("Aborted 3"));
            cmd->callback->addData(ICMD_ERRNO, -ECANCELED);
            LOGTRACE(logger, "ioservice stopped. (3)");
            cmd->HandleCompletion();
            return;
        }
		avail = sockt->available();
		numrecvd += tmp;

		if (tmp) receivestr.append(recved, tmp);

		// check if an error occurred.
		if (ec) {
			int error;

			switch (ec.value()) {
			case boost::asio::error::eof:
				error = -ENOTCONN;
				break;
			default:
				error = -EIO;
				break;
			}

			cmd->callback->addData(ICONN_TOKEN_RECEIVE_STRING, receivestr);
	        LOGDEBUG(logger, "Error while read remaining bytes: " << ec.message());
			cmd->callback->addData(ICMD_ERRNO_STR, ec.message());
			cmd->callback->addData(ICMD_ERRNO, error);
			cmd->HandleCompletion();
			return;
		}
	}

	cmd->callback->addData(ICONN_TOKEN_RECEIVE_STRING, receivestr);
	cmd->callback->addData(ICMD_ERRNO, 0);
	cmd->HandleCompletion();
	return ;
}

/** handles async sending */
void CConnectTCPAsio::HandleSend( CAsyncCommand *cmd ) {
    std::string s;
	boost::system::error_code ec;
	boost::system::error_code write_handler_ec;
	volatile int result_timer = 0;
	size_t wrote_bytes = 0;
	unsigned long timeout;
	struct asyncASIOCompletionHandler write_handler(&wrote_bytes, &write_handler_ec);
	try {
		s = boost::any_cast<std::string>(cmd->callback->findData(ICONN_TOKEN_SEND_STRING));
	}
	catch (std::invalid_argument &e) {
		LOGDEBUG_SA(logger, __COUNTER__, "BUG: HandleSend: "
		    << ICONN_TOKEN_SEND_STRING << " argument missing");
	}
	catch (boost::bad_any_cast &e)
	{
		LOGDEBUG(logger, "BUG: HandleSend: Bad cast " << e.what());
	}

    try {
        timeout = boost::any_cast<long>(cmd->callback->findData(ICONN_TOKEN_TIMEOUT));
    } catch (std::invalid_argument &e) {
        CConfigHelper cfghelper(ConfigurationPath);
        cfghelper.GetConfig("tcptimeout", timeout, 3000UL);
        LOGDEBUG_SA(logger, __COUNTER__, "BUG: Depreciated fallback to tcptimeout");
    } catch (boost::bad_any_cast &e) {
        LOGDEBUG_SA(logger, __COUNTER__,
            "BUG: Unexpected exception in HandleSend: Bad cast " << e.what());
        timeout = TCP_ASIO_DEFAULT_TIMEOUT;
    }

	deadline_timer timer(*(this->ioservice));
	boost::posix_time::time_duration td = boost::posix_time::millisec(timeout);
	timer.expires_from_now(td);
	timer.async_wait(boost::bind(&boosthelper_set_result, (int*) &result_timer,
			1));

	// socket preparation
	//  async_write the whole std::string
	boost::asio::async_write(*sockt, boost::asio::buffer(s), write_handler);

	// run one of the scheduled services
	size_t num = ioservice->run_one(ec);

	// ioservice error or timeout
	if (num == 0 || result_timer) {
		timer.cancel(ec);
		sockt->cancel(ec);
		LOGDEBUG(logger, "Async write timeout");
		cmd->callback->addData(ICMD_ERRNO, -ETIMEDOUT);
		cmd->HandleCompletion();
		ioservice->poll();
        return;
	}

	timer.cancel();
	ioservice->poll();

	LOGTRACE(logger,"Sent " << wrote_bytes << " Bytes of " << s.length());
	if (write_handler_ec) {
		if (write_handler_ec != boost::asio::error::eof) {
			LOGDEBUG(logger,"Async write failed with ec=" << write_handler_ec
					<< " msg="<< write_handler_ec.message());
			cmd->callback->addData(ICMD_ERRNO, -EIO);
			cmd->callback->addData(ICMD_ERRNO_STR, write_handler_ec.message());
		} else {
			cmd->callback->addData(ICMD_ERRNO, -ENOTCONN);
			LOGDEBUG(logger, "Received eof during socket write");
		}
		cmd->HandleCompletion();
		return ;
	}

	if (s.length() != wrote_bytes) {
		LOGDEBUG(logger,"Sent "
				<< wrote_bytes << " but expected "<< s.length() );
		cmd->callback->addData(ICMD_ERRNO, -EIO);
		cmd->HandleCompletion();
		return ;
	}

	cmd->callback->addData(ICMD_ERRNO, 0);
	cmd->HandleCompletion();
	return ;
}

bool CConnectTCPAsio::AbortAll(void)
{
    // obtain mutex
    mutex.lock();
    LOGDEBUG(logger, __PRETTY_FUNCTION__ << " Aborting " << cmds.size() << " backlog entries");
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
    LOGDEBUG(logger, __PRETTY_FUNCTION__ << " Done");
    return true;
}

// Server mode. Listen to incoming connections.
void CConnectTCPAsio::HandleAccept(CAsyncCommand *cmd)
{
    //LOGDEBUG(logger, __PRETTY_FUNCTION__ << " handling " << cmd << "with ICmd " << cmd->callback );
    int port;
    std::string ipadr;
    // Do not accept if already connected.
    // Pretend success in this case.
    if (IsConnected()) {
        cmd->callback->addData(ICMD_ERRNO, 0);
        cmd->HandleCompletion();
        return;
    }

    // Fail if this is not configured as a server.
    if (!configured_as_server) {
        cmd->callback->addData(ICMD_ERRNO,-EPERM);
        cmd->HandleCompletion();
        return;
    }

    CConfigHelper cfghelper(ConfigurationPath);

    cfghelper.GetConfig("tcpadr", ipadr,std::string("any"));
    cfghelper.GetConfig("tcpport", port);

    boost::scoped_ptr<ip::tcp::endpoint> endpoint;

    if (ipadr == "any") {
        endpoint.reset(new ip::tcp::endpoint(ip::tcp::v4(),port));
    } else if ( ipadr == "any_v6")
    {
       endpoint.reset(new ip::tcp::endpoint(ip::tcp::v6(),port));
    } else {
        ip::address adr = ip::address::from_string(ipadr);
       endpoint.reset(new ip::tcp::endpoint(adr,port));
    }

    boost::system::error_code ec;
    LOGINFO(logger,"Waiting for inbound connection on " << ipadr << ":" << port);

    try {
        ip::tcp::acceptor acceptor(*ioservice, *endpoint);
        acceptor.listen();
        acceptor.accept(*sockt, ec);
    } catch (boost::system::system_error &e) {
        std::string errmsg = e.what();
        LOGINFO(logger, "Boost: exception received while accepting: " << errmsg);
        cmd->callback->addData(ICMD_ERRNO,(long)-EIO);
        cmd->callback->addData(ICMD_ERRNO_STR, errmsg);
        cmd->HandleCompletion();
        return;
    }
    if (ec) {
        int eval = -ec.value();
        if (!eval) { eval = -EIO; }
        cmd->callback->addData(ICMD_ERRNO, eval);
        if (!ec.message().empty()) {
            cmd->callback->addData(ICMD_ERRNO_STR, ec.message());
        }
        cmd->HandleCompletion();
        LOGINFO( logger, "Connection failed. Error " << eval << "("
            << ec.message() << ")");
        return;
    }

    LOGINFO(logger, "Connected.");
    _connected = true;
    cmd->callback->addData(ICMD_ERRNO, 0);
    cmd->HandleCompletion();
    return;
}

#endif /* HAVE_COMMS_ASIOTCPIO */
