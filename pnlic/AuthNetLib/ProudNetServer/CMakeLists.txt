cmake_minimum_required(VERSION 3.0.0)
project(AuthNetServer)

set(CMAKE_CXX_STANDARD 11)

file(GLOB SOURCES
    ../src/acrimpl_s.cpp
    #../src/AdoWrap.cpp
    #../src/AgentConnector.cpp
    ../src/Base64.cpp
    #../src/DbCache2Isolate.cpp
    #../src/dbcacheclient2blocked.cpp
    #../src/DbCacheClient2Impl.cpp
    #../src/dbcacheclient2load.cpp
    #../src/DBCacheJob.cpp
    #../src/DbCacheObject.cpp
    #../src/DbCacheServer2Impl.cpp
    #../src/dbcacheserver2load.cpp
    #../src/DbConfig.cpp
    #../src/DbLoadedData.cpp
    #../src/DbLoadedData2.cpp
    #../src/dbloadeddata2_s.cpp
    #../src/DbLogWriter.cpp
    #../src/DbLogWriterImpl.cpp
    #../src/DumpServer.cpp
    #../src/EmergencyLogServer.cpp
    ../src/FallbackableUdpLayer_S.cpp
    ../src/HlaEntityImpl_S.cpp
    ../src/HlaSessionHostImpl_S.cpp
    ../src/HlaSpaceImpl_S.cpp
    ../src/HostIDFactory.cpp
    ../src/NetClientInfo.cpp
    ../src/NetServer.cpp
    ../src/NetServerAccept.cpp
    ../src/NetServerEvent.cpp
    ../src/NetServerStartup.cpp
    ../src/NetServerStats.cpp
    ../src/NetworkerThread_S.cpp
    #../src/NTService.cpp
    ../src/OdbcByteData.cpp
    ../src/OdbcCommand.cpp
    ../src/OdbcCommandImpl.cpp
    ../src/OdbcConnection.cpp
    ../src/OdbcConnectionImpl.cpp
    ../src/OdbcErrorCheck.cpp
    ../src/OdbcException.cpp
    ../src/OdbcHandle.cpp
    ../src/OdbcRecordset.cpp
    ../src/OdbcRecordsetImpl.cpp
    ../src/OdbcVariant.cpp
    ../src/P2PGroup_S.cpp
    ../src/P2PPair.cpp
    ../src/P2PPair_S.cpp
    ../src/P2P_S.cpp
    ../src/PropNode.cpp
    ../src/ReaderWriterMonitor.cpp
    ../src/ReaderWriterMonitorTester.cpp
    ../src/RemoteClient.cpp
    ../src/ServerParam.cpp
    ../src/stdafxserver.cpp
    ../src/SuperPeer_S.cpp
    ../src/Tests.cpp
    ../src/TestUdpConnReset.cpp
    ../src/Variant.cpp
)

file(GLOB HEADERS
    ../src/AgentConnector.h
    ../src/Base64.h
    #../src/dbcacheclient2blocked.h
    #../src/DbCacheClient2Impl.h
    #../src/DBCacheJob.h
    #../src/DbCacheObject.h
    #../src/DbCacheServer2Impl.h
    #../src/DbConfig.h
    #../src/dbloadeddata2_s.h
    #../src/DbLogWriterImpl.h
    #../src/DumpServer.h
    #../src/EmergencyLogServer.h
    ../src/FallbackableUdpLayer_S.h
    ../src/HlaEntityImpl_S.h
    ../src/HlaSessionHostImpl_S.h
    ../src/HlaSpaceImpl_S.h
    ../src/HostIDFactory.h
    ../src/IVizAgentDg.h
    ../src/NetServer.h
    ../src/OdbcCommandImpl.h
    ../src/OdbcConnectionImpl.h
    ../src/OdbcErrorCheck.h
    ../src/OdbcHandle.h
    ../src/OdbcRecordsetImpl.h
    ../src/P2PGroup_S.h
    ../src/P2PPair.h
    ../src/P2PPair_S.h
    ../src/RCPair.h
    ../src/ReaderWriterMonitorTester.h
    ../src/RemoteClient.h
    ../src/stdafx.h
    ../src/Tests.h
    ../src/TestUdpConnReset.h
    ../src/ThreadPoolImpl.h
)

# options
add_definitions(
    -D__x86_64__
    -D__linux__
    -D_LINUX
    -D_LIB
    -D_NO_NTTNTRACE
    -DPROUD_STATIC_LIB
    -DPROUDSRCCOPY
)

add_compile_options(
    -fPIC
    -W -Wall -Wextra
#        -Wno-unused-parameter -Wno-unused-variable
#        -Wno-comment -Wno-extra -Wno-ignored-qualifiers -Wno-reorder -Wno-unknown-pragmas -Wno-unused-but-set-variable
    -O2
    -std=c++11
)

# link
add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    COMPILE_DEFINITIONS  $<$<CONFIG:DEBUG>:DEBUG _DEBUG> $<$<CONFIG:RELEASE>:NDEBUG>
)

target_link_libraries(${PROJECT_NAME}
    -lpthread
)
