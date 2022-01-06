/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

 Copyright (C) 2009-2014 Tobias Frost

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

#ifndef ISPUTNIKCOMMAND_H_
#define ISPUTNIKCOMMAND_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "patterns/CValue.h"
#include "Inverters/Capabilites.h"
#include "Inverters/interfaces/InverterBase.h"
#include "Inverters/SputnikEngineering/SputnikCommand/BackoffStrategies/ISputnikCommandBackoffStrategy.h"
#include "configuration/ILogger.h"

// TODO Refactor interface to IInverterCommand and abstract as far a possible
// to have a general inverter command abstraction class, not specific to sputnik
// (maybe after Danfoss branch has been merged)

/**
 * \file ISputnikCommand.h
 *
 *  Abstracts the definition and handling of a single command for the Sputnik Inverter.
 *
 *  The past implementation had a function for every command the inverter
 *  understands, consisting some thousand lines of basically identical code.
 *  To add one command the file had to be edited on several page.
 *
 *  To fight this and increase maintainability along reducing code complexity this
 *  "strategy" pattern has been implemented.
 *  Using this pattern the inverter has (except for the creation of the
 *  CSputnikCommand) no special knowledge about the commands it will issue.
 *
 *  The objects encapsulates all information needed to handle the command
 *   - information about the "command token" (token itself, length, max answer
 *     length)
 *   - information about the data we receive (type, capability name)
 *   - handles the registration of this data with the inverter.
 *
 *  This interface is the base class for the commands. There are two basic
 *  variants of derived classes:
 *  - Most of the commands use simple datatype (integer, float, boolean ...)
 *  - Some commands needs special implementation (for example the Software-
 *    Version needs to assemble the information from two inverter queries)
 *
 *  For the first category, the templated "CSputnikCommand<type>" can be used,
 *  for the others derived classes like the "CSputnikCommandSoftwareVersion"
 *  come into the game.
 */

/*
 *
 * Created on: 19.05.2012
 *      Author: tobi
*/

/** Interface Class for the Commands.
 *
 * Implements also the common storage and methods */
class ISputnikCommand
{

public:
    /** Constructs the ISputnikCommand.
     *
     * @param parentlogger to connect to
     * @param cmd to be issued to get this data
     * @param maxanswerlen estimated maximum answer length
     * @param inv inverter belonwing to this command
     * @param capname Capabilityname associated with this command
     * @param backoffstrategy backoffs for this command. If núll "BOAlways" will
     *   be assumed. Object ownership is transferred to this class.
     */
    ISputnikCommand(const ILogger &parentlogger, const std::string &cmd,
        int maxanswerlen, IInverterBase *inv, const std::string & capname,
        ISputnikCommandBackoffStrategy *backoffstrategy);

    virtual ~ISputnikCommand();

    /** Returnes the maximium expected length (usually set at compile time)
     *
     * \returns max answer length for this command
     * */
    virtual int GetMaxAnswerLen(void) {
        return max_answer_len;
    }

    /** Setter for the Max Answer Len, if needed
     *
     * @param max new maximum length
     */
    virtual void SetMaxAnswerLen(int max) {
        max_answer_len = max;
    }

    /** Should we consider this command to be issued?
     *
     * This function also handles the calls to the backoff strategies.
     *
     * \note If this function is overriden in your class, call
     * strat->ConsiderCommand -- if it returns false, you also return false.
     *
     * \returns true if so, else do not issue command at this time.
     */
    virtual bool ConsiderCommand();

    /** Returns a const reference to the token string to be used for the communication
     * when requesting the data from the inverter.
     * (note: can be overriden for complex datas, for example if more than one command is
     * required to get thw whole set.
     *
     * \returns reference to string containing the command.
     */
    virtual const std::string& GetCommand(void) {
        return command;
    }

    /** Helper: Return the length of the command to be issued.
     *
     * @return length of the command to be issued.
     */
    virtual unsigned int GetCommandLen(void)
    {
        return command.length();
    }

    /** Check if the token is handled by this instance.
     * \returns true if it is, else false
     *
     * For complex data, which is assembled from more than one command, this
     * needs be overridden.
    */
    virtual bool IsHandled(const std::string &token)
    {
        return (token == command);
    }

    /** handles the parsing and then the capability.
     * must be implemented in the derived class.
     *
     * \note You must call strat->CommandAnswered() in your derived class before
     * you return true.
     *
     * @param vector containing tokenized components. [0] is the command echoed
     * by the Inverter and [1] is the value
     *
     * @return true when sucessuflly handled, false if e.g parse error occoured.
     */
    virtual bool handle_token(const std::vector<std::string> &) = 0;

    /** command was sent, but no answer received
     *  (will only be called when there was no other error, like communication
     * etc)
     */

    /** Notify the class that the command has not been answered by the inverter
     *
     * If a command has been issued but not answered, e.g ignored by the inveter
     * due to being not supported, this function must be called.
     *
     * This is for the backoff strategy handling.
     *
     * \sa ISputnikCommandBackoffStrategy::CommandAnswered
     * \sa ISputnikCommandBackoffStrategy::CommandNotAnswered
     */
    virtual void CommandNotAnswered() {
        this->strat->CommandNotAnswered();
    }

    /** Inform the class that the inverter has been disconnected
     *
     * Used to reset the backoff strategies to have a fresh start on reconnects.
     *
     */
    virtual void InverterDisconnected() {

        CCapability *cap = inverter->GetConcreteCapability(this->capaname);
        if (cap) {
            cap->getValue()->Invalidate();
            cap->Notify();
        }
        this->strat->Reset();
    }

protected:
    /** Makes the complete capability handling:
     *
     * Please see the overridden version (with capaname as parameter)
     * for details.
     */
    template <class T>
    void CapabilityHandling(T value) const throw() {
        this->CapabilityHandling<T>(value, this->capaname);
    };

    /** Makes the complete capability handling:
     * - Registers capability if not existing (incl. notifyng of the observers)
     * - Updates capability with new value (incl. notifying of observers.)
     * - Notifcation is only done when the value actually changes.
     * - Type-check of capabilities (throws if not) (only happens if capability
     *   handling is not done consequently, that is using types-defines in
     *   Capabilites.h)
     *
     *   \param value Value to be stored
     *   \param capname Capability-name.
     *
     *   \throw exception if types are mismatching.
     */
    template <class T>
    void CapabilityHandling(T value, const std::string &capname) const throw() {
        assert(inverter);
        CCapability *cap = inverter->GetConcreteCapability(capname);

        if (!cap) {
           IValue *v = new CValue<T>;
           ((CValue<T>*)v)->Set(value);
           cap = new CCapability(capname,v,inverter);
           inverter->AddCapability(cap);
           inverter->GetConcreteCapability(CAPA_CAPAS_UPDATED)->Notify();
           cap->Notify();
           return;
        }

        // Check for the type and throw an exception if not equal.
        // (as capabilities are created by this class, this should be not happen)
         if ( CValue<T>::IsType(cap->getValue())) {
            CValue<T> *v = (CValue<T> *)cap->getValue();
            if (!v->IsValid() || value != v->Get()) {
                v->Set(value);
                cap->Notify();
            }
            return;
        } else {
            std::bad_cast e;
            LOGERROR(inverter->logger,"Bad cast for command " + command);
            throw e;
        }
    }

protected:
    std::string command;
    int max_answer_len;
    IInverterBase *inverter;
    std::string capaname;
    ISputnikCommandBackoffStrategy *strat;
    ILogger logger;
};

#endif /* ISPUTNIKCOMMAND_H_ */
