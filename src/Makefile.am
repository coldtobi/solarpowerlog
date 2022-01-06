## Makefile.am -- Process this file with automake to produce Makefile.in
ACLOCAL_AMFLAGS = -I m4

AM_CXXFLAGS = $(BOOST_CPPFLAGS)

bin_PROGRAMS = solarpowerlog

solarpowerlog_SOURCES = configuration/CConfigHelper.cpp \
configuration/CConfigHelper.h \
configuration/ConfigCentral/CConfigCentral.cpp \
configuration/ConfigCentral/CConfigCentralEntry.cpp \
configuration/ConfigCentral/CConfigCentralEntry.h \
configuration/ConfigCentral/CConfigCentral.h \
configuration/ConfigCentral/IConfigCentralEntry.cpp \
configuration/ConfigCentral/IConfigCentralEntry.h \
configuration/ConfigCentral/CConfigCentralEntryRangeCheck.cpp \
configuration/ConfigCentral/CConfigCentralEntryRangeCheck.h \
configuration/ConfigCentral/CConfigCentralEntryText.cpp \
configuration/ConfigCentral/CConfigCentralEntryText.h \
configuration/ConfigCentral/ConfigCentralHelpers.cpp \
configuration/ConfigCentral/ConfigCentralHelpers.h \
configuration/ILogger.cpp \
configuration/ILogger.h \
configuration/ILogger_hashmacro.h \
configuration/Registry.cpp \
configuration/Registry.h \
Connections/CAsyncCommand.cpp \
Connections/CAsyncCommand.h \
Connections/CConnectDummy.cpp \
Connections/CConnectDummy.h \
Connections/CConnectSerialAsio.cpp \
Connections/CConnectSerialAsio.h \
Connections/CConnectTCPAsio.cpp \
Connections/CConnectTCPAsio.h \
Connections/factories/IConnectFactory.cpp \
Connections/factories/IConnectFactory.h \
Connections/interfaces/IConnect.cpp \
Connections/interfaces/IConnect.h \
Connections/sharedconnection/CSharedConnection.cpp \
Connections/sharedconnection/CSharedConnection.h \
Connections/sharedconnection/CSharedConnectionMaster.cpp \
Connections/sharedconnection/CSharedConnectionMaster.h \
Connections/sharedconnection/CSharedConnectionSlave.cpp \
Connections/sharedconnection/CSharedConnectionSlave.h \
ctemplate/ctemplate.c \
ctemplate/ctemplate.h porting.h \
daemon.cpp \
daemon.h \
DataFilters/CCSVOutputFilter.cpp \
DataFilters/CCSVOutputFilter.h DataFilters/HTMLWriter/CHTMLWriter.h \
DataFilters/CDumpOutputFilter.cpp \
DataFilters/DBWriter/CdbInfo.cpp \
DataFilters/DBWriter/CdbInfo.h \
DataFilters/DBWriter/CDBWHSpecialTokens.cpp \
DataFilters/DBWriter/CDBWHSpecialTokens.h \
DataFilters/DBWriter/CDBWriterFilter.cpp \
DataFilters/DBWriter/CDBWriterFilter.h \
DataFilters/DBWriter/CDBWriterHelper.cpp \
DataFilters/DBWriter/CDBWriterHelper.h \
DataFilters/HTMLWriter/CHTMLWriter.cpp \
DataFilters/HTMLWriter/formatter/CFormaterWebRootStrip.cpp \
DataFilters/HTMLWriter/formatter/CFormaterWebRootStrip.h \
DataFilters/HTMLWriter/formatter/CFormatterSearchCSVEntry.cpp \
DataFilters/HTMLWriter/formatter/CFormatterSearchCSVEntry.h \
DataFilters/HTMLWriter/formatter/IFormater.cpp \
DataFilters/HTMLWriter/formatter/IFormater.h \
DataFilters/interfaces/factories/IDataFilterFactory.cpp \
DataFilters/interfaces/factories/IDataFilterFactory.h \
DataFilters/interfaces/IDataFilter.cpp \
DataFilters/interfaces/IDataFilter.h DataFilters/CDumpOutputFilter.h \
interfaces/CCapability.cpp \
interfaces/CCapability.h \
interfaces/CDebugHelper.cpp \
interfaces/CDebugHelper.h \
interfaces/CMutexHelper.cpp \
interfaces/CMutexHelper.h \
interfaces/CTimedWork.cpp \
interfaces/CTimedWork.h \
interfaces/CWorkScheduler.cpp \
Inverters/BasicCommands.h \
Inverters/Capabilites.h \
Inverters/DummyInverter/CInverterDummy.cpp  \
Inverters/DummyInverter/CInverterDummy.h  \
Inverters/DummyInverter/CInverterFactoryDummy.cpp \
Inverters/DummyInverter/CInverterFactoryDummy.h \
Inverters/factories/IInverterFactory.cpp \
Inverters/factories/IInverterFactory.h interfaces/CWorkScheduler.h \
Inverters/factories/InverterFactoryFactory.cpp \
Inverters/factories/InverterFactoryFactory.h \
Inverters/interfaces/CNestedCapaIterator.cpp \
Inverters/interfaces/CNestedCapaIterator.h \
Inverters/interfaces/ICapaIterator.cpp\
Inverters/interfaces/ICapaIterator.h \
Inverters/interfaces/InverterBase.cpp \
Inverters/interfaces/InverterBase.h \
Inverters/SputnikEngineering/CInverterFactorySputnik.cpp \
Inverters/SputnikEngineering/CInverterFactorySputnik.h \
Inverters/SputnikEngineering/CInverterSputnikSSeries.cpp \
Inverters/SputnikEngineering/CInverterSputnikSSeries.h \
Inverters/SputnikEngineering/CInverterSputnikSSeriesSimulator.cpp \
Inverters/SputnikEngineering/CInverterSputnikSSeriesSimulator.h \
Inverters/SputnikEngineering/SputnikCommand/BackoffStrategies/CSputnikCmdBOAlways.cpp \
Inverters/SputnikEngineering/SputnikCommand/BackoffStrategies/CSputnikCmdBOAlways.h \
Inverters/SputnikEngineering/SputnikCommand/BackoffStrategies/CSputnikCmdBOIfSupported.cpp \
Inverters/SputnikEngineering/SputnikCommand/BackoffStrategies/CSputnikCmdBOIfSupported.h \
Inverters/SputnikEngineering/SputnikCommand/BackoffStrategies/CSputnikCmdBOOnce.cpp \
Inverters/SputnikEngineering/SputnikCommand/BackoffStrategies/CSputnikCmdBOOnce.h \
Inverters/SputnikEngineering/SputnikCommand/BackoffStrategies/CSputnikCmdBOTimed.cpp \
Inverters/SputnikEngineering/SputnikCommand/BackoffStrategies/CSputnikCmdBOTimed.h \
Inverters/SputnikEngineering/SputnikCommand/BackoffStrategies/ISputnikCommandBackoffStrategy.cpp \
Inverters/SputnikEngineering/SputnikCommand/BackoffStrategies/ISputnikCommandBackoffStrategy.h \
Inverters/SputnikEngineering/SputnikCommand/CSputnikCommand.cpp \
Inverters/SputnikEngineering/SputnikCommand/CSputnikCommand.h \
Inverters/SputnikEngineering/SputnikCommand/CSputnikCommandSoftwareVersion.cpp \
Inverters/SputnikEngineering/SputnikCommand/CSputnikCommandSoftwareVersion.h \
Inverters/SputnikEngineering/SputnikCommand/CSputnikCommandSYS.cpp \
Inverters/SputnikEngineering/SputnikCommand/CSputnikCommandSYS.h \
Inverters/SputnikEngineering/SputnikCommand/CSputnikCommandTYP.cpp \
Inverters/SputnikEngineering/SputnikCommand/CSputnikCommandTYP.h \
Inverters/SputnikEngineering/SputnikCommand/ISputnikCommand.cpp \
Inverters/SputnikEngineering/SputnikCommand/ISputnikCommand.h \
patterns/CValue.h \
patterns/ICommand.cpp \
patterns/ICommand.h \
patterns/ICommandTarget.cpp \
patterns/ICommandTarget.h \
patterns/IObserverObserver.cpp \
patterns/IObserverObserver.h \
patterns/IObserverSubject.cpp \
patterns/IObserverSubject.h \
patterns/IValue.cpp \
patterns/IValue.h \
solarpowerlog.cpp

solarpowerlog_CPPFLAGS = $(CONFIG_CFLAGS) $(LOG4CXX_CFLAGS) $(APR_CFLAGS) \
	$(APRUTIL_CFLAGS) $(CTEMPLATE_CFLAGS)

solarpowerlog_LDADD = $(CONFIG_LIBS) $(LOG4CXX_LIBS) $(APR_LIBS) \
	$(APRUTIL_LIBS) $(BOOST_LDFLAGS) $(BOOST_THREAD_LIBS) \
	$(BOOST_DATE_TIME_LIBS) $(BOOST_SYSTEM_LIBS) $(BOOST_ASIO_LIBS) \
	$(BOOST_PROGRAM_OPTIONS_LIBS) $(WIN32_LIBS) $(CPPDB_LIBS)

LIBS = $(DEPS_LIBS)

# add the search path to the linker path
#solarpowerlog_LDFLAGS = $(pwd)

