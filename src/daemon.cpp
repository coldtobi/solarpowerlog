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

/*
 * signalhandler.c
 *
 *  Created on: 01.10.2012
 *      Author: tobi
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "daemon.h"

#include "configuration/ILogger.h"
#include <boost/any.hpp>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdio.h>
#include "configuration/Registry.h"
#ifdef HAVE_BACKTRACE_SYMBOLS_FD
#include <execinfo.h>
#endif
#include "Inverters/BasicCommands.h"
#include "patterns/ICommand.h"

volatile sig_atomic_t killsignal = false;
volatile sig_atomic_t sigusr1 = false;

char *progname;
std::string rundir("/");
std::string daemon_stdout("/dev/null");
std::string daemon_stderr("/dev/null");

std::string pidfile="";
int pidfile_fd = 0;

boost::thread terminator_thread;

bool background = false;

void cleanup(void) {
    if (background && pidfile_fd) unlink(pidfile.c_str());
    pidfile_fd=0;
}


void logreopen(bool rotate) {

    ILogger mainlogger;

    if (!rotate) {
        if (!freopen("/dev/null", "r", stdin)) {
            LOGWARN(mainlogger, "daemonize: Could not reopen stdin. errno=" << errno);
        }
    }

    // do not rotate stdout when redirected to /dev/null
    if (!rotate || daemon_stdout != "/dev/null" ) {
        if (!freopen(daemon_stdout.c_str(), "a", stdout)) {
            LOGFATAL(mainlogger, "daemonize: Could not reopen stdout. errno=" << errno);
            // try to rectify things... a closed stdout might not be a good idea...
            exit(1);
        }
    }

    // also, do not rotate /dev/null on stderr
    if (!rotate || daemon_stderr != "/dev/null" ) {
        if (!freopen(daemon_stderr.c_str(), "a", stderr)) {
            LOGFATAL(mainlogger, "daemonize: Could not reopen stderr. errno=" << errno);
            // try to rectify things... a closed stderr might not be a good idea...
            exit(1);
        }
    }

}

void daemonize(void)
{
    ILogger mainlogger;

    // generate pidfile
    if (pidfile != "") {
        LOGINFO(mainlogger, "Using PID file " << pidfile );
        pidfile_fd = open(pidfile.c_str(),O_WRONLY | O_CREAT | O_EXCL,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

        if( pidfile_fd == -1) {
            LOGFATAL(mainlogger,"Cannot generate pidfile " << pidfile <<" Reason: " << strerror(errno));
            exit(1);
        }
    } else {
        pidfile_fd = 0;
    }

    // dameonize.
    LOGDEBUG(mainlogger, "daemonize: Rising a daemon.");

    // reopen stdxxx now, because afterwards we will loose our communication channel.
    LOGDEBUG(mainlogger, "reopening stdout / stderr to log file");
    logreopen(false);

    LOGDEBUG(mainlogger, "daemonize: logfiles redirected.");

    pid_t pid, sid;
    pid = fork();
    if (pid < 0) {
        LOGFATAL(mainlogger, "Could not dameonize. fork() error="<<errno );
        cleanup();
        exit(1);
    }

    if (pid > 0) {
        // on daemons, the parent use _exit.
        _exit(EXIT_SUCCESS);
    }

    sid = setsid();
    if (sid < 0) {
        cleanup();
        LOGFATAL(mainlogger, "Could not dameonize. setsid() error="<<errno );
        exit(EXIT_FAILURE);
    }

    // the second fork seems not to be necessary on linux, but believing
    // references, it is for for some unixes to avoid reattached to some
    // controlling tty
    // http://web.archive.org/web/20020416015637/www.whitefang.com/unix/faq_2.html#SEC16
    pid = fork();
    if (pid < 0) {
        cleanup();
        LOGFATAL(mainlogger, "Could not dameonize. 2nd fork() error="<<errno );
        exit(1);
    }

    if (pid > 0) {
        // on daemons, the parent use _exit.
        _exit(EXIT_SUCCESS);
    }

    // change umask to 000, change dir to rundir to avoid locking the directory
    // we have been started from
    umask(0);
    if (0 != chdir(rundir.c_str())) {
        LOGERROR(mainlogger, "Could not chdir() to " << rundir << " error=" <<errno);
    }

    // write pid to pid file
    if (pidfile_fd) {
        char buf[64];
        snprintf(buf,63,"%d\n",getpid());
        write(pidfile_fd,buf,strlen(buf));
        close(pidfile_fd);
    }
}


struct boostthreadcallable{
    void operator()(ICommand* cmd ) {
        // we will just wait until we get the mutex in this case and then terminate.
        Registry::GetMainScheduler()->ScheduleWork(cmd);
    };
};

void SignalHandler(int signal)
{
    switch (signal) {

        case SIGTERM:
            // die.
            if (!killsignal) {
                killsignal = true;
                LOGINFO(
                    Registry::GetMainLogger(),
                    progname << " Termination requested. Will terminate at next opportunity.");


                ICommand* cmd = new ICommand(BasicCommands::CMD_BRC_SHUTDOWN, NULL);
                if (!Registry::GetMainScheduler()->ScheduleWork(cmd,true)) {
                    // bad luck -- received SIGTERM in the moment when the mutex was hold.
                    // so we spawn a task to do it for us...
                    boostthreadcallable callable;
                    terminator_thread = boost::thread(callable,boost::ref(cmd));
                }
            } else {
                LOGFATAL(
                    Registry::GetMainLogger(),
                    progname << " Termination signal received. Please be patient.");
            }
        break;
        case SIGSEGV:
        {
            cleanup();
            cerr << progname << " Segmentation fault. " << endl;
            cerr << "Trying to dump internal state information" << endl;
            Registry::Instance().DumpDebugCollection();
            LOGFATAL(Registry::GetMainLogger(),
                progname << " Segmentation fault.");
#ifdef HAVE_BACKTRACE_SYMBOLS_FD
            // try to print a backtrace.
            LOGFATAL(Registry::GetMainLogger(),"Trying a backtrace to stderr:");
            {
                void *trace[64];
                int count = backtrace( trace, 64 );
                backtrace_symbols_fd(trace, count, 2);
            }
#endif
            raise(signal);
            break;
        }
        case SIGUSR1: {
            LOGINFO(Registry::GetMainLogger(),"SIGUSR1 received");
            sigusr1 = true;
            break;
        }

        case SIGUSR2: {
            cerr << "SIGUSR1 received" << endl;
            cerr << "Trying to dump internal state information" << endl;
            Registry::Instance().DumpDebugCollection();
            break;
        }
    }
}


void SetupSignalHandler(void)
{
    struct sigaction sa;
    sa.sa_handler = SignalHandler;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    // Sigsegv - after trigered, set to default behaviour.
    sa.sa_flags = SA_RESETHAND;
    sigaction(SIGSEGV, &sa, NULL);
}
