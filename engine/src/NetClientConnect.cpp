// NetClient의 연결/연결 해제를 담당

#include "stdafx.h"

// STL header file은 xcode에서는 cpp에서만 include 가능하므로
#include <stack>

#include "NetClient.h"
#include "CollUtil.h"
#include "RmiContextImpl.h"
#include "SocketUtil.h"

namespace Proud
{
	// 연결 과정을 시작하기 전에, this와 parameter의 validation check를 한다.
	void CNetClientImpl::Connect_CheckStateAndParameters(const CNetConnectionParam &param)
	{
		/* 값의 유효성 검사는, this의 멤버에 변화를 가하기 전에 해야 합니다.
		그런데 여기까지 왔다면 이미 this의 멤버에 변화를 가한 상태입니다.
		따라서 아래 검사 후 throw를 하는 루틴은 검사를 하는 과정 즉 Connect_CheckStateAndParameters로 옮겨 주시기 바랍니다.
		같은 수정을, C#,java에도 해주세요. */

		// 파라메터 정당성 체크
		// m_serverIP로 ipv4 literal + 도메인 주소를 입력받기 떄문에 아래 조건 검사에 대한 부분을 재고해야 합니다.
		// => 이 코드가 있어야 할 이유가 없음. 따라서 제거.
// 		if (CNetUtil::IsAddressUnspecified(param.m_serverIP)
// 			|| CNetUtil::IsAddressAny(param.m_serverIP)
// 			|| param.m_serverPort == 0)
// 		{
// 			throw Exception(ErrorInfo::TypeToString(ErrorType_UnknownAddrPort));
// 		}

		// UDP port pool에 잘못된 값이나 중복된 값이 있으면 에러 처리
		for (int i = 0; i < param.m_localUdpPortPool.GetCount(); i++)
		{
			if (param.m_localUdpPortPool[i] <= 0)
			{
				throw Exception(ErrorInfo::TypeToString(ErrorType_InvalidPortPool));
			}
		}

		m_usedUdpPorts.Clear();
		m_unusedUdpPorts.Clear();
		for (int i = 0; i < param.m_localUdpPortPool.GetCount(); i++)
		{
			if (param.m_localUdpPortPool[i] <= 0)
			{
				throw Exception(ErrorInfo::TypeToString(ErrorType_InvalidPortPool));
			}
			if (m_unusedUdpPorts.ContainsKey((uint16_t)param.m_localUdpPortPool[i]) == true)
			{
				throw Exception(ErrorInfo::TypeToString(ErrorType_InvalidPortPool));
			}
			m_unusedUdpPorts.Add((uint16_t)param.m_localUdpPortPool[i]);
		}

// 		CFastArray<int> udpPortPool = param.m_localUdpPortPool; // copy
// 		UnionDuplicates<CFastArray<int>, int, intptr_t >(udpPortPool);
// 		if (udpPortPool.GetCount() != param.m_localUdpPortPool.GetCount())
// 		{
// 			// 길이가 다름 => 중복된 값이 있었음
// 			throw Exception(ErrorInfo::TypeToString(ErrorType_InvalidPortPool));
// 		}
	}

	// ErrorInfoPtr을 따로 주지 않는 레거시 함수
	bool CNetClientImpl::Connect(const CNetConnectionParam &param)
	{
		LockMain_AssertIsNotLockedByCurrentThread();

		CriticalSectionLock phaseLock(m_connectDisconnectFramePhaseLock, true);

		// 사용자가 user callback에서 Disconnect()를 호출했다면=>아직 스레드풀 객체들이 남아있다.
		// 이 상태에서 사용자가 Connect()를 호출한다면, 기존 스레드풀 잔존을 모두 정리 후 재개해야 한다.
		if (m_worker->GetState() == CNetClientWorker::Disconnecting)
		{
			Disconnect();
		}


		CriticalSectionLock mainLock(GetCriticalSection(), true);

		// 위에서 Disconnect()했으니.
		// --> Connect_Internal 코드는 아래에 있으며, IThreadReferrer 의 액세스 허용 그쪽에 한다. 따라서 이 assert 는 무의미
		//assert(!m_referrerHeart);

		if (m_worker->GetState() == CNetClientWorker::Disconnected)
		{
			return Connect_Internal(param);
		}
		else
		{
			throw Exception(_PNT("Can't do Disconnect in NetClient.Connect(). You might call it outside RMI or event callback routine."));
		}
	}

	// ErrorInfoPtr을 따로 주는 함수. 그리고 no throw를 보장한다. UE4에서 쓸 수 있는 함수.
	bool CNetClientImpl::Connect(const CNetConnectionParam &param, ErrorInfoPtr& outError)
	{
		try
		{
			outError = ErrorInfoPtr();
			return Connect(param);
		}
		NOTHROW_FUNCTION_CATCH_ALL

		return false;
	}

	// connect 과정의 내부 본체
	bool CNetClientImpl::Connect_Internal(const CNetConnectionParam &param)
	{
		CriticalSectionLock clk(GetCriticalSection(), true); // for atomic oper

		// 우선 NetCore 모듈이 동작해도 된다는 신호를 날려도 된다.
		// 이게 on 되어 있다고 문제가 되는게 아니므로
		AllowAccess();

		m_connectionParam = param;

		if (m_connectionParam.m_closeNoPingPongTcpConnections == false)
		{
			// 하위호환성때문에 false이지만, 사용자에게는 true로 세팅함을 권고한다.
			EnqueWarning(ErrorInfo::From(ErrorType_Ok, HostID_None, 
				_PNT("It is recommended to set closeNoPingPongTcpConnections to FALSE. For details, check out closeNoPingPongTcpConnections documentation.")));
		}

		m_HolsterMoreCallbackUntilNextProcessCall_flagged = false;

		// 외부 쓰레드 모델을 사용할 경우 Parameter Check.
		if (m_netThreadPool == nullptr && param.m_netWorkerThreadModel == ThreadModel_UseExternalThreadPool)
		{
			if (param.m_externalNetWorkerThreadPool == nullptr)
			{
				throw Exception(_PNT("External thread pool is not set!"));
			}
		}

		// 외부 쓰레드 모델을 사용할 경우 Parameter Check.
		if (m_userThreadPool == nullptr && param.m_userWorkerThreadModel == ThreadModel_UseExternalThreadPool)
		{
			if (param.m_externalUserWorkerThreadPool == nullptr)
			{
				throw Exception();
			}
		}

		Connect_CheckStateAndParameters(param);

		AtomicIncrement32(&m_connectCount);

		// 		CServerConnectionState cs;
		// 		ConnectionState retcs = GetServerConnectionState(cs);
		// 		if ( retcs != ConnectionState_Disconnected)
		// 		{
		// 			throw Exception(_PNT("Wrong state(%s)! Disconnect() or GetServerConnectionState() may be required."), ToString(retcs));
		// 		}
		SetTimerCallbackParameter(param.m_timerCallbackIntervalMs, param.m_timerCallbackParallelMaxCount, param.m_timerCallbackContext);

		assert(m_sendReadyList == nullptr);
		m_sendReadyList = shared_ptr<CSendReadySockets>(new CSendReadySockets);

		// loopback host와 remote server host 인스턴스를 생성한다.
		// 이들은 Connect 호출 순간부터 Disconnect 리턴 까지 시간대에서 항상 존재한다.
		m_loopbackHost = shared_ptr<CLoopbackHost_C>(new CLoopbackHost_C(this, HostID_None));

		// 일단은 candidate에 들어간다. local hostID를 아직 모르기에.
		m_candidateHosts.Add(m_loopbackHost.get(), m_loopbackHost);

		m_remoteServer = shared_ptr<CRemoteServer_C>(new CRemoteServer_C(this));
		if (!m_remoteServer->m_ToServerTcp)
		{
			// TCP socket을 생성 못했다. 자원이 부족해서라던지.
			// 그냥 실패 처리하자.
			throw Exception("TCP socket creation failure. Did you make too many sockets already?");
		}

		// NOTE: 서버와의 연결을 시작도 안했는데 서버 주소를 dns lookup하거나 string to ip 같은 짓을 하지 말자.
		// 전자는 소폭 블러킹 때문에 이 함수가 블러킹을 일으키고, 후자는 hostname을 처리 못하니까.

		m_candidateHosts.Add(m_remoteServer.get(), m_remoteServer); // 아직 서버와의 연결이 된 것은 아니니, 원리원칙대로 candidate에 들어가야.

		SocketToHostsMap_SetForAnyAddr(m_remoteServer->m_ToServerTcp, m_remoteServer);

		if (m_remoteServer->m_ToServerUdp /*|| m_remoteServer->GetToServerUdpFallbackable()*/) // 하지만 이건 이미 제거된 상태이어야 한다.
		{
			String txt;
			txt.Format(_PNT("Unstability in Connect #1! Process=%s"), GetProcessName().GetString());
			CErrorReporter_Indeed::Report(txt);
		}

		/* 이제, 상태 청소를 해도 된다!
		서버 접속에 관련된 모든 값들을 초기화한다.
		(disconnected state에서도 이게 안 비어있을 수 있다.
		왜냐하면 서버에서 추방직전 쏜 RMI를 클라가 모두 처리하려면 disconnected state에서도 미처리 항목을 유지해야 하기 때문이다.) */
		if (param.m_tunedNetworkerSendIntervalMs_TEST > 0)
		{
			m_everyRemoteIssueSendOnNeedIntervalMs = param.m_tunedNetworkerSendIntervalMs_TEST;
		}
		else
		{
			m_everyRemoteIssueSendOnNeedIntervalMs = CNetConfig::EveryRemoteIssueSendOnNeedIntervalMs;
		}

		// thread pool 초기화.
		if (m_netThreadPool == nullptr)
		{
			switch (param.m_netWorkerThreadModel)
			{
			case ThreadModel_SingleThreaded:
				// singleton으로서 존재하는 zero thread pool을 이용.
				m_netThreadPool = m_manager->m_netWorkerZeroThreadPool;
				break;

			case ThreadModel_MultiThreaded:
				// singleton으로서 존재하는 multi threaded thread pool을 이용.
				m_netThreadPool = m_manager->m_netWorkerMultiThreadPool;
				break;
			case ThreadModel_UseExternalThreadPool:
				// 외부 쓰레드를 사용한다.
				m_netThreadPool = ((CThreadPoolImpl*)param.m_externalNetWorkerThreadPool);
				break;
			}

			m_netThreadPool->RegisterReferrer(this, true);
		}

		if (m_userThreadPool == nullptr)
		{
			switch (param.m_userWorkerThreadModel)
			{
			case ThreadModel_SingleThreaded:
				// zero thread pool을 사용한다.
				m_userThreadPool = ((CThreadPoolImpl*)CThreadPool::Create(nullptr, 0));
				break;

			case ThreadModel_MultiThreaded:
				// thread가 이미 존재하는 thread pool을 사용한다.
				m_userThreadPool = ((CThreadPoolImpl*)CThreadPool::Create(nullptr, GetNoofProcessors()));
				break;
			case ThreadModel_UseExternalThreadPool:
				// 외부 쓰레드를 사용한다.
				m_userThreadPool = ((CThreadPoolImpl*)param.m_externalUserWorkerThreadPool);
				break;
			}

			m_userThreadPool->RegisterReferrer(this, false);
		}

		g_TEST_CloseSocketOnly_CallStack = String();

		m_hasWarnedStarvation = false;

		// 마지막 Disconnect에서 이게 리셋 안되어 있고 예전에 쓰던 값이 있으면 곤란하므로
		m_settings = CNetSettings();

		int64_t currentTimeMs = GetPreciseCurrentTimeMs();
		m_RemoveTooOldUdpSendPacketQueueOnNeed_Timer.SetIntervalMs(CNetConfig::LongIntervalMs);
		m_RemoveTooOldUdpSendPacketQueueOnNeed_Timer.Reset(currentTimeMs);
		m_ReliablePing_Timer.SetIntervalMs(GetReliablePingTimerIntervalMs());
		m_ReliablePing_Timer.Reset(currentTimeMs);

		m_natDeviceNameDetected = false;

		m_P2PConnectionTrialEndTime = CNetConfig::GetP2PHolepunchEndTimeMs();
		m_P2PHolepunchInterval = CNetConfig::P2PHolepunchIntervalMs;

		// reset it
		m_stats = CNetClientStats();

		// ACR
		m_enableAutoConnectionRecovery = param.m_enableAutoConnectionRecovery;
		m_closeNoPingPongTcpConnections = param.m_closeNoPingPongTcpConnections;

		// ACR은 simple packet mode에서 강제로 꺼진다.
		// message id 교환 등 복잡한 것들은 사용자가 다루기 힘들다.
		if (param.m_simplePacketMode)
		{
			m_enableAutoConnectionRecovery = false;
			m_closeNoPingPongTcpConnections = false;
		}

		m_autoConnectionRecoveryDelayMs = 0;
		m_autoConnectionRecoveryContext = CAutoConnectionRecoveryContextPtr();

		m_lastFrameMoveInvokedTime = currentTimeMs;

		m_toServerEncryptCount = 0;
		m_toServerDecryptCount = 0;

		m_lastRequestServerTimeTime = 0;
		m_RequestServerTimeCount = 0;

		m_serverTimeDiff = 0;

		m_serverUdpRecentPingMs = 0;
		m_serverUdpLastPingMs = 0;

		m_serverTcpRecentPingMs = 0;
		m_serverTcpLastPingMs = 0;
		m_firstTcpPingedTime = 0;

		m_disconnectCallTime = 0;

		//m_lastMessageOverloadCheckingTime = GetPreciseCurrentTimeMs() + CNetConfig::NetClientMessageOverloadTimerIntervalMs;

		// 서버에 접속을 시작하는 상황이므로, 전부다 리셋.
		m_loopbackHost->m_backupHostID = m_loopbackHost->m_HostID = HostID_None;

		m_speedHackDetectorPingTime = CNetConfig::EnableSpeedHackDetectorByDefault ? 0 : INT64_MAX;

		m_selfEncryptCount = 0;
		m_selfDecryptCount = 0;

		// m_slowReliableP2P 모드가 사용중인가 판단하여 적절한 값을 실제 사용할 변수 공간에 저장한다.
		// m_slowReliableP2P 는 per-Client 값
		// m_slowReliableP2P 을 참조해야 하는 곳에서 이값만 참조하여 3항 연산자(?:) 를 사용해도 되겠지만 하드코딩(0.001L) 값이 다른 값으로 바뀌어야 하거나(유지보수)
		// 계속 값을 참조하여 3항 연산자(?:) 를 써야 한다면 속도상 약간 손해를 볼 수 있어서 이렇게 구현하기로 결정
		// 또한 Renewal 된 ReliableUDP 는 ReliableUdpSender/ReliableUdpReceiver 가 ReliableUDPHost 로 통합되어 포인터 참조 횟수가 줄어든다
		if (param.m_slowReliableP2P)
		{
			m_ReliableUdpHeartbeatInterval_USE = CNetConfig::ReliableUdpHeartbeatIntervalMs_ForDummyTest;
			m_StreamToSenderWindowCoalesceInterval_USE = ReliableUdpConfig::StreamToSenderWindowCoalesceIntervalMs_ForDummyTest;
		}
		else // case false
		{
			m_ReliableUdpHeartbeatInterval_USE = CNetConfig::ReliableUdpHeartbeatIntervalMs_Real/*0.001L*/;		// Maximize P2P trans throughtput
			m_StreamToSenderWindowCoalesceInterval_USE = ReliableUdpConfig::StreamToSenderWindowCoalesceIntervalMs_Real/*0.001L*/;
		}

		m_supressSubsequentDisconnectionEvents = false;
		m_simplePacketMode = param.m_simplePacketMode;
		m_allowExceptionEvent = param.m_allowExceptionEvent;



		//m_ToServerUdp_fallbackable = CFallbackableUdpLayerPtr_C(new CFallbackableUdpLayer_C(this));

		m_checkTcpUnstable_timeToDo = m_lastHeartbeatTime = GetPreciseCurrentTimeMs();
		m_recentElapsedTime = 0;

		m_issueSendOnNeedWorking = 0;

		// 클라이언트 워커 스레드에 이 객체를 등록시킨다. 없으면 새로 만든다.
		// 이 객체가 생성되므로 인해 state machine이 작동을 시작한다.
		// m_worker를 새로 할당하지 말고, value를 overwrite한다. Selvas case에서, lock-free access case가 의심되었다.
		m_worker->Reset();
		m_worker->SetState(CNetClientWorker::IssueConnect);

		// heartbeat과 coalesce send를 일으키는 event push를 일정 시간마다 일어나게 한다.
// #if 0
// 		if (m_periodicPoster_Heartbeat == nullptr)
// 		{
// 			m_periodicPoster_Heartbeat.Attach(new CThreadPoolPeriodicPoster(
// 				this,
// 				CustomValueEvent_Heartbeat,
// 				m_netThreadPool,
// 				CNetConfig::HeartbeatIntervalMs));
// 		}
// #endif
		if (m_periodicPoster_GarbageCollectInterval == nullptr)
		{
			m_periodicPoster_GarbageCollectInterval.Attach(new CThreadPoolPeriodicPoster(
				this,
				CustomValueEvent_GarbageCollect,
				m_netThreadPool,
				CNetConfig::GarbageCollectIntervalMs));
		}
// #if 0
// 		if (m_periodicPoster_SendEnqueued == nullptr)
// 		{
// 			m_periodicPoster_SendEnqueued.Attach(new CThreadPoolPeriodicPoster(
// 				this,
// 				CustomValueEvent_SendEnqueued,
// 				m_netThreadPool,
// 				CNetConfig::EveryRemoteIssueSendOnNeedIntervalMs));
// 		}
// #endif

		if (m_timerCallbackInterval > 0)
		{
			m_timerCallbackParallelCount = 0;
			m_periodicPoster_Tick.Attach(new CThreadPoolPeriodicPoster(
				this,
				CustomValueEvent_OnTick,
				m_userThreadPool,
				m_timerCallbackInterval));
		}

		return true;
	}



	void CNetClientImpl::Disconnect()
	{
		CDisconnectArgs args;
		Disconnect(args);
	}

	void CNetClientImpl::Disconnect(ErrorInfoPtr& outError)
	{
		try
		{
			outError = ErrorInfoPtr();
			Disconnect(); // NOTE: outError는 여기서 안 씀. 대신, outError는 아래 catch에서 채워짐.
		}
		NOTHROW_FUNCTION_CATCH_ALL
	}

	void CNetClientImpl::Disconnect(const CDisconnectArgs &args)
	{
		LockMain_AssertIsNotLockedByCurrentThread();

		CriticalSectionLock phaseLock(m_connectDisconnectFramePhaseLock, true);

		// 		// 드물지만 CNetClientManager가 먼저 이미 파괴된 경우라면 프로세스 종료 상황이겠다. 이럴 때는 이 함수는 즉시 리턴한다.
		// 		if(!m_manager->m_cs.IsValid())
		// 			goto LEXIT;
		//

		m_HolsterMoreCallbackUntilNextProcessCall_flagged = false;

		// 여기서 연결 해제에 대한 주요 기능을 한다.
		DisconnectAsync(args);

		/* user thread pool callback에서 실행되는 경우, 스레드풀 종료를 기다릴 수 없으므로 그냥 나간다.
		Q: 그러면 user thread pool은 언제 정리되나요? 즉 이 아래 코드의 실행은 언제?
		A: ~CNetClientImpl()에서 Disconnect()가 또 호출되는데, 그때 파괴됩니다. 단, delete를 user thread pool에서 하면 안되죠. 
		
		Q: single thread mode 즉 사용자가 FrameMove를 호출하는 형태이면, 아래 코드는 언제 실행되나요?
		A: FrameMove 함수 실행 중에 사용자가 Disconnect를 호출했던 상황입니다. FrameMove에서는 그러한 상황임을 감지하면 아래 코드를 실행합니다.
		*/
		if (CurrentThreadIsRunningUserCallback())
		{
			// #NetClientDisconnectOnCallback
			// NOTE: 예전에는, 여기서 throw를 했다. 하지만 많은 사용자는 RMI 즉 user callback 안에서 
			// Disconnect를 호출하는 경향이 많다.
			// 따라서 여기서 throw를 하지 말고, FrameMove 함수 끝자락에서 아래 코드를 실행하자.
			// 여기서는 그냥 나가도 된다. 이미 DisconnectAsync에서 "이미 DisconnectAsync를 호출했었음"이 mark되었고
			// NetClientImpl.FrameMove 안에서 CleanupAfterDisconnectIsCalled를 대신 호출할거다.
			return;
		}

		// 뒷단 마무리 처리
		m_disconnectArgs = args;
		CleanupAfterDisconnectIsCalled();
	}

	void CNetClientImpl::Disconnect(const CDisconnectArgs &args, ErrorInfoPtr& outError)
	{
		try
		{
			outError = ErrorInfoPtr();
			Disconnect(args);
		}
		NOTHROW_FUNCTION_CATCH_ALL
	}

	/* Disconnect()와 달리, 이함수는 블러킹 없이 연결 해제를 건다.
	user callback 안에서 호출해도 된다.
	다만 Disconnect()와 달리 이 함수 호출 후에도 OnLeaveServer등
	유저콜백이 발생할 수 있다. */
	void CNetClientImpl::DisconnectAsync(const CDisconnectArgs &args)
	{
		CriticalSectionLock mainLock(GetCriticalSection(), true);

		// Disconnect가 중복으로 실행하는 경우
		if (m_worker->GetState() == CNetClientWorker::Disconnected)
			return;

		// ACR 용 remote server 와 super socket garbage 요청
		// Disconnecting state가 될 때 호출되므로 여기서 불필요. 해서도 안됨.
		//AutoConnectionRecovery_DestroyCandidateRemoteServersAndSockets(shared_ptr<CSuperSocket>());

		// 사용자가 Disconnect를 호출한 경우에는 처리하지 않습니다.
		if (m_worker->GetState() == CNetClientWorker::Disconnecting)
			return;

		// 통계
		AtomicIncrement32(&m_disconnectInvokeCount);

		if (m_enableLog || m_settings.m_emergencyLogLineCount > 0)
		{
			Log(0, LogCategory_System, _PNT("User call CNetClient.Disconnect."));
		}

		if (m_disconnectCallTime == 0)
		{
			if (m_worker->GetState() == CNetClientWorker::Connected)
			{
				// 언제 디스 작업을 시작할건가를 지정
				if (m_remoteServer != nullptr)
				{
					// NetClient.Disconnect()이 호출된 경우에는 아래의 값을 True로 지정합니다.
					m_remoteServer->m_ToServerTcp->m_userCalledDisconnectFunction = true;
					m_remoteServer->m_shutdownIssuedTime = GetPreciseCurrentTimeMs(); // 이거에 의해 Networker State가 disconnecting으로 곧 바뀐다.
				}

				// 서버와의 연결 해제를 서버에 먼저 알린다.
				// 바로 소켓을 닫고 클라 프로세스가 바로 종료하면 shutdown 신호가 TCP 서버로 넘어가지 못하는 경우가 있다.
				// 따라서 서버에서 연결 해제를 주도시킨 후 클라에서 종료하되 시간 제한을 두는 형태로 한다.
				// (즉 TCP의 graceful shutdown을 대신한다. )
				m_worker->m_gracefulDisconnectTimeout = args.m_gracefulDisconnectTimeoutMs;

				if (GetVolatileLocalHostID() != HostID_None)
				{
					// 아직 HostID조차 발급받지 못한 상황이면 ShutdownTcp 메시지를 보내지 않습니다.
					m_c2sProxy.ShutdownTcp(HostID_Server, g_ReliableSendForPN, args.m_comment,
						CompactFieldMap());
				}
			}
			else if (m_worker->GetState() < CNetClientWorker::Connected)
			{
				// 서버와 연결이 완료되기 전이었으면 당장 디스모드로 전환
				m_worker->SetState(CNetClientWorker::Disconnecting);
			}

			m_disconnectCallTime = GetPreciseCurrentTimeMs();
		}
	}

	// 사용자가 Disconnect를 호출해서 내부 함수 DisconnectAsync가 호출되었으면, 스레드 등을 정리해 버려야 한다. 그걸 한다.
	void CNetClientImpl::CleanupAfterDisconnectIsCalled()
	{
		int64_t timeOut = PNMAX(m_disconnectArgs.m_gracefulDisconnectTimeoutMs * 2, 100000); // modify by rekfkno1 - 시간을 100초로 늘립니다.
		int64_t lastWarnTime = GetPreciseCurrentTimeMs();
		int64_t startTime = lastWarnTime;

		int32_t waitCount = 0;

		// 완전히 디스될 때까지 루프를 돈다. 디스 이슈는 필요시 하고.
		while (true)
		{
			int64_t currTime = GetPreciseCurrentTimeMs();
			{
				CriticalSectionLock lock(GetCriticalSection(), true); // 이걸로 보호한 후 m_worker등을 모두 체크하도록 하자.

																	  // tcp,udp socket들이 모두 io event가 더 이상 없음이 보장되는 상황이면 리턴.
				if (m_worker->GetState() == CNetClientWorker::Disconnected)
				{
					break;
				}

#if defined(_WIN32)
				// 프로세스 종료중이면 아무것도 하지 않는다.
				if (Thread::m_dllProcessDetached_INTERNAL)
				{
					break;
				}
#endif
				// 원리원칙대로라면 다른 스레드가 this를 액세스하는 것이 더 이상 없다는 것이 보장되면 루프를 나가야 하나,
				// 만약에 버그가 있거나 해서 영원히 보장되는 상황이 확인되지 못하고 있으면,
				// 어쩔 수 없지만 잠재 버그(크래시)가 뒤에 발생하더라도 일단은 루프를 빠져나오게 하는 기능.
				// 가급적이면 ConcealDeadlockOnDisconnect를 사용하지 말 것. 위험하다. 응급 땜빵용이다.
				if (CNetConfig::ConcealDeadlockOnDisconnect && currTime - startTime > timeOut)
				{
#if defined(_WIN32)
					// 오류 상황을 제보한다.
					String txt;
					txt.Format(_PNT("CNetClient.Disconnect seems to be freezed ## State=%d##Process=%s"),
						m_worker->GetState(),
						GetProcessName().GetString());
					CErrorReporter_Indeed::Report(txt);
#endif
					m_worker->SetState(CNetClientWorker::Disconnected);
					break;
				}

				// async io가 모두 끝난게 확인될 떄까지 disconnecting mode 지속.
				// 어차피 async io는 이 스레드 하나에서만 이슈하기 떄문에 이 스레드 issue 없는 상태인 것을 확인하면 더 이상 async io가 진행중일 수 없다.

				// 모든 garbage socket,host가 collect된 상태이고, 시간도 꽤 지났고,
				// 기타 종료 조건(DLL어쩌구 등)을 만족하면
				int64_t currtime = GetPreciseCurrentTimeMs();

				// 모든 리소스가 제거 되었는지 확인하고
				if (CanDeleteNow())
				{
					/* 전부 garbage collected라 함은 어떠한 스레드도 this 및
					하부 객체를 더 이상 건드리지 않음을 의미.
					따라서 여기서 모든 데이터를 청소해도 된다.
					단, Disconnect()가 state is disconnected가 될 때까지 기다리는데서
					UnregisterReferrer도 해주어야 하므로
					UnregisterReferrer는 여기서는 안한다. */

					//assert(m_remotePeers.GetCount()==0);

					// FinalUserWorkItemList_RemoveReceivedMessagesOnly(); 이걸 하지 말자. 서버측에서 RMI직후추방시 클라에서는 수신RMI를 폴링해야하는데 그게 증발해버리니까. 대안을 찾자.
					// 이러면 disconnected 상황이 되어도 미수신 콜백을 유저가 폴링할 수 있다. 이래야 서버측에서 디스 직전 보낸 RMI를 클라가 처리할 수 있다.

					m_worker->SetState(CNetClientWorker::Disconnected);
				}
				else if (!m_worker->m_DisconnectingModeWarned && currtime - m_worker->m_DisconnectingModeStartTime > 5)
				{
#if defined(_WIN32)
					CErrorReporter_Indeed::Report(_PNT("Too long time elapsed since disconnecting mode!"));
#endif
					m_worker->m_DisconnectingModeWarned = true;
				}

				// gracefulDisconnectTimeout 지정 시간이 지난 후 실 종료 과정이 시작된다
				// 실 종료 과정이 완전히 끝날 때까지 대기
				if (waitCount > 0) // 보다 빠른 응답성을 위해
				{
					if (((GetPreciseCurrentTimeMs() - lastWarnTime)) > 5000)
					{
						//OutputDebugString(_PNT("NetClient Disconnect is not yet ended.\n"));
						NTTNTRACE("NetClient Disconnect is not yet ended.\n");

						// test call
						//						CanDeleteNow();

						lastWarnTime = currTime;
					}
				}
			}

			{
				/* Disconnect()는 모든 thread 작업이 다 정리될 때까지 기다리되,
				zero thread로 지정된 user worker thread pool이 다 정리되게 하기 위해서
				여기서 Process를 호출한다.
				그런데 current thread가 delete NetClient 를 함으로 인해 Disconnect()가 호출되었다고 치자.
				이때 사용자가 만든 user callback에서 또 Disconnect()를 호출하는 경우도 있을 수 있다.
				이러한 경우 오동작하지 않으려면 user callback을 하지 말아야 한다.
				물론 이렇게 하면 OnLeaveServer 등을 못 받겠지만 사용자가 Disconnect() or delete NetClient를 한 이상
				이정도 쯤은 사용자도 이미 안다. */
				ZeroThreadPoolUsageMarker zeroThreadPoolUsageMarker(this);

				/* Disconnect 함수에서 Blocking이 되어 UserThreadPool 이벤트를 처리하는 FrameMove 함수를 호출하지 못합니다.
				이로인하여 UserThreadPool으로 Post한 DoUserTask 메세지가 정상적으로 처리가 되지 않는 문제가 발생할 수 있다.
				그래서 여기서 대신 처리해 준다. */
				if (m_connectionParam.m_netWorkerThreadModel == ThreadModel_SingleThreaded)
				{
					m_netThreadPool->ProcessButDropUserCallback(this, 0);
				}

				if (m_connectionParam.m_userWorkerThreadModel == ThreadModel_SingleThreaded)
				{
					m_userThreadPool->ProcessButDropUserCallback(this, 0);
				}
			}

			Proud::Sleep(m_disconnectArgs.m_disconnectSleepIntervalMs);

			waitCount++;
		}

		LockMain_AssertIsNotLockedByCurrentThread();

		CleanThreads();

		// 스레드도 싹 종료. 이제 완전히 클리어하자.
		{
			CriticalSectionLock lock(GetCriticalSection(), true); // 이걸로 보호한 후 m_worker등을 모두 체크하도록 하자.
			CleanExceptForThreads();

			// m_worker를 새로 할당하지 말고, value를 overwrite한다. Selvas case에서, lock-free access case가 의심되었다.
			m_worker->Reset();
		}
#if defined(_WIN32)
		XXXHeapChk();
#endif
	}



	CNetClientImpl::CNetClientImpl()
		:
		m_ReliablePing_Timer(CNetConfig::DefaultNoPingTimeoutTimeMs), // 어차피 이 인터벌은 중간에 바뀜.
		//		m_isProcessingHeartbeat(0),
        m_addedToClientInstanceCount(false),
		m_enablePingTestEndTime(0),
		m_disconnectCallTime(0),
		m_RemoveTooOldUdpSendPacketQueueOnNeed_Timer(CNetConfig::LongIntervalMs),
		m_lastCheckSendQueueTime(0),
		m_sendQueueHeavyStartTime(0),
		m_lastUpdateNetClientStatCloneTime(0),
		m_sendQueueTcpTotalBytes(0),
		m_sendQueueUdpTotalBytes(0)
	{
		m_RefreshServerAddrInfoState = RefreshServerAddrInfoState_NotWorking;
		m_refreshServerAddrInfoWorkerThreadStartTimeMs = -1;

		m_everyRemoteIssueSendOnNeedIntervalMs = CNetConfig::EveryRemoteIssueSendOnNeedIntervalMs;
		m_checkNextTransitionNetworkTimeMs = GetPreciseCurrentTimeMs();

#if defined(_WIN32)
		CKernel32Api::Instance();
#endif
		m_manager = CNetClientManager::GetSharedPtr(); // 이게 CNetClientImpl.Disconnect의 간헐적 무한 비종료를 해결할지도

#if defined(_WIN32)
		EnableLowFragmentationHeap();
#endif

		// 초기 설정
		m_worker = RefCount<CNetClientWorker>(new CNetClientWorker(this));

		m_natDeviceNameDetected = false;
		m_internalVersion = CNetConfig::InternalNetVersion;
		//m_minExtraPing = 0;
		//m_extraPingVariance = 0;
		m_eventSink_NOCSLOCK = nullptr;
		m_lastFrameMoveInvokedTime = 0;
		m_disconnectInvokeCount = m_connectCount = 0;
		m_toServerUdpSendCount = 0;
		
		Reset_ReportUdpCountTime_timeToDo();

		m_selfP2PSessionKey = shared_ptr<CSessionKey>(new CSessionKey);
		m_toServerSessionKey = shared_ptr<CSessionKey>(new CSessionKey);
		m_acrCandidateSessionKey = shared_ptr<CSessionKey>(new CSessionKey);

		m_c2cProxy.m_internalUse = true;
		m_c2sProxy.m_internalUse = true;
		m_s2cStub.m_internalUse = true;
		m_c2cStub.m_internalUse = true;

		AttachProxy(&m_c2cProxy);
		AttachProxy(&m_c2sProxy);
		AttachStub(&m_s2cStub);
		AttachStub(&m_c2cStub);
		

		m_c2cStub.m_owner = this;
		m_s2cStub.m_owner = this;

		// 접속하는 서버의 ip와 port 등을 확인하기 위해 서버접속 시점부터 로그를 남기기 위함이였습니다.
		m_enableLog = false;
		m_virtualSpeedHackMultiplication = 1;

		CFastHeapSettings fss;
		fss.m_pHeap = nullptr;
		fss.m_accessMode = FastHeapAccessMode_UnsafeSingleThread;
		fss.m_debugSafetyCheckCritSec = &GetCriticalSection();

		m_selfP2PSessionKey = shared_ptr<CSessionKey>(new CSessionKey);
		m_toServerSessionKey = shared_ptr<CSessionKey>(new CSessionKey);;

		//m_preFinalRecvQueue.UseMemoryPool();CFastList2 자체가 node pool 기능이 有
		//m_finalUserWorkItemList.UseMemoryPool();CFastList2 자체가 node pool 기능이 有
		//m_postponedFinalUserWorkItemList.UseMemoryPool();CFastList2 자체가 node pool 기능이 有
		//m_loopbackFinalReceivedMessageQueue.UseMemoryPool();CFastList2 자체가 node pool 기능이 有

		m_ioPendingCount = 0;

		m_simplePacketMode = false;
		m_destructorIsRunning = false;

		m_lastHeartbeatTime = GetPreciseCurrentTimeMs();
		m_recentElapsedTime = 0;

		m_ReliableUdpHeartbeatInterval_USE = CNetConfig::ReliableUdpHeartbeatIntervalMs_Real;
		m_StreamToSenderWindowCoalesceInterval_USE = ReliableUdpConfig::StreamToSenderWindowCoalesceIntervalMs_Real;

		m_loopbackHost = shared_ptr<CLoopbackHost_C>();
		m_remoteServer = shared_ptr<CRemoteServer_C>();

		m_TcpAndUdp_DoForShortInterval_lastTime = 0;

		m_Heartbeat_DetectNatDeviceName_timeToDo = 0;

		m_zeroThreadPool_processingThreadID = 0;
		m_zeroThreadPool_processingThreadRecursionCount = 0;

		m_perPeerUdpSocketCountLimit = INT_MAX;

		m_HolsterMoreCallbackUntilNextProcessCall_flagged = false;
	}

	CNetClientImpl::~CNetClientImpl()
	{
		m_destructorIsRunning = true;
		LockMain_AssertIsNotLockedByCurrentThread();

		// Disconnect()는 throwable이다. 그러나 여기는 dtor다. 따라서 throw하지 말고 딱 정리해 주어야.
		try
		{
			Disconnect();
		}
		catch (std::exception& e)
		{
			CFakeWin32::OutputDebugStringA(e.what());
		}

		// PN 내부에서도 쓰는 RMI까지 더 이상 참조되지 않음을 확인해야 하므로 여기서 시행해야 한다.
		CleanupEveryProxyAndStub(); // 꼭 이걸 호출해서 미리 청소해놔야 한다.

		//m_hlaSessionHostImpl.Free();

#ifdef VIZAGENT
		m_vizAgent.Free();
#endif
	}


	void CNetClientImpl::OnConnectSuccess(const shared_ptr<CSuperSocket>& socket)
	{
		CriticalSectionLock mainLock(GetCriticalSection(), true);

		if (socket == m_remoteServer->m_ToServerTcp)
		{
			// 기존 TCP 연결 소켓
			ProcessFirstToServerTcpConnectOk();
		}
		else
		{
			// ACR 연결이 들어온 듯. 처리하자.
			if (AutoConnectionRecovery_OnTcpConnection(socket) == false)
			{
				ProcessAcrCandidateFailure();
			}
		}
	}


	void CNetClientImpl::OnConnectFail(const shared_ptr<CSuperSocket>& socket, SocketErrorCode code)
	{
		CriticalSectionLock mainLock(GetCriticalSection(), true);

		if (m_remoteServer->m_ToServerTcp == socket)
		{
			// to-server TCP의 실패인 경우
			// 통상 기존 연결실패 과정(go to disconnecting mode)
			String comment;
			String tt = g_TEST_CloseSocketOnly_CallStack; // 중간에 다른 스레드에 의해 사라지는 것을 막기 위해
			comment.Format(_PNT("Diag: OCF with (code=%d, callStack=%s, serverAddrPort=%s, IC=%d)"), code, tt.c_str(), m_serverAddrPort.ToString().c_str(), socket->m_isConnectingSocket);
//#ifndef _WIN32
//			// 실험 코드.
//			if (code == ENOTCONN)
//			{
//				int64_t t0 = GetPreciseCurrentTimeMs();
//				SocketErrorCode code2;
//				while (true)
//				{
//					code2 = socket->m_fastSocket->Tcp_Send0ByteForConnectivityTest();
//					if ((int)code != ENOTCONN && !CFastSocket::IsWouldBlockError(code2))
//						break;
//					if (GetPreciseCurrentTimeMs() - t0 > 5000)
//						break;
//					Sleep(10);
//				}
//
//				// code2가 그래도 ENOTCONN인가? 아무튼 출력 해보자.
//				String comment2;
//				comment2.Format(_PNT(" and code2=%d, waited for %dms"), code2, GetPreciseCurrentTimeMs() - t0);
//				comment += comment2;
//			}
//#endif
			Heartbeat_ConnectFailCase(code, comment);
		}
		else
		{
			// ACR TCP 연결이 실패한 경우
			// 재시도를 또 한다. 명세서 참고 바람.
			ProcessAcrCandidateFailure();
		}
	}

	// to-server 본 TCP 연결이 끊어졌거나, TCP 상태가 살아있지만 랙이 심각하다. 즉 불량.
	// TCP 재접속을 시작하던지, disconnecting mode가 되던지 하자.
	void CNetClientImpl::DisconnectOrStartAutoConnectionRecovery(const ErrorInfo& errorInfo)
	{
		if (m_enableAutoConnectionRecovery && (m_remoteServer->m_shutdownIssuedTime == 0))
		{
			// NOTE: 기존 to-server TCP는 닫지 않는다. ACR 재접속 후보가 성공이 되면 그때야 비로소 닫도록 한다.
			// ACR 과정을 시작한다.
			StartAutoConnectionRecovery(errorInfo); // 이 함수가 끝나고 나면 ACR context가 생성되어 있거나 실패했다면 없을 것임
		}

		if (m_autoConnectionRecoveryContext == nullptr)
		{
			// 위 과정 처리를 했음에도 불구하고 ACR is null이다.
			// 즉 ACR 비활성 클라이거나 ACR context 생성이 실패했다.
			// => 그냥 연결 해제 처리.
			// NOTE: deleting remote server will be done later in Disconnecting mode.
			EnqueueDisconnectionEvent(errorInfo.m_errorType, errorInfo.m_detailType, errorInfo.m_comment);
			m_worker->SetState(CNetClientWorker::Disconnecting);
		}
	}

	// TCP 연결이 끊어졌을 때 처리
	void CNetClientImpl::ProcessDisconnecting(const shared_ptr<CSuperSocket>& socket, const ErrorInfo& errorInfo)
	{
		CriticalSectionLock mainLock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		if (socket == m_remoteServer->m_ToServerTcp)
		{
			// 연결유지기능을 활성화 시키던지 disconnecting mode로 바뀌던지 해야.
			DisconnectOrStartAutoConnectionRecovery(errorInfo);
		}
		else
		{
			// 재접속 연결이 중도 실패한 경우, 재시도를 해야 한다.
			// 명세서 참고 바람.
			ProcessAcrCandidateFailure();
		}
	}

	void CNetClientImpl::InduceDisconnect()
	{
		if (m_remoteServer->m_ToServerTcp != nullptr)
		{
			m_remoteServer->m_ToServerTcp->RequestStopIo();
			if (m_enableLog || m_settings.m_emergencyLogLineCount > 0)
				Log(0, LogCategory_Udp, _PNT("InduceDisconnect, CloseSocketOnly called."));
		}
	}

	void CNetClientImpl::EnqueueDisconnectionEvent(const ErrorType errorType,
		const ErrorType detailType, const String &comment)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (!m_supressSubsequentDisconnectionEvents)
		{
			// 아직 서버로부터 hostid를 받지도 않았는데 연결 해제되는 경우에는, OnClientLeave가 아니라 OnJoinServerComplete(fail)이 떠야 한다.
			if (GetVolatileLocalHostID() == HostID_None)
			{
				EnqueueConnectFailEvent(errorType, comment, SocketErrorCode_Ok);
			}
			else
			{
				LocalEvent e;
				e.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
				e.m_type = LocalEventType_ClientServerDisconnect;
				e.m_errorInfo->m_errorType = errorType;
				e.m_errorInfo->m_detailType = detailType;
				e.m_errorInfo->m_comment = comment;
				e.m_remoteHostID = HostID_Server;
				EnqueLocalEvent(e, m_remoteServer);

				m_supressSubsequentDisconnectionEvents = true;
			}

#ifdef VIZAGENT
			// VIZ
			if (m_vizAgent)
			{
				CriticalSectionLock vizlock(m_vizAgent->m_cs, true);
				CServerConnectionState ignore;
				m_vizAgent->m_c2sProxy.NotifyCli_ConnectionState(HostID_Server, g_ReliableSendForPN, GetServerConnectionState(ignore));
			}
#endif
		}
	}

	// 클라가 서버에 접속 과정을 시행 중에 실패하는 이벤트를 쌓는다.
	void CNetClientImpl::EnqueueConnectFailEvent(ErrorType errorType, ErrorInfoPtr errorInfo)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (m_supressSubsequentDisconnectionEvents == false)
		{
			LocalEvent e;
			e.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
			e.m_type = LocalEventType_ConnectServerFail;
			e.m_errorInfo->m_errorType = errorType;
			e.m_errorInfo->m_comment = errorInfo->m_comment;
			e.m_remoteHostID = HostID_Server;

			SocketErrorCode errorCode = SocketErrorCode_Error;
			if (AddrPort::FromHostNamePort(&e.m_remoteAddr, errorCode, m_connectionParam.m_serverIP, m_connectionParam.m_serverPort) == false)
			{
				this->EnqueError(ErrorInfo::From(ErrorType_Unexpected, HostID_None, String::NewFormat(_PNT("Before OnJoinServerComplete with fail, we got DNS lookup failure. Error=%d"), (int)errorCode)));
			}

			e.m_socketErrorCode = SocketErrorCode_Ok;
			EnqueLocalEvent(e, m_remoteServer);

			m_supressSubsequentDisconnectionEvents = true;
		}
	}

	// 클라가 서버와 연결하는 도중 에러가 남을 이벤트 큐
	void CNetClientImpl::EnqueueConnectFailEvent(ErrorType errorType,
		const String& comment,
		SocketErrorCode socketErrorCode,
		ByteArrayPtr reply)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		if (m_supressSubsequentDisconnectionEvents == false)
		{
			LocalEvent e;
			e.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
			e.m_type = LocalEventType_ConnectServerFail;
			e.m_errorInfo->m_errorType = errorType;
			e.m_errorInfo->m_comment = comment;
			e.m_remoteHostID = HostID_Server;

			SocketErrorCode errorCode = SocketErrorCode_Error;
			if (AddrPort::FromHostNamePort(&e.m_remoteAddr, errorCode, m_connectionParam.m_serverIP, m_connectionParam.m_serverPort) == false)
			{
				this->EnqueError(ErrorInfo::From(ErrorType_Unexpected, HostID_None, String::NewFormat(_PNT("Before OnJoinServerComplete with fail, we got DNS lookup failure. Error=%d"), (int)errorCode)));
			}

			e.m_socketErrorCode = socketErrorCode;
			e.m_userData = reply;
			EnqueLocalEvent(e, m_remoteServer);

			m_supressSubsequentDisconnectionEvents = true;
		}
	}

	Proud::ConnectionState CNetClientImpl::GetServerConnectionState(CServerConnectionState &output)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		if (m_remoteServer != nullptr)
		{
			output.m_realUdpEnabled = m_remoteServer->IsRealUdpEnable();
		}

			switch (m_worker->GetState())
			{
			case CNetClientWorker::Disconnected:
				return ConnectionState_Disconnected;
			case CNetClientWorker::IssueConnect:
				return ConnectionState_Connecting;
			case CNetClientWorker::Connecting:
				return ConnectionState_Connecting;
			case CNetClientWorker::JustConnected:
				return ConnectionState_Connecting;
			case CNetClientWorker::Connected:
				return ConnectionState_Connected;
			case CNetClientWorker::Disconnecting:
				return ConnectionState_Disconnecting;
			}

		return ConnectionState_Disconnected;

		// 		if(GetVolatileLocalHostID() != HostID_None)
		// 		{
		// 			output.m_realUdpEnabled = m_ToServerUdp_fallbackable != nullptr ? m_ToServerUdp_fallbackable->IsRealUdpEnabled() : false;
		// 			return ConnectionState_Connected;
		// 		}
		// 		else if (m_ToServerTcp != nullptr && Get_ToServerTcp()->m_socket != nullptr && Get_ToServerTcp()->m_socket->IsSocketEmpty() == false)
		// 		{
		// 			return ConnectionState_Connecting;
		// 		}
		// 		else
		// 		{
		// 			return ConnectionState_Disconnected;
		// 		}
	}


	// 스레드만 청소한다. CleanExceptForThreads와는 안 겹친다.
	void CNetClientImpl::CleanThreads()
	{
		// TimeThreadPool에 등록되었던것들을 지운다.
		m_periodicPoster_GarbageCollectInterval.Free();
		m_periodicPoster_Tick.Free();
//		m_periodicPoster_Heartbeat.Free();
//		m_periodicPoster_SendEnqueued.Free();

		// thread pool로부터 독립한다.
		if (m_userThreadPool != nullptr)
		{
			m_userThreadPool->UnregisterReferrer(this);
			if (m_connectionParam.m_userWorkerThreadModel == ThreadModel_UseExternalThreadPool)
			{
				// 외장 스레드 풀
				m_userThreadPool = nullptr;
			}
			else
			{
				// ikpil.choi : 2016-11-03 safe series
				// 내장 스레드 풀
				//SAFE_DELETE(m_userThreadPool);
				SafeDelete(m_userThreadPool);
			}
		}

		if (m_netThreadPool != nullptr)
		{
			m_netThreadPool->UnregisterReferrer(this);
			// NetClientManager가 가지거나 외장 스레드풀이므로 안 delete.
			m_netThreadPool = nullptr;
		}
	}


	// 스레드를 제외한 나머지를 청소한다.
	void CNetClientImpl::CleanExceptForThreads()
	{
		LockMain_AssertIsLockedByCurrentThread();

		// NOTE: 본 함수가 호출되었을 시점에는 각 Hosts들과 Socket들이 모두 정리가 되어야만 합니다.
		if (!CanDeleteNow())
		{
			throw Exception("Cannot delete now!");
		}

		if (m_remoteServer)
		{
			m_remoteServer->CleanupToServerUdpFallbackable();

			// 여기까지 왔으면 socket들은 이미 다 사라진 상태이어야 한다.
			// 그런데 아직까지 garbage에 뭔가가 들어있으면 잠재적 크래시 위험.
			if (m_remoteServer->m_ToServerTcp != nullptr)
			{
				m_remoteServer->m_ToServerTcp->RequestStopIo();
				if (m_enableLog || m_settings.m_emergencyLogLineCount > 0)
					Log(0, LogCategory_Udp, _PNT("CleanupEvenUnstableSituation, CloseSocketOnly called."));
			}
		}

		//추가로 클린업해야 할것들을 한다.
		m_selfP2PSessionKey->Clear();
		m_selfEncryptCount = 0;
		m_selfDecryptCount = 0;

		m_supressSubsequentDisconnectionEvents = false;
		m_RequestServerTimeCount = 0;
		m_serverTimeDiff = 0;
		m_serverUdpRecentPingMs = 0;
		m_serverUdpLastPingMs = 0;
		m_serverTcpRecentPingMs = 0;
		m_serverTcpLastPingMs = 0;
		m_firstTcpPingedTime = 0;
		m_lastRequestServerTimeTime = 0;
		m_P2PGroups.Clear(); // 확인사살.
		m_P2PConnectionTrialEndTime = CNetConfig::GetP2PHolepunchEndTimeMs();
		m_P2PHolepunchInterval = CNetConfig::P2PHolepunchIntervalMs;
		m_natDeviceNameDetected = false;
		m_stats = CNetClientStats();
		m_internalVersion = CNetConfig::InternalNetVersion;
		m_settings = CNetSettings();
		m_serverInstanceGuid = Guid();
		m_toServerEncryptCount = 0;
		m_toServerDecryptCount = 0;
		m_toServerSessionKey->Clear();
		//m_toServerUdpSocketFailed = false;
		//m_minExtraPing = 0;
		//m_extraPingVariance = 0;
		if (m_loopbackHost)
			m_loopbackHost->m_HostID = HostID_None;
		m_connectionParam = CNetConnectionParam();

		//FinalUserWorkItemList_RemoveReceivedMessagesOnly(); // 몽땅 지우면 안됨. CNetClient.Disconnect 콜 후에도 CNetClient.FrameMove 콜 시 디스에 대한 콜백이 있어야 하니까.

		// 		if(m_timer != nullptr)
		// 		{
		// 			m_timer->Stop();
		// 			m_timer.Free();
		// 		}

		m_lastFrameMoveInvokedTime = 0;
		//m_remotePeers.Clear();
		ClearGarbagedHostMap();
		m_RemoveTooOldUdpSendPacketQueueOnNeed_Timer = CTimeAlarm(CNetConfig::LongIntervalMs);
		//		m_processSendReadyRemotes_Timer = CTimeAlarm(CNetConfig::EveryRemoteIssueSendOnNeedIntervalMs);
		m_virtualSpeedHackMultiplication = 1;
		m_speedHackDetectorPingTime = CNetConfig::EnableSpeedHackDetectorByDefault ? 0 : INT64_MAX;

		m_testStats2 = CTestStats2();
		m_applicationHint.m_recentFrameRate = 0;
		m_lastCheckSendQueueTime = 0;
		m_lastUpdateNetClientStatCloneTime = 0;
		m_sendQueueHeavyStartTime = 0;

		m_TcpAndUdp_DoForShortInterval_lastTime = 0;

		m_unusedUdpPorts.Clear();
		m_usedUdpPorts.Clear();

		// 디버거로 Disconnect 끝자락에서 미정리된 변수들이 발견되어, 여기서 추가로 그것들을 청소.
		m_ReliablePing_Timer.Reset(GetPreciseCurrentTimeMs());
		m_toServerUdpSendCount = 0;
		
		Reset_ReportUdpCountTime_timeToDo();

		m_ioPendingCount = 0;
		m_enablePingTestEndTime = 0;

		m_disconnectCallTime = 0;

		// NetClient의 경우 Connect ~ Disconnect 범위에서만 RemoteServer 사용
		m_remoteServer = shared_ptr<CRemoteServer_C>();

		// NetClient의 경우 Connect ~ Disconnect 범위에서만 LoopbackHost 사용
		m_loopbackHost = shared_ptr<CLoopbackHost_C>();

		DeleteSendReadyList();
	}

#ifdef TEST_DISCONNECT_CLEANUP
	bool CNetClientImpl::IsAllCleanup()
	{
		if (m_ToServerUdp_fallbackable != nullptr)
			return false;

		if (m_ToServerTcp != nullptr)
			return false;

		if (m_selfP2PSessionKey != nullptr)
			return false;

		if (m_selfDecryptCount != 0 || m_selfEncryptCount != 0)
			return false;

		if (m_supressSubsequentDisconnectionEvents)
			return false;

		if (m_RequestServerTimeCount != 0)
			return false;
		if (m_serverTimeDiff != 0)
			return false;
		if (m_serverUdpRecentPingMs != 0)
			return false;
		if (m_serverUdpLastPing != 0)
			return false;

		if (m_lastRequestServerTimeTime != 0)
			return false;

		if (m_P2PGroups.GetCount() != 0)
			return false;
		if (m_P2PConnectionTrialEndTime != CNetConfig::GetP2PHolepunchEndTimeMs())
			return false;

		if (m_P2PHolepunchInterval != CNetConfig::P2PHolepunchIntervalMs)
			return false;

		if (m_natDeviceNameDetected)
			return false;

		if (m_internalVersion != CNetConfig::InternalVersion)
			return false;

		if (m_serverInstanceGuid != Guid())
			return false;

		if (m_preFinalRecvQueue.GetCount() != 0)
			return false;

		if ((m_ToServerEncryptCount | m_ToServerDecryptCount) != 0)
			return false;

		if (m_toServerUdpSocket != nullptr)
			return false;

		if (m_toServerUdpSocketFailed)
			return false;

		if (!m_loopbackFinalReceivedMessageQueue.IsEmpty())
			return false;

		//if(m_minExtraPing != 0)
		//	return false;

		if (m_extraPingVariance != 0)
			return false;

		if (GetVolatileLocalHostID() != HostID_None)
			return false;

		if (!m_postponedFinalUserWorkItemList.IsEmpty())
			return false;

		if (m_lastFrameMoveInvokedTime != 0)
			return false;

		if (!m_remotePeers.IsEmpty())
			return false;
		if (!m_garbagedSockets.IsEmpty())
			return false;
		if (m_virtualSpeedHackMultiplication != 1)
			return false;

		// 		if(m_ToServerTcp->m_lastReceivedTime != 0)
		// 			return false;

		if (m_lastTcpStreamReceivedTime != 0)
			return false;

		return true;
	}
#endif

}
