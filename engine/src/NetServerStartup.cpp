#include "stdafx.h"
#include <stack>
#include "NetServer.h"
#include "ReportError.h"
//#include "SuperSocket.h"
#include "LoopbackHost.h"
//#include "../include/ThreadPool.h"

#ifndef PROUDSRCCOPY
#include "../PNLic/PNLicenseManager/PNLic.h"
#endif

#include "quicksort.h"
#include "../include/ClassBehavior.h"

#ifdef SUPPORTS_WEBGL
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "SendOpt.h"
#include "Base64.h"
#endif

namespace Proud
{
	void CNetServerImpl::Start(CStartServerParameter &param)
	{
		CriticalSectionLock ssLock(m_startStopCS, true);

		// ikpil.choi 2016-11-02 : Start가 이중으로 불릴 수 있으므로, 여기서 먼저 체크 합니니다.
		if (m_netThreadPool || m_userThreadPool)
		{
			throw Exception(ErrorInfo::From(ErrorType_ServerPortListenFailure, HostID_None, _PNT("Thread pool already exists!")));
		}

		AllowAccess();

		// hostname을 ip addr로 강제로 바꿈. localhost라고 넣으면 느려지는 희귀 경우 때문에.
		// Windows XP처럼 ipv4 only OS를 고려 안하므로 ::1로 바꾸어도 된다. IPv4에서도 ::1은 정상적으로 인지된다.
		if (param.m_localNicAddr == _PNT("localhost"))
			param.m_localNicAddr = _PNT("::1");

		// 사용자가 원하면 bottleneck warning 기능을 켜준다.
		GetCriticalSection().Reset(param.m_bottleneckWarningSettings);

		// 이미 서버 시작했는데 또 서버 시작을 하면 에러 처리
		if (m_TcpListenSockets.GetCount() > 0)
		{
			throw Exception(ErrorInfo::From(ErrorType_AlreadyConnected, HostID_None, _PNT("Listen socket already exists!")));
		}

		if (param.m_threadCount == 0)
			param.m_threadCount = GetNoofProcessors();

		if (param.m_threadCount < 0)
			throw Exception("Invalid thread count");

#ifdef SUPPORTS_WEBGL
		if (param.m_webSocketParam.threadCount == 0)
			param.m_webSocketParam.threadCount = GetNoofProcessors();

		if (param.m_webSocketParam.threadCount < 0)
			throw Exception("Invalid thread count");
#endif

		if (param.m_udpAssignMode == ServerUdpAssignMode_PerClient && param.m_udpPorts.GetCount() > 0 && param.m_udpPorts.GetCount() < 1000)
		{
			NTTNTRACE("WARNING: You specified too few UDP ports with ServerUdpAssignMode_PerClient. This may cause inefficient P2P communication and unreliable messagings.\n");
		}

		// 파라메터 validation을 체크한다.
		bool hasNonZeroPortNum = false, hasZeroPortNum = false;
		for (int i = 0; i < (int)param.m_udpPorts.GetCount(); i++)
		{
			if (param.m_udpPorts[i] == 0)
				hasZeroPortNum = true;
			else
				hasNonZeroPortNum = true;
		}
		if (hasZeroPortNum && hasNonZeroPortNum)
		{
			throw Exception("Cannot assign non-zero UDP port number with zero UDP port number! All non-zero or all zero must be guaranteed!");
		}

		CFastArray<int> tempList = param.m_udpPorts;
		StacklessQuickSort(tempList.GetData(), (int)tempList.GetCount());
		for (intptr_t i = 0; i < tempList.GetCount() - 1; i++)
		{
			if (tempList[i] == tempList[i + 1] && tempList[i] != 0)
			{
				throw Exception("Cannot add duplicated UDP port number!");
			}
		}

		CFastArray<String> nicList;
		CNetUtil::GetLocalIPAddresses(nicList);
		if (nicList.GetCount()>1 && param.m_localNicAddr.IsEmpty())
		{
			ShowWarning_NOCSLOCK(ErrorInfo::From(ErrorType_Unexpected, HostID_None, _PNT("Server has multiple network devices, however, no device is specified for listening. Is it your intention?")));

			String txt;
			txt.Format(_PNT("No NIC binding though multiple NIC detected##Process=%s\n"), GetProcessName().GetString());
			CErrorReporter_Indeed::Report(txt);
		}

		if (param.m_tunedNetworkerSendIntervalMs_TEST > 0)
		{
			// 사용자가 실수로 너무 큰 값을 넣으면 기본적인 ping-pong도 못해서 디스나기도 하므로 1000 이하로 강제 설정을 한다.
			m_everyRemoteIssueSendOnNeedIntervalMs = PNMIN(param.m_tunedNetworkerSendIntervalMs_TEST, 1000);
		}
		else
			m_everyRemoteIssueSendOnNeedIntervalMs = CNetConfig::EveryRemoteIssueSendOnNeedIntervalMs;

		m_settings.m_allowServerAsP2PGroupMember = param.m_allowServerAsP2PGroupMember;

		// youngjun.ko
		m_settings.m_enableEncryptedMessaging = param.m_enableEncryptedMessaging;
		if (param.m_enableEncryptedMessaging == false)
			param.m_enableP2PEncryptedMessaging = false;

		m_settings.m_enableP2PEncryptedMessaging = param.m_enableP2PEncryptedMessaging;
		m_settings.m_upnpDetectNatDevice = param.m_upnpDetectNatDevice;
		m_settings.m_upnpTcpAddPortMapping = param.m_upnpTcpAddPortMapping;
		m_settings.m_emergencyLogLineCount = param.m_clientEmergencyLogMaxLineCount;
		// 2010.07.16 add by ulelio : param에 m_enableNagleAlgorithm 추가
		m_settings.m_enableNagleAlgorithm = param.m_enableNagleAlgorithm;
		if (m_settings.m_enableNagleAlgorithm)
		{
			NTTNTRACE("Setting m_enableNagleAlgorithm to false is RECOMMENDED. ProudNet affords better anti-silly window syndrome technology than TCP Nagle Algorithm.\n");
		}

		m_usingOverBlockIcmpEnvironment = param.m_usingOverBlockIcmpEnvironment;

		// 2011.2.18 by LSH AES 에서는 오로지 128, 192, 256bit 만 지원합니다.
		if (param.m_encryptedMessageKeyLength != EncryptLevel_Low && param.m_encryptedMessageKeyLength != EncryptLevel_Middle
			&& param.m_encryptedMessageKeyLength != EncryptLevel_High)
		{
			throw Exception("EncryptedMessageKeyLength is invalid. Use 128,192 or 256.");
		}

		// Fast 키 길이 Bit 안쓸 떄에는 0
		m_settings.m_fastEncryptedMessageKeyLength = param.m_fastEncryptedMessageKeyLength;
		if (m_settings.m_fastEncryptedMessageKeyLength != FastEncryptLevel_Low &&
			m_settings.m_fastEncryptedMessageKeyLength != FastEncryptLevel_Middle &&
			m_settings.m_fastEncryptedMessageKeyLength != FastEncryptLevel_High)
		{
			throw Exception("FastEncryptedMessageKeyLength is invalid. Use 0, 512, 1024 or 2048.");
		}

		if ((param.m_encryptedMessageKeyLength > 0 && param.m_fastEncryptedMessageKeyLength <= 0) || (param.m_encryptedMessageKeyLength <= 0 && param.m_fastEncryptedMessageKeyLength > 0))
		{
			throw Exception("You must set both encryption keys or neither.");
		}

		m_settings.m_enablePingTest = param.m_enablePingTest;
		m_settings.m_ignoreFailedBindPort = param.m_ignoreFailedBindPort;

		// 공개키와 개인키를 미리 생성하고 루프백용 aes 키를 생성합니다.
		ByteArray randomBlock;
		ByteArray FastRandomBlock;
		if (CCryptoRsa::CreatePublicAndPrivateKey(m_selfXchgKey, m_publicKeyBlob) != true ||
			CCryptoRsa::CreateRandomBlock(randomBlock, m_settings.m_encryptedMessageKeyLength) != true ||
			CCryptoAes::ExpandFrom(m_selfSessionKey->m_aesKey, randomBlock.GetData(), m_settings.m_encryptedMessageKeyLength / 8) != true ||
			CCryptoRsa::CreateRandomBlock(FastRandomBlock, m_settings.m_fastEncryptedMessageKeyLength) != true ||
			CCryptoFast::ExpandFrom(m_selfSessionKey->m_fastKey, FastRandomBlock.GetData(), m_settings.m_fastEncryptedMessageKeyLength / 8) != true)
		{
			throw Exception("Failed to create Session Key");
		}

		m_selfEncryptCount = m_selfDecryptCount = 0;
		// create IOCP & user worker IOCP
		//이제 completionport는 threadpool이 갖는다.
		//m_completionPort.Attach(new CompletionPort(this, true));  //AcceptEx때문에 무조건 true
		//m_TcpAcceptCompletionPort.Attach(new CompletionPort(this, 1));

		// 		if(m_timer != NULL)
		// 		{
		// 			m_timer.Free();
		// 		}
		// 		m_timer.Attach(CMilisecTimer::New(param.m_useTimerType));
		// 시작
		//m_timer->Start();

		//m_sendReadyList = new CSendReadyList;

		//m_cachedTimes.Refresh(m_timer);

		// 서버 인스턴스 GUID 생성
		m_instanceGuid = Guid::RandomGuid();

		// 파라메터 복사
		m_udpAssignMode = param.m_udpAssignMode;
		m_protocolVersion = param.m_protocolVersion;
		m_serverAddrAlias = param.m_serverAddrAtClient;
		m_enableAutoConnectionRecoveryOnServer = param.m_enableAutoConnectionRecoveryOnServer;

		m_localNicAddr = param.m_localNicAddr;

		SetTimerCallbackParameter(param.m_timerCallbackIntervalMs, param.m_timerCallbackParallelMaxCount, param.m_timerCallbackContext);

		m_startCreateP2PGroup = false;

		m_simplePacketMode = param.m_simplePacketMode;

		m_allowExceptionEvent = param.m_allowExceptionEvent;

		param.m_netWorkerThreadCount = PNMIN(param.m_netWorkerThreadCount, (int)GetNoofProcessors());
		param.m_netWorkerThreadCount = PNMAX(param.m_netWorkerThreadCount, 0);
		if (param.m_netWorkerThreadCount == 0)
			param.m_netWorkerThreadCount = GetNoofProcessors();

		//threadpool 초기화.
		if (param.m_externalNetWorkerThreadPool != NULL)
		{
			CThreadPoolImpl* pthreadImpl = dynamic_cast<CThreadPoolImpl*>(param.m_externalNetWorkerThreadPool);
			if (pthreadImpl == NULL /*|| !pthreadImpl->IsActive()*/)
			{
				throw Exception("Networker thread pool is not set!");
			}

			m_netThreadPool = pthreadImpl;
			m_netThread_useExternal = true;
		}
		else
		{
			m_netThreadPool = ((CThreadPoolImpl*)CThreadPool::Create(NULL, param.m_netWorkerThreadCount));
			//m_netThreadPool->Start(param.m_netWorkerThreadCount);
			m_netThread_useExternal = false;
		}

		if (param.m_externalUserWorkerThreadPool != NULL)
		{
			CThreadPoolImpl* pThreadPoolImpl = dynamic_cast<CThreadPoolImpl*>(param.m_externalUserWorkerThreadPool);

			if (pThreadPoolImpl == NULL /*|| !pThreadPoolImpl->IsActive()*/)
			{
				throw Exception("User worker thread pool is not set!");
			}

			m_userThreadPool = pThreadPoolImpl;

			m_userThread_useExternal = true;
		}
		else
		{
			m_userThreadPool = ((CThreadPoolImpl*)CThreadPool::Create(this, param.m_threadCount));//내장일 경우 eventsink는 this가 된다.
			m_userThread_useExternal = false;
		}

		// TCP listen을 하기 전에 Loopback host, Authed host map, HostID factory를 만든다.
		// TCP listen을 한 후에 할 경우, 클라가 직후 막 들어온 것들을 HostIDFactory가 못 처리하는 문제가 라이브 서버에서 있었다.
		m_loopbackHost = shared_ptr<CLoopbackHost_S>(new CLoopbackHost_S(this));
		m_authedHostMap.Add(HostID_Server, m_loopbackHost);

		m_HostIDFactory.Attach(new CHostIDFactory(m_HostIDRecycleAllowTimeMs, param.m_hostIDGenerationPolicy == HostIDGenerationPolicy_Recycle));
		m_HostIDFactory->SetReservedList(param.m_reservedHostIDFirstValue, param.m_reservedHostIDCount);

		// 소켓 갯수가 CPU 갯수보다 적을 때는 경고를 내도록 하자.
		int desiredNetWorkerThreadCount = param.m_netWorkerThreadCount;
		if (desiredNetWorkerThreadCount == 0)
			desiredNetWorkerThreadCount = GetNoofProcessors();

		if (param.m_udpAssignMode == ServerUdpAssignMode_Static
			&& param.m_udpPorts.GetCount() < desiredNetWorkerThreadCount)
		{
			NTTNTRACE("Warning: It is recommended that number of static-assigned UDP sockets is greater than the number of Networker thread.\n");
		}

		CSuperSocketArray udpSockets;
		try
		{
			CreateSocketFailInfo createFailInfo;
			SocketErrorCode createSocketsResult = CreateTcpListenSocketsAndInit(param.m_tcpPorts, /*out*/createFailInfo);
			if (createSocketsResult != SocketErrorCode_Ok)
			{
				stringstream ss;
				ss << "TCP socket creation failed. Error=" << createSocketsResult << ", Array index=" << createFailInfo.m_arrayIndex << ", Port=" << createFailInfo.m_portNum;
				throw Exception(ErrorInfo::From(ErrorType_ServerPortListenFailure, HostID_None, StringA2T(ss.str().c_str())));
			}

			createSocketsResult = CreateAndInitUdpSockets(param.m_udpAssignMode, param.m_udpPorts, param.m_failedBindPorts, udpSockets, /*out*/createFailInfo);
			if (createSocketsResult != SocketErrorCode_Ok)
			{
				stringstream ss;
				ss << "UDP socket creation failed. Error=" << createSocketsResult << ", Array index=" << createFailInfo.m_arrayIndex << ", Port=" << createFailInfo.m_portNum;
				throw Exception(ErrorInfo::From(ErrorType_ServerPortListenFailure, HostID_None, StringA2T(ss.str().c_str())));
			}

			m_sendReadyList = shared_ptr<CSendReadySockets>(new CSendReadySockets);

			// 성공시 udp socket 들을 net thread pool 에 등록
			for (int i = 0; i < udpSockets.GetCount(); ++i)
			{
				m_netThreadPool->AssociateSocket(udpSockets[i], true);
			}

			// thread pool에 등록을 한다.
			// RegisterReferrer는 같은 것에 대해 여러번 호출해도 되게 만들어져 있다.
			m_netThreadPool->RegisterReferrer(this, true);
			m_userThreadPool->RegisterReferrer(this, false);

#if defined (_WIN32)
			// 모든 준비가 완료됐다. 생성한 static assigned UDP socket에 대해서 first recv issue를 걸자.
			if (m_netThreadPool->PostCustomValueEvent(this, CustomValueEvent_IssueFirstRecv) == false)
			{
				// 실패시 롤백 exception 을 날린다.
				throw Exception("cannot post custom event for unregistered IThreadReferrer!");
			}
#endif
		}
		catch (Exception& e)
		{
			// 앞서 시행했던 것들을 모두 롤백한다.
			//m_timer->Stop();

			{
				CriticalSectionLock clk(GetCriticalSection(), true);

				m_staticAssignedUdpSockets.Clear();
//				m_localAddrToUdpSocketMap.Clear(); // 윗줄에 이어 이것도 해야 제대로 파괴됨. smart ptr로 공유하니까.

				for (CSuperSocketArray::iterator i = m_TcpListenSockets.begin(); i != m_TcpListenSockets.end(); i++)
				{
					shared_ptr<CSuperSocket> sk = *i;
					sk->RequestStopIo();
				}
			}

			//m_TcpAcceptCompletionPort.Free();
			m_instanceGuid = Guid();

			// 위에서 만들었던 thread pool을 롤백한다.
			// 이래도 안전하다. 아직 socket들에 대해서 thread-pool add를 하지 않았으니까.
			CriticalSectionLock clk(GetCriticalSection(), true);
			udpSockets.Clear();
			StaticAssignedUdpSockets_Clear(true);

			if (!m_netThread_useExternal)
			{
				// PN 이 생성한 쓰레드풀 객체이므로 실제 메모리까지 삭제
				// ikpil.choi : 2016-11-03 safe series
				//SAFE_DELETE(m_netThreadPool);
				SafeDelete(m_netThreadPool);
			}
			else
			{
				// 사용자가 생성한 쓰레드풀이므로 삭제 의무는 사용자에게 있음
				m_netThreadPool = NULL;
			}
			if (!m_userThread_useExternal)
			{
				// ikpil.choi : 2016-11-03 safe series
				//SAFE_DELETE(m_userThreadPool);
				SafeDelete(m_userThreadPool);
			}
			else
			{
				m_userThreadPool = NULL;
			}

			m_sendReadyList.reset();

			// 위에서 생성했던 것들을 롤백.
			m_HostIDFactory.Free();
			m_authedHostMap.RemoveKey(HostID_Server);
			m_loopbackHost = shared_ptr<CLoopbackHost_S>();

			// 유저에게 re-throw exception
			throw e;
		}

#ifdef SUPPORTS_WEBGL
		Start_WebSocketServer(param);
#endif

		m_freqFailLogMostRank = 0;
		m_freqFailLogMostRankText = _PNT("");
		m_lastHolepunchFreqFailLoggedTime = 0;
		m_fregFailNeed = 0;

		// NS,LS.Start()에서는 m_authedHosts에 루프백 즉 (HostID_Server, new CLoopbackHost)를 add합시다.
		// 		m_loopbackHost = new CLoopbackHost(HostID_Server, LeanType_CNetServer);
		// 		m_authedHosts.Add(HostID_Server, m_loopbackHost);

		// TCP, UDP를 받아 처리하는 스레드 풀을 만든다.
		m_listening = true;


		m_PurgeTooOldUnmatureClient_Timer = CTimeAlarm(CNetConfig::PurgeTooOldAddMemberAckTimeoutMs);
		m_PurgeTooOldAddMemberAckItem_Timer = CTimeAlarm(CNetConfig::PurgeTooOldAddMemberAckTimeoutMs);
		m_disposeGarbagedHosts_Timer = CTimeAlarm(CNetConfig::DisposeGarbagedHostsTimeoutMs);
		m_electSuperPeer_Timer = CTimeAlarm(CNetConfig::ElectSuperPeerIntervalMs);
		m_removeTooOldUdpSendPacketQueue_Timer = CTimeAlarm(CNetConfig::LongIntervalMs);
		m_DisconnectRemoteClientOnTimeout_Timer = CTimeAlarm(CNetConfig::UnreliablePingIntervalMs);
		m_removeTooOldRecyclePair_Timer = CTimeAlarm(CNetConfig::RecyclePairReuseTimeMs / 2);







		// heartbeat TimeThreadPool에 등록
#if 0
		m_periodicPoster_Heartbeat.Attach(new CThreadPoolPeriodicPoster(
			this,
			CustomValueEvent_Heartbeat,
			m_netThreadPool,
			CNetConfig::HeartbeatIntervalMs));
#endif
		m_periodicPoster_GarbageCollectInterval.Attach(new CThreadPoolPeriodicPoster(
			this,
			CustomValueEvent_GarbageCollect,
			m_netThreadPool,
			CNetConfig::GarbageCollectIntervalMs));

		// Ontick TimeThreadPool에 등록
		if (m_timerCallbackInterval > 0)
		{
			m_timerCallbackParallelCount = 0;
			m_periodicPoster_Tick.Attach(new CThreadPoolPeriodicPoster(
				this,
				CustomValueEvent_OnTick,
				m_userThreadPool,
				m_timerCallbackInterval));
		}

		// issueSend TimeThreadPool에 등록
		m_issueSendOnNeedWorking = 0;
#if 0
		m_periodicPoster_SendEnqueued.Attach(new CThreadPoolPeriodicPoster(
			this,
			CustomValueEvent_SendEnqueued,
			m_netThreadPool,
			m_everyRemoteIssueSendOnNeedIntervalMs));
#endif

		// TODO: 나중에는 이런 초딩짓이 아니라 모든 스레드의 시작이 준비되는 순간에 리턴하게 만들자.
		Sleep(100);
	}

	void CNetServerImpl::Start(CStartServerParameter &param, ErrorInfoPtr& outError)
	{
		outError = ErrorInfoPtr();
		try
		{
			Start(param);
		}
		NOTHROW_FUNCTION_CATCH_ALL
	}

	// startFunctionCalls: caller가 서버시작용 함수인가?
	void CNetServerImpl::StaticAssignedUdpSockets_Clear(bool startFunctionCalls)
	{
		if(startFunctionCalls)
		{
			assert(m_authedHostMap.GetCount() == 1); // 유일한 1개는 Loopback Host이다.
		}
		else
		{
			assert(m_authedHostMap.GetCount() == 0); // 유일한 1개는 Loopback Host이다.
		}
		assert(m_candidateHosts.GetCount() == 0);

//		m_localAddrToUdpSocketMap.Clear();
		m_staticAssignedUdpSockets.Clear();
	}

	void CNetServerImpl::Stop()
	{
		Stop_Internal(false);
	}

	// bool destructorCalls: true이면, 파괴자에서 호출되고 있다는 뜻.
	void CNetServerImpl::Stop_Internal(bool destructorCalls)
	{
#ifdef SUPPORTS_WEBGL
		if (m_webSocketServer != nullptr)
			m_webSocketServer->stop();
		else if (m_webSocketServerSecured != nullptr)
			m_webSocketServerSecured->stop();

		// io_service stop을 시키면 가능한 빨리 리턴된다고 한다. WebSocketServer의 모든 스레드가 종료될때까지 WebServerThread join
		if(m_webSocketServerThread.joinable())
			m_webSocketServerThread.join();
#endif

		int64_t start;

		StopAsync();

		CriticalSectionLock ssLock(m_startStopCS, true);
		Sleep(1000);

		if(!destructorCalls) // 파괴자에서는 throw를 못한다. 그냥 데드락으로 이어지더라도 어쩔 수 없다.
		{
			// thread pool내부에 lock가 있으므로 여기서 criticalsection을 걸면 안됨.
			if (m_userThreadPool && m_userThreadPool->ContainsCurrentThread())
			{
				//modify by rekfkno1 - 지우지 말것.
				//이상하게도 userworkerthread내에서 try catch로 잡으면 파괴자가 호출되지 않는 현상이 생긴다.
				ssLock.Unlock();
				//end
				throw Exception("You called Stop() in your user worker thread pool. Don't do that.");
			}
		}

		{
			CriticalSectionLock mainLock(GetCriticalSection(), true);
			// 더 이상 accept은 없다.
			// 이게 선결되지 않으면 클라를 쫓는 과정에서 신규 클라가 들어오니 즐

			//listener스레드가 먼저 죽어야 한다. 그래야 accpetex가 실패로 나오는 것이 없다.
			m_listening = false;

			for (int i = 0; i < m_TcpListenSockets.GetCount(); i++)
			{
				if (m_TcpListenSockets[i] != NULL)
				{
					GarbageSocket(m_TcpListenSockets[i]);
				}
			}
			m_TcpListenSockets.Clear();

			GarbageAllHosts();

			// 이것이 없으면 recycles에 있는 UDP socket들이 계속해서 살아있어서 종료를 못한다.
			AllClearRecycleToGarbage();

			//XXXCheckHeap(0);

			// overlapped io 스레드를 종료하기 전에 socket을 닫아서(버퍼는 남긴채로) 더 이상 worker의 issue가 없게 만들어야 한다.
			// 그리고 종료해야 메모리를 안긁는다.
			for (int i = 0; i < (int)m_staticAssignedUdpSockets.GetCount(); i++)
			{
				shared_ptr<CSuperSocket> s = m_staticAssignedUdpSockets[i];
				GarbageSocket(s);
			}
			m_staticAssignedUdpSockets.Clear();
		}

		// 모든 remote 및 UDP socket의 비동기 io가 끝나고, delete까지 완료할 때까지 기다린다.
		int loopCount = 0;
		start = GetPreciseCurrentTimeMs();
		while (true)
		{
			if (CNetConfig::ConcealDeadlockOnDisconnect)
				loopCount++;

			if (loopCount > 10000)
				break;

			AssertIsNotLockedByCurrentThread();

			{
				CriticalSectionLock clk(GetCriticalSection(), true);

				if (m_netThreadPool && m_netThreadPool->GetThreadCount() == 0)
					m_netThreadPool->Process(0);

				if (m_userThreadPool && m_userThreadPool->GetThreadCount() == 0)
					m_userThreadPool->Process(0);

				// recyclable들을 garbage 처리해 버린다. 중간에 족족 생기더라도.
				AllClearRecycleToGarbage();

				// socket도 마찬가지로.
				DoGarbageCollect();

				// 이제 모두 청소될 수 있다면, 나가자.
				if (CanDeleteNow())
				{
					// reactor 모델의 경우엔 NetCoreHeart 가 제거 되었는지 확인까지 하고 나간다.
					shared_ptr<CReferrerHeart> acq;
					TryGetReferrerHeart(acq);
					if (!acq)
					{
						break;
					}
				}
			}

			// 청소 실패. 잠시 재시도하자.

			Proud::Sleep(10);

			if (((GetPreciseCurrentTimeMs() - start)) > 5000)
			{
				cout << "NetServer socket cleaning is not yet ended.\n";
				cout << "====PDI=====\n";
				PrintDebugInfo(3);
				cout << "====Dump=====\n";
				CanDeleteNow_DumpStatus();
				start = GetPreciseCurrentTimeMs();
			}
		}

		AssertIsNotLockedByCurrentThread();

		// thread pool로부터 독립한다.
		if (m_userThreadPool)
		{
			m_userThreadPool->UnregisterReferrer(this);
		}

		if (m_netThreadPool && m_netThreadPool != m_userThreadPool)
		{
			m_netThreadPool->UnregisterReferrer(this);
		}

		// TimeThreadPool에 등록되었던것들을 지운다.
		m_periodicPoster_Tick.Free();
//		m_periodicPoster_SendEnqueued.Free();
//		m_periodicPoster_Heartbeat.Free();
		m_periodicPoster_GarbageCollectInterval.Free();

		if (!m_netThread_useExternal)
		{
			if (m_netThreadPool == m_userThreadPool)
			{
				// ikpil.choi : 2016-11-03 safe series
				//SAFE_DELETE(m_netThreadPool);
				SafeDelete(m_netThreadPool);
				m_userThreadPool = NULL;
			}
			else
			{
				// ikpil.choi : 2016-11-03 safe series
				//SAFE_DELETE(m_netThreadPool);
				SafeDelete(m_netThreadPool);
			}
		}
		else
		{
			m_netThreadPool = NULL;
		}
		//
		if (!m_userThread_useExternal)
		{
			// ikpil.choi : 2016-11-03 safe series
			//SAFE_DELETE(m_userThreadPool);
			SafeDelete(m_userThreadPool);
		}
		else
		{
			m_userThreadPool = NULL;
		}

		// 이제 모든 스레드가 종료됐으니 remote 및 UDP socket 객체들을 모두 파괴
		{
			CriticalSectionLock clk(GetCriticalSection(), true);
			CHECK_CRITICALSECTION_DEADLOCK(this);

			m_TcpListenSockets.Clear();

#ifndef PROUDSRCCOPY
			((CPNLic*)m_lic)->POSE();
#endif

			m_P2PGroups.Clear();

			// 최종 의존 관계인 이것들이 여기서야 파괴됨
			StaticAssignedUdpSockets_Clear(false);
		}

		// 아래 과정은 unsafe heap을 쓰는 것들을 CS lock 상태에서 청소하지 않으면 괜한 assert fail이 발생하므로 필요
		CriticalSectionLock lock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		//m_finalUserWorkItemList.Clear();
		//ClearFinalUserWorkItemList();

		m_p2pConnectionPairList.Clear();

		m_HostIDFactory.Free();

		// Garbage Pattern for Host Object 명세가 나오기 전까지는 임시 방편으로 구현
		// NetServer의 경우 Start ~ Stop 범위에서만 LoopbackHost 사용
		m_loopbackHost = nullptr;

		// 모든 socket이 다 파괴되어야만 안전하게 iocp도 제거 가능
		//m_TcpAcceptCompletionPort.Free();
#ifdef VIZAGENT
		m_vizAgent.Free();
#endif
		DeleteSendReadyList();
		//m_shutdowning = false;
	}

	void CNetServerImpl::StopAsync()
	{
		CriticalSectionLock ssLock(m_startStopCS, true);

		AssertIsNotLockedByCurrentThread();

		//이미 stop된 상황이면 신경쓰지 말자...
		if (m_TcpListenSockets.GetCount() == 0 /*|| m_shutdowning*/)
			return;

		StopAccess();

		// 연결된 클라에서 연결을 끊도록 알린다.
		CloseEveryConnection();
	}



#if defined (_WIN32)
	void CNetServerImpl::StaticAssignedUdpSockets_IssueFirstRecv()
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		for (CSuperSocketArray::iterator i = m_staticAssignedUdpSockets.begin(); i != m_staticAssignedUdpSockets.end(); i++)
		{
			shared_ptr<CSuperSocket> udpSocket = *i;

			// 각 UDP socket에 overlapped recv를 건다.
			udpSocket->IssueRecv(udpSocket, true);
		}
	}

	void CNetServerImpl::TcpListenSockets_IssueFirstAccept()
	{
		for (int i = 0; i < m_TcpListenSockets.GetCount(); i++)
		{
			// acceptex가 실패하는 경우는 드물지만, 어쨌거나 없으란 법은 없다.
			// 이러한 경우 서버를 종료하지는 말고, 에러를 뱉도록 하자.
			SocketErrorCode e = m_TcpListenSockets[i]->IssueAccept(false);
			EnqueErrorOnIssueAcceptFailed(e);
		}
	}
#endif // _WIN32

	// 의도적으로 byval. 데이터 변경을 하지만 콜러에게 주면 안되므로.
	// tcpPorts: 의도적으로 const&가 아님
	SocketErrorCode CNetServerImpl::CreateTcpListenSocketsAndInit(CFastArray<int> tcpPorts, CreateSocketFailInfo& outFailInfo)
	{
		// NOTE: TcpPorts는 &가 아니다. 즉 사본을 떠서 작업하자. 왜냐하면 아래 때문.
		// TCP 소켓이 하나도 없으면 곤란. 따라서 임의의 포트를 갖도록 만든다.
		if (tcpPorts.GetCount() == 0)
			tcpPorts.Add(0);

		for (int i = 0; i < (int)tcpPorts.GetCount(); i++)
		{
			SuperSocketCreateResult r = CSuperSocket::New(this, SocketType_Tcp);
			if (!r.socket)
			{
				// 실패했으므로 기존의 listen socket들도 삭제한다.
				m_TcpListenSockets.Clear();
				outFailInfo.m_arrayIndex = i;
				outFailInfo.m_portNum = 0;
				return SocketErrorCode_Error;
			}

			shared_ptr<CSuperSocket> tcpListenSocket(r.socket);

			//tcpListenSocket->m_ignoreNotSocketError = true;

			if (CNetConfig::EnableSocketReuseAddress == true)
			{
#if defined (_WIN32)
				// 1. reuse를 하는 이유는 TIME_WAIT로 인해 bind가 실패하는 것이 불편하기 때문.다른이유는 없음.
				// 2. Windows에서 reuse를 사용하면 해당 포트를 다수의 프로세스가 공유하는 현상이 위험하므로 0번으로 bind할 경우에는 reuse를 사용하지 않음.
				tcpListenSocket->GetSocket()->SetSocketReuseAddress(tcpPorts[i] != 0);
#else
				// 3. Linux는 2번의 위험을 비롯하여 특별히 문제될 부분이 없으므로 무조건 reuse를 사용하도록 결정.
				tcpListenSocket->GetSocket()->SetSocketReuseAddress(true);
#endif
			}

			SocketErrorCode socketErrorCode = SocketErrorCode_Ok;
			if (m_localNicAddr.IsEmpty())
				socketErrorCode = tcpListenSocket->Bind(tcpPorts[i]);
			else
				socketErrorCode = tcpListenSocket->Bind(m_localNicAddr.GetString(), tcpPorts[i]);

			if (socketErrorCode != SocketErrorCode_Ok)
			{
				// TCP 리스닝 자체를 시작할 수 없는 상황이다. 이런 경우 서버는 제 기능을 할 수 없는 심각한 상황이다.
				//outError = ErrorInfo::From(ErrorType_ServerPortListenFailure, HostID_Server, _PNT(""));
				tcpListenSocket = nullptr;

				// 실패했으므로 기존의 listen socket들도 삭제한다.
				m_TcpListenSockets.Clear();

				outFailInfo.m_arrayIndex = i;
				outFailInfo.m_portNum = tcpPorts[i];

				return socketErrorCode;
			}
			else
			{
				tcpListenSocket->GetSocket()->Listen();
				//AssertIsLockedByCurrentThread();
				m_netThreadPool->AssociateSocket(tcpListenSocket, true);
#if !defined (_WIN32)
				tcpListenSocket->m_isListeningSocket = true;
#endif
				SocketToHostsMap_SetForAnyAddr(tcpListenSocket, m_loopbackHost);

				m_TcpListenSockets.Add(tcpListenSocket);
				// #ifdef _WIN32
				// 				IssueFirstRecvNeededSocket_Set(tcpListenSocket);
				// #endif
				// NOTE: Start함수의 끝자락에서 first AcceptEx를 시전하는 custom value event를 post한다.
			}
		}

		return SocketErrorCode_Ok;
	}

	SocketErrorCode CNetServerImpl::CreateAndInitUdpSockets(ServerUdpAssignMode udpAssignMode,
		CFastArray<int> udpPorts, // 의도적으로 const&가 아님
		CFastArray<int> &failedBindPorts,
		CSuperSocketArray& outCreatedUdpSocket,
		CreateSocketFailInfo& outFailInfo)
	{
		CriticalSectionLock lock(GetCriticalSection(), true); // needed.
		CHECK_CRITICALSECTION_DEADLOCK(this);

		// UDP 소켓이 하나도 없으면 곤란. 따라서 임의의 포트를 갖도록 만든다.
		if (udpPorts.GetCount() == 0)
			udpPorts.Add(0);

		// socket 객체들을 만든다.
		StaticAssignedUdpSockets_Clear(true);

		for (int i = 0; i < (int)udpPorts.GetCount(); i++)
		{
			NamedAddrPort addrPort = NamedAddrPort::FromAddrPort(m_localNicAddr, (uint16_t)udpPorts[i]);
			SuperSocketCreateResult r = CSuperSocket::New(this, SocketType_Udp);
			if (!r.socket)
			{
				StaticAssignedUdpSockets_Clear(true);
				outFailInfo.m_arrayIndex = i;
				outFailInfo.m_portNum = udpPorts[i];
				return SocketErrorCode_Error;
			}
			shared_ptr<CSuperSocket> udpSocket(r.socket);

			int port = udpPorts[i];
			SocketErrorCode socketErrorCode = SocketErrorCode_Ok;
			if (m_localNicAddr.IsEmpty())
				socketErrorCode = udpSocket->Bind(port);
			else
				socketErrorCode = udpSocket->Bind(m_localNicAddr.GetString(), port);

			if (SocketErrorCode_Ok != socketErrorCode)
			{
				if (m_settings.m_ignoreFailedBindPort == false)
				{
					//outError = ErrorInfo::From(ErrorType_ServerPortListenFailure, HostID_Server, _PNT(""));

					StaticAssignedUdpSockets_Clear(true);
					outFailInfo.m_arrayIndex = i;
					outFailInfo.m_portNum = port;

					return socketErrorCode;
				}
				else
				{
					failedBindPorts.Add(port);
					continue;
				}
			}

			// socket local 주소를 얻고 그것에 대한 매핑 정보를 설정한다.
			if (udpSocket->GetLocalAddr().m_port == 0) // NOTE: port 65535는 정상값 중 하나임.
			{
				throw Exception("m_cachedLocalAddr has an unexpected value!");
			}
			m_staticAssignedUdpSockets.Add(udpSocket);
			// #ifdef _WIN32
			// 			IssueFirstRecvNeededSocket_Set(udpSocket);
			// #endif

//			m_localAddrToUdpSocketMap.Add(udpSocket->GetLocalAddr(), udpSocket);

			// IOCP에 연계한다.
			AssertIsLockedByCurrentThread();

			// NOTE: NetServer Start 가 실패했을때 롤백 과정중 I/O Avail 이 와서 access violation 이 뜬다.
			// 따라서 해당 함수내에서 netThreadPool 에 등록하지 말고 콜러에서 하도록 한다.
			// m_netThreadPool->AssociateSocket(udpSocket->GetSocket());
			outCreatedUdpSocket.Add(udpSocket);

			// overlapped send를 하므로 송신 버퍼는 불필요하다.
			// socket의 send buffer를 없앤다. CSocketBuffer가 non swappable이므로 안전하다.
			// send buffer 없애기는 coalsecse, throttling을 위해 필수다.
			// recv buffer 제거는 백해무익이므로 즐

			// CSuperSocket::Bind 내부에서 처리하기때문에 불필요
			// #ifdef ALLOW_ZERO_COPY_SEND // zero copy send는 빠르지만 너무 많은 nonpaged를 유발 위험. 따라서 이걸로 막자. 막으니 성능도 더 나은데?
			// 			udpSocket->m_socket->SetSendBufferSize(0);
			// #endif // ALLOW_ZERO_COPY_SEND

			/* 소켓 갯수가 너무 적다면 UDP 소켓당 버퍼 갯수를 충분히 키우자.
			어차피 I/O 처리 루프가 UDP 소켓 하나가 처리하는 엄청난 패킷량을
			감당할 속도는 받춰주어야 하지만 소켓 버퍼가 너무 작으면 잔여
			소켓 버퍼만큼을 처리해주지 못해 패킷을 드랍할 확률이 증가하기 떄문. */
			if (udpAssignMode == ServerUdpAssignMode_Static && udpPorts.GetCount() < 100)
			{
				SetUdpDefaultBehavior_ServerStaticAssigned(udpSocket->GetSocket());
			}
			else
			{
				SetUdpDefaultBehavior_Server(udpSocket->GetSocket());
			}
			// 여러 클라를 상대로 UDP 수신을 하는 만큼 버퍼의 양이 충분히 커야 한다.
			//udpSocket->m_socket->SetRecvBufferSize(CNetConfig::ServerUdpRecvBufferLength);
		}
		return SocketErrorCode_Ok;
	}

}


