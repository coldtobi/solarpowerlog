dnl Process this file with autoconf to produce a configure script.

AC_INIT([solarpowerlog],[0.22-dev],[tobi@coldtobi.de])
AC_PREREQ([2.68])

# use own directory for the autotools stuff
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([m4])

LT_INIT

CPPFLAGS="-Wall -Wextra"

# we want this header to be created. 
AC_CONFIG_HEADERS(src/config.h)

# TODO
#AC_COPYRIGHT (copyright-notice )

AM_INIT_AUTOMAKE(solarpowerlog, 0.22-dev)

AC_CONFIG_SRCDIR([src/solarpowerlog.cpp])

# configure extlibs 
AC_CHECK_FILE([extlibs/dbixx/configure])

if test "x$ac_cv_file_extlibs_dbixx_configure" != "xno" ; then
	AC_MSG_NOTICE([dbixx found and will be configured])
	AC_CONFIG_SUBDIRS([extlibs/dbixx])
fi 
AM_CONDITIONAL([MAKE_DBIXX],test "x$ac_cv_file_extlibs_dbixx_configure" != "xno")


### options for some features that might not always be needed 
### (for shrinking the footprint)

## option for disable logging (default: on)
AC_ARG_ENABLE([logging],
	[AS_HELP_STRING([--disable-logging], [Disable logging facility. \
		Error output is limited to stdout/err and not configureable.])
	]
)

## option for disable command-line options support
## as this pulls in a boost library
AC_ARG_ENABLE([commandlineoptions],
	[AS_HELP_STRING([--disable-commandlineoptions], [Disable the ability to specify \
		command-line options. Instead, the program will always use defaults  \
		(that is e.g reading solarpowerlog.conf from the current dir). Avoids using \
		some external libraries.])
	]
)

if test "x$enable_commandlineoptions" != "xno" ; then
	enable_commandlineoptions=yes
else
	AC_MSG_NOTICE([Support for commandlineoptions disabled as requested.])
	enable_commandlineoptions=no
fi

### option to disable some modules. Default is to compile everything. 

## Inverters
# Sputnik support.
AC_ARG_ENABLE([sputnik],
	[AS_HELP_STRING([--disable-sputnik],
			[Do not support inverters by Sputnik engineering.])
	]
)

if test "x$enable_sputnik" != "xno" ; then
	AC_DEFINE([HAVE_INV_SPUTNIK], [1], [Sputnik inverter support])	
	enable_sputnik=yes
else
	AC_MSG_NOTICE([Support for Sputnik inverters disabled as requested.])
fi

# Dummy Inverter
AC_ARG_ENABLE([dummyinverter],
    [AS_HELP_STRING([--disable-dummyinverter],
            [Do not support the dummy inverter.])
    ]
)

if test "x$enable_dummyinverter" != "xno" ; then
    AC_DEFINE([HAVE_INV_DUMMY], [1], [Dummy inverter support])  
    enable_dummyinverter=yes
else
    AC_MSG_NOTICE([Dummy inverter support disabled as requested.])
fi


## Filters / Dumpers

# CSV Writer
AC_ARG_ENABLE([csvlogger],
	[AS_HELP_STRING([--disable-csvlogger],
			[Do not support logging to CSV files.])
	]
)

if test "x$enable_csvlogger" != "xno" ; then
	AC_DEFINE([HAVE_FILTER_CSVDUMP], [1], [CSV Writer Support])	
	enable_csvlogger=yes
else
	AC_MSG_NOTICE([Support for CSV Writer disabled as requested.])
fi

# Dumb Dumper
AC_ARG_ENABLE([dumbdumper],
	[AS_HELP_STRING([--disable-dumbdumper],
			[Do not support the "dumb dumper" logging filter.])
	]
)

if test "x$enable_dumbdumper" != "xno" ; then
	AC_DEFINE([HAVE_FILTER_DUMBDUMP], [1], [Dumb Dumper Support])	
	enable_dumbdumper=yes
else
	AC_MSG_NOTICE([Support for the Dumb Dumper disabled as requested.])
fi


# CSV Writer
AC_ARG_ENABLE([csvlogger],
    [AS_HELP_STRING([--disable-csvlogger],
            [Do not support logging to CSV files.])
    ]
)

if test "x$enable_csvlogger" != "xno" ; then
    AC_DEFINE([HAVE_FILTER_CSVDUMP], [1], [CSV Writer Support]) 
    enable_csvlogger=yes
else
    AC_MSG_NOTICE([Support for CSV Writer disabled as requested.])
fi

# HTML Writer
AC_ARG_ENABLE([htmlwriter],
    [AS_HELP_STRING([--disable-htmlwriter],
            [Do not support the html writer module for writing static HTML files.])
    ]
)

if test "x$enable_htmlwriter" != "xno" ; then
    AC_DEFINE([HAVE_FILTER_HTMLWRITER], [1], [HTML Writer Support])   
    enable_htmlwriter=yes
    CTEMPLATE_LIB=-lctemplate
    CTEMPLATE_CFLAGS="-I ../extlibs"
    CTEMPLATE_DEPS=libctemplate.a
else
    AC_MSG_NOTICE([Support for the HTMLWriter disabled as requested.])
fi

## Communication Methods

# Boost::asio::tcpip connection 
AC_ARG_ENABLE([tcpcomms],
    [AS_HELP_STRING([--disable-tcpcomms],
            [Do not support TCP/IP connections to the inverter.])
    ]
)

if test "x$enable_tcpcomms" != "xno" ; then
    AC_DEFINE([HAVE_COMMS_ASIOTCPIO], [1], [TCP/IP communication support]) 
    enable_tcpcomms=yes
else
    AC_MSG_NOTICE([Support for TCP/IP Communications disabled as requested.])
fi

# Boost::asio::serial_port connection 
AC_ARG_ENABLE([serialcomms],
    [AS_HELP_STRING([--disable-serialcomms],
            ["Do not support serial port -- like RS232 -- connections to the inverter."])
    ]
)

if test "x$enable_serialcomms" != "xno" ; then
    AC_DEFINE([HAVE_COMMS_ASIOSERIAL], [1], [Serial port communication support]) 
    enable_serialcomms=yes
else
    AC_MSG_NOTICE([Support for serial port communications disabled as requested.])
fi

# Shared Connection provider
AC_ARG_ENABLE([sharedcomms],
   [AS_HELP_STRING([--enable-sharedcomms],
            ["Support sharing a communication object between two or more inverters. (experimental)"])
    ]
)

if test "x$enable_sharedcomms" = "xyes" ; then
    AC_DEFINE([HAVE_COMMS_SHAREDCONNECTION], [1], [Shared communication support]) 
    enable_sharedcomms=yes
else
    AC_MSG_NOTICE([Support for shared communications disabled as requested.])
    enable_sharedcomms=no
fi


AC_SUBST(CTEMPLATE_LIB)
AC_SUBST(CTEMPLATE_CFLAGS)
AC_SUBST(CTEMPLATE_DEPS)

### Checks for programs.
# C Compiler
AC_PROG_CC
# C++ Compiler
AC_PROG_CXX
# pkg-config
PKG_CHECK_EXISTS

# We only use C++...
AC_LANG(C++)

### Checks for header files.
AC_CHECK_HEADER([w32api/wtypes.h],
 [AC_DEFINE([HAVE_WIN32_API], [1], [Windows API]) ])

if test "x$ac_cv_header_w32api_wtypes_h" = "xyes"; then
  AC_MSG_RESULT([WIN32 API detected. ])
  WIN32_LIBS="-lws2_32"
  AC_SUBST(WIN32_LIBS)
fi

### Checks for libraries.

## Check for libconfig. Always required.
PKG_CHECK_MODULES(CONFIG, libconfig++ >= 1.3.1)

## Check for library liblog4cxx. Can be turned off with --disable-logging.  
if test "x$enable_logging" != "xno" ; then
	logging_dependencies_fullfilled="missing"

	PKG_CHECK_MODULES(LOG4CXX, liblog4cxx >= 0.10.0, , AC_MSG_RESULT([LibLog4CXX not found]))
	if test "x$LOG4CXX_PKG_ERRORS" != "x"; then
		logging_dependencies_fullfilled="missing liblog4cxx " 
	fi

	# libapr / aprutils only for win32 build needed, dont ask me why
	if test  "x$ac_cv_header_w32api_wtypes_h" = "xyes"; then
		PKG_CHECK_MODULES(APR, apr-1 > 1, , AC_MSG_RESULT([apache portable runtime apr library not found]))
		if test "x$APR_PKG_ERRORS" != "x"; then
			logging_dependencies_fullfilled="$logging_dependencies_fullfilled libapr " 
		fi
		
		PKG_CHECK_MODULES(APRUTIL, apr-util-1 > 1, , AC_MSG_RESULT([apr utilities library not found]))
		if test "x$APRUTIL_PKG_ERRORS" != "x"; then
			logging_dependencies_fullfilled="$logging_dependencies_fullfilled libaprutil " 
		fi
	fi	

	if test "$logging_dependencies_fullfilled" != "missing" ; then
		AC_MSG_NOTICE([liblog4cxx: Dependecy problem(s): $logging_dependencies_fullfilled])
		enable_logging="no (dependency problem(s): $logging_dependencies_fullfilled)"
		APR_LIBS=""
		APRUTIL_LIBS=""
		LOG4CXX_LIBS=""
	else
		AC_DEFINE([HAVE_LIBLOG4CXX], [1], [Logging Library])	
		enable_logging="yes"
	fi

else
	AC_MSG_NOTICE([Logging disabled as requested.])
fi

## Check for Boost 
BOOST_REQUIRE([1.38])

# Add the option to statically link to the boost libraries 
BOOST_STATIC()

#Check for ASIO (header only file)
BOOST_ASIO()

#Check for boost libraries
BOOST_SYSTEM()
BOOST_THREADS()

#BOOST_DATE_TIME()

if test "x$enable_commandlineoptions" = "xyes"; then
	BOOST_PROGRAM_OPTIONS
	if test "x$BOOST_PROGRAM_OPTIONS_LIBS" = "x"; then
		AC_MSG_NOTICE([boost::program_options library $BOOST_PROGRAM_OPTIONS_LIBS not found ])
		enable_commandlineoptions="no (dependency problem)"
	else
		AC_DEFINE([HAVE_CMDLINEOPTS], [1], [Use Boost for parsing command line options.])	
	fi
fi

### Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_HEADER_TIME
AC_HEADER_ASSERT
AC_TYPE_SIZE_T


### Checks for library functions.
#AC_CHECK_FUNCS([socket])

# Cygwin has no open_memstream, at least not in the current stable version. 
AC_CHECK_FUNC(open_memstream, AC_DEFINE(HAVE_OPEN_MEMSTREAM, 1, [1 if open_memstream is available]))

# Lets check if we have a syslog here, as we want to prepare it if we run in background. 
AC_CHECK_FUNC(openlog, [have_openlog=yes], [have_openlog=no])
if test "$have_openlog" = "yes" ;then
 AC_DEFINE([HAVE_OPENLOG], [1], [Are the syslog-calls available on this system])
fi  
  
AC_PROG_INSTALL

AC_CONFIG_FILES(
	[Makefile]
	[src/Makefile]
)
AC_OUTPUT

AC_MSG_NOTICE([]);
AC_MSG_NOTICE([CONFIGURATION SUMMARY:]);


### Print summary
AC_MSG_NOTICE([Logging support: ........................... $enable_logging])
if test "$have_openlog" = "no"; then
AC_MSG_NOTICE([                  No support for syslog detected])
fi
AC_MSG_NOTICE([Commandline options: ....................... $enable_commandlineoptions])

AC_MSG_NOTICE([INVERTERS:]);
AC_MSG_NOTICE([Sputnik support: ........................... $enable_sputnik])
AC_MSG_NOTICE([Dummy inverter: ............................ $enable_dummyinverter])

AC_MSG_NOTICE([COMMUNICATIONS:]);
AC_MSG_NOTICE([TCP/IP: .................................... $enable_tcpcomms])
AC_MSG_NOTICE([Serial Ports: .............................. $enable_serialcomms])
AC_MSG_NOTICE([Shared Communication: ...................... $enable_sharedcomms])

AC_MSG_NOTICE([FILTERS AND LOGGERS:]);
AC_MSG_NOTICE([CSV logger support: ........................ $enable_csvlogger])
AC_MSG_NOTICE([Dumb Dumper support: ....................... $enable_dumbdumper])
AC_MSG_NOTICE([HTML Writer support: ....................... $enable_htmlwriter])
