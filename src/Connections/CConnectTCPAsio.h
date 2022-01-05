/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

Copyright (C) 2009-2012 Tobias Frost

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


/** \file ConnectionTCPAsio.h
 *
 *  Created on: May 21, 2009
 *      Author: tobi
 */

#ifndef CONNECTIONTCPASIO_H_
#define CONNECTIONTCPASIO_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "porting.h"
#endif

#ifdef HAVE_COMMS_ASIOTCPIO

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/io_service.hpp>
#include <semaphore.h>

#include "interfaces/IConnect.h"
#include "interfaces/CWorkScheduler.h"
#include "configuration/Registry.h"
#include "patterns/ICommand.h"
#include "Connections/CAsyncCommand.h"

/// Default timeout for all operations, if not configured
#define TCP_ASIO_DEFAULT_TIMEOUT (3000UL)

using namespace std;

/** This class implements a method to connect to TCP/IP via the boost ASIO
 * library.
 *
 * For the interface documentation, please see IConnect.
 *
 *
 */
class CConnectTCPAsio: public IConnect
{
protected:
	friend class IConnectFactory;
	CConnectTCPAsio( const string & configurationname );

public:
	virtual ~CConnectTCPAsio();

    /** Request Connection
     *
     * Please see the base class IConnect for details
     *
     * @param callback will be called after completion, containing also the
     * result
     *
     * \sa IConnect::Conncect()
    */

    virtual void Connect(ICommand *callback);

    /** Request disconnect
     *
     * Please see the base class IConnect for details
     *
     * @param callback will be called after completion.
     *
     * \sa IConnect::Disconncect()
    */
    virtual void Disconnect(ICommand *callback);

    virtual void Send(ICommand *callback);

    /** Try to read from stream
     *
     * Please see the base class IConnect for details
     *
     * @param callback will be called after completion.
     *
     * \sa IConnect::Receive()
    */
    virtual void Receive(ICommand *callback);

    /**
     * Get current connection status
     *
     * \sa IConnect::IsConnected()
     */
    virtual bool IsConnected(void);

	virtual void Accept(ICommand *cmd);

	virtual bool CanAccept()
    {
        return this->configured_as_server;
    }

	/// Aborts all IOs.
	/// Note: The current executed I/O will also be cancelled, but error
	/// reporting might report a wrong error. (e.g timeout instead of cancelled)
    virtual bool AbortAll(void);

    virtual bool CheckConfig(void);

    virtual void SetupLogger(const string& parentlogger, const string & = "")
    {
        IConnect::SetupLogger(parentlogger, "Comms_TCP_ASIO");
    }

private:
    boost::asio::io_service *ioservice;
    boost::asio::ip::tcp::socket *sockt;

    virtual void _main(void);

    /** push some new work to the worker thread.
     *  \note: If the work can be handled synchronously more efficent, the
     *  work might be executed right away.
     *
     * \param cmd to be executed. Will take ownership of object and destroy
     * it after use. (in other words: will be deleted. But only the struct,
     * not containing objects!)
     *
     * \returns false if work could not be pushed or true if it worked out.
     */
    bool PushWork(CAsyncCommand *cmd);

    /** Handle "Connect-Command"
     *
     * Connects to the configured target.
     *
     * \returns true, if job can be removed from queue, false, if it needs to
     * be handled again
     *
     *
     * */
    void HandleConnect(CAsyncCommand *cmd);

    /** Handle the disconnect command.
     *
     * \returns true, if job can be removed from queue, false, if it needs to
     * be handled again
     */

    void HandleDisconnect(CAsyncCommand *cmd);

    void HandleReceive(CAsyncCommand *cmd);

    void HandleSend(CAsyncCommand *cmd);

    void HandleAccept(CAsyncCommand *cmd);

    list<CAsyncCommand*> cmds;
    sem_t cmdsemaphore;

    bool configured_as_server;

    // Work-around for https://svn.boost.org/trac/boost/ticket/7392
    bool _connected;
};


#endif /* HAVE_COMMS_ASIOTCPIO */
#endif /* CONNECTIONTCP_H_ */
