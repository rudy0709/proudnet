cmake_minimum_required(VERSION 3.0.0)
project(ProudNetServer)

set(CMAKE_CXX_STANDARD 11)

file(GLOB SOURCES
    ../src/acrimpl_s.cpp
    ../src/AdoWrap.cpp
    ../src/DbCache2Isolate.cpp
    ../src/dbcacheclient2blocked.cpp
    ../src/DbCacheClient2Impl.cpp
    ../src/dbcacheclient2load.cpp
    ../src/DBCacheJob.cpp
    ../src/DbCacheObject.cpp
    ../src/DbCacheServer2Impl.cpp
    ../src/dbcacheserver2load.cpp
    ../src/DbConfig.cpp
    ../src/DbLoadedData.cpp
    ../src/DbLoadedData2.cpp
    ../src/dbloadeddata2_s.cpp
    ../src/DbLogWriter.cpp
    ../src/DbLogWriterImpl.cpp
    ../src/DumpServer.cpp
    ../src/EmergencyLogServer.cpp
    ../src/FallbackableUdpLayer_S.cpp
    ../src/HlaEntityImpl_S.cpp
    ../src/HlaSessionHostImpl_S.cpp
    ../src/HlaSpaceImpl_S.cpp
    ../src/HostIDFactory.cpp
    ../src/LoopbackHost_S.cpp
    ../src/NetClientInfo.cpp
    ../src/NetServer.cpp
    ../src/NetServerAccept.cpp
    ../src/NetServerEvent.cpp
    ../src/NetServerP2P.cpp
    ../src/NetServerSend.cpp
    ../src/NetServerStartup.cpp
    ../src/NetServerStartup_WebSocket.cpp
    ../src/NetServerStats.cpp
    ../src/NetServerUdp.cpp
    ../src/NetworkerThread_S.cpp
    ../src/NTService.cpp
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
    ../src/P2P_S.cpp
    ../src/P2PGroup_S.cpp
    ../src/p2ppair.cpp
    ../src/P2PPair_S.cpp
    ../src/PooledObjects_S.cpp
    ../src/PropNode.cpp
    ../src/ReaderWriterMonitor.cpp
    ../src/ReaderWriterMonitorTester.cpp
    ../src/RemoteClient.cpp
    ../src/ServerParam.cpp
    ../src/SuperPeer_S.cpp
    ../src/TcpLayer_S.cpp
    ../src/TestUdpConnReset.cpp
    ../src/UdpSocket_S.cpp
    ../src/UserTask.cpp
    ../src/Variant.cpp
    ../src/variant-marshaler.cpp
    ../src/WebSocket.cpp
    ../src/stdafx.cpp
)

file(GLOB HEADERS
    ../src/crypto.hpp
    ../src/dbcacheclient2blocked.h
    ../src/DbCacheClient2Impl.h
    ../src/DBCacheJob.h
    ../src/DbCacheObject.h
    ../src/DbCacheServer2Impl.h
    ../src/DbConfig.h
    ../src/dbloadeddata2_s.h
    ../src/DbLogWriterImpl.h
    ../src/DumpServer.h
    ../src/EmergencyLogServer.h
    ../src/FallbackableUdpLayer_S.h
    ../src/FavoriteLV_S.h
    ../src/HlaEntityImpl_S.h
    ../src/HlaSessionHostImpl_S.h
    ../src/HlaSpaceImpl_S.h
    ../src/HostIDFactory.h
    ../src/IVizAgentDg.h
    ../src/LoopbackHost_S.h
    ../src/NetServer.h
    ../src/OdbcCommandImpl.h
    ../src/OdbcConnectionImpl.h
    ../src/OdbcErrorCheck.h
    ../src/OdbcHandle.h
    ../src/OdbcRecordsetImpl.h
    ../src/P2PGroup_S.h
    ../src/P2PPair.h
    ../src/P2PPair_S.h
    ../src/PooledObjects_C.h
    ../src/PooledObjects_S.h
    ../src/RCPair.h
    ../src/ReaderWriterMonitorTester.h
    ../src/RemoteClient.h
    ../src/SendDest_S.h
    ../src/server_ws.hpp
    ../src/server_wss.hpp
    ../src/SuperPeer_S.h
    ../src/TcpLayer_S.h
    ../src/TestUdpConnReset.h
    ../src/ThreadPoolImpl.h
    ../src/UdpSocket_S.h
    ../src/UserTask.h
    ../src/WebSocket.h
    ../src/stdafx.h
    ../src/NetServer_WebSocket.inl
    ../src/NetServerImpl.inl
)

file(GLOB SRCS_UNITY_BUILD
    ../src/UnityBuild/ServerUnityBuild001.cpp
    ../src/UnityBuild/ServerUnityBuild002.cpp
    ../src/UnityBuild/ServerUnityBuild003.cpp
    ../src/UnityBuild/ServerUnityBuild004.cpp
    ../src/UnityBuild/ServerUnityBuild005.cpp
    ../src/UnityBuild/ServerUnityBuild006.cpp
    ../src/UnityBuild/ServerUnityBuild007.cpp
    ../src/UnityBuild/ServerUnityBuild008.cpp
)

include_directories(
    ../OpenSources/openssl-v1.1.1d/x86_64/include
    ../OpenSources/standalone-asio/include
)

# options
add_definitions(
    -D__x86_64__
    -D__linux__
    -D_LINUX
    #-DPROUDNETSERVER_EXPORTS
    -D_NO_NTTNTRACE
    -DPRIVATE
    -DASIO_STANDALONE
    #-D_USRDLL
    #-DUSE_PROUDNET_DLL
    -DUSE_OPENSSL
)

add_compile_options(
    -fPIC
    -W -Wall -Wextra
#        -Wno-unused-but-set-variable -Wno-unused-function -Wno-unused-label -Wno-unused-parameter -Wno-unused-variable
#        -Wno-comment -Wno-extra -Wno-ignored-qualifiers -Wno-missing-field-initializers -Wno-parentheses -Wno-reorder
#        -Wno-sign-compare -Wno-strict-aliasing -Wno-type-limits -Wno-unknown-pragmas
    -O2
    -std=c++11
)

# link
add_library(${PROJECT_NAME} SHARED ${SRCS_UNITY_BUILD})

set_target_properties(${PROJECT_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE 1
    COMPILE_DEFINITIONS  $<$<CONFIG:DEBUG>:DEBUG _DEBUG> $<$<CONFIG:RELEASE>:NDEBUG>
)

target_link_libraries(${PROJECT_NAME}
    -L/usr/local/ssl/lib -lcrypto -lssl
    -lcurl
    -lodbc
    -lpthread
)

# event
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND mkdir -p "${CMAKE_SOURCE_DIR}/../../lib/x86_64-linux/${CMAKE_BUILD_TYPE}/"
    COMMAND cp -f "${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/lib${PROJECT_NAME}.so" "${CMAKE_SOURCE_DIR}/../../lib/x86_64-linux/${CMAKE_BUILD_TYPE}/"
)
