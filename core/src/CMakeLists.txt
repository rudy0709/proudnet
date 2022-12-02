#The list FILTER command requires above 3.6
cmake_minimum_required (VERSION 3.6.0 FATAL_ERROR)

set(CMAKE_SUPPRESS_REGENERATION true)

#To set COMPILE_DEFINITIONS property
#In the future, This should be changed.
cmake_policy(SET CMP0043 OLD)

#The name of the .sln file
project(ProudNet)

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
		*.pidl
		)

file(GLOB DEV_YAML_CLIENT
		ErrorType.yaml
		)
		
file(GLOB DEV_HEADER_CLIENT_WITHOUT_PLATFORM_DEPENDENCY
		acrimpl_c.h
		acrimpl.h
		AsyncWork.h
		Base64.h
		BufferSegment.h
		CachedTimes.h
		CollUtil.h
		CompactFieldMap.h
		CompatibleMode.h
		CriticalSect.inl
		CriticalSectImpl.h
		CustomValueEventQueue.h
		DefaultStringEncoder.h
		DeviationDetector.h
		DumpClient.h
		EmergencyLogClient.h
		EmergencyLogData.h
		enumimpl.h
		Epoll_Reactor.h
		FakeWinsock.h
		FallbackParam.h
		FastArayPtrImpl.h
		FastArrayImpl.h
		FastHeapImpl.h
		FastList2.h
		FastMap2.h
		FastSocket.h
		FastSortedMap2.h
		FavoriteLV.h
		FilterTag.h
		FilterTag.inl
		FinalUserWorkItem.h
		FragHeader.h
		FreeList.h
		Functors.h
		GlobalTimerThread.h
		HeartbeatWork.h
		HlaCritSec.h
		HlaEntityImpl_C.h
		HlaSessionHostImpl_C.h
		IHostObject.h
		IncludeFileSystem.h
		IntraTracer.h
		IOCP.h
		ioevent.h
		ISendDest_C.h
		Kqueue_Reactor.h
		LambdaEventHandlerImpl.h
		LeanDynamicCast.h
		LeanType.h
		LicenseType.h
		LocalEvent.h
		LogWriterImpl.h
		LookasideAllocatorImpl.h
		LoopbackHost.h
		LowContextSwitchingLoop.h
		LowFragMemArray.h
		marshaler-private.h
		MessagePrivateImpl.h
		MessagePrivateImpl.inl
		MilisecTimerImpl.h
		mmmilisectimer.h
		mmtime64.h
		NetClient.h
		NetClientManager.h
		NetCore.h
		NetCoreImpl.inl
		NetSettings.h
		NetUtil.h
		Networker_C.h
		NumUtil.h
		P2PGroup_C.h
		PacketFrag.h
		PNCrc.h
		PnIconv.h
		PNS3ETimer.h
		PNVersion.h
		Poll_Reactor.h
		PooledObjectAsLocalVar.h
		PooledObjects_C.h
		PooledObjects.h
		PooledObjects.inl
		PtrImpl.h
		qpcmilisectimer.h
		quicksort.h
		Reactor.h
		ReceivedMessageList.h
		Relayer.h
		ReliableUDP.h
		ReliableUDPConfig.h
		ReliableUDPFrame.h
		ReliableUdpHelper.h
		ReliableUDPHost.h
		RemoteBase.h
		RemotePeer.h
		RemoteServer.h
		ReportError.h
		ReusableLocalVar.h
		RmiContextImpl.h
		RunOnScopeOut.h
		ScopeExit_CAS.h
		SendBrake.h
		SendDestInfo.h
		SendFragRefs.h
		SendFragRefs.inl
		SendMessage.h
		SendOpt.h
		SendReadyList.h
		Socket.h
		SocketPtrAndSerial.h
		SocketPtrAndSerial.inl
		SocketToHostsMap.inl
		SocketUtil.h
		SpinLock.h
		stdafx.h
		STLUtil.h
		StreamQueue.h
		SuperSocket.h
		sysutil-private.h
		TCPLayer.h
		ThreadPoolImpl.h
		ThreadUnsafeAllocator.h
		TimeAlarm.h
		TimeEventThreadPool.h
		TimeOfDayTimer.h
		UniqueID.h
		UnreliableMessageLossMeasurer.h
		Upnp.h
		UseCount.h
		UserTaskQueue.h
		UserTaskQueue.inl
		UseZlib.h
		VizAgent.h
		VizMessageSummary.h
		)
		
file(GLOB DEV_SRC_CLIENT_WITHOUT_PLATFORM_DEPENDENCY
		acrimpl_c.cpp
		acrimpl.cpp
		AddrPort.cpp
		AsyncWork.cpp
		atomic.cpp
		Base64.inl
		Base64Encode.cpp
		BufferSegment.cpp
		CoInit.cpp
		CompactFieldMap.cpp
		ConnectParam.cpp
		CriticalSect.cpp
		CryptoAes.cpp
		CryptoFast.cpp
		CryptoRc4.cpp
		CryptoRsa.cpp
		CustomValueEventQueue.cpp
		DefaultStringEncoder.cpp
		DeviationDetector.cpp
		DumpClient.cpp
		DumpCommon.cpp
		EmergencyLogClient.cpp
		EmergencyLogCommon.cpp
		EmergencyLogData.cpp
		Enums.cpp
		Epoll_Reactor.cpp
		ErrorInfo.cpp
		Event.cpp
		Exception.cpp
		ExternalBufferArray.cpp
		FakeClrBase.cpp
		FakeCLRRandom.cpp
		FakeWinsock.cpp
		FastArray.cpp
		FastArrayImpl.cpp
		FastArrayPtr.cpp
		FastHeap.cpp
		FastHeapImpl.cpp
		FastHeapSettings.cpp
		FastMap.cpp
		FastSocket.cpp
		FavoriteLV.cpp
		FilterTag.cpp
		FinalUserWorkItem.cpp
		GlobalTimerThread.cpp
		HeartbeatWork.cpp
		HlaCritSec.cpp
		HlaEntityImpl_C.cpp
		HlaField.cpp
		HlaSessionHostImpl_C.cpp
		IOCP.cpp
		IRmiHost.cpp
		IRMIProxy.cpp
		IRMIStub.cpp
		ISendDest_C.cpp
		Kqueue_Reactor.cpp
		LeanDynamicCast.cpp
		LeanType.cpp
		ListNode.cpp
		LogWriterImpl.cpp
		LookasideAllocator.cpp
		LookasideAllocatorImpl.cpp
		LoopbackHost.cpp
		Marshaler.cpp
		Message.cpp
		MilisecTimerImpl.cpp
		MiniDumper.cpp
		mmmilisectimer.cpp
		mmtime64.cpp
		NetClient.cpp
		NetClientConnect.cpp
		NetClientEvent.cpp
		NetClientManager.cpp
		NetClientSend.cpp
		NetClientStats.cpp
		NetConfig.cpp
		NetCore.cpp
		NetCoreEvent.cpp
		NetCoreStats.cpp
		NetPeerInfo.cpp
		NetSettings.cpp
		NetUtil.cpp
		NetVariant.cpp
		Networker_C.cpp
		NumUtil.cpp
		P2P_C.cpp
		P2PGroup_C.cpp
		P2PGroup.cpp
		PacketFrag.cpp
		PNCrc.cpp
		pnguid.cpp
		PnIconv.cpp
		pnmutex.cpp
		PNPath.cpp
		PNS3ETimer.cpp
		PNSemaphore.cpp
		PNString.cpp
		PnThread.cpp
		PnTime.cpp
		PNVersion.cpp
		Poll_Reactor.cpp
		PooledObjects_C.cpp
		PooledObjects.cpp
		ProcessAllEvents.cpp
		Ptr.cpp
		PtrImpl.cpp
		qpcmilisectimer.cpp
		Quantizer.cpp
		Random.cpp
		ReliableUDP.cpp
		ReliableUDPConfig.cpp
		ReliableUDPFrame.cpp
		ReliableUdpHelper.cpp
		ReliableUDPHost.cpp
		RemoteBase.cpp
		RemotePeer.cpp
		RemotePeerRecycle_C.cpp
		RemoteServer.cpp
		ReportError.cpp
		RMIContext.cpp
		SendBrake.cpp
		SendData.cpp
		SendMessage.cpp
		SendReadyList.cpp
		SingletonImpl.cpp
		Socket.cpp
		SocketToHostsMap.cpp
		SocketUtil.cpp
		stacktrace.cpp
		StreamQueue.cpp
		StringEncorder.cpp
		strpool.cpp
		SuperSocket.cpp
		SuperSocketProactor.cpp
		SuperSocketReactor.cpp
		SuperSocketSend.cpp
		SysTime.cpp
		sysutil-private.cpp
		sysutil.cpp
		TcpLayer_C.cpp
		TCPLayer.cpp
		ThreadPoolImpl.cpp
		ThreadPoolImplRunAsync.cpp
		ThreadUtil.cpp
		TimeAlarm.cpp
		TimeEventThreadPool.cpp
		TimeOfDayTimer.cpp
		TimerThread.cpp
		TimeUtil.cpp
		Tracer.cpp
		UdpSocket_C.cpp
		UnreliableMessageLossMeasurer.cpp
		Upnp.cpp
		UseCount.cpp
		UserTaskQueue.cpp
		UseZlib.cpp
		variant-marshaler.cpp
		VizAgent.cpp
		VizMessageSummary.cpp
		)
		
file(GLOB DEV_CLIENT_WITHOUT_PLATFORM_DEPENDENCY
		${DEV_SRC_CLIENT_WITHOUT_PLATFORM_DEPENDENCY}
		${DEV_HEADER_CLIENT_WITHOUT_PLATFORM_DEPENDENCY}
		)
		
file(GLOB DEV_SRC_CLIENT_WINDOWS
		${DEV_CLIENT_WITHOUT_PLATFORM_DEPENDENCY}
		URI.h
		stacktrace-win32.h
		pnguid-win32.cpp
		stacktrace-win32.cpp
		URI.cpp
		)
		
file(GLOB DEV_SRC_CLIENT_UNIX	
		${DEV_CLIENT_WITHOUT_PLATFORM_DEPENDENCY}
		stacktrace-unix.h
		stacktrace-unix.cpp
		)
		
file(GLOB DEV_HEADER_SERVER_WITHOUT_PLATFORM_DEPENDENCY
		crypto.hpp
		DbConfig.h
		EmergencyLogServer.h
		FallbackableUdpLayer_S.h
		FavoriteLV_S.h
		HlaEntityImpl_S.h
		HlaSessionHostImpl_S.h
		HlaSpaceImpl_S.h
		HostIDFactory.h
		IVizAgentDg.h
		LoopbackHost_S.h
		NetServer_WebSocket.inl
		NetServer.h
		NetServerImpl.inl
		OdbcCommandImpl.h
		OdbcConnectionImpl.h
		OdbcErrorCheck.h
		OdbcHandle.h
		OdbcRecordsetImpl.h
		P2PGroup_S.h
		P2PPair_S.h
		P2PPair.h
		PooledObjects_C.h
		PooledObjects_S.h
		RCPair.h
		ReaderWriterMonitorTester.h
		RemoteClient.h
		SendDest_S.h
		server_ws.hpp
		server_wss.hpp
		stdafx.h
		SuperPeer_S.h
		TcpLayer_S.h
		TestUdpConnReset.h
		ThreadPoolImpl.h
		UdpSocket_S.h
		UserTask.h
		WebSocket.h
		)
		
file(GLOB DEV_SRC_SERVER_WITHOUT_PLATFORM_DEPENDENCY
		acrimpl_s.cpp
		AdoWrap.cpp
		DbConfig.cpp
		EmergencyLogServer.cpp
		FallbackableUdpLayer_S.cpp
		HlaEntityImpl_S.cpp
		HlaSessionHostImpl_S.cpp
		HlaSpaceImpl_S.cpp
		HostIDFactory.cpp
		LoopbackHost_S.cpp
		NetClientInfo.cpp
		NetServer.cpp
		NetServerAccept.cpp
		NetServerEvent.cpp
		NetServerP2P.cpp
		NetServerSend.cpp
		NetServerStartup_WebSocket.cpp
		NetServerStartup.cpp
		NetServerStats.cpp
		NetServerUdp.cpp
		NetworkerThread_S.cpp
		OdbcByteData.cpp
		OdbcCommand.cpp
		OdbcCommandImpl.cpp
		OdbcConnection.cpp
		OdbcConnectionImpl.cpp
		OdbcErrorCheck.cpp
		OdbcException.cpp
		OdbcHandle.cpp
		OdbcRecordset.cpp
		OdbcRecordsetImpl.cpp
		OdbcVariant.cpp
		P2P_S.cpp
		P2PGroup_S.cpp
		P2PPair_S.cpp
		p2ppair.cpp
		PooledObjects_S.cpp
		PropNode.cpp
		ReaderWriterMonitor.cpp
		ReaderWriterMonitorTester.cpp
		RemoteClient.cpp
		ServerParam.cpp
		SuperPeer_S.cpp
		TcpLayer_S.cpp
		TestUdpConnReset.cpp
		UdpSocket_S.cpp
		UserTask.cpp
		WebSocket.cpp
		)
		
file(GLOB DEV_SERVER_WITHOUT_PLATFORM_DEPENDENCY
		${DEV_HEADER_SERVER_WITHOUT_PLATFORM_DEPENDENCY}
		${DEV_SRC_SERVER_WITHOUT_PLATFORM_DEPENDENCY}
		)
		
file(GLOB DEV_SRC_SERVER_WINDOWS
		${DEV_SERVER_WITHOUT_PLATFORM_DEPENDENCY}
		DbCache2Isolate.cpp
		dbcacheclient2blocked.cpp
		dbcacheclient2blocked.h
		DbCacheClient2Impl.cpp
		DbCacheClient2Impl.h
		dbcacheclient2load.cpp
		DBCacheJob.cpp
		DBCacheJob.h
		DbCacheObject.cpp
		DbCacheObject.h
		DbCacheServer2Impl.cpp
		DbCacheServer2Impl.h
		dbcacheserver2load.cpp
		DbLoadedData.cpp
		dbloadeddata2_s.cpp
		dbloadeddata2_s.h
		DbLoadedData2.cpp
		DbLogWriter.cpp
		DbLogWriterImpl.cpp
		DbLogWriterImpl.h
		DumpServer.cpp
		DumpServer.h
		NTService.cpp
		variant-marshaler.cpp
		Variant.cpp
		)
		
file(GLOB DEV_SRC_SERVER_UNIX	
		${DEV_SERVER_WITHOUT_PLATFORM_DEPENDENCY}
		)

file(GLOB DEV_SRC_UNITYBUILD_CLIENT
		UnityBuild/Client*.cpp
		stdafx.cpp
		)

file(GLOB DEV_SRC_UNITYBUILD_SERVER
		UnityBuild/Server*.cpp
		stdafx.cpp
		)

file(GLOB DEV_SRC_Z
		zlib/adler32.c
		zlib/compress.c
		zlib/crc32.c
		zlib/deflate.c
		zlib/gzclose.c
		zlib/gzlib.c
		zlib/gzread.c
		zlib/gzwrite.c
		zlib/infback.c
		zlib/inffast.c
		zlib/inflate.c
		zlib/inftrees.c
		zlib/trees.c
		zlib/uncompr.c
		zlib/zutil.c
		)
		
file(GLOB_RECURSE DEV_SRC_LIBTOM_CRYPTO
		libtom/crypto/*.h
		libtom/crypto/*.c
		)

list(FILTER DEV_SRC_LIBTOM_CRYPTO EXCLUDE REGEX ".*libtom/crypto/ciphers/aes/aes_tab.c$")
list(FILTER DEV_SRC_LIBTOM_CRYPTO EXCLUDE REGEX ".*libtom/crypto/hashes/sha2/sha224.c$")
list(FILTER DEV_SRC_LIBTOM_CRYPTO EXCLUDE REGEX ".*libtom/crypto/hashes/sha2/sha384.c$")
list(FILTER DEV_SRC_LIBTOM_CRYPTO EXCLUDE REGEX ".*libtom/crypto/hashes/whirl/whirltab.c$")
list(FILTER DEV_SRC_LIBTOM_CRYPTO EXCLUDE REGEX ".*libtom/crypto/prngs/sober128tab.c$")
list(FILTER USER_HEADER EXCLUDE REGEX ".*ErrorType.h$")
list(FILTER USER_HEADER EXCLUDE REGEX ".*ErrorTypeOldSpec.h$")

file(GLOB_RECURSE DEV_SRC_LIBTOM_MATH
		libtom/math/*.h
		libtom/math/*.c
		)

file(GLOB DEV_SRC_ICONV
		../iconv-embed/*.h
		../iconv-embed/*.c
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
if(WIN32)
	#Setting Environment Variables
	if(MSVC12)
		set(VSVER 2013)
	elseif(MSVC14)
		set(VSVER 2015)
	elseif(MSVC15)
		set(VSVER 2017)
	endif()
	
	if(8 EQUAL CMAKE_SIZEOF_VOID_P)
		#64 bit Windows
		set(OSBIT 64)
	else()
		#32 bit Windows
		set(OSBIT 32)
	endif()
	
	set(CMAKE_CONFIGURATION_TYPES Debug Release Debug_static_CRT Release_static_CRT)
	set(CMAKE_CXX_FLAGS_DEBUG_STATIC_CRT "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
	set(CMAKE_CXX_FLAGS_RELEASE_STATIC_CRT "${CMAKE_CXX_FLAGS_RELEASE} /MT")

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP") # multiprocessor build
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP") # multiprocessor build

	set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS "Debug;Debug_static_CRT")

	#Creating targets
	add_library(ProudNetInclude_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} STATIC ${USER_HEADER})  # 실제로 빌드하지는 않음
	add_library(ProudNetPidlBuild_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} STATIC ${DEV_PIDL_CLIENT} ${DEV_YAML_CLIENT})
	add_library(ProudNetClient_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} STATIC ${DEV_SRC_CLIENT_WINDOWS} ${DEV_SRC_UNITYBUILD_CLIENT} ${DEV_SRC_LIBTOM_CRYPTO} ${DEV_SRC_LIBTOM_MATH} ${DEV_SRC_Z} ${DEV_SRC_ICONV} ProudNet.natvis zlib/zconf.h)
	add_library(ProudNetServer_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} STATIC ${DEV_SRC_SERVER_WINDOWS} ${DEV_SRC_UNITYBUILD_SERVER})
	
	#Variables for OpenSSL
	if(MSVC12 OR MSVC14)
		set(OPENSSL_VERSION OpenSSL-1.1.1d-shared)
		set(OPENSSL_CONFIG_FOLDER_NAME "$<$<OR:$<CONFIG:DEBUG>,$<CONFIG:DEBUG_STATIC_CRT>>:Debug>$<$<OR:$<CONFIG:RELEASE>,$<CONFIG:RELEASE_STATIC_CRT>>:Release>")
	endif()
	
	# ProudNet Dynamic Library Version
	# The MSVC14 covers both VS2017 VS2015, but you must build dll version only with newest Visual Studio.
	# With below MSVC14, There are some build error, because CMake does not recognize DEBUG:FULL option.
	if(MSVC14)
		add_library(ProudNetClientDll_${CMAKE_VS_PLATFORM_NAME} SHARED ${DEV_SRC_CLIENT_WINDOWS} ${DEV_SRC_UNITYBUILD_CLIENT} ${DEV_SRC_LIBTOM_CRYPTO} ${DEV_SRC_LIBTOM_MATH} ${DEV_SRC_Z} ${DEV_SRC_ICONV} ProudNet.natvis zlib/zconf.h dllmain.cpp)
		add_library(ProudNetServerDll_${CMAKE_VS_PLATFORM_NAME} SHARED ${DEV_SRC_SERVER_WINDOWS} ${DEV_SRC_UNITYBUILD_SERVER} dllmain.cpp libtom/crypto/misc/base64/base64_encode.c libtom/crypto/misc/base64/base64_decode.c)
		
		add_dependencies(ProudNetServerDll_${CMAKE_VS_PLATFORM_NAME} ProudNetClientDll_${CMAKE_VS_PLATFORM_NAME})
		add_dependencies(ProudNetClientDll_${CMAKE_VS_PLATFORM_NAME} ProudNetPidlBuild_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME})
		
		set_property(TARGET ProudNetClientDll_${CMAKE_VS_PLATFORM_NAME} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ../lib/${CMAKE_VS_PLATFORM_NAME}/dll)
		set_property(TARGET ProudNetClientDll_${CMAKE_VS_PLATFORM_NAME} PROPERTY ARCHIVE_OUTPUT_NAME ProudNetClient)
		set_property(TARGET ProudNetClientDll_${CMAKE_VS_PLATFORM_NAME} PROPERTY RUNTIME_OUTPUT_DIRECTORY ../lib/${CMAKE_VS_PLATFORM_NAME}/dll)
		set_property(TARGET ProudNetClientDll_${CMAKE_VS_PLATFORM_NAME} PROPERTY RUNTIME_OUTPUT_NAME ProudNetClient)
		
		set_property(TARGET ProudNetServerDll_${CMAKE_VS_PLATFORM_NAME} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ../lib/${CMAKE_VS_PLATFORM_NAME}/dll)
		set_property(TARGET ProudNetServerDll_${CMAKE_VS_PLATFORM_NAME} PROPERTY ARCHIVE_OUTPUT_NAME ProudNetServer)
		set_property(TARGET ProudNetServerDll_${CMAKE_VS_PLATFORM_NAME} PROPERTY RUNTIME_OUTPUT_DIRECTORY ../lib/${CMAKE_VS_PLATFORM_NAME}/dll)
		set_property(TARGET ProudNetServerDll_${CMAKE_VS_PLATFORM_NAME} PROPERTY RUNTIME_OUTPUT_NAME ProudNetServer)
		
		set_property(TARGET ProudNetClientDll_${CMAKE_VS_PLATFORM_NAME} PROPERTY COMPILE_DEFINITIONS_DEBUG PRIVATE _UNICODE UNICODE _USRDLL PROUDNETCLIENT_EXPORTS DEBUG _DEBUG ${COMPILE_DEFINITIONS_DEBUG})
		set_property(TARGET ProudNetServerDll_${CMAKE_VS_PLATFORM_NAME} PROPERTY COMPILE_DEFINITIONS_DEBUG PRIVATE _UNICODE UNICODE _USRDLL USE_PROUDNET_DLL PROUDNETSERVER_EXPORTS  ASIO_STANDALONE USE_OPENSSL DEBUG _DEBUG ${COMPILE_DEFINITIONS_DEBUG})
		
		set_property(TARGET ProudNetClientDll_${CMAKE_VS_PLATFORM_NAME} PROPERTY COMPILE_DEFINITIONS_DEBUG_STATIC_CRT PRIVATE _UNICODE UNICODE _USRDLL DEBUG _DEBUG PROUDNETCLIENT_EXPORTS ${COMPILE_DEFINITIONS_DEBUG_STATIC_CRT})
		set_property(TARGET ProudNetServerDll_${CMAKE_VS_PLATFORM_NAME} PROPERTY COMPILE_DEFINITIONS_DEBUG_STATIC_CRT PRIVATE _UNICODE UNICODE _USRDLL USE_PROUDNET_DLL PROUDNETSERVER_EXPORTS  ASIO_STANDALONE USE_OPENSSL DEBUG _DEBUG ${COMPILE_DEFINITIONS_DEBUG_STATIC_CRT})
		
		set_property(TARGET ProudNetClientDll_${CMAKE_VS_PLATFORM_NAME} PROPERTY COMPILE_DEFINITIONS_RELEASE PRIVATE _UNICODE UNICODE _USRDLL PROUDNETCLIENT_EXPORTS NDEBUG ${COMPILE_DEFINITIONS_RELEASE})
		set_property(TARGET ProudNetServerDll_${CMAKE_VS_PLATFORM_NAME} PROPERTY COMPILE_DEFINITIONS_RELEASE PRIVATE _UNICODE UNICODE _USRDLL USE_PROUDNET_DLL PROUDNETSERVER_EXPORTS  ASIO_STANDALONE USE_OPENSSL NDEBUG ${COMPILE_DEFINITIONS_RELEASE})
		
		set_property(TARGET ProudNetClientDll_${CMAKE_VS_PLATFORM_NAME} PROPERTY COMPILE_DEFINITIONS_RELEASE_STATIC_CRT PRIVATE _UNICODE UNICODE _USRDLL PROUDNETCLIENT_EXPORTS NDEBUG ${COMPILE_DEFINITIONS_RELEASE_STATIC_CRT})
		set_property(TARGET ProudNetServerDll_${CMAKE_VS_PLATFORM_NAME} PROPERTY COMPILE_DEFINITIONS_RELEASE_STATIC_CRT PRIVATE _UNICODE UNICODE _USRDLL USE_PROUDNET_DLL PROUDNETSERVER_EXPORTS  ASIO_STANDALONE USE_OPENSSL NDEBUG ${COMPILE_DEFINITIONS_RELEASE_STATIC_CRT})
		
		target_link_libraries(ProudNetServerDll_${CMAKE_VS_PLATFORM_NAME} PRIVATE ProudNetClientDll_${CMAKE_VS_PLATFORM_NAME})
		
		target_include_directories(ProudNetServerDll_${CMAKE_VS_PLATFORM_NAME} PRIVATE
				"Libs/${OPENSSL_VERSION}/${CMAKE_VS_PLATFORM_NAME}/${OPENSSL_CONFIG_FOLDER_NAME}/include"
				"Libs/StandAloneAsio"
				)

		#This CMake sentences must exist for preventing CMake build errors.
		set(CMAKE_SHARED_LINKER_FLAGS_RELEASE_STATIC_CRT ${CMAKE_SHARED_LINKER_FLAGS_RELEASE})
		set(CMAKE_SHARED_LINKER_FLAGS_DEBUG_STATIC_CRT ${CMAKE_SHARED_LINKER_FLAGS_DEBUG})
				
		#Setting linker options for shared libraries
		set_property(TARGET ProudNetClientDll_${CMAKE_VS_PLATFORM_NAME} PROPERTY LINK_OPTIONS /DEBUG:FULL /ignore:4221)
		set_property(TARGET ProudNetServerDll_${CMAKE_VS_PLATFORM_NAME} PROPERTY LINK_OPTIONS libcrypto.lib libssl.lib /LIBPATH:Libs/${OPENSSL_VERSION}/${CMAKE_VS_PLATFORM_NAME}/${OPENSSL_CONFIG_FOLDER_NAME}/lib /DEBUG:FULL /ignore:4221)
		
		#LINK_OPTIONS can not cover the options for *_STATIC_CRT
		set_property(TARGET ProudNetServerDll_${CMAKE_VS_PLATFORM_NAME} PROPERTY LINK_FLAGS_RELEASE_STATIC_CRT ${LINK_OPTIONS})
		set_property(TARGET ProudNetServerDll_${CMAKE_VS_PLATFORM_NAME} PROPERTY LINK_FLAGS_DEBUG_STATIC_CRT ${LINK_OPTIONS})
	endif()
	
	#Setting the targets
	set_property(TARGET ProudNetInclude_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} PROPERTY LINKER_LANGUAGE CXX)
	set_property(TARGET ProudNetPidlBuild_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} PROPERTY LINKER_LANGUAGE CXX)
	add_dependencies(ProudNetServer_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} ProudNetClient_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME})
	add_dependencies(ProudNetClient_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} ProudNetPidlBuild_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME})
	set_property(TARGET ProudNetClient_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ../lib/${CMAKE_VS_PLATFORM_NAME}/${CMAKE_VS_PLATFORM_TOOLSET})
	set_property(TARGET ProudNetClient_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} PROPERTY ARCHIVE_OUTPUT_NAME ProudNetClient)
	set_property(TARGET ProudNetServer_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ../lib/${CMAKE_VS_PLATFORM_NAME}/${CMAKE_VS_PLATFORM_TOOLSET})
	set_property(TARGET ProudNetServer_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} PROPERTY ARCHIVE_OUTPUT_NAME ProudNetServer)
	set_target_properties( ProudNetClient_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME}  PROPERTIES LINK_FLAGS "/ignore:4221")
	set_target_properties( ProudNetInclude_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} PROPERTIES LINK_FLAGS "/ignore:4221")  
	set_target_properties( ProudNetServer_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME}  PROPERTIES LINK_FLAGS "/ignore:4221")
	
	#Setting some preprocesors on specific configurations.
	#Debug
	set_property(TARGET ProudNetClient_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} PROPERTY COMPILE_DEFINITIONS_DEBUG PRIVATE _UNICODE UNICODE _LIB PROUD_STATIC_LIB DEBUG _DEBUG ${COMPILE_DEFINITIONS_DEBUG})
	set_property(TARGET ProudNetServer_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} PROPERTY COMPILE_DEFINITIONS_DEBUG PRIVATE _UNICODE UNICODE _LIB PROUD_STATIC_LIB  ASIO_STANDALONE USE_OPENSSL DEBUG _DEBUG ${COMPILE_DEFINITIONS_DEBUG})
	
	#Debug Static CRT
	set_property(TARGET ProudNetClient_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} PROPERTY COMPILE_DEFINITIONS_DEBUG_STATIC_CRT PRIVATE _UNICODE UNICODE _LIB PROUD_STATIC_LIB DEBUG _DEBUG ${COMPILE_DEFINITIONS_DEBUG_STATIC_CRT})
	
	set_property(TARGET ProudNetServer_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} PROPERTY COMPILE_DEFINITIONS_DEBUG_STATIC_CRT PRIVATE _UNICODE UNICODE _LIB PROUD_STATIC_LIB  ASIO_STANDALONE USE_OPENSSL DEBUG _DEBUG ${COMPILE_DEFINITIONS_DEBUG_STATIC_CRT})
	
	#Release
	set_property(TARGET ProudNetClient_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} PROPERTY COMPILE_DEFINITIONS_RELEASE PRIVATE _UNICODE UNICODE _LIB PROUD_STATIC_LIB NDEBUG ${COMPILE_DEFINITIONS_RELEASE})
	set_property(TARGET ProudNetServer_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} PROPERTY COMPILE_DEFINITIONS_RELEASE PRIVATE _UNICODE UNICODE _LIB PROUD_STATIC_LIB  ASIO_STANDALONE USE_OPENSSL NDEBUG ${COMPILE_DEFINITIONS_RELEASE})
	
	#Release Static CRT
	set_property(TARGET ProudNetClient_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} PROPERTY COMPILE_DEFINITIONS_RELEASE_STATIC_CRT PRIVATE _UNICODE UNICODE _LIB PROUD_STATIC_LIB NDEBUG ${COMPILE_DEFINITIONS_RELEASE_STATIC_CRT})
	set_property(TARGET ProudNetServer_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} PROPERTY COMPILE_DEFINITIONS_RELEASE_STATIC_CRT PRIVATE _UNICODE UNICODE _LIB PROUD_STATIC_LIB  ASIO_STANDALONE USE_OPENSSL NDEBUG ${COMPILE_DEFINITIONS_RELEASE_STATIC_CRT})

	#Setting Custom Build for PIDL Compiling
	foreach(PIDL_FILE ${DEV_PIDL_CLIENT})
		get_filename_component(PFNAME ${PIDL_FILE} NAME_WE)
		add_custom_command(
			OUTPUT ${PFNAME}_common.cpp ${PFNAME}_common.h ${PFNAME}_proxy.cpp ${PFNAME}_proxy.h ${PFNAME}_stub.cpp ${PFNAME}_stub.h
			MAIN_DEPENDENCY ${PIDL_FILE}
			DEPENDS ..\\util\\PIDL_unprotect.exe    #역슬래쉬가 안 먹혀서 이렇게 했다. 이게 없으면 PIDL 컴파일러 소스에 변화가 있어도 적용을 못한다.
			COMMAND ../util/PIDL_unprotect.exe
			ARGS ${PIDL_FILE})
	endforeach(PIDL_FILE)

	foreach(YAML_FILE ${DEV_YAML_CLIENT})
		get_filename_component(PFNAME ${YAML_FILE} NAME_WE)
		add_custom_command(
			OUTPUT ${PFNAME}.inl 
			MAIN_DEPENDENCY ${YAML_FILE}
			DEPENDS ..\\util\\PIDL_unprotect.exe    #역슬래쉬가 안 먹혀서 이렇게 했다. 이게 없으면 PIDL 컴파일러 소스에 변화가 있어도 적용을 못한다.
			COMMAND ../util/PIDL_unprotect.exe
			ARGS ${YAML_FILE})
	endforeach(YAML_FILE)

	#미리 컴파일된 헤더
	set_source_files_properties( stdafx.cpp		PROPERTIES COMPILE_FLAGS "/Ycstdafx.h")
	set_source_files_properties( ${DEV_SRC_LIBTOM_CRYPTO}	PROPERTIES COMPILE_FLAGS "/Y-")
	set_source_files_properties( ${DEV_SRC_LIBTOM_MATH}		PROPERTIES COMPILE_FLAGS "/Y-")
	set_source_files_properties( ${DEV_SRC_Z}				PROPERTIES COMPILE_FLAGS "/Y-")
	set_source_files_properties( ${DEV_SRC_ICONV}			PROPERTIES COMPILE_FLAGS "/Y-")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Yustdafx.h")
	
	#Setting compile options for optimizing
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Oy /Ot /Oi")
	set(CMAKE_C_FLAGS_RELEASE_STATIC_CRT "${CMAKE_C_FLAGS_RELEASE_STATIC_CRT} /Oy /Ot /Oi")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Oy /Ot /Oi")
	set(CMAKE_CXX_FLAGS_RELEASE_STATIC_CRT "${CMAKE_CXX_FLAGS_RELEASE_STATIC_CRT} /Oy /Ot /Oi")

	#OpenSSL & ASIO of WebGL only works for over VS2013 version.
	#MSVC14 can cover over VS2015 version.
	if(MSVC12 OR MSVC14)
		target_include_directories(ProudNetServer_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} PRIVATE
				"Libs/${OPENSSL_VERSION}/${CMAKE_VS_PLATFORM_NAME}/${OPENSSL_CONFIG_FOLDER_NAME}/include"
				"Libs/StandAloneAsio"
				)
				
		set_property(TARGET ProudNetServer_${CMAKE_VS_PLATFORM_TOOLSET}_${CMAKE_VS_PLATFORM_NAME} PROPERTY STATIC_LIBRARY_OPTIONS libcrypto.lib;libssl.lib;/LIBPATH:Libs/${OPENSSL_VERSION}/${CMAKE_VS_PLATFORM_NAME}/${OPENSSL_CONFIG_FOLDER_NAME}/lib;/DEBUG:FULL;/ignore:4221)
	endif()
else(WIN32)
	set(CMAKE_CONFIGURATION_TYPES Debug Release)
	
	#POSIX Unix( Linux & Darwin ( macOS, iOS )) Makefiles
	#아래의 문장 대신에 CXX_STANDARD 속성에 11을 주는 문장을 쓰지는 말자,
	#그 이유는 CMake의 CXX_STANDARD 페이지에 이 속성을 변경하는 것은 VS의 경우 2015 update3 미만에 대해서는 효과가 없다고 써있는데,
	#행여나 CMAKE_STANDARD를 적용하고 윈도우쪽에도 적용되게 CMake 코드 정리를 해버릴 여지가 있기 때문이다.
	#CXX_STANDARD 속성 변경 문장을 보면 윈도우에서도 적용될 것처럼 생겨먹었다.
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	
	if(XCODE)
		# MacOS & iOS는 Client에 대해 static library만 만든다.
		add_library(ProudNetClient_static STATIC ${DEV_SRC_UNITYBUILD_CLIENT} ${DEV_SRC_LIBTOM_CRYPTO} ${DEV_SRC_LIBTOM_MATH} ${DEV_SRC_Z} ${DEV_SRC_ICONV} swprintf.cpp)
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
					"Libs/StandAloneAsio"
					)
	elseif(ANDROID)
		# ANDROID는 Client에 대해 static library만 만든다.
		add_library(ProudNetClient_static STATIC ${DEV_SRC_UNITYBUILD_CLIENT} ${DEV_SRC_LIBTOM_CRYPTO} ${DEV_SRC_LIBTOM_MATH} ${DEV_SRC_Z} ${DEV_SRC_ICONV} swprintf.cpp)
		set_property(TARGET ProudNetClient_static PROPERTY OUTPUT_NAME ProudNetClient)
		
		#Setting client's preprocessors
		#You should not use 'COMPILE_DEFINITIONS_<CONFIG>' because it was deprecated since CMake 2.8.12 so it will be ignored.
		#cf.CMP004 policy
		set_property(TARGET ProudNetClient_static APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:DEBUG>:DEBUG _DEBUG> $<$<CONFIG:RELEASE>:NDEBUG>)
	else()
		#Client
		add_library(ProudNetClient_obj OBJECT ${DEV_SRC_UNITYBUILD_CLIENT} ${DEV_SRC_LIBTOM_CRYPTO} ${DEV_SRC_LIBTOM_MATH} ${DEV_SRC_Z} ${DEV_SRC_ICONV} swprintf.cpp $<$<NOT:$<STREQUAL:$<PLATFORM_ID>,"Linux">>:zlib/zconf.h>)
		
		#Use this sentence only for debugging
		#add_library(ProudNetClient_obj OBJECT ${DEV_SRC_CLIENT_WITHOUT_PLATFORM_DEPENDENCY} ${DEV_SRC_LIBTOM_CRYPTO} ${DEV_SRC_LIBTOM_MATH} ${DEV_SRC_Z} ${DEV_SRC_ICONV} swprintf.cpp $<$<NOT:$<STREQUAL:$<PLATFORM_ID>,"Linux">>:zlib/zconf.h>)
		
		#stackoverflow "is it possible to get cmake to build both a static and shared version" 어쩌고에서 참고
		set_property(TARGET ProudNetClient_obj PROPERTY POSITION_INDEPENDENT_CODE 1)
		#set_property(TARGET ProudNetClient_obj PROPERTY LINKER_LANGUAGE NONE)

		add_library(ProudNetClient_static STATIC $<TARGET_OBJECTS:ProudNetClient_obj>)
		add_library(ProudNetClient_shared SHARED $<TARGET_OBJECTS:ProudNetClient_obj>)

		set_property(TARGET ProudNetClient_static PROPERTY OUTPUT_NAME ProudNetClient)
		set_property(TARGET ProudNetClient_shared PROPERTY OUTPUT_NAME ProudNetClient)
			
		#Setting client's preprocessors
		#You should not use 'COMPILE_DEFINITIONS_<CONFIG>' because it was deprecated since CMake 2.8.12 so it will be ignored.
		#cf.CMP004 policy
		set_property(TARGET ProudNetClient_obj APPEND PROPERTY COMPILE_DEFINITIONS  $<$<CONFIG:DEBUG>:DEBUG _DEBUG> $<$<CONFIG:RELEASE>:NDEBUG>)
		
		target_include_directories(ProudNetClient_obj PRIVATE
					"Libs/StandAloneAsio"
					)
	
		# Server
		# note: ${DEV_SRC_LIBTOM_CRYPTO} ${DEV_SRC_LIBTOM_MATH}는 배제한다. server lib는 client lib를 의존하는데 그 안에 이게 다 있으니까.
		add_library(ProudNetServer_obj OBJECT ${DEV_SRC_UNITYBUILD_SERVER})
		
		#Use this sentence only for debugging
		#add_library(ProudNetServer_obj OBJECT ${DEV_SRC_SERVER_WITHOUT_PLATFORM_DEPENDENCY})
		
		#stackoverflow "is it possible to get cmake to build both a static and shared version" 어쩌고에서 참고
		set_property(TARGET ProudNetServer_obj PROPERTY POSITION_INDEPENDENT_CODE 1)

		add_library(ProudNetServer_static STATIC $<TARGET_OBJECTS:ProudNetServer_obj>)
		add_library(ProudNetServer_shared SHARED $<TARGET_OBJECTS:ProudNetServer_obj>)
		
		#아래의 문장은 리눅스에서는 안넣어도 빌드와 실행이 잘되는 것으로 보이나,
		#윈도우에서는 넣어야 동작하는 문장이므로 일관성을 위해 넣어둔다.
		target_link_libraries(ProudNetServer_shared ProudNetClient_shared)

		set_property(TARGET ProudNetServer_static PROPERTY OUTPUT_NAME ProudNetServer)
		set_property(TARGET ProudNetServer_shared PROPERTY OUTPUT_NAME ProudNetServer)

		#Setting Server's preprocessors
		set_property(TARGET ProudNetServer_obj APPEND PROPERTY COMPILE_DEFINITIONS  ASIO_STANDALONE USE_OPENSSL $<$<CONFIG:DEBUG>:DEBUG> $<$<CONFIG:DEBUG>:_DEBUG> $<$<CONFIG:RELEASE>:NDEBUG>)
		
		target_include_directories(ProudNetServer_obj PRIVATE
					"Libs/StandAloneAsio"
					)
	endif()

	#모든 변수를 다 출력한다. 디버그용.
#	get_cmake_property(_variableNames VARIABLES)
#	list(SORT _variableNames)
#	foreach(_variableName ${_variableNames})
#		message(STATUS "${_variableName}=${${_variableName}}")
#	endforeach()

endif(WIN32)
