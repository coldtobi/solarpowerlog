/* ----------------------------------------------------------------------------
 solarpowerlog
 Copyright (C) 2009-2011  Tobias Frost

 This file is part of solarpowerlog.

 Solarpowerlog is free software; However, it is dual-licensed
 as described in the file "COPYING".

 For this file (CAsyncCommand.h), the license terms are:

 You can redistribute it and/or  modify it under the terms of the GNU Lesser
 General Public License (LGPL) as published by the Free Software Foundation;
 either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Library General Public
 License along with this proramm; if not, see
 <http://www.gnu.org/licenses/>.
 ----------------------------------------------------------------------------
 */

/*
 * CAsyncCommand.h
 *
 *  Created on: Dec 14, 2009
 *      Author: tobi
 */

#ifndef CASYNCCOMMAND_H_
#define CASYNCCOMMAND_H_

#include <semaphore.h>
#include "patterns/ICommand.h"

class CAsyncCommand
{
public:
	enum Commando
	{
		DISCONNECT, /// Tear down a connection
		CONNECT, /// Connect
		SEND, /// Send data
		RECEIVE,
	/// Try to receive data
	};

	/** Constructor which create the object
	 *
	 * \param c Commando to be used
	 * \param callback ICommand used as callback. Might be NULL. In this case the semaphore mechanism is used.
	 * \param sem Semaphore. If callback is NULL, for the completion handling this semaphore is used.
	 * The caller just waits for the semaphore (sem_wait) and the program will wait for the completion of the
	 * async command.
	 *
	 */
	CAsyncCommand( enum Commando c, ICommand *callback, sem_t *sem = NULL );

	/** Destructor */
	~CAsyncCommand();

	/** Setter for "late setting" the to-be used semaphore.
	*/
	inline void SetSemaphore( sem_t *sem )
	{
		this->sem = sem;
	}


	/** Handle this jobs completion by notifying the sender
	 *
	 */
	void HandleCompletion( void );

	/** Is the asyncCommnd really async, or was it only pretended?
	 *
	 * As syncronous operations are also dispatched asynchronous,
	 * but we need a ICommand-object for this, we need the information
	 * if it is sync or not to decide when to delete the object.
	 * */
	inline bool IsAsynchronous()
	{
		return !private_icommand;
	}

	enum Commando c; ///< what to do

	/** callback for completion handling
	 * In this ICommand, the comand data is stored, results and data...
	 * This ICommand is privately created, if private_icommand is true,
	 * and will not be executed if so, but can be still be used for
	 * storage  */
	ICommand *callback;

private:
	sem_t *sem; ///< if not-null, use this semaphore to notify completion.
	/// \note: it is context specific to check if it worked out or not.
	/// The semaphore is intended to be used for the "synchronous fallback"

	bool private_icommand;

};
#endif /* CASYNCCOMMAND_H_ */
