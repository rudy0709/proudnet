#The list FILTER command requires above 3.6
cmake_minimum_required (VERSION 3.6.0 FATAL_ERROR)

set(CMAKE_SUPPRESS_REGENERATION true)

#To set COMPILE_DEFINITIONS property
#In the future, This should be changed.
cmake_policy(SET CMP0043 OLD)

#The name of the .sln file
project(ProudNetClient)

#######ReadMe#######
# DEV_* : only for Nettention development employee
# USER_* : for all user
# *_HEADER_* : header & inl
# *_SRC_* : cpp, c, cxx files
# *_WITHOUT_PLATFORM_DEPENDENCY : for all platform
# *_WINDOWS : only for windows
# *_UNIX : only for Unix OS ( Android, Linux, macOS, iOS )
#####################

#Globbing the files
file(GLOB USER_HEADER
	../include/*.h
	../include/*.hpp
	../include/*.inl
)

file(GLOB DEV_PIDL_CLIENT
	../ProudNetPidlBuild/*.pidl
)

file(GLOB DEV_YAML_CLIENT
	../ProudNetPidlBuild/ErrorType.yaml
)

file(GLOB DEV_HEADER_CLIENT_WITHOUT_PLATFORM_DEPENDENCY
	../src/acrimpl_c.h
	../src/acrimpl.h
	../src/AsyncWork.h
	../src/Base64.h
	../src/BufferSegment.h
	../src/CachedTimes.h
	../src/CollUtil.h
	../src/CompactFieldMap.h
	../src/CompatibleMode.h
	../src/CriticalSect.inl
	../src/CriticalSectImpl.h
	../src/CustomValueEventQueue.h
	../src/DefaultStringEncoder.h
	../src/DeviationDetector.h
	../src/DumpClient.h
	../src/EmergencyLogClient.h
	../src/EmergencyLogData.h
	../src/enumimpl.h
	../src/Epoll_Reactor.h
	../src/FakeWinsock.h
	../src/FallbackParam.h
	../src/FastArayPtrImpl.h
	../src/FastArrayImpl.h
	../src/FastHeapImpl.h
	../src/FastList2.h
	../src/FastMap2.h
	../src/FastSocket.h
	../src/FastSortedMap2.h
	../src/FavoriteLV.h
	../src/FilterTag.h
	../src/FilterTag.inl
	../src/FinalUserWorkItem.h
	../src/FragHeader.h
	../src/FreeList.h
	../src/Functors.h
	../src/GlobalTimerThread.h
	../src/HeartbeatWork.h
	../src/HlaCritSec.h
	../src/HlaEntityImpl_C.h
	../src/HlaSessionHostImpl_C.h
	../src/IHostObject.h
	../src/IncludeFileSystem.h
	../src/IntraTracer.h
	../src/IOCP.h
	../src/ioevent.h
	../src/ISendDest_C.h
	../src/Kqueue_Reactor.h
	../src/LambdaEventHandlerImpl.h
	../src/LeanDynamicCast.h
	../src/LeanType.h
	../src/LicenseType.h
	../src/LocalEvent.h
	../src/LogWriterImpl.h
	../src/LookasideAllocatorImpl.h
	../src/LoopbackHost.h
	../src/LowContextSwitchingLoop.h
	../src/LowFragMemArray.h
	../src/marshaler-private.h
	../src/MessagePrivateImpl.h
	../src/MessagePrivateImpl.inl
	../src/MilisecTimerImpl.h
	../src/mmmilisectimer.h
	../src/mmtime64.h
	../src/NetClient.h
	../src/NetClientManager.h
	../src/NetCore.h
	../src/NetCoreImpl.inl
	../src/NetSettings.h
	../src/NetUtil.h
	../src/Networker_C.h
	../src/NumUtil.h
	../src/P2PGroup_C.h
	../src/PacketFrag.h
	../src/PNCrc.h
	../src/PnIconv.h
	../src/PNS3ETimer.h
	../src/PNVersion.h
	../src/Poll_Reactor.h
	../src/PooledObjectAsLocalVar.h
	../src/PooledObjects_C.h
	../src/PooledObjects.h
	../src/PooledObjects.inl
	../src/PtrImpl.h
	../src/qpcmilisectimer.h
	../src/quicksort.h
	../src/Reactor.h
	../src/ReceivedMessageList.h
	../src/Relayer.h
	../src/ReliableUDP.h
	../src/ReliableUDPConfig.h
	../src/ReliableUDPFrame.h
	../src/ReliableUdpHelper.h
	../src/ReliableUDPHost.h
	../src/RemoteBase.h
	../src/RemotePeer.h
	../src/RemoteServer.h
	../src/ReportError.h
	../src/ReusableLocalVar.h
	../src/RmiContextImpl.h
	../src/RunOnScopeOut.h
	../src/ScopeExit_CAS.h
	../src/SendBrake.h
	../src/SendDestInfo.h
	../src/SendFragRefs.h
	../src/SendFragRefs.inl
	../src/SendMessage.h
	../src/SendOpt.h
	../src/SendReadyList.h
	../src/Socket.h
	../src/SocketPtrAndSerial.h
	../src/SocketPtrAndSerial.inl
	../src/SocketToHostsMap.inl
	../src/SocketUtil.h
	../src/SpinLock.h
	../src/stdafx.h
	../src/STLUtil.h
	../src/StreamQueue.h
	../src/SuperSocket.h
	../src/sysutil-private.h
	../src/TCPLayer.h
	../src/ThreadPoolImpl.h
	../src/ThreadUnsafeAllocator.h
	../src/TimeAlarm.h
	../src/TimeEventThreadPool.h
	../src/TimeOfDayTimer.h
	../src/UniqueID.h
	../src/UnreliableMessageLossMeasurer.h
	../src/Upnp.h
	../src/UseCount.h
	../src/UserTaskQueue.h
	../src/UserTaskQueue.inl
	../src/UseZlib.h
	../src/VizAgent.h
	../src/VizMessageSummary.h
)

file(GLOB DEV_SRC_CLIENT_WITHOUT_PLATFORM_DEPENDENCY
	../src/acrimpl_c.cpp
	../src/acrimpl.cpp
	../src/AddrPort.cpp
	../src/AsyncWork.cpp
	../src/atomic.cpp
	../src/Base64.inl
	../src/Base64Encode.cpp
	../src/BufferSegment.cpp
	../src/CoInit.cpp
	../src/CompactFieldMap.cpp
	../src/ConnectParam.cpp
	../src/CriticalSect.cpp
	../src/CryptoAes.cpp
	../src/CryptoFast.cpp
	../src/CryptoRc4.cpp
	../src/CryptoRsa.cpp
	../src/CustomValueEventQueue.cpp
	../src/DefaultStringEncoder.cpp
	../src/DeviationDetector.cpp
	../src/DumpClient.cpp
	../src/DumpCommon.cpp
	../src/EmergencyLogClient.cpp
	../src/EmergencyLogCommon.cpp
	../src/EmergencyLogData.cpp
	../src/Enums.cpp
	../src/Epoll_Reactor.cpp
	../src/ErrorInfo.cpp
	../src/Event.cpp
	../src/Exception.cpp
	../src/ExternalBufferArray.cpp
	../src/FakeClrBase.cpp
	../src/FakeCLRRandom.cpp
	../src/FakeWinsock.cpp
	../src/FastArray.cpp
	../src/FastArrayImpl.cpp
	../src/FastArrayPtr.cpp
	../src/FastHeap.cpp
	../src/FastHeapImpl.cpp
	../src/FastHeapSettings.cpp
	../src/FastMap.cpp
	../src/FastSocket.cpp
	../src/FavoriteLV.cpp
	../src/FilterTag.cpp
	../src/FinalUserWorkItem.cpp
	../src/GlobalTimerThread.cpp
	../src/HeartbeatWork.cpp
	../src/HlaCritSec.cpp
	../src/HlaEntityImpl_C.cpp
	../src/HlaField.cpp
	../src/HlaSessionHostImpl_C.cpp
	../src/IOCP.cpp
	../src/IRmiHost.cpp
	../src/IRMIProxy.cpp
	../src/IRMIStub.cpp
	../src/ISendDest_C.cpp
	../src/Kqueue_Reactor.cpp
	../src/LeanDynamicCast.cpp
	../src/LeanType.cpp
	../src/ListNode.cpp
	../src/LogWriterImpl.cpp
	../src/LookasideAllocator.cpp
	../src/LookasideAllocatorImpl.cpp
	../src/LoopbackHost.cpp
	../src/Marshaler.cpp
	../src/Message.cpp
	../src/MilisecTimerImpl.cpp
	../src/MiniDumper.cpp
	../src/mmmilisectimer.cpp
	../src/mmtime64.cpp
	../src/NetClient.cpp
	../src/NetClientConnect.cpp
	../src/NetClientEvent.cpp
	../src/NetClientManager.cpp
	../src/NetClientSend.cpp
	../src/NetClientStats.cpp
	../src/NetConfig.cpp
	../src/NetCore.cpp
	../src/NetCoreEvent.cpp
	../src/NetCoreStats.cpp
	../src/NetPeerInfo.cpp
	../src/NetSettings.cpp
	../src/NetUtil.cpp
	../src/NetVariant.cpp
	../src/Networker_C.cpp
	../src/NumUtil.cpp
	../src/P2P_C.cpp
	../src/P2PGroup_C.cpp
	../src/P2PGroup.cpp
	../src/PacketFrag.cpp
	../src/PNCrc.cpp
	../src/pnguid.cpp
	../src/PnIconv.cpp
	../src/pnmutex.cpp
	../src/PNPath.cpp
	../src/PNS3ETimer.cpp
	../src/PNSemaphore.cpp
	../src/PNString.cpp
	../src/PnThread.cpp
	../src/PnTime.cpp
	../src/PNVersion.cpp
	../src/Poll_Reactor.cpp
	../src/PooledObjects_C.cpp
	../src/PooledObjects.cpp
	../src/ProcessAllEvents.cpp
	../src/Ptr.cpp
	../src/PtrImpl.cpp
	../src/qpcmilisectimer.cpp
	../src/Quantizer.cpp
	../src/Random.cpp
	../src/ReliableUDP.cpp
	../src/ReliableUDPConfig.cpp
	../src/ReliableUDPFrame.cpp
	../src/ReliableUdpHelper.cpp
	../src/ReliableUDPHost.cpp
	../src/RemoteBase.cpp
	../src/RemotePeer.cpp
	../src/RemotePeerRecycle_C.cpp
	../src/RemoteServer.cpp
	../src/ReportError.cpp
	../src/RMIContext.cpp
	../src/SendBrake.cpp
	../src/SendData.cpp
	../src/SendMessage.cpp
	../src/SendReadyList.cpp
	../src/SingletonImpl.cpp
	../src/Socket.cpp
	../src/SocketToHostsMap.cpp
	../src/SocketUtil.cpp
	../src/stacktrace.cpp
	../src/StreamQueue.cpp
	../src/StringEncorder.cpp
	../src/strpool.cpp
	../src/SuperSocket.cpp
	../src/SuperSocketProactor.cpp
	../src/SuperSocketReactor.cpp
	../src/SuperSocketSend.cpp
	../src/SysTime.cpp
	../src/sysutil-private.cpp
	../src/sysutil.cpp
	../src/TcpLayer_C.cpp
	../src/TCPLayer.cpp
	../src/ThreadPoolImpl.cpp
	../src/ThreadPoolImplRunAsync.cpp
	../src/ThreadUtil.cpp
	../src/TimeAlarm.cpp
	../src/TimeEventThreadPool.cpp
	../src/TimeOfDayTimer.cpp
	../src/TimerThread.cpp
	../src/TimeUtil.cpp
	../src/Tracer.cpp
	../src/UdpSocket_C.cpp
	../src/UnreliableMessageLossMeasurer.cpp
	../src/Upnp.cpp
	../src/UseCount.cpp
	../src/UserTaskQueue.cpp
	../src/UseZlib.cpp
	../src/variant-marshaler.cpp
	../src/VizAgent.cpp
	../src/VizMessageSummary.cpp
)

file(GLOB DEV_CLIENT_WITHOUT_PLATFORM_DEPENDENCY
	${DEV_SRC_CLIENT_WITHOUT_PLATFORM_DEPENDENCY}
	${DEV_HEADER_CLIENT_WITHOUT_PLATFORM_DEPENDENCY}
)

file(GLOB DEV_SRC_CLIENT_WINDOWS
	${DEV_CLIENT_WITHOUT_PLATFORM_DEPENDENCY}
	../src/URI.h
	../src/URI.cpp
	../src/stacktrace-win32.h
	../src/stacktrace-win32.cpp
	../src/pnguid-win32.cpp
)

file(GLOB DEV_SRC_CLIENT_UNIX
	${DEV_CLIENT_WITHOUT_PLATFORM_DEPENDENCY}
	../src/stacktrace-unix.h
	../src/stacktrace-unix.cpp
)

file(GLOB DEV_HEADER_SERVER_WITHOUT_PLATFORM_DEPENDENCY
	../src/crypto.hpp
	../src/DbConfig.h
	../src/EmergencyLogServer.h
	../src/FallbackableUdpLayer_S.h
	../src/FavoriteLV_S.h
	../src/HlaEntityImpl_S.h
	../src/HlaSessionHostImpl_S.h
	../src/HlaSpaceImpl_S.h
	../src/HostIDFactory.h
	../src/IVizAgentDg.h
	../src/LoopbackHost_S.h
	../src/NetServer_WebSocket.inl
	../src/NetServer.h
	../src/NetServerImpl.inl
	../src/OdbcCommandImpl.h
	../src/OdbcConnectionImpl.h
	../src/OdbcErrorCheck.h
	../src/OdbcHandle.h
	../src/OdbcRecordsetImpl.h
	../src/P2PGroup_S.h
	../src/P2PPair_S.h
	../src/P2PPair.h
	../src/PooledObjects_C.h
	../src/PooledObjects_S.h
	../src/RCPair.h
	../src/ReaderWriterMonitorTester.h
	../src/RemoteClient.h
	../src/SendDest_S.h
	../src/server_ws.hpp
	../src/server_wss.hpp
	../src/stdafx.h
	../src/SuperPeer_S.h
	../src/TcpLayer_S.h
	../src/TestUdpConnReset.h
	../src/ThreadPoolImpl.h
	../src/UdpSocket_S.h
	../src/UserTask.h
	../src/WebSocket.h
)

file(GLOB DEV_SRC_SERVER_WITHOUT_PLATFORM_DEPENDENCY
	../src/acrimpl_s.cpp
	../src/AdoWrap.cpp
	../src/DbConfig.cpp
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
	../src/NetServerStartup_WebSocket.cpp
	../src/NetServerStartup.cpp
	../src/NetServerStats.cpp
	../src/NetServerUdp.cpp
	../src/NetworkerThread_S.cpp
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
	../src/P2PPair_S.cpp
	../src/p2ppair.cpp
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
	../src/WebSocket.cpp
)

file(GLOB DEV_SERVER_WITHOUT_PLATFORM_DEPENDENCY
	${DEV_HEADER_SERVER_WITHOUT_PLATFORM_DEPENDENCY}
	${DEV_SRC_SERVER_WITHOUT_PLATFORM_DEPENDENCY}
)

file(GLOB DEV_SRC_SERVER_WINDOWS
	${DEV_SERVER_WITHOUT_PLATFORM_DEPENDENCY}
	../src/DbCache2Isolate.cpp
	../src/dbcacheclient2blocked.cpp
	../src/dbcacheclient2blocked.h
	../src/DbCacheClient2Impl.cpp
	../src/DbCacheClient2Impl.h
	../src/dbcacheclient2load.cpp
	../src/DBCacheJob.cpp
	../src/DBCacheJob.h
	../src/DbCacheObject.cpp
	../src/DbCacheObject.h
	../src/DbCacheServer2Impl.cpp
	../src/DbCacheServer2Impl.h
	../src/dbcacheserver2load.cpp
	../src/DbLoadedData.cpp
	../src/dbloadeddata2_s.cpp
	../src/dbloadeddata2_s.h
	../src/DbLoadedData2.cpp
	../src/DbLogWriter.cpp
	../src/DbLogWriterImpl.cpp
	../src/DbLogWriterImpl.h
	../src/DumpServer.cpp
	../src/DumpServer.h
	../src/NTService.cpp
	../src/variant-marshaler.cpp
	../src/Variant.cpp
)

file(GLOB DEV_SRC_SERVER_UNIX
	${DEV_SERVER_WITHOUT_PLATFORM_DEPENDENCY}
)

file(GLOB DEV_SRC_UNITYBUILD_CLIENT
	../src/UnityBuild/Client*.cpp
	../src/stdafx.cpp
)

file(GLOB DEV_SRC_UNITYBUILD_SERVER
	../src/UnityBuild/Server*.cpp
	../src/stdafx.cpp
)

file(GLOB DEV_SRC_Z
	../OpenSources/zlib-v1.2.8/adler32.c
	../OpenSources/zlib-v1.2.8/compress.c
	../OpenSources/zlib-v1.2.8/crc32.c
	../OpenSources/zlib-v1.2.8/deflate.c
	../OpenSources/zlib-v1.2.8/gzclose.c
	../OpenSources/zlib-v1.2.8/gzlib.c
	../OpenSources/zlib-v1.2.8/gzread.c
	../OpenSources/zlib-v1.2.8/gzwrite.c
	../OpenSources/zlib-v1.2.8/infback.c
	../OpenSources/zlib-v1.2.8/inffast.c
	../OpenSources/zlib-v1.2.8/inflate.c
	../OpenSources/zlib-v1.2.8/inftrees.c
	../OpenSources/zlib-v1.2.8/trees.c
	../OpenSources/zlib-v1.2.8/uncompr.c
	../OpenSources/zlib-v1.2.8/zutil.c
)

file(GLOB_RECURSE DEV_SRC_LIBTOM_CRYPTO
	../OpenSources/libtomcrypt/*.h
	../OpenSources/libtomcrypt/*.c
)

list(FILTER DEV_SRC_LIBTOM_CRYPTO EXCLUDE REGEX ".*/OpenSources/libtomcrypt/ciphers/aes/aes_tab.c$")
list(FILTER DEV_SRC_LIBTOM_CRYPTO EXCLUDE REGEX ".*/OpenSources/libtomcrypt/hashes/sha2/sha224.c$")
list(FILTER DEV_SRC_LIBTOM_CRYPTO EXCLUDE REGEX ".*/OpenSources/libtomcrypt/hashes/sha2/sha384.c$")
list(FILTER DEV_SRC_LIBTOM_CRYPTO EXCLUDE REGEX ".*/OpenSources/libtomcrypt/hashes/whirl/whirltab.c$")
list(FILTER DEV_SRC_LIBTOM_CRYPTO EXCLUDE REGEX ".*/OpenSources/libtomcrypt/prngs/sober128tab.c$")
list(FILTER USER_HEADER EXCLUDE REGEX ".*ErrorType.h$")
list(FILTER USER_HEADER EXCLUDE REGEX ".*ErrorTypeOldSpec.h$")

file(GLOB_RECURSE DEV_SRC_LIBTOM_MATH
	../OpenSources/libtommath/*.h
	../OpenSources/libtommath/*.c
)

file(GLOB DEV_SRC_ICONV
	../OpenSources/libiconv-v1.14/*.h
	../OpenSources/libiconv-v1.14/*.c
)

#Setting the property of 'Do not participate in build'
set_property(SOURCE ${DEV_SRC_CLIENT_WINDOWS} PROPERTY LANGUAGE NONE)
set_property(SOURCE ${DEV_SRC_SERVER_WINDOWS} PROPERTY LANGUAGE NONE)
set_property(SOURCE ${DEV_SRC_CLIENT_WINDOWS} PROPERTY LINKER_LANGUAGE NONE)
set_property(SOURCE ${DEV_SRC_SERVER_WINDOWS} PROPERTY LINKER_LANGUAGE NONE)

#Setting the language property to unity build files for using C++ compile options ( ex - frtti )
set_property(SOURCE ${DEV_SRC_UNITYBUILD_CLIENT} PROPERTY LANGUAGE CXX)
set_property(SOURCE ${DEV_SRC_UNITYBUILD_SERVER} PROPERTY LANGUAGE CXX)
set_property(SOURCE ${DEV_SRC_UNITYBUILD_CLIENT} PROPERTY LINKER_LANGUAGE CXX)
set_property(SOURCE ${DEV_SRC_UNITYBUILD_SERVER} PROPERTY LINKER_LANGUAGE CXX)

#Grouping the files
source_group("libtom" FILES ${DEV_SRC_LIBTOM_CRYPTO} ${DEV_SRC_LIBTOM_MATH})
source_group("zlib" FILES ${DEV_SRC_Z})
source_group("iconv" FILES ${DEV_SRC_ICONV})
source_group("PIDL" FILES ${DEV_PIDL_CLIENT})
source_group("PIDL" FILES ${DEV_YAML_CLIENT})
source_group("Unity Build Files" FILES ${DEV_SRC_UNITYBUILD_CLIENT})
source_group("Unity Build Files" FILES ${DEV_SRC_UNITYBUILD_SERVER})

#Setting the properties of targets depends on its architecture
set(CMAKE_CONFIGURATION_TYPES Debug Release)

#POSIX Unix( Linux & Darwin ( macOS, iOS )) Makefiles
#아래의 문장 대신에 CXX_STANDARD 속성에 11을 주는 문장을 쓰지는 말자,
#그 이유는 CMake의 CXX_STANDARD 페이지에 이 속성을 변경하는 것은 VS의 경우 2015 update3 미만에 대해서는 효과가 없다고 써있는데,
#행여나 CMAKE_STANDARD를 적용하고 윈도우쪽에도 적용되게 CMake 코드 정리를 해버릴 여지가 있기 때문이다.
#CXX_STANDARD 속성 변경 문장을 보면 윈도우에서도 적용될 것처럼 생겨먹었다.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# MacOS & iOS는 Client에 대해 static library만 만든다.
add_library(ProudNetClient_static STATIC ${DEV_SRC_UNITYBUILD_CLIENT} ${DEV_SRC_LIBTOM_CRYPTO} ${DEV_SRC_LIBTOM_MATH} ${DEV_SRC_Z} ${DEV_SRC_ICONV} ../src/swprintf.cpp)
set_property(TARGET ProudNetClient_static PROPERTY OUTPUT_NAME ProudNetClient)

set_property(TARGET ProudNetClient_static PROPERTY XCODE_ATTRIBUTE_ARCHS "$(ARCHS_STANDARD)")
set_property(TARGET ProudNetClient_static PROPERTY XCODE_ATTRIBUTE_VALID_ARCHS "armv7 armv7s arm64 arm64e i386 x86_64")
set_property(TARGET ProudNetClient_static PROPERTY XCODE_ATTRIBUTE_OTHER_CFLAGS "-fembed-bitcode")
set_property(TARGET ProudNetClient_static PROPERTY XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH NO)
set_property(TARGET ProudNetClient_static PROPERTY XCODE_ATTRIBUTE_COPY_PHASE_STRIP NO)

#Setting client's preprocessors
#You should not use 'COMPILE_DEFINITIONS_<CONFIG>' because it was deprecated since CMake 2.8.12 so it will be ignored.
#cf.CMP004 policy
set_property(TARGET ProudNetClient_static APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:DEBUG>:DEBUG _DEBUG> $<$<CONFIG:RELEASE>:NDEBUG>)

target_include_directories(ProudNetClient_static PRIVATE
	"../OpenSources/standalone-asio/include"
)

# 모든 변수를 다 출력한다. 디버그용.
#get_cmake_property(_variableNames VARIABLES)
#list(SORT _variableNames)
#foreach(_variableName ${_variableNames})
#	message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()
