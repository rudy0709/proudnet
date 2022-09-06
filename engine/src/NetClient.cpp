/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"

// STL header file은 xcode에서는 cpp에서만 include 가능하므로
#include <new>
#include <stack>
#include <stdexcept>
#include <numeric>
#include <math.h>		// pow(), sqrt()
#ifdef _WIN32
#include <time.h>
#endif // _WIN32

#include "../include/INetClientEvent.h"
#include "../include/Marshaler.h"
#include "../include/NetConfig.h"
#include "../include/NetPeerInfo.h"
#include "../include/FakeClrBase.h"
#include "../include/sysutil.h"
#include "../include/atomic.h"

#include "NetClient.h"

#include "ThreadPoolImpl.h"
#include "Relayer.h"
#include "ReliableUDPFrame.h"
#include "STLUtil.h"
#include "CollUtil.h"
#include "SocketUtil.h"
#include "RmiContextImpl.h"
#include "LoopbackHost.h"

#include "EmergencyLogClient.h"
#ifdef VIZAGENT
#include "VizAgent.h"
#endif
//#include "Upnp.h"
#include "ReusableLocalVar.h"
#include <typeinfo>

#include "NetS2C_stub.cpp"
#include "NetC2C_stub.cpp"
#include "NetC2S_proxy.cpp"
#include "NetC2C_proxy.cpp"
#include "LeanDynamicCast.h"
#include "SendFragRefs.h"
#include "../include/ClassBehavior.h"
#include "ReceivedMessageList.h"
#include "../include/numutil.h"

namespace Proud
{
	void AssureIPAddressIsUnicastEndpoint(const AddrPort &udpLocalAddr)
	{
		if (udpLocalAddr.IsUnicastEndpoint() == false)
		{
#if defined(_WIN32)
			String text;
			text.Format(_PNT("FATAL: It should already have been TCP-Connected prior to the Creation of the UDP Socket but localAddr of TCP Appeared to be %s!"), udpLocalAddr.ToString().GetString());
			CErrorReporter_Indeed::Report(text);
#endif
			assert(0);
		}

		/*Q: throw 하지 않고 assert를 하는 이유는?
		A: 조건 불충분이라고 해봤자 홀펀칭이 안될 뿐입니다. 그런데 여기서 throw를 하면 클라는 뻗어버립니다.
		NetClient가 UDP socket의 bind address가 TCP의 address이어야 하는데 그거 대신 any를 넣는 실수를 막기 위해서 이것이 존재합니다.
		*/
	}

	//////////////////////////////////////////////////////////////////////////
	const PNTCHAR* NoServerConnectionErrorText = _PNT("Cannot send messages unless connection to server exists!");

	DEFRMI_ProudS2C_ReliablePong(CNetClientImpl::S2CStub)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		if (m_owner->m_enableAutoConnectionRecovery == true && messageID != 0)
		{
			m_owner->m_remoteServer->m_ToServerTcp->AcrMessageRecovery_RemoveBeforeAckedMessageID(messageID);
		}

		// 보낼때 기재했던 ping값과의 시간값 차이를 통해 RTT를 구한다.
		int clientTime = (int)GetPreciseCurrentTimeMs();
		int newLag = (clientTime - localTimeMs) / 2;

		m_owner->ServerTcpPing_UpdateValues(newLag);

		return true;
	}

	DEFRMI_ProudS2C_EnableLog(CNetClientImpl::S2CStub)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		m_owner->m_enableLog = true;

		return true;
	}

	DEFRMI_ProudS2C_DisableLog(CNetClientImpl::S2CStub)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		m_owner->m_enableLog = false;

		return true;
	}

	// 서버로부터 TCP fallback의 필요함을 노티받을 때의 처리
	DEFRMI_ProudS2C_NotifyUdpToTcpFallbackByServer(CNetClientImpl::S2CStub)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
		FallbackParam param;
		param.m_notifyToServer = false;
		param.m_reason = ErrorType_ServerUdpFailed;
		m_owner->FirstChanceFallbackServerUdpToTcp(param);

		return true;
	}

	void CNetClientImpl::ProcessRoundTripLatencyPong(const HostID& remote, int32_t pingTime)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		shared_ptr<CHostBase> rc;
		if (m_authedHostMap.TryGetValue(remote, rc))
		{
			int32_t pongTime = (int32_t)GetPreciseCurrentTimeMs();
			int32_t latency = pongTime - pingTime;
			rc->m_rttTotalTimeMs += latency;
			rc->m_rttTestCount++;

			// 최근 CNetConfig::RountTripLatencyTestMaxCount개만큼에 대해서만 표준편차를 구하기 위해서 담아둡니다.
			rc->m_latencies.push_back(latency);

			// CNetConfig::RountTripLatencyTestMaxCount개가 넘어가면 괜히 메모리와 연산량만 먹으므로 오래된건 폐기.
			if (rc->m_latencies.size() > CNetConfig::RountTripLatencyTestMaxCount)
			{
				rc->m_rttTotalTimeMs -= rc->m_latencies.front();
				rc->m_latencies.pop_front();
			}
		}
	}

	DEFRMI_ProudS2C_RoundTripLatencyPong(CNetClientImpl::S2CStub)
	{
		_pn_unused(rmiContext);

		m_owner->ProcessRoundTripLatencyPong(remote, pingTime);

		return true;
	}

	DEFRMI_ProudC2C_RoundTripLatencyPong(CNetClientImpl::C2CStub)
	{
		_pn_unused(rmiContext);

		m_owner->ProcessRoundTripLatencyPong(remote, pingTime);

		return true;
	}

	DEFRMI_ProudC2C_RoundTripLatencyPing(CNetClientImpl::C2CStub)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		// 서버로부터 받은 것을 그대로 보냅니다
		m_owner->m_c2cProxy.RoundTripLatencyPong(remote, g_UnreliableSendForPN, pingTime);

		return true;
	}

	// cache된 로컬 주소들.
	// ACR 기능들은 로컬 주소의 변화를 감지해야 하므로 이것이 사용된다.
	static CFastArray<String> g_cachedLocalIpAddresses_INTERNAL;
	static CriticalSection g_cachedLocalIpAddresses_critSec;
	volatile static int64_t g_nextCacheLocalIpAddressTimeMs = 0;

	// 로컬 주소를 얻어서 캐시에 저장한다.
	// NOTE: 모바일 기기에서는 주소 핸드오버에 의해 로컬 주소가 바뀔 수 있으니, 그걸 감지하기 위함.
	void CNetClientImpl::CacheLocalIpAddress_MustGuaranteeOneThreadCalledByCaller()
	{
		int64_t currTimeMs = GetPreciseCurrentTimeMs();

		// 1초에 한번씩만 검사를 한다.
		if (currTimeMs - g_nextCacheLocalIpAddressTimeMs >= 0)
		{
			CFastArray<String> addresses;
			CNetUtil::GetLocalIPAddresses(addresses);

			CriticalSectionLock lock(g_cachedLocalIpAddresses_critSec, true);
			g_cachedLocalIpAddresses_INTERNAL = addresses;

			// 주의: 과거 시간에 1000 더하지 말 것! 앱 행 걸림!
			g_nextCacheLocalIpAddressTimeMs = currTimeMs + 1000;
		}
	}

	// cache된 로컬 주소들을 얻는다.
	void CNetClientImpl::GetCachedLocalIpAddressesSnapshot(CFastArray<String>& outAddresses)
	{
		CriticalSectionLock lock(g_cachedLocalIpAddresses_critSec, true);
		outAddresses = g_cachedLocalIpAddresses_INTERNAL;
	}

	void CNetClientImpl::RefreshServerAddrInfo_WorkerProcedure(void* ctx)
	{
		CHeldPtr<NetClient_LegacyThreadContext> context( (NetClient_LegacyThreadContext*)ctx);

		if (context->m_referrerHeart != nullptr)
		{
			// 위 if에 의해 referrerHeart를 쥐고 있다. 따라서 아래 구문이 실행되는 동안 this가 dangle되지 않는다.

			String errorText;
			if (SocketErrorCode_Ok != context->m_netClient->RefreshServerAddrInfo(errorText))
			{
				CriticalSectionLock lock(context->m_netClient->GetCriticalSection(), true);

				// 이 프로시져는 멀티 스레드로 돌아가도록 수정이 되었으므로 실패 했을 경우에는 이미 성공 처리가 무시 될 수 있기 때문에
				// 아래처럼 Working 상태일때만 State를 Failed로 바꾼다.
				if (context->m_netClient->m_RefreshServerAddrInfoState == RefreshServerAddrInfoState_Working)
				{
					context->m_netClient->m_RefreshServerAddrInfoState = RefreshServerAddrInfoState_Failed;
				}
			}
			else
			{
				CriticalSectionLock lock(context->m_netClient->GetCriticalSection(), true);
				context->m_netClient->m_RefreshServerAddrInfoState = RefreshServerAddrInfoState_Finished;
			}
		}
		else
		{
			// 꽝! 다음 기회에
			// referrerHeart 가 nullptr 이라면 netclient 객체도 정상적이라고 장담못함
			// 따라서 이 Worker Thread를 만들기전에 referrer 를 한번 더 체크하여 상태를 바꾸도록 해야함
			// 해당 else문은 아무것도 안한다. 다만 context의 특징을 설명하기 위해 남겨둠
		}

		// note: context는 동적으로 생성한 메모리이므로 여기서 자동 삭제 한다.
	}

	bool CNetClientImpl::IsCalledByWorkerThread()
	{
		if (m_netThreadPool == nullptr)
		{
			assert(0);
		}

		// netWorkerThreadModel이 ThreadModel_SingleThreaded이면 ThreadPool에 thread가 만들어지지 않습니다.
		if (m_connectionParam.m_netWorkerThreadModel == ThreadModel_SingleThreaded)
		{
			return true;
		}

		return m_netThreadPool->ContainsCurrentThread();
	}

	void CNetClientImpl::SetInitialTcpSocketParameters()
	{
		// TCP only & relay only
		m_remoteServer->ResetUdpEnable();
	}

	INetCoreEvent * CNetClientImpl::GetEventSink_NOCSLOCK()
	{
		return m_eventSink_NOCSLOCK;
	}

	DEFRMI_ProudS2C_RequestAutoPrune(CNetClientImpl::S2CStub)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock lock(m_owner->GetCriticalSection(), true);

		// 서버와의 연결을 당장 끊는다. Shutdown-shake 과정을 할 필요가 없다. 클라는 디스가 불특정 시간에 일어나는 셈이므로.
		if (m_owner->m_worker->GetState() <= CNetClientWorker::Connected)
		{
			if (m_owner->m_remoteServer != nullptr)
			{
				// NetServer.CloseConnection()이 호출된 경우에는 아래의 변수를 True로 설정합니다.
				m_owner->m_remoteServer->m_ToServerTcp->m_userCalledDisconnectFunction = true;
			}

			String comment = _PNT("VIA-AP");
			m_owner->EnqueueDisconnectionEvent(ErrorType_DisconnectFromRemote,
				ErrorType_TCPConnectFailure, comment);
			m_owner->m_worker->SetState(CNetClientWorker::Disconnecting);
		}

		return true;
	}

	DEFRMI_ProudS2C_ShutdownTcpAck(CNetClientImpl::S2CStub)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);
		_pn_unused(fieldMap);

		CriticalSectionLock lock(m_owner->GetCriticalSection(), true);

		// shutdown ack를 받으면 바로 종료 처리를 진행하도록 한다.
		/* 종료시간 시프트 문제로 인한 삭제
		if(m_owner->m_worker && m_owner->Get_ToServerTcp()->GetShutdownIssueTime() > 0 && m_owner->m_worker->m_gracefulDisconnectTimeout)
		{
		//gracefultime에 따라서 처리한다.바로 처리하면, gracefultime가 의미 없음.
		m_owner->Get_ToServerTcp()->SetShutdownIssueTime(GetPreciseCurrentTimeMs());// - m_owner->m_worker->m_gracefulDisconnectTimeout * 2;
		} */

		/* 예전에는 서버로 ShutdownTcpHandshake RMI를 보내고, 서버에서 TCP close를 했다.
		그러나 이 방식은 서버에서 한시적 TIME_WAIT가 계속 쌓이는 버그가 있다.
		자세한 설명은 'client-server 연결 해제.pptx'에.
		*/
		m_owner->EnqueueDisconnectionEvent(ErrorType_DisconnectFromLocal, ErrorType_TCPConnectFailure,
			_PNT("User called Disconnect()."));
		m_owner->m_worker->SetState(CNetClientWorker::Disconnecting);

		return true;
	}

	DEFRMI_ProudS2C_RequestMeasureSendSpeed(CNetClientImpl::S2CStub)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);
		_pn_unused(enable);

		//CriticalSectionLock lock(m_owner->GetCriticalSection(),true);

		//m_owner->m_manager->RequestMeasureSendSpeed(enable);

		return true;
	}

	/* remote에 대한 session key를 리턴한다.

	이쪽,저쪽이 모두 non webgl이면, 클라간 end to end session key를 리턴한다.

	한쪽이라도 webgl이면, 얘기가 달라진다. [1]
	C1에서 Server로 relay1을 보낼 때 C1<->S session key로 암호화해서 보낸다.
	S는 C2에게 relay2를 보낼 때 S->C2 session key로 암호화해서 보낸다.
	이 함수는 [1] 상황에서는 C1 or C2 <-> S session key 즉 C/S session key를 리턴한다.
	*/
	bool CNetClientImpl::TryGetCryptSessionKey(HostID remote, shared_ptr<CSessionKey>& output, String &errorOut, LogLevel& outLogLevel)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		output = shared_ptr<CSessionKey>();

		if (GetVolatileLocalHostID() == remote)
		{
			// loopback
			output = m_selfP2PSessionKey;
		}
		else if (remote == HostID_Server)
		{
			// server
			output = m_toServerSessionKey;
		}
		else
		{
			// P2P peer
			shared_ptr<CHostBase> rp0 = GetPeerByHostID_NOLOCK(remote);

			if (rp0.get() != nullptr)
			{
				CRemotePeer_C* rp = (CRemotePeer_C*)rp0.get();

				// 상대피어의 플랫폼이 WebGL이면 서버를 통해 Relay될것이다. C2S키로 암/복호화한다.
				if (rp->m_runtimePlatform == RuntimePlatform_UWebGL)
					output = m_toServerSessionKey;
				else
					output = rp->m_p2pSessionKey;
			}
		}

		if (output == nullptr)
		{
			// warning enqueue
			Tstringstream ss;
			ss << _PNT("Peer ") << remote << _PNT(" does not exist!");
			errorOut = ss.str().c_str();

			outLogLevel = LogLevel_InvalidHostID;

			return false;
		}

		if (!output->EveryKeyExists())
		{
			// error 처리를 해주어야 한다.
			errorOut = _PNT("Key does not exist. Note that P2P encryption can be enabled on NetServer.Start().");
			outLogLevel = LogLevel_Error;

			return false;
		}

		// 성공
		return true;
	}

	void CNetClientImpl::SendServerHolepunch()
	{
		CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
		Message_Write(sendMsg, MessageType_ServerHolepunch);
		sendMsg.Write(m_remoteServer->GetServerUdpHolepunchMagicNumber());

		// C/S 홀펀징에서 RTT 처리
		sendMsg.Write((int)GetPreciseCurrentTimeMs());
		sendMsg.Write(m_serverUdpLastPingMs);

		if (m_enableLog || m_settings.m_emergencyLogLineCount > 0)
		{
			String a = m_remoteServer->GetToServerUdpFallbackable()->m_serverAddr.ToString();
			Log(0, LogCategory_P2P, String::NewFormat(_PNT("Sending ServerHolepunch: %s"), a.GetString()));
		}

		m_remoteServer->m_ToServerUdp->AddToSendQueueWithSplitterAndSignal_Copy(
			m_remoteServer->m_ToServerUdp,
			HostID_Server,
			FilterTag::CreateFilterTag(GetVolatileLocalHostID(), HostID_Server),
			m_remoteServer->GetServerUdpAddr(),
			sendMsg,
			GetPreciseCurrentTimeMs(),
			SendOpt(MessagePriority_Holepunch, true));
	}

	void CNetClientImpl::SendServerHolePunchOnNeed()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		// UDP를 강제로 끈 상태가 아니라면 시도조차 하지 않도록 한다.
		if (m_settings.m_fallbackMethod == FallbackMethod_ServerUdpToTcp)
			return;

		//소켓이 준비 되지 않았다면 하지 않는다.
		if (m_remoteServer->m_ToServerUdp == nullptr || m_remoteServer->m_ToServerUdp->StopIoRequested())
			return;

		// 아직 HostID를 배정받지 않은 클라이면 서버 홀펀칭을 시도하지 않는다.
		// 어차피 서버로 시도해봤자 서버측에서 HostID=0이면 splitter에서 실패하기 때문에 무시된다.
		if (GetVolatileLocalHostID() == HostID_None)
			return;

		// 필요시 서버와의 홀펀칭 수행
		if (m_remoteServer->MustDoServerHolepunch())
			SendServerHolepunch();
	}

	void CNetClientImpl::RequestServerTimeOnNeed()
	{
		if (GetVolatileLocalHostID() != HostID_None)
		{
			int64_t currTime = GetPreciseCurrentTimeMs();

			if (m_lastRequestServerTimeTime == 0)
			{
				int randomShift = ((int)currTime) % (CNetConfig::UnreliablePingIntervalMs / 2);
				m_lastRequestServerTimeTime = currTime + randomShift;
			}
			// UDP 핑
			if (currTime - m_lastRequestServerTimeTime > CNetConfig::UnreliablePingIntervalMs / m_virtualSpeedHackMultiplication)
			{
				m_RequestServerTimeCount++;
				m_lastRequestServerTimeTime = currTime;

				// UDP로 주고 받는 메시지
				// 핑으로도 쓰인다.
				CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
				Message_Write(sendMsg, MessageType_UnreliablePing);
				sendMsg.Write(currTime);
				sendMsg.Write(m_serverUdpRecentPingMs);

				int CSLossPercent = 0;
				GetUnreliableMessagingLossRatioPercent(HostID_Server, &CSLossPercent);

				int64_t speed = 0;
				if (m_remoteServer->m_ToServerUdp != nullptr)
				{
					// 서버->클라 실제 수신한 속도
					speed = m_remoteServer->m_ToServerUdp->GetRecentReceiveSpeed(m_remoteServer->GetToServerUdpFallbackable()->m_serverAddr);
				}

				// 수신속도, 패킷로스율 전송
				sendMsg.WriteScalar(speed);
				sendMsg.Write(CSLossPercent);

				m_remoteServer->GetToServerUdpFallbackable()->SendWithSplitterViaUdpOrTcp_Copy(HostID_Server, CSendFragRefs(sendMsg), SendOpt(MessagePriority_Ring0, true));
			}

			if (m_ReliablePing_Timer.IsTimeToDo(currTime))
			{
				int ackNum = 0;
				m_remoteServer->m_ToServerTcp->AcrMessageRecovery_PeekMessageIDToAck(&ackNum);
				m_c2sProxy.ReliablePing(HostID_Server, g_ReliableSendForPN, m_applicationHint.m_recentFrameRate, (int)currTime, ackNum, m_serverTcpLastPingMs,
					CompactFieldMap());
			}
		}
		else
		{
			// 서버에 연결 상태가 아니면 미리미리 이렇게 준비해둔다.
			m_remoteServer->UpdateServerUdpReceivedTime();
			//m_lastReliablePongReceivedTime = m_lastServerUdpPacketReceivedTime;
		}
	}

	void CNetClientImpl::SpeedHackPingOnNeed(int64_t currentTimeMs)
	{
		if (GetVolatileLocalHostID() != HostID_None)
		{
			if (currentTimeMs - m_speedHackDetectorPingTime > 0)
			{
				// virtual speed hack
				m_speedHackDetectorPingTime = currentTimeMs + CNetConfig::SpeedHackDetectorPingIntervalMs / m_virtualSpeedHackMultiplication;

				//ATLTRACE("%d client Send SpeedHackDetectorPing\n",GetVolatileLocalHostID());
				// UDP로 주고 받는 메시지
				// 핑으로도 쓰인다.
				CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
				Message_Write(sendMsg, MessageType_SpeedHackDetectorPing);
				SendOpt sendOpt(MessagePriority_Ring0, true);
				sendOpt.m_reliability = MessageReliability_Unreliable;		// 이렇게 해놔야 ACR 상황에서 재전송하지 않는다. 재전송될 경우 서버쪽에서 한꺼번에 AMR로 인해 도착하는 이 메시지때문에 스핵으로 오탐한다.

				m_remoteServer->GetToServerUdpFallbackable()->SendWithSplitterViaUdpOrTcp_Copy(HostID_Server, CSendFragRefs(sendMsg), sendOpt);
			}
		}
	}

	int64_t CNetClientImpl::GetReliablePingTimerIntervalMs()
	{
		/* 이제는 TCP in crisis 체크 기능이 사라졌다. UDP 혼잡제어 기능이 알아서 처리하니까.
		그리고 reliable 메시징의 ping-pong은 unreliable과 달리 20-30% 이런 식이 아니라
		(time out - unfeasible RTO)가 기준이 되어야 한다.
		(패킷 로스면 알아서 RTT based retransmission을 하기 때문임.)
		단, 1초 이상은 보장해 주어야 함.
		따라서 아래와 같이 연산해서 리턴한다.

		NOTE: TCP retransmission timeout은 OS딴에서 이미 20초이다. 따라서 이 값은 20-5=15초를 넘길 이유가 없다.
		*/

		// defaultTimeoutTime 의 값은 tcp retransmission timeout 시간보다 작아야 한다.
		// NOTE: GetDefaultNoPingTimeoutTimeMs 함수 내에 설명 되어 있음
		//assert(m_settings.m_defaultTimeoutTimeMs < 20000);

		// 값 범위는 1000~9000 안에서 결정되게 한다. (랜선 뽑아도 9+20=29초 이내에 TCP가 끊어짐을 보장)
		// int64_t t1 = PNMAX(m_settings.m_defaultTimeoutTimeMs - 5000, 1000);
		// return PNMIN(t1, 10000);
		// ...이었다가, 수퍼피어 선정 기능을 위해 클라는 서버에게 frame rate를 보내야 하는데, 이것 때문에 3초 이상은 곤란.
		return 3000;

		// NOTE: 위의 식은 m_defaultTimeoutTimeMs 의 최대 20초로 세팅되어있는걸 가정하여 -5 초를 한것이다.
		//		따라서 20 - 5 = 15초 근사치에 맞게 defaultTimeoutTime 의 75% 로 잡는다.
		//return PNMAX((m_settings.m_defaultTimeoutTimeMs * 75) / 100, 1000);

		// ==퇴역 노트==
		// 연속 두세번 보낸 것이 일정 시간 안에 도착 안하면 사실상 막장이므로 이정도 주기면 OK.
		// (X / 10) * 3 보다 (X * 3) / 10 이 정확도가 더 높다.
		// int64_t 로 캐스팅 해주지 않으면 int 범위를 벗어날 경우 음수가 return  되어 문제가 발생할 수 있다.

		// 과거에는 30%였으나 TCP timeout을 30초로 지정할 경우, 9초마다 주고받는다.
		// 그러나 이러한 경우 마지막 TCP 데이터 수신 시간 후 60% 즉 18초가 지나야 위험 상황임을 알고
		// 혼잡 제어를 들어가는데 윈도 OS는 20초동안 retransmission이 실패하면 10053 디스를 일으킨다.
		// 따라서 무용지물. 이를 막기 위해서는 10%로 줄이고 위험 수위를 30%로 재조정해야 한다.
		// 그래서 1로 변경.
		//return (((int64_t)m_settings.m_defaultTimeoutTimeMs) * 1) / 10;
		//		return (((int64_t)m_settings.m_defaultTimeoutTimeMs) * 3) / 10;
	}

	DEFRMI_ProudS2C_NotifySpeedHackDetectorEnabled(CNetClientImpl::S2CStub)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		if (enable)
		{
			m_owner->m_speedHackDetectorPingTime = 0;
		}
		else
		{
			m_owner->m_speedHackDetectorPingTime = INT64_MAX;
		}

		return true;
	}

	DEFRMI_ProudS2C_NotifyChangedTimeoutTime(CNetClientImpl::S2CStub)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		m_owner->m_settings.m_defaultTimeoutTimeMs = val;

		return true;
	}

	DEFRMI_ProudS2C_NotifyChangedAutoConnectionRecoveryTimeoutTimeMs(CNetClientImpl::S2CStub)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

		m_owner->m_settings.m_autoConnectionRecoveryTimeoutTimeMs = val;

		return true;
	}

	int64_t CNetClientImpl::GetServerTimeMs()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		//		if(m_timer == nullptr)
		//			return -1;

		int64_t clientTime = GetPreciseCurrentTimeMs();

		return clientTime - m_serverTimeDiff;
	}

	void CNetClientImpl::FallbackServerUdpToTcpOnNeed()
	{
		int64_t currTime = GetPreciseCurrentTimeMs();
		// 너무 오랜 시간동안 서버에 대한 UDP ping이 실패하면 UDP도 TCP fallback mode로 전환한다.
		// [CaseCMN] 간혹 섭->클 UDP 핑은 되면서 반대로의 핑이 안되는 경우로 인해 UDP fallback이 계속 안되는 경우가 있는 듯.
		// 그러므로 서버에서도 클->섭 UDP 핑이 오래 안오면 fallback한다.
		if (m_remoteServer->FallbackServerUdpToTcpOnNeed(currTime))
		{
			if (m_enableLog || m_settings.m_emergencyLogLineCount > 0)
			{
				String TrafficStat = GetTrafficStatText();
				int64_t UdpDuration = GetPreciseCurrentTimeMs() - m_remoteServer->GetToServerUdpFallbackable()->m_realUdpEnabledTimeMs;
				int64_t LastUdpRecvIssueDuration = 0;

				if (m_remoteServer->m_ToServerUdp)
					LastUdpRecvIssueDuration = GetPreciseCurrentTimeMs() - m_remoteServer->m_ToServerUdp->m_lastReceivedTime;

				String s1 = Get_ToServerUdpSocketLocalAddr().ToString();

				int rank = 1;
				bool LBN = false;
				if (IsLocalHostBehindNat(LBN) && !LBN)
					rank++;
#ifdef _WIN32
				if (!m_manager->m_upnp.GetNatDeviceName().IsEmpty())
				{
					rank++;
				}
#endif
				String txt;
				txt.Format(_PNT("(first chance) to-server UDP punch lost##Reason:%d##CliInstCount=%d##DisconnedCount=%d##recv count=%d##last ok recv interval=%3.3lf##Recurred:%d##LocalIP:%s##UDP kept time:%3.3lf##Time diff since RecvIssue:%3.3lf##NAT name=%s##%s##Process=%s"),
					ErrorType_ServerUdpFailed,
					m_manager->m_instanceCount,
					m_manager->m_disconnectInvokeCount,
					m_remoteServer->GetToServerUdpFallbackable()->m_lastServerUdpPacketReceivedCount,
					double(m_remoteServer->GetToServerUdpFallbackable()->m_lastUdpPacketReceivedIntervalMs) / 1000,
					m_remoteServer->GetToServerUdpFallbackable()->m_fallbackCount,
					s1.GetString(),
					double(UdpDuration) / 1000,
					double(LastUdpRecvIssueDuration) / 1000,
#ifdef _WIN32
					m_manager->m_upnp.GetNatDeviceName().GetString(),
#else
					_PNT("Not support"),
#endif
					TrafficStat.GetString(),
					GetProcessName().GetString());

				LogHolepunchFreqFail(rank, _PNT("%s"), txt.GetString());
			}
			// 서버에 TCP fallback을 해야 함을 노티.
			m_c2sProxy.NotifyUdpToTcpFallbackByClient(HostID_Server, g_ReliableSendForPN,
				CompactFieldMap());
			/* 클라에서 to-server-UDP가 증발해도 per-peer UDP는 증발하지 않는다. 아예 internal port 자체가 다르니까.
			따라서 to-peer UDP는 그대로 둔다. */
		}
	}

	void CNetClientImpl::HlaFrameMove()
	{
#ifdef USE_HLA
		if (m_hlaSessionHostImpl != nullptr)
			return m_hlaSessionHostImpl->HlaFrameMove();
		else
			throw Exception(HlaUninitErrorText);
#endif
	}

	// PNTEST에 의해 호출됨.
	void CNetClientImpl::GarbageAllHosts()
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		CNetCoreImpl::GarbageAllHosts(); // base call

		for (HostIDToRemotePeerMap::iterator i = m_remotePeerRecycles.begin(); i != m_remotePeerRecycles.end(); i++)
		{
			shared_ptr<CRemotePeer_C> r = i->GetSecond();
			GarbageHost(r, ErrorType_DisconnectFromLocal, ErrorType_TCPConnectFailure,
				ByteArray(), _PNT("G-ALL"), SocketErrorCode_Ok);
		}
	}

	bool CNetClientImpl::CanDeleteNow()
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		// 모든 remote peer가 파괴되었는지?
		if (!m_remotePeerRecycles.IsEmpty())
			return false;
		// 연결유지기능의 재접속 기능이 죽었는지?
		if (m_autoConnectionRecoveryContext != nullptr)
			return false;
		// loopback host, remote host, socket들이 모두 파괴되었는지?
		if (!CNetCoreImpl::CanDeleteNow())
			return false;

		return true;
	}

	void CNetClientImpl::TEST_SetPacketTruncatePercent(Proud::HostType hostType, int percent)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (m_remoteServer->m_ToServerUdp && (hostType == HostType_All || hostType == HostType_Server))
		{
			m_remoteServer->m_ToServerUdp->m_packetTruncatePercent = percent;
		}

		if (hostType == HostType_All || hostType == HostType_Peer)
		{
			for (AuthedHostMap::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++)
			{
				shared_ptr<CRemotePeer_C> peer = LeanDynamicCast_RemotePeer_C(i->GetSecond());

				if (peer && peer->m_udpSocket)
				{
					peer->m_udpSocket->m_packetTruncatePercent = percent;
				}
			}
		}
	}

	Proud::AddrPort CNetClientImpl::GetServerAddrPort()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		if (m_remoteServer->m_ToServerTcp == nullptr)
			return AddrPort::Unassigned;

		return m_remoteServer->m_ToServerTcp->GetLocalAddr();
	}

	void CNetClientImpl::TcpAndUdp_DoForLongInterval()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		int64_t currTime = GetPreciseCurrentTimeMs();
		uint32_t queueLength = 0;

		if (m_RemoveTooOldUdpSendPacketQueueOnNeed_Timer.IsTimeToDo(currTime))
		{
			if (m_remoteServer->m_ToServerUdp != nullptr)
			{
				m_remoteServer->m_ToServerUdp->DoForLongInterval(currTime, queueLength);

				m_sendQueueUdpTotalBytes = 0;

				// per-peer UDP socket에 대해서도 처리하기.
				for (AuthedHostMap::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++)
				{
					shared_ptr<CRemotePeer_C> peer = LeanDynamicCast_RemotePeer_C(i->GetSecond());

					if (peer)
					{
						if (peer->m_garbaged)
						{
							continue;
						}

						if (peer->m_udpSocket)
						{
							peer->m_udpSocket->DoForLongInterval(currTime, queueLength);

							m_sendQueueUdpTotalBytes += queueLength;

							// 송신량 측정 결과도 여기다 업뎃한다.
							peer->m_sendQueuedAmountInBytes = peer->m_ToPeerUdp.GetUdpSendBufferPacketFilledCount();

							peer->m_udpSocket->SetCoalesceInteraval(peer->m_UdpAddrFromServer,
								peer->m_coalesceIntervalMs);
						}
					}
				}
			}

			// TCP에 대해서도 처리하기
			if (m_remoteServer->m_ToServerTcp)
			{
				m_remoteServer->m_ToServerTcp->DoForLongInterval(currTime, queueLength);

				m_remoteServer->m_ToServerTcp->SendDummyOnTcpOnTooLongUnsending(currTime);

				m_sendQueueTcpTotalBytes = queueLength;
			}
		}

		if (currTime - m_TcpAndUdp_DoForShortInterval_lastTime > m_ReliableUdpHeartbeatInterval_USE)
		{
			if (m_remoteServer->m_ToServerUdp != nullptr)
			{
				m_remoteServer->m_ToServerUdp->DoForShortInterval(currTime);
				//m_remoteServer->m_ToServerUdp->DoForShortInterval(currTime);

#if defined(_WIN32)
				// addrportqueueMapCount 갱신
				if (m_settings.m_emergencyLogLineCount > 0)
				{
					m_emergencyLogData.m_serverUdpAddrCount = (uint32_t)m_remoteServer->m_ToServerUdp->GetUdpSendQueueDestAddrCount();
				}
#endif
			}

			// per-peer UDP socket에 대해서도 처리하기.
			for (AuthedHostMap::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++)
			{
				shared_ptr<CRemotePeer_C> peer = LeanDynamicCast_RemotePeer_C(i->GetSecond());

				if (peer)
				{
					if (peer->m_garbaged)
					{
						continue;
					}

					if (peer->m_udpSocket)
					{
						peer->m_udpSocket->DoForShortInterval(currTime);
						//peer->m_udpSocket->DoForShortInterval(currTime);

#if defined(_WIN32)
						// addrportqueueMapCount 갱신
						if (m_settings.m_emergencyLogLineCount > 0)
							m_emergencyLogData.m_serverUdpAddrCount = (uint32_t)peer->m_udpSocket->GetUdpSendQueueDestAddrCount();
#endif
					}
				}
			}

			RemotePeerRecycles_GarbageTooOld();

			m_TcpAndUdp_DoForShortInterval_lastTime = currTime;
		}
	}

	// P2P group HostID를 개별 HostID로 바꾼 후 중복되는 것들을 병합한다.
	void CNetClientImpl::ExpandHostIDArray(int numberOfsendTo, const HostID * sendTo, HostIDArray& sendDestList)
	{
		for (int i = 0; i < numberOfsendTo; ++i)
		{
			ExpandHostIDArray_Append(sendTo[i], sendDestList);
		}

		UnionDuplicates<HostIDArray, HostID, int>(sendDestList);
	}

	//	void CNetClientImpl::Send_ToServer_Directly_Copy(HostID destHostID,MessageReliability reliability, const CSendFragRefs& sendData2, const SendOpt& sendOpt )
	//	{
	//		// send_core to server via UDP or TCP
	//		if (reliability == MessageReliability_Reliable)
	//		{
	//			Get_ToServerTcp()->AddToSendQueueWithSplitterAndSignal_Copy(sendData2, sendOpt, m_simplePacketMode);
	//		}
	//		else
	//		{
	//			// JIT UDP trigger하기
	//			RequestServerUdpSocketReady_FirstTimeOnly();
	//
	//			// unrealiable이 되기전까지 TCP로 통신
	//			// unreliable일때만 uniqueid를 사용하므로...
	//			m_ToServerUdp_fallbackable->SendWithSplitterViaUdpOrTcp_Copy(destHostID,sendData2,sendOpt);
	//		}
	//	}

	/* 1개의 메시지를 0개 이상의 상대에게 전송한다.
	메시지로 보내는 것의 의미는, 받는 쪽에서도 메시지 단위로 수신해야 함을 의미한다.
	msg: 보내려고 하는 메시지이다. 이 함수의 caller에 의해 header 등이 붙어버린 상태이므로, 메모리 복사 비용을 줄이기 위해 fragment list로서 받는다.
	sendContext: reliable로 보낼 것인지, unique ID를 붙일 것인지 등.
	sendTo, numberOfSendTo: 수신자들
	compressedPayloadLength: 보낸 메시지가 내부적으로 압축됐을 경우 압축된 크기.
	*/
	bool CNetClientImpl::Send(const CSendFragRefs &msg,
		const SendOpt& sendContext0,
		const HostID *sendTo, int numberOfsendTo, int &compressedPayloadLength)
	{
		SendOpt sendContext = sendContext0;
		AdjustSendOpt(sendContext);

		/* 네트워킹 비활성 상태이면 무조건 그냥 아웃.
		여기서 사전 검사를 하는 이유는, 아래 하위 callee들은 많은 validation check를 하는데, 그걸
		다 거친 후 안보내는 것 보다는 앗싸리 여기서 먼저 안보내는 것이 성능상 장점이니까.*/
		if (m_worker->GetState() == CNetClientWorker::Disconnected)
		{
			FillSendFailListOnNeed(sendContext, sendTo, numberOfsendTo, ErrorType_DisconnectFromLocal);
			return false;
		}

		// 설정된 한계보다 큰 메시지인 경우
		if (msg.GetTotalLength() > m_settings.m_clientMessageMaxLength)
		{
			stringstream ss;
			ss << "Too long message cannot be sent. length=" << msg.GetTotalLength();
			throw Exception(ss.str().c_str());
		}

		// 메시지 압축 레이어를 통하여 메시지에 압축 여부 관련 헤더를 삽입한다.
		// 암호화 된 후에는 데이터의 규칙성이 없어져서 압축이 재대로 되지 않기 때문에 반드시 암호화 전에 한다.
		return Send_CompressLayer(msg,
			sendContext,
			sendTo, numberOfsendTo, compressedPayloadLength);
	}

	bool CNetClientImpl::NextEncryptCount(HostID remote, CryptCount &output)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		//modify by rekfkno1 - 아직 서버와 연결이 되지 않았다면 encryptcount는 존재하지 않는다.
		if (m_remoteServer->m_ToServerTcp == nullptr || HostID_None == GetVolatileLocalHostID())
		{
			return false;
		}

		shared_ptr<CHostBase> rp0 = GetPeerByHostID_NOLOCK(remote);
		CRemotePeer_C* rp = (CRemotePeer_C*)rp0.get();
		if (rp != nullptr)
		{
			output = rp->m_encryptCount;
			rp->m_encryptCount++;
			return true;
		}
		else if (GetVolatileLocalHostID() == remote)
		{
			output = m_selfEncryptCount;
			m_selfEncryptCount++;
			return true;
		}
		else if (remote == HostID_Server)
		{
			output = m_toServerEncryptCount;
			m_toServerEncryptCount++;
			return true;
		}
		return false;
	}

	void CNetClientImpl::PrevEncryptCount(HostID remote)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		//modify by rekfkno1 - 아직 서버와 연결이 되지 않았다면 encryptcount는 존재하지 않는다.
		if (m_remoteServer->m_ToServerTcp == nullptr || HostID_None == GetVolatileLocalHostID())
		{
			return;
		}

		shared_ptr<CHostBase> rp0 = GetPeerByHostID_NOLOCK(remote);
		CRemotePeer_C* rp = (CRemotePeer_C*)rp0.get();
		if (rp != nullptr)
		{
			rp->m_encryptCount--;
		}
		else if (GetVolatileLocalHostID() == remote)
		{
			m_selfEncryptCount--;
		}
		else if (remote == HostID_Server)
		{
			m_toServerEncryptCount--;
		}
	}

	bool CNetClientImpl::GetExpectedDecryptCount(HostID remote, CryptCount &output)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		shared_ptr<CHostBase> rp0 = GetPeerByHostID_NOLOCK(remote);
		CRemotePeer_C* rp = (CRemotePeer_C*)rp0.get();
		if (rp != nullptr)
		{
			output = rp->m_decryptCount;
			return true;
		}
		else if (GetVolatileLocalHostID() == remote)
		{
			output = m_selfDecryptCount;
			return true;
		}
		else if (remote == HostID_Server)
		{
			output = m_toServerDecryptCount;
			return true;
		}
		return false;
	}

	bool CNetClientImpl::NextDecryptCount(HostID remote)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		shared_ptr<CHostBase> rp0 = GetPeerByHostID_NOLOCK(remote);
		CRemotePeer_C* rp = (CRemotePeer_C*)rp0.get();
		if (rp != nullptr)
		{
			rp->m_decryptCount++;
			return true;
		}
		else if (GetVolatileLocalHostID() == remote)
		{
			m_selfDecryptCount++;
			return true;
		}
		else if (remote == HostID_Server)
		{
			m_toServerDecryptCount++;
			return true;
		}
		return false;
	}

	void CNetClientImpl::Log(int logLevel, LogCategory logCategory, const String &logMessage, const String &logFunction /*= _PNT("")*/, int logLine /*= 0*/)
	{
		if (m_enableLog && GetVolatileLocalHostID() != HostID_None)
		{
			m_c2sProxy.NotifyLog(HostID_Server, g_ReliableSendForPN, logLevel, logCategory, GetVolatileLocalHostID(), logMessage, logFunction, logLine,
				CompactFieldMap());
		}

#if defined(_WIN32)
		if (m_settings.m_emergencyLogLineCount > 0)
		{
			AddEmergencyLogList(logLevel, logCategory, logMessage, logFunction, logLine);
		}
#endif
	}

	void CNetClientImpl::LogHolepunchFreqFail(int rank, const PNTCHAR* format, ...)
	{
		String s;

		va_list	pArg;
		va_start(pArg, format);
		s.FormatV(format, pArg);
		va_end(pArg);

		if (m_enableLog)
		{
			m_c2sProxy.NotifyLogHolepunchFreqFail(HostID_Server, g_ReliableSendForPN, rank, s,
				CompactFieldMap());
		}

#if defined(_WIN32)
		if (m_settings.m_emergencyLogLineCount > 0)
		{
			//AddEmergencyLogList(TID_HolepunchFreqFail, s);
			//AddEmergencyLogList(LogCategory_P2P, s);

			//###  // LogHolepunchFreqFail 메소드의 매개변수도 바뀌어야 하는지?
			AddEmergencyLogList(1, LogCategory_P2P, _PNT(""));
		}
#endif
	}

	// 클라-서버간 UDP가 receive timeout이 발생했을 때를 처리한다.
	// TCP fallback을 수행한다.
	// notifyToServer=true이면 서버에게 노티 RMI를 보낸다.
	// 만약 서버로부터 TCP fallback 을 명령 받은 경우에는 또 보내는 삽질을 하면 안되므로 false로 할 것.
	void CNetClientImpl::FirstChanceFallbackServerUdpToTcp(const FallbackParam &param)
	{
		bool notifyToServer = param.m_notifyToServer;
		//ErrorType reasonToShow = param.m_reason;

		if (m_remoteServer->FirstChanceFallbackServerUdpToTcp_WITHOUT_NotifyToServer(param))
		{
			if (notifyToServer)
				m_c2sProxy.NotifyUdpToTcpFallbackByClient(HostID_Server, g_ReliableSendForPN,
					CompactFieldMap());
		}
	}

	void CNetClientImpl::TEST_EnableVirtualSpeedHack(int64_t multipliedSpeed)
	{
		if (multipliedSpeed <= 0)
			throw Exception("Invalid parameter!");

		m_virtualSpeedHackMultiplication = multipliedSpeed;
	}

	Proud::HostID CNetClientImpl::GetVolatileLocalHostID() const
	{
		// 이 함수는 UDP packet frag/defrag board에서 filterTag를 검사하기 위해 callback되는 함수입니다.
		// 그런데 UDP packet frag/defrag board는 super socket의 send queue lock에 의해 보호됩니다.
		// 따라서 여기서 main lock을 하면 deadlock이 발생합니다.
		if (!m_loopbackHost)
			return HostID_None;

		return m_loopbackHost->m_HostID;
	}

	Proud::HostID CNetClientImpl::GetLocalHostID()
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		/* Q: main lock을 왜 안하나요?
		A: 사용자가 이 함수를 호출하는 경우, 어차피 사용자는 NetClient에 대한 main lock 액세스를 못합니다. 즉 이 함수를 호출해서 얻은 값은 volatile입니다.
		따라서 main lock이 무의미합니다. 따라서 이렇게 구별하였습니다. => 즐! 하위호환성 위해, 미증유 위해, lock 꼭 해야 함!!!
		*/
		return GetVolatileLocalHostID();
	}

	void CNetClientImpl::FrameMove(int maxWaitTimeMs, CFrameMoveResult *outResult /*= nullptr*/)
	{
		AssertIsNotLockedByCurrentThread();

		// Connect,Disconnect,FrameMove는 병렬로 실행되지 않는다.
		// 그래야 2개 이상의 스레드에서 같은 NC에 대해 FrameMove를 병렬로 실행할 때
		// 이상한 현상 안 생기니까.
		CriticalSectionLock phaseLock(m_connectDisconnectFramePhaseLock, true);

		{
			ZeroThreadPoolUsageMarker zeroThreadPoolUsageMarker(this);

			if (m_lastFrameMoveInvokedTime != -1)
			{
				m_lastFrameMoveInvokedTime = GetPreciseCurrentTimeMs(); // 어차피 이 값은 부정확해도 되므로.
			}

			// NetThreadPool 이 SingleThreaded 모델일 때 콜 쓰레드가 처리 하도록 작업 수행
			if (m_netThreadPool != nullptr &&
				m_connectionParam.m_netWorkerThreadModel == ThreadModel_SingleThreaded)
			{
				// FrameMove의 매개변수 CFrameMoveResult는 하위호환성을 위해 그대로 유지하도록 하였습니다.
				CWorkResult processEventResult;

				m_netThreadPool->Process(this, &processEventResult, maxWaitTimeMs);

				//				if (outResult != nullptr)
				//				{
				//					outResult->m_processedEventCount = processEventResult.m_processedEventCount;
				//					outResult->m_processedMessageCount = processEventResult.m_processedMessageCount;
				//				}
			}

			// UserThreadPool 이 SingleThreaded 모델일 때 콜 쓰레드가 처리 하도록 작업 수행
			if (m_userThreadPool != nullptr &&
				m_connectionParam.m_userWorkerThreadModel == ThreadModel_SingleThreaded)
			{
				// FrameMove의 매개변수 CFrameMoveResult는 하위호환성을 위해 그대로 유지하도록 하였습니다.
				CWorkResult processEventResult;

				m_userThreadPool->Process(this, &processEventResult, maxWaitTimeMs);

				if (outResult != nullptr)
				{
					outResult->m_processedEventCount = processEventResult.m_processedEventCount;
					outResult->m_processedMessageCount = processEventResult.m_processedMessageCount;
				}
			}

			AssertIsNotLockedByCurrentThread();
		}

		// #NetClientDisconnectOnCallback
		// 사용자가 RMI 함수 안에서 Disconnect를 호출했었다면
		// 여기에서 DisconnectAsync의 뒷단 처리 즉 리소스 정리를 하자.
		if (m_disconnectCallTime != 0)
		{
			CleanupAfterDisconnectIsCalled();
		}

		// #NEED_RAII
		if (CurrentThreadIsRunningUserCallback())
		{
			// 사용자가 Disconnect()를 호출 후에도 FrameMove()를 계속 호출하고 있었다면
			// 스레드를 정리할 기회가 있다.
			if (m_worker->GetState() == CNetClientWorker::Disconnected)
			{
				CleanThreads();
			}
		}
	}

	void CNetClientImpl::FrameMove(int maxWaitTime, CFrameMoveResult* outResult, ErrorInfoPtr& outError)
	{
		try
		{
			outError = ErrorInfoPtr();
			FrameMove(maxWaitTime, outResult); // NOTE: outError는 여기서 안 씀. 대신, outError는 아래 catch에서 채워짐.
		}
		NOTHROW_FUNCTION_CATCH_ALL
	}

	AddrPort CNetClientImpl::GetLocalUdpSocketAddr(HostID remotePeerID)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		shared_ptr<CHostBase> peer0 = GetPeerByHostID_NOLOCK(remotePeerID);
		CRemotePeer_C* peer = (CRemotePeer_C*)peer0.get();
		if (peer != nullptr && peer->m_udpSocket)
		{
			return peer->m_udpSocket->GetLocalAddr();
		}
		return AddrPort::Unassigned;
	}

	bool CNetClientImpl::GetPeerReliableUdpStats(HostID peerID, ReliableUdpHostStats &output)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		shared_ptr<CHostBase> hostBase;

		m_authedHostMap.TryGetValue(peerID, hostBase);

		shared_ptr<CRemotePeer_C> remotePeer = LeanDynamicCast_RemotePeer_C(hostBase);
		if (remotePeer && remotePeer->m_ToPeerReliableUdp.m_host)
		{
			remotePeer->m_ToPeerReliableUdp.m_host->GetStats(output);
			return true;
		}

		return false;
	}

	void CNetClientImpl::GetStats(CNetClientStats &outVal)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		CriticalSectionLock statsLock(m_statsCritSec, true);
		outVal = m_stats;

		// 통계 정보는 좀 부정확해도 된다.
		// 하지만 P2P 카운트 정보는 정확해야 한다.
		// 그래서 여기서 즉시 계산한다.

		if (m_remoteServer)
			outVal.m_serverUdpEnabled = m_remoteServer->GetToServerUdpFallbackable() ? m_remoteServer->GetToServerUdpFallbackable()->IsRealUdpEnabled() : false;

		m_stats.m_directP2PEnabledPeerCount = 0;
		for (AuthedHostMap::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++)
		{
			shared_ptr<CRemotePeer_C> peer = LeanDynamicCast_RemotePeer_C(i->GetSecond());
			if (peer && !peer->IsRelayedP2P())
				m_stats.m_directP2PEnabledPeerCount++;
		}

		outVal.m_sendQueueTcpTotalBytes = m_sendQueueTcpTotalBytes;
		outVal.m_sendQueueUdpTotalBytes = m_sendQueueUdpTotalBytes;
		outVal.m_sendQueueTotalBytes = outVal.m_sendQueueTcpTotalBytes + outVal.m_sendQueueUdpTotalBytes;
	}

	// 릴레이 메시지에 들어갈 '릴레이 최종 수신자들' 명단을 만든다.
	void RelayDestList_C::ToSerializable(RelayDestList &ret)
	{
		ret.Clear();
		for (int i0 = 0; i0 < (int)GetCount(); i0++)
		{
			RelayDest_C &i = (*this)[i0];
			RelayDest r;
			r.m_frameNumber = i.m_frameNumber;
			r.m_sendTo = i.m_remotePeer->m_HostID;
			ret.Add(r);
		}
	}

	//	void CNetClient::UseNetworkerThread_EveryInstance(bool enable)
	//	{
	//		CNetClientManager& man = CNetClientManager::Instance();
	//		man.m_useNetworkerThread = enable;
	//	}
	//
	//	void CNetClient::NetworkerThreadHeartbeat_EveryInstance()
	//	{
	//		CNetClientManager& man = CNetClientManager::Instance();
	//		// 이 함수는 대기가 없어야 한다. 따라서 timeout 값으로 0을 넣는다.
	//		man.m_netWorkerThread->Process(0);
	//	}

	CNetClient* CNetClient::Create_Internal()
	{
		// 여기서 PROUDNET_H_LIB_SIGNATURE를 건드린다. 이렇게 함으로 이 전역 변수가 컴파일러에 의해 사라지지 않게 한다.
		PROUDNET_H_LIB_SIGNATURE++;

		return new CNetClientImpl();
	}

	// 핑 측정되면 값들을 갱신
	void CNetClientImpl::ServerTcpPing_UpdateValues(int newLag)
	{
		m_serverTcpLastPingMs = max(newLag, 1); // 핑이 측정됐으나 0이 나오는 경우 아직 안 측정됐다는 오류를 범하지 않기 위함.

		if (m_serverTcpRecentPingMs == 0)
			m_serverTcpRecentPingMs = m_serverTcpLastPingMs;
		else
			m_serverTcpRecentPingMs = LerpInt(m_serverTcpRecentPingMs, m_serverTcpLastPingMs, CNetConfig::LagLinearProgrammingFactorPercent, 100);
	}

	void CNetClientImpl::ServerUdpPing_UpdateValues(int newLag)
	{
		m_serverUdpLastPingMs = max(newLag, 1); // 핑이 측정됐으나 0이 나오는 경우 아직 안 측정됐다는 오류를 범하지 않기 위함.

		if (m_serverUdpRecentPingMs == 0)
			m_serverUdpRecentPingMs = m_serverUdpLastPingMs;
		else
			m_serverUdpRecentPingMs = LerpInt(m_serverUdpRecentPingMs, m_serverUdpLastPingMs, CNetConfig::LagLinearProgrammingFactorPercent, 100);
	}


	bool CNetClientImpl::IsLocalHostBehindNat(bool &output)
	{
		if (!HasServerConnection())
			return false;

		output = (Get_ToServerUdpSocketLocalAddr() != Get_ToServerUdpSocketAddrAtServer());
		return true;
	}

	// 디버깅 용도
	Proud::String CNetClientImpl::GetTrafficStatText()
	{
		CNetClientStats ss;
		GetStats(ss);

		Tstringstream retStream;

		retStream << _PNT("TotalSend=") << ss.m_totalUdpSendBytes;
		retStream << _PNT(" TotalRecv=") << ss.m_totalUdpReceiveBytes;
		retStream << _PNT(" PeerCount=") << ss.m_directP2PEnabledPeerCount;
		retStream << _PNT("/") << ss.m_remotePeerCount;
		retStream << _PNT(" NAT Name=") << GetNatDeviceName().GetString();

		String ret;

		ret = retStream.str().c_str();

		return ret;
	}

	Proud::String CNetClientImpl::TEST_GetDebugText()
	{
		return String();
		//		String ret;
		//		ret.Format(_PNT("%s %s"),m_localUdpSocketAddr.ToString(),m_localUdpSocketAddrFromServer.ToString());
		//		return ret;
	}

	Proud::AddrPort CNetClientImpl::Get_ToServerUdpSocketLocalAddr()
	{
		return m_remoteServer->Get_ToServerUdpSocketLocalAddr();
	}

	// RTT 측정용 핑을 보내야 하면 보낸다.
	void CNetClientImpl::SendRoundTripLatencyPingOnNeed(shared_ptr<CHostBase>& host, int64_t currTime)
	{
		if (true == host->m_isTestingRtt)
		{
			// 만약 측정 마지노선을 넘겼다면 측정을 자동 중단합니다.
			// 넘기지 않았다면, 핑을 새로 보내야되는 시점인지 체크합니다.
			if (host->m_rttEndTimeMs < currTime)
			{
				host->m_isTestingRtt = false;
			}
			else if (host->m_rttLastPingMs + host->m_rttIntervalMs <= currTime)
			{
				if (HostID_Server == host->GetHostID())
				{
					m_c2sProxy.RoundTripLatencyPing(host->GetHostID(), g_UnreliableSendForPN, (int32_t)currTime);
				}
				else
				{
					m_c2cProxy.RoundTripLatencyPing(host->GetHostID(), g_UnreliableSendForPN, (int32_t)currTime);
				}

				host->m_rttLastPingMs = currTime;
			}
		}
	}

	AddrPort CNetClientImpl::Get_ToServerUdpSocketAddrAtServer()
	{
		// 서버에서 인식한 이 클라의 UDP 소켓 주소
		if (m_remoteServer->m_ToServerUdp == nullptr)
			return AddrPort::Unassigned;

		return m_remoteServer->m_ToServerUdp->m_localAddrAtServer;
	}

	//	void CNetClientImpl::WaitForGarbageCleanup()
	//	{
	//		assert(GetCriticalSection().IsLockedByCurrentThread()==false);
	//
	//		// per-peer UDP socket들의 completion도 모두 끝났음을 보장한다.
	//		for(int i=0;i<1000;i++)
	//		{
	//			{
	//				CriticalSectionLock lock(GetCriticalSection(),true);
	//				if(m_garbages.GetCount()==0)
	//					return;
	//				DoGarbageCollect();
	//			}
	//			Sleep(10);
	//		}
	//	}

	void CNetClientImpl::OnHostGarbaged(const shared_ptr<CHostBase>& remote, ErrorType /*errorType*/, ErrorType /*detailType*/, const ByteArray& /*comment*/, SocketErrorCode /*socketErrorCode*/)
	{
		LockMain_AssertIsLockedByCurrentThread();

		if (remote == m_loopbackHost)
		{
			// NOTE: LoopbackHost는 별도의 Socket을 가지지 않기 때문에 여기서는 처리하지 않습니다.
			return;
		}
		else if (remote == m_remoteServer)
		{
			shared_ptr<CSuperSocket> toServerTcp = m_remoteServer->m_ToServerTcp;
			assert(toServerTcp != nullptr);

			// 서버와 마찬가지로, OnHostGarbaged가 아니라 OnHostGarbageCollected에서
			// GarbageSocket and 참조 해제를 해야 합니다. 수정 바랍니다.
			// 단, socket to host map에서의 제거는 OnHostGarbaged에서 하는 것이 맞습니다.
			SocketToHostsMap_RemoveForAnyAddr(toServerTcp);

			shared_ptr<CSuperSocket> toServerUdp = m_remoteServer->m_ToServerUdp;
			if (toServerUdp != nullptr)
			{
				SocketToHostsMap_RemoveForAnyAddr(toServerUdp);
			}
		}
		else
		{
			shared_ptr<CRemotePeer_C> remotePeer = LeanDynamicCast_RemotePeer_C(remote);

			// ACR 처리가 들어가면서 해당 assert 구문을 없앴다.
			//assert(remotePeer != nullptr);

			// P2P peer를 garbage 처리한다.
			// OnP2PMemberLeave가 호출되기 전까지는 remote peer가 살아야 한다.
			// remote peer용 UDP socket은 garbages에 들어가면서 remote peer와의 관계 끊어짐.
			if (remotePeer != nullptr && remotePeer->m_garbaged == false)
			{
				remotePeer->m_garbaged = true;

				if (remotePeer->m_owner == this)
				{
					const shared_ptr<CSuperSocket>& udpSocket = remotePeer->m_udpSocket;
					if (udpSocket != nullptr)
					{
						// 일관성을 위해, 여기서도 위에처럼 OnHostGarbageCollected에서 remote peer의 UDP socket을 garbageSocket합시다.
						SocketToHostsMap_RemoveForAnyAddr(udpSocket);
						udpSocket->ReceivedAddrPortToVolatileHostIDMap_Remove(remotePeer->m_UdpAddrFromServer);
					}
				}
			}
		}

		// 재접속 연결 후보에 매치된 temp remote server이면
		if (m_autoConnectionRecovery_temporaryRemoteServers.ContainsKey(remote.get()))
		{
			// SocketToHostsMap 에서 삭제
			shared_ptr<CRemoteServer_C> remoteServer = dynamic_pointer_cast<CRemoteServer_C>(remote);
			SocketToHostsMap_RemoveForAnyAddr(remoteServer->m_ToServerTcp);
		}
	}

	Proud::String CNetClientImpl::GetNatDeviceName()
	{
#if defined(_WIN32)
		return m_manager->m_upnp.GetNatDeviceName();
#else
		return String();
#endif
	}

	void CNetClientImpl::OnSocketWarning(CFastSocket* /*socket*/, String msg)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		if (m_enableLog || m_settings.m_emergencyLogLineCount > 0)
		{
			Log(0, LogCategory_System, String::NewFormat(_PNT("Socket Warning: %s"), msg.GetString()));
		}
	}

	// #if !defined(_WIN32)
	//
	//	// edge trigger 방식. 그래서, socket 內 send buffer량이 변화가 없으면 아예 안보냄.
	//	// 따라서 이게 있어야 함.
	//	void CNetClientImpl::EveryRemote_NonBlockedSendOnNeed(CIoEventStatus &comp)
	//	{
	//		CriticalSectionLock lock(GetCriticalSection(),true);
	//
	//		// 서버 TCP
	//		if(Get_ToServerTcp())
	//		{
	//			Get_ToServerTcp()->NonBlockSendUntilWouldBlock(comp);
	//		}
	//
	//		// 서버 UDP
	//		if(Get_ToServerUdpSocket())
	//		{
	//			Get_ToServerUdpSocket()->NonBlockSendUntilWouldBlock(comp);
	//		}
	//
	//		// 피어 UDP
	//		for (RemotePeers_C::iterator i = m_remotePeers.begin();i != m_remotePeers.end();i++)
	//		{
	//			CRemotePeerPtr_C rp = i->GetSecond();
	//
	//			if(rp->m_garbaged)
	//				continue;
	//
	//			if(rp->m_udpSocket)
	//			{
	//				rp->Get_ToPeerUdpSocket()->NonBlockSendUntilWouldBlock(comp);
	//			}
	//		}
	//	}
	// #else
	//  이 함수를 CNetCoreImpl로 옮기자.
	//	void CNetClientImpl::EveryRemote_IssueSendOnNeed()
	//	{
	//		CriticalSectionLock lock(GetCriticalSection(),true);
	//
	//		// 서버 TCP
	//		if(Get_ToServerTcp())
	//		{
	//			Get_ToServerTcp()->IssueSendOnNeed();
	//
	//			m_ioPendingCount = Get_ToServerTcp()->m_socket->m_ioPendingCount;
	//			m_totalTcpIssueSendBytes = Get_ToServerTcp()->m_totalIssueSendBytes;
	//		}
	//
	//			// 서버 UDP
	//		if(Get_ToServerUdpSocket())
	//			{
	//			Get_ToServerUdpSocket()->IssueSendOnNeed_IfPossible(false);
	//			}
	//
	//			// 피어 UDP
	//			for (RemotePeers_C::iterator i = m_remotePeers.begin();i != m_remotePeers.end();i++)
	//			{
	//			CRemotePeerPtr_C rp = i->GetSecond();
	//
	//				if(rp->m_garbaged)
	//					continue;
	//
	//			if(rp->m_udpSocket)
	//			{
	//				rp->Get_ToPeerUdpSocket()->IssueSendOnNeed_IfPossible(false);
	//			}
	//		}
	//	}
	//#endif

	// NetCore 에서 요구하는 것을 구현
	CriticalSection& CNetClientImpl::GetCriticalSection()
	{
		return m_critSec;
	}

	int CNetClientImpl::GetInternalVersion()
	{
		return m_internalVersion;
	}

	void CNetClientImpl::SetEventSink(INetClientEvent *eventSink)
	{
		if (AsyncCallbackMayOccur())
			throw Exception(AsyncCallbackMayOccurErrorText);
		LockMain_AssertIsNotLockedByCurrentThread();
		m_eventSink_NOCSLOCK = eventSink;
	}

	void CNetClientImpl::SetEventSink(INetClientEvent *eventSink, ErrorInfoPtr& outError)
	{
		try
		{
			outError = ErrorInfoPtr();
			SetEventSink(eventSink);
		}
		NOTHROW_FUNCTION_CATCH_ALL
	}

	INetClientEvent* CNetClientImpl::GetEventSink()
	{
		return (INetClientEvent*)m_eventSink_NOCSLOCK;
	}

	void CNetClientImpl::EnqueError(ErrorInfoPtr info)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		LocalEvent e;
		e.m_type = LocalEventType_Error;
		e.m_errorInfo = info;
		e.m_remoteHostID = info->m_remote;
		e.m_remoteAddr = info->m_remoteAddr;

		EnqueLocalEvent(e, m_loopbackHost);
	}

	void CNetClientImpl::EnqueWarning(ErrorInfoPtr info)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		LocalEvent e;
		e.m_type = LocalEventType_Warning;
		e.m_errorInfo = info;
		e.m_remoteHostID = info->m_remote;
		e.m_remoteAddr = info->m_remoteAddr;

		EnqueLocalEvent(e, m_loopbackHost);
	}

	void CNetClientImpl::ShowError_NOCSLOCK(ErrorInfoPtr errorInfo)
	{
		CNetCoreImpl::ShowError_NOCSLOCK(errorInfo);
	}

	void CNetClientImpl::ShowNotImplementedRmiWarning(const PNTCHAR* RMIName)
	{
		CNetCoreImpl::ShowNotImplementedRmiWarning(RMIName);
	}

	void CNetClientImpl::PostCheckReadMessage(CMessage &msg, const PNTCHAR* RMIName)
	{
		CNetCoreImpl::PostCheckReadMessage(msg, RMIName);
	}

	bool CNetClientImpl::AsyncCallbackMayOccur()
	{
		return m_worker->GetState() != CNetClientWorker::Disconnected;
	}

	int CNetClientImpl::GetMessageMaxLength()
	{
		return m_settings.m_clientMessageMaxLength;
	}

	void CNetClientImpl::EnquePacketDefragWarning(shared_ptr<CSuperSocket> superSocket, AddrPort addrPort, const String& text)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		shared_ptr<CRemotePeer_C> rc = GetPeerByUdpAddr(addrPort, false);

		if (CNetConfig::EnablePacketDefragWarning)
		{
			EnqueWarning(ErrorInfo::From(ErrorType_InvalidPacketFormat, rc ? rc->m_HostID : HostID_None, text));
		}
	}

	Proud::AddrPort CNetClientImpl::GetPublicAddress()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (m_remoteServer->m_ToServerTcp == nullptr)
			return AddrPort::Unassigned;

		return m_remoteServer->m_ToServerTcp->m_localAddrAtServer;
	}

	void CNetClientImpl::StartupUpnpOnNeed()
	{
#if defined(_WIN32)
		CriticalSectionLock clk(GetCriticalSection(), true); // 이게 없으면 정확한 체크가 안되더라.
		if (m_settings.m_upnpDetectNatDevice &&
			m_remoteServer->m_ToServerTcp->GetLocalAddr().IsUnicastEndpoint() &&
			m_remoteServer->m_ToServerTcp->m_localAddrAtServer.IsUnicastEndpoint() &&
			IsBehindNat())
		{

			m_manager->m_upnp.Reset();
		}
#endif
	}

	void CNetClientImpl::TEST_GetTestStats(CTestStats2 &output)
	{
		output = m_testStats2;
	}

#ifdef VIZAGENT
	void CNetClientImpl::EnableVizAgent(const PNTCHAR* vizServerAddr, int vizServerPort, const String &loginKey)
	{
		if (!m_vizAgent)
		{
			m_vizAgent.Attach(new CVizAgent(this, vizServerAddr, vizServerPort, loginKey));
		}
	}
#endif

#ifdef VIZAGENT
	void CNetClientImpl::Viz_NotifySendByProxy(const HostID* remotes, int remoteCount, const MessageSummary &summary, const RmiContext& rmiContext)
	{
		if (m_vizAgent)
		{
			CFastArray<HostID> tgt;
			tgt.SetCount(remoteCount);
			UnsafeFastMemcpy(tgt.GetData(), remotes, sizeof(HostID) * remoteCount);
			m_vizAgent->m_c2sProxy.NotifyCommon_SendRmi(HostID_Server, g_ReliableSendForPN, tgt, summary);
		}
	}

	void CNetClientImpl::Viz_NotifyRecvToStub(HostID sender, RmiID RmiID, const PNTCHAR* RmiName, const PNTCHAR* paramString)
	{
		if (m_vizAgent)
		{
			m_vizAgent->m_c2sProxy.NotifyCommon_ReceiveRmi(HostID_Server, g_ReliableSendForPN, sender, RmiName, RmiID);
		}
	}
#endif

	// to-server UDP socket을 생성한다.
	// 생성 후 m_remoteServer->m_ToServerUdp is null로 생성 여부를 검사하라.
	void CNetClientImpl::New_ToServerUdpSocket()
	{
		// UDP 소켓생성이 1번이라도 실패한다면, 생성 시도를 하지 않는다.
		if (m_remoteServer->m_toServerUdpSocketCreateHasBeenFailed)
			return;

		if (m_remoteServer->m_ToServerUdp == nullptr)
		{
			// NOTE: 여기서 try-catch 하지 말 것. 여기서 catch를 하더라도 rethrow를 하면 모를까.
			AddrPort udpLocalAddr = m_remoteServer->m_ToServerTcp->GetLocalAddr();

			if (!udpLocalAddr.IsUnicastEndpoint())
			{
#if defined(_WIN32)
				String text;
				text.Format(_PNT("FATAL: new UDP socket - Cannot create UDP socket! Cannot get TCP NIC address %s."), udpLocalAddr.ToString().GetString());
				CErrorReporter_Indeed::Report(text);
#endif
				m_remoteServer->m_toServerUdpSocketCreateHasBeenFailed = true;
				EnqueWarning(ErrorInfo::From(ErrorType_LocalSocketCreationFailed, GetVolatileLocalHostID(), _PNT("UDP socket for server connection")));
				return;
			}

			SuperSocketCreateResult r = CSuperSocket::New(this, SocketType_Udp);
			if (!r.socket)
			{
				m_remoteServer->m_toServerUdpSocketCreateHasBeenFailed = true;
				EnqueWarning(ErrorInfo::From(
					ErrorType_LocalSocketCreationFailed,
					GetVolatileLocalHostID(),
					r.errorText));
				return;
			}

#ifdef _WIN32
			// #FAST_REACTION Windows Client에서는 이렇게 해서, 성능보다 빠른 반응성을 추구하도록 하자.
			r.socket->m_forceImmediateSend = true;
#endif

			shared_ptr<CSuperSocket> newUdpSocket(r.socket);

			BindUdpSocketToAddrAndAnyUnusedPort(newUdpSocket, udpLocalAddr);

			AssertIsLockedByCurrentThread();
			m_netThreadPool->AssociateSocket(newUdpSocket, true);

			m_remoteServer->m_ToServerUdp = newUdpSocket;

/*// 3003!!!!
			m_remoteServer->m_ToServerUdp->m_tagName = _PNT("to-server UDP"); */



			SocketToHostsMap_SetForAnyAddr(newUdpSocket, m_remoteServer);

#if !defined(_WIN32)
			// issue UDP first recv
			//((CUdpSocket_C*)m_toServerUdpSocket.get())->NonBlockedRecv(CNetConfig::UdpIssueRecvLength); // issue UDP first recv
#else
			// issue UDP first recv
			m_remoteServer->m_ToServerUdp->IssueRecv(
				m_remoteServer->m_ToServerUdp,
				true);
#endif
		}
	}

	//	void CNetClientImpl::RequestServerUdpSocketReady_FirstTimeOnly()
	//	{
	//		//udp소켓 생성을 요청.
	//		if(m_toServerUdpSocket == nullptr && m_ToServerUdp_fallbackable->m_serverUdpReadyWaiting == false &&
	//			m_settings.m_fallbackMethod <= FallbackMethod_PeersUdpToTcp && m_toServerUdpSocketFailed == false)
	//		{
	//			m_c2sProxy.C2S_RequestCreateUdpSocket(HostID_Server,g_ReliableSendForPN);
	//			m_ToServerUdp_fallbackable->m_serverUdpReadyWaiting = true;
	//		}
	//	}
	//
	void CNetClientImpl::GetUserWorkerThreadInfo(CFastArray<CThreadInfo> &output)
	{
		if (m_userThreadPool != nullptr)
		{
			m_userThreadPool->GetThreadInfos(output);
		}
	}

	void CNetClientImpl::GetNetWorkerThreadInfo(CFastArray<CThreadInfo> &output)
	{
		if (m_netThreadPool != nullptr)
		{
			m_netThreadPool->GetThreadInfos(output);
		}
	}

	void CNetClientImpl::SetApplicationHint(const CApplicationHint &hint)
	{
		m_applicationHint = hint;
	}
	void CNetClientImpl::GetApplicationHint(CApplicationHint &hint)
	{
		hint = m_applicationHint;
	}

	// 송신한뒤 나머지 tcp stream 처리작업
	void CNetClientImpl::OnMessageSent(int doneBytes, SocketType socketType/*, CSuperSocket* socket*/)
	{
		CriticalSectionLock clk(m_statsCritSec, true);
		// 통계를 기록한다.
		if (socketType == SocketType_Tcp)
		{
			m_stats.m_totalTcpSendBytes += doneBytes;
		}
		else
		{
			m_stats.m_totalUdpSendCount++;
			m_stats.m_totalUdpSendBytes += doneBytes;
		}
		// caller 안에서 next issue send on need를 하므로 여기서 next issue send on need 하지 말자.
	}

	// overrided function
	void CNetClientImpl::OnMessageReceived(int doneBytes, CReceivedMessageList& messages, const shared_ptr<CSuperSocket>& socket)
	{
		SocketType socketType = socket->GetSocketType();

		CriticalSectionLock mainLock(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		shared_ptr<CHostBase> hostBase = SocketToHostsMap_Get_NOLOCK(socket, AddrPort::Unassigned);

		/* HostBase 객체가 Socket 객체보다 먼저 삭제된 경우에는 SocketToHostsMap_Get_NOLOCK에서는 nullptr이 반환될 수 있습니다.
		이러한 경우 경우에는 지금 처리중인 메세지를 무시합니다. 아래는 그 이유.

		socket to host map에서 제거되었다는 의미는, 이미 host를 garbage collect 즉 delete가 되었다는 상태입니다.
		이는 host가 처리해야 할 user work가 완전히 다 처리되어 제거되어도 되는 조건을 이미 만족했다는 뜻입니다.
		그리고 그러한 상황에서는 이미 host가 가졌던 socket들의 소유권도 다 포기한 상태입니다. */
		if (hostBase.get() == nullptr)
			return;

		CriticalSectionLock statsLock(m_statsCritSec, true);

		// 통계
		if (socketType == SocketType_Tcp)
		{
			m_stats.m_totalTcpReceiveBytes += doneBytes;
			statsLock.Unlock(); // stats 갱신 끝. 이제 lock을 풀어버리자. 아래에서 main lock을 하니까.
		}
		else if (socketType == SocketType_Udp)
		{
			m_stats.m_totalUdpReceiveCount++;
			m_stats.m_totalUdpReceiveBytes += doneBytes;
			statsLock.Unlock(); // stats 갱신 끝. 이제 lock을 풀어버리자. 아래에서 main lock을 하니까.

			if (hostBase == m_remoteServer)
			{
				// 마지막 받은 시간을 갱신해서 회선이 살아있음을 표식
				m_remoteServer->UpdateServerUdpReceivedTime();
			}
			else
			{
				shared_ptr<CRemotePeer_C> peer = LeanDynamicCast_RemotePeer_C(hostBase);

				// 마지막 받은 시간을 갱신해서 회선이 살아있음을 표식
				int64_t curTime = GetPreciseCurrentTimeMs();
				int64_t okTime = curTime - peer->m_lastDirectUdpPacketReceivedTimeMs;
				if (okTime > 0)
					peer->m_lastUdpPacketReceivedInterval = okTime;

				peer->m_lastDirectUdpPacketReceivedTimeMs = curTime;
				peer->m_directUdpPacketReceiveCount++;
			}
		}

		ProcessMessageOrMoveToFinalRecvQueue(socket, messages);
	}

	void CNetClientImpl::ProcessMessageOrMoveToFinalRecvQueue(const shared_ptr<CSuperSocket>& socket, CReceivedMessageList &extractedMessages)
	{
		SocketType socketType = socket->GetSocketType();

		// main lock을 걸었으니 송신자의 HostID를 이제 찾을 수 있다.
		AssertIsLockedByCurrentThread();

		for (CReceivedMessageList::iterator i = extractedMessages.begin(); i != extractedMessages.end(); i++)
		{
			CReceivedMessage& receivedMessage = *i;

			/*  TCP, UDP 모두 Proud.CNetCoreImpl.SocketToHostMap_Get로 찾도록 하자.
			이를 위해 per-peer UDP socket을 생성하는 곳에서는 SocketToHostMap_Set을,
			UDP socket을 버리는 곳에서는 SocketToHostMap_Remove를 호출하게 수정하자.
			VA find ref로 Proud.CRemotePeer_C.m_udpSocket를 건드리는 곳을 찾으면 됨. */
			// NetClient에서는 하나의 UDP Socket이 여러 AddrPort를 가지는 경우가 없으므로 AddrPort::Unassigned로 처리하였습니다.
			shared_ptr<CHostBase> pFrom = SocketToHostsMap_Get_NOLOCK(socket, AddrPort::Unassigned);


			receivedMessage.m_remoteHostID = pFrom->m_HostID;

			bool useMessage;

			// TCP socket ptr를 체크하므로
			AssertIsLockedByCurrentThread();

			if (receivedMessage.m_hasMessageID == false
				|| m_remoteServer->m_ToServerTcp == nullptr
				|| socketType != SocketType_Tcp)
			{
				useMessage = true;
			}
			else
			{
				useMessage = m_remoteServer->m_ToServerTcp->AcrMessageRecovery_ProcessReceivedMessageID(receivedMessage.m_messageID);
			}

			if (useMessage)
			{
				assert(receivedMessage.m_unsafeMessage.GetReadOffset() == 0);
				m_worker->ProcessMessage_ProudNetLayer(socket, receivedMessage);
			}
		}
	}

	//	void CNetClientImpl::SendReadyInstances_Add()
	//	{
	//		m_manager->SendReadyInstances_Add(this);
	//	}

	bool CNetClientImpl::SetHostTag(HostID hostID, void* hostTag)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		shared_ptr<CHostBase> hp= GetHostBase_includingRecyclableRemotePeer(hostID);
		if (hp)
		{
			hp->m_hostTag = hostTag;
			return true;
		}
		else
		{
			return false;
		}
	}

	void* CNetClientImpl::GetHostTag(HostID hostID)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		shared_ptr<CHostBase> hp = GetHostBase_includingRecyclableRemotePeer(hostID);
		if (hp)
		{
			return hp->m_hostTag;
		}
		else
			return NULL;

		return nullptr;
	}

	/* NetClient에서, get or set host tag 전용으로 사용되는, host 객체를 얻는 함수이다.
	authed가 아니라, recyclable 상태의 remote peer도 유효하게 처리한다.

	Q: 이 함수가 왜 필요해요?
	A: Set or GetHostTag에서 필요합니다. 유저 코드가 실행되는 동안, remote peer는 networker thread 안에서
	돌연 사라지거나, 또 재생성될 수도 있습니다. 사용자가 짧은 시간동안 대량의 P2P group join or leave를 하는
	경우에 말이죠. 이러한 경우 해당 peer에 대해서 OnP2PMemberJoinEvent or leave event가 대량으로 발생할텐데,
	이때 재사용 가능한 remote peer 객체에 있는 hostTag 값이 사용될 수 있어야 합니다. 안그러면 Get or SetHostTag가
	실패할테니까요. */
	shared_ptr<CHostBase> CNetClientImpl::GetHostBase_includingRecyclableRemotePeer(HostID hostID)
	{
		if (hostID == HostID_None)
			return shared_ptr<CHostBase>();

		if (hostID == HostID_Server)
			return m_remoteServer;

		if (hostID == GetVolatileLocalHostID())
			return m_loopbackHost;

		shared_ptr<CHostBase> hp = AuthedHostMap_Get(hostID);
		if (hp)
		{
			return hp;
		}

		hp = RemotePeerRecycles_Get(hostID);
		if (hp)
		{
			return hp;
		}

		// NOTE: garbaged host는, garbage collect 즉 delete가 되기 직전까지는 authed hosts에 계속 유지된다.
		// CNetCoreImpl::DoGarbageCollect_Host를 보면 알 수 있다.
		// 따라서 여기서 m_garbagedHosts까지 뒤질 필요는 없다.

		return shared_ptr<CHostBase>();
	}

	AddrPort CNetClientImpl::GetTcpLocalAddr()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (m_remoteServer->m_ToServerTcp == nullptr)
			return AddrPort::Unassigned;

		return m_remoteServer->m_ToServerTcp->GetLocalAddr();
	}

	Proud::AddrPort CNetClientImpl::GetUdpLocalAddr()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (m_remoteServer->m_ToServerUdp == nullptr)
			return AddrPort::Unassigned;

		return m_remoteServer->m_ToServerUdp->GetLocalAddr();
	}

	// 송신큐가 너무 증가하면 사용자에게 경고를 쏜다.
	void CNetClientImpl::CheckSendQueue()
	{
		int64_t curTime = GetPreciseCurrentTimeMs();
		if (m_remoteServer->m_ToServerTcp != nullptr
			&& curTime - m_lastCheckSendQueueTime > CNetConfig::SendQueueHeavyWarningCheckCoolTimeMs)
		{
			int length = m_remoteServer->m_ToServerTcp->GetSendQueueLength();

			if (m_remoteServer->m_ToServerUdp)
				length += m_remoteServer->m_ToServerUdp->GetPacketQueueTotalLengthByAddr(m_remoteServer->GetServerUdpAddr());

			if (m_sendQueueHeavyStartTime != 0)
			{
				if (length > CNetConfig::SendQueueHeavyWarningCapacity)
				{
					if (curTime - m_sendQueueHeavyStartTime > CNetConfig::SendQueueHeavyWarningTimeMs)
					{
						m_sendQueueHeavyStartTime = curTime;

						String str;
						str.Format(_PNT("%d bytes in send queue"), length);
						EnqueWarning(ErrorInfo::From(ErrorType_SendQueueIsHeavy, HostID_Server, str));
					}
				}
				else
					m_sendQueueHeavyStartTime = 0;
			}
			else if (length > CNetConfig::SendQueueHeavyWarningCapacity)
				m_sendQueueHeavyStartTime = curTime;

			m_lastCheckSendQueueTime = curTime;
		}
	}

#if defined(_WIN32)
	void CNetClientImpl::UpdateNetClientStatClone()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		// emergency log가 활성화되어있을때만 처리
		if (m_settings.m_emergencyLogLineCount > 0 && m_worker->GetState() < CNetClientWorker::Disconnecting)
		{
			int64_t curTime = GetPreciseCurrentTimeMs();
			if (curTime - m_lastUpdateNetClientStatCloneTime > CNetConfig::UpdateNetClientStatCloneCoolTimeMs)
			{
				m_lastUpdateNetClientStatCloneTime = curTime;
				GetStats(m_recentBackedUpStats);
			}
		}
	}
#endif

	void CNetClientImpl::GetWorkerState(CClientWorkerInfo &output)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		//output.m_isWorkerThreadEnded = m_manager->m_threadEnd;
		output.m_disconnectCallCount = m_disconnectInvokeCount;
		output.m_finalWorkerItemCount = (int)GetFinalUserWotkItemCount();
		if (GetVolatileLocalHostID() == HostID_None)
			output.m_peerCount = 0;
		else
			// GetVolatileLocalHostID()가 None이 아닌 이상, RemoteServer와 LoopBackHost는 AuthedHosts로 추가가 되었으므로 제외
			output.m_peerCount = (int)m_authedHostMap.GetCount() - 2;
		output.m_connectCallCount = m_connectCount;
		output.m_currentTimeMs = GetPreciseCurrentTimeMs();
		//		output.m_lastTcpStreamTimeMs = m_lastTcpStreamReceivedTime;
		output.m_workerThreadID = 0;

		//		if(!output.m_isWorkerThreadEnded)
		//		{
		//			output.m_workerThreadID = m_manager->GetWorkerThreadID();
		//		}

		CServerConnectionState state;
		output.m_connectionState = GetServerConnectionState(state);
	}

#ifdef _DEBUG
	void CNetClientImpl::CheckCriticalsectionDeadLock_INTERNAL(const PNTCHAR* /*functionname*/)
	{}
#endif // _DEBUG

	bool CNetClientImpl::GetSocketInfo(HostID remoteHostID, CSocketInfo& output)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

#if !defined(_WIN32)
		output.m_tcpSocket = -1;
		output.m_udpSocket = -1;
#else
		output.m_tcpSocket = InvalidSocket;
		output.m_udpSocket = InvalidSocket;
#endif

		// 서버와의 연결
		if (m_remoteServer->m_ToServerTcp && m_remoteServer->m_ToServerTcp->GetSocket())
		{
			output.m_tcpSocket = m_remoteServer->m_ToServerTcp->GetSocket()->m_socket;
		}
		else
			return false;

		if (HostID_Server == remoteHostID)		// 서버이면
		{
			if (m_remoteServer->m_ToServerUdp && m_remoteServer->m_ToServerUdp->GetSocket())
			{
				output.m_udpSocket = m_remoteServer->m_ToServerUdp->GetSocket()->m_socket;

				return true;
			}
		}
		else	// peer이면
		{
			shared_ptr<CRemotePeer_C> peer = GetPeerByHostID_NOLOCK(remoteHostID);

			if (peer == nullptr)
				return false;

			if (peer->IsRelayedP2P() && m_remoteServer->m_ToServerUdp && m_remoteServer->m_ToServerUdp->GetSocket())
			{
				// 릴레이인 경우
				output.m_udpSocket = m_remoteServer->m_ToServerUdp->GetSocket()->m_socket;

				return true;
			}
			else if (!peer->IsRelayedP2P() && peer->m_udpSocket && peer->m_udpSocket->GetSocket())
			{
				// 직접 P2P인 경우
				output.m_udpSocket = peer->m_udpSocket->GetSocket()->m_socket;

				return true;
			}
		}

		return false;
	}

#if defined(_WIN32)
	void CNetClientImpl::AddEmergencyLogList(int logLevel, LogCategory logCategory, const String &logMessage, const String& /*logFunction*/ /*= _PNT("")*/, int /*logLine*/ /*= 0*/)
	{
		//lock하에서 진행되어야 함.
		LockMain_AssertIsLockedByCurrentThread();

		if (m_settings.m_emergencyLogLineCount == 0)
			return;

		//###	// Emergency 로그 추가 시간을 클라이언트에 맞춰야하는지 서버에 맞춰야하는지??
		// 우선 클라에 맞춤
		//		CEmergencyLogData::EmergencyLog logNode;
		//		logNode.m_logCategory = logCategory;
		//		logNode.m_text = text;
		//		logNode.m_addedTime = CTime::GetCurrentTime().GetTime();

		CEmergencyLogData::EmergencyLog logNode;
		logNode.m_logLevel = logLevel;
		logNode.m_logCategory = logCategory;
		logNode.m_hostID = GetVolatileLocalHostID();
		logNode.m_message = logMessage;

		m_emergencyLogData.m_logList.AddTail(logNode);

		if (m_emergencyLogData.m_logList.GetCount() > m_settings.m_emergencyLogLineCount)
		{
			m_emergencyLogData.m_logList.RemoveHead();
		}
	}

	bool CNetClientImpl::SendEmergencyLogData(String serverAddr, uint16_t serverPort)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		CEmergencyLogData* logData = new CEmergencyLogData();

		// NetClient 가 날라갈것을 대비해 지역으로 데이터를 옮기자.

		// 기존걸 복사하자.
		m_emergencyLogData.CopyTo(*logData);

		// logData의 나머지를 채운다.
		logData->m_HostID = m_loopbackHost->m_backupHostID;

		if (m_remoteServer != nullptr)
			logData->m_totalTcpIssueSendBytes = m_remoteServer->m_ToServerTcp->GetTotalIssueSendBytes();
		else
			logData->m_totalTcpIssueSendBytes = 0;

		logData->m_ioPendingCount = m_ioPendingCount;
		timespec_get_pn(&logData->m_loggingTime, TIME_UTC);
		logData->m_connectCount = m_connectCount;
		logData->m_connResetErrorCount = g_connResetErrorCount;
		logData->m_msgSizeErrorCount = g_msgSizeErrorCount;
		logData->m_netResetErrorCount = g_netResetErrorCount;
		logData->m_intrErrorCount = g_intrErrorCount;
		logData->m_directP2PEnablePeerCount = m_recentBackedUpStats.m_directP2PEnabledPeerCount;
		logData->m_remotePeerCount = m_recentBackedUpStats.m_remotePeerCount;
		logData->m_totalTcpReceiveBytes = m_recentBackedUpStats.m_totalTcpReceiveBytes;
		logData->m_totalTcpSendBytes = m_recentBackedUpStats.m_totalTcpSendBytes;
		logData->m_totalUdpReceiveCount = m_recentBackedUpStats.m_totalUdpReceiveCount;
		logData->m_totalUdpSendBytes = m_recentBackedUpStats.m_totalUdpSendBytes;
		logData->m_totalUdpSendCount = m_recentBackedUpStats.m_totalUdpSendCount;
		logData->m_natDeviceName = GetNatDeviceName();

		SYSTEM_INFO sysinfo;
		::GetSystemInfo(&sysinfo);
		logData->m_processorArchitecture = sysinfo.wProcessorArchitecture;

		DWORD major = 0;
		DWORD minor = 0;
		DWORD productType = 0;
		GetWindowsVersion(major, minor, productType);

		logData->m_osMajorVersion = major;
		logData->m_osMinorVersion = minor;
		logData->m_productType = (BYTE)productType;

		logData->m_serverAddr = serverAddr;
		logData->m_serverPort = serverPort;

		::QueueUserWorkItem(RunEmergencyLogClient, (PVOID)logData, WT_EXECUTELONGFUNCTION);

		return true;
	}

	DWORD WINAPI CNetClientImpl::RunEmergencyLogClient(void* context)
	{
		CEmergencyLogData* logData = (CEmergencyLogData*)context;
		CEmergencyLogClient* client = new CEmergencyLogClient();

		try
		{
			client->Start(logData->m_serverAddr, logData->m_serverPort, logData);

			while (client->GetState() != CEmergencyLogClient::Stopped)
			{
				client->FrameMove();
				Sleep(10);
			}
		}
		catch (Exception& e)
		{
			CFakeWin32::OutputDebugStringA(e.what());
		}

		// client를 먼저삭제해야 logData의 개런티 보장
		if (client)
			delete client;

		// logData 삭제
		if (logData)
			delete logData;

		return 0;
	}
#endif

	// udpLocalAddr이 가리키는 주소와, m_unusedUdpPorts 안의 포트값 중 하나 중 bind를 할 수 있는 것을 udpSocket에 bind를 한다.
	// 이 함수는 반드시 bind는 성공시킨다. 다만, m_unusedUdpPorts 혹은 임의의 port의 값이 쓰일 뿐이다.
	// returns: m_unusedUdpPorts의 것으로 bind하면 true, m_unusedUdpPorts 외의 임의의 port에 bind하면 false.
	bool CNetClientImpl::BindUdpSocketToAddrAndAnyUnusedPort(const shared_ptr<CSuperSocket>& udpSocket, AddrPort &udpLocalAddr)
	{
		int trialCount = 0;
		for (CFastSet<uint16_t>::iterator iter = m_unusedUdpPorts.begin(); iter != m_unusedUdpPorts.end(); iter++)
		{
			udpLocalAddr.m_port = *iter;

			AssureIPAddressIsUnicastEndpoint(udpLocalAddr);

			if (SocketErrorCode_Ok == udpSocket->Bind(udpLocalAddr))
			{
				// 사용한 UDP 포트 목록에 집어 넣습니다.
				m_usedUdpPorts.Add(udpLocalAddr.m_port);
				m_unusedUdpPorts.Remove(udpLocalAddr.m_port);
				return true;
			}
			trialCount++;
		}

		// 모든 사용자 udp포트에 대해서 실패했으니, 임의로 지정합니다.
		udpLocalAddr.m_port = 0;
		udpSocket->Bind(udpLocalAddr);
		AssureIPAddressIsUnicastEndpoint(udpSocket->GetLocalAddr());

		if (m_usedUdpPorts.GetCount() > 0 || m_unusedUdpPorts.GetCount() > 0)
		{
			// 유저가 UDP 포트를 지정했다면 실패를 알려줍니다.
			String str;
			str.Format(_PNT("Trial count:%d, Arbitrary port number used: %d"),
				trialCount,
				udpSocket->m_localAddr_USE_FUNCTION.m_port);
			EnqueWarning(ErrorInfo::From(ErrorType_NoneAvailableInPortPool, GetVolatileLocalHostID(), str));
		}

		return false;
	}

	// upnp 라우터가 기 감지된 상태이고 TCP 연결이 있는 상태이고 내외부 주소가 모두 인식되었고
	// 아직 안 했다면 홀펀칭된 정보를 근거로 TCP 포트 매핑을 upnp에 건다.
	void CNetClientImpl::AddUpnpTcpPortMappingOnNeed()
	{
#if defined(_WIN32)
		if (!m_settings.m_upnpTcpAddPortMapping)
			return;

		if (m_upnpTcpPortMappingState != nullptr)
			return;

		if (!HasServerConnection())
			return;

		// 서버,클라가 같은 LAN에 있거나 리얼IP이면 할 필요가 없다. (해서도 안됨)
		if (!m_remoteServer->m_ToServerTcp->GetLocalAddr().IsUnicastEndpoint() || !m_remoteServer->m_ToServerTcp->m_localAddrAtServer.IsUnicastEndpoint())
			return;

		if (!IsBehindNat())
			return;

		if (m_manager->m_upnp.GetCommandAvailableNatDevice() == nullptr)
			return;

		m_upnpTcpPortMappingState.Attach(new CUpnpTcpPortMappingState);
		m_upnpTcpPortMappingState->m_lanAddr = m_remoteServer->m_ToServerTcp->GetLocalAddr();
		m_upnpTcpPortMappingState->m_wanAddr = m_remoteServer->m_ToServerTcp->m_localAddrAtServer;

		m_manager->m_upnp.AddTcpPortMapping(
			m_upnpTcpPortMappingState->m_lanAddr,
			m_upnpTcpPortMappingState->m_wanAddr,
			true);
#endif
	}

	// 하나라도 포함되는 grouphostid를 찾으면 true를 리턴한다.
	bool CNetClientImpl::GetIntersectionOfHostIDListAndP2PGroupsOfRemotePeer(HostIDArray sortedHostIDList, shared_ptr<CRemotePeer_C> rp, HostIDArray* outSubsetGroupHostIDList)
	{
		bool ret = false;

		// 이 객체 자체는 재사용되므로 청소 필수
		outSubsetGroupHostIDList->Clear();

		// remote peer가 소속된 각 p2p group에 대해
		for (CRemotePeer_C::JoinedP2PGroups::iterator it = rp->m_joinedP2PGroups.begin(); it != rp->m_joinedP2PGroups.end(); it++)
		{
			// 2진 검색을 해서, 입력받은 host ID list 안에 있으면 목록에 추가한다.
			if (BinarySearch(sortedHostIDList.GetData(), sortedHostIDList.GetCount(), it->GetFirst()))
			{
				outSubsetGroupHostIDList->Add(it->GetFirst());
				ret = true;
			}
		}
		return ret;
	}

	void CNetClientImpl::DeleteUpnpTcpPortMappingOnNeed()
	{
#if defined(_WIN32)
		if (!m_upnpTcpPortMappingState)
			return;

		if (HasServerConnection())
			return;

		m_manager->m_upnp.DeleteTcpPortMapping(
			m_upnpTcpPortMappingState->m_lanAddr,
			m_upnpTcpPortMappingState->m_wanAddr,
			true);

		m_upnpTcpPortMappingState.Free();
#endif
	}

	// 로컬 호스트가 공유기 뒤에 있는지
	bool CNetClientImpl::IsBehindNat()
	{
		if (m_remoteServer->m_ToServerTcp &&
			m_remoteServer->m_ToServerTcp->GetLocalAddr() != m_remoteServer->m_ToServerTcp->m_localAddrAtServer)
			return true;

		return false;
	}

	void CNetClientImpl::HlaAttachEntityTypes(CHlaEntityManagerBase_C* /*entityManager*/)
	{
#ifdef USE_HLA
		if (m_hlaSessionHostImpl != nullptr)
			m_hlaSessionHostImpl->HlaAttachEntityTypes(entityManager);
		else
		{
			throw Exception(HlaUninitErrorText);
		}
#endif // USE_HLA
	}

	void CNetClientImpl::HlaSetDelegate(IHlaDelegate_C* /*dg*/)
	{
#ifdef USE_HLA
		if (m_hlaSessionHostImpl == nullptr)
		{
			m_hlaSessionHostImpl.Attach(new CHlaSessionHostImpl_C(this));
		}
		m_hlaSessionHostImpl->HlaSetDelegate(dg);
#endif // USE_HLA
	}

	int CNetClientImpl::GetLastReliablePingMs(HostID peerHostID, ErrorType* error /*= nullptr */)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		// 서버
		if (peerHostID == HostID_Server)
		{
			if (error != nullptr)
			{
				*error = ErrorType_Ok;
			}

			return m_serverTcpLastPingMs;
		}

		// 피어
		shared_ptr<CRemotePeer_C> p = GetPeerByHostID_NOLOCK(peerHostID);
		if (p != nullptr)
		{
			if (error != nullptr)
			{
				*error = ErrorType_Ok;
			}

			if (!p->m_forceRelayP2P)
				p->m_jitDirectP2PNeeded = true;

			return p->m_lastReliablePingMs;
		}

		// p2p group을 얻으려고 하는 경우 모든 멤버들의 평균 핑을 구한다.
		CP2PGroupPtr_C group = GetP2PGroupByHostID_Internal(peerHostID);

		if (group)
		{
			// touch jit p2p & get recent ping ave
			int cnt = 0;
			int total = 0;
			for (CP2PGroup_C::P2PGroupMembers::iterator iMember = group->m_members.begin(); iMember != group->m_members.end(); iMember++)
			{
				int ping = GetLastReliablePingMs(iMember.GetFirst());//touch jit p2p
				if (ping >= 0)
				{
					cnt++;
					total += ping;
				}
			}

			if (cnt > 0)
			{
				if (error != nullptr)
					*error = ErrorType_Ok;
				return total / cnt;
			}
		}
		if (error != nullptr)
		{
			*error = ErrorType_ValueNotExist;
		}

		return -1;
	}

	int CNetClientImpl::GetRecentReliablePingMs(HostID peerHostID, ErrorType* error /*= nullptr */)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		// 서버
		if (peerHostID == HostID_Server)
		{
			if (error != nullptr)
			{
				*error = ErrorType_Ok;
			}

			return m_serverTcpRecentPingMs;
		}

		// 피어
		shared_ptr<CRemotePeer_C> p = GetPeerByHostID_NOLOCK(peerHostID);
		if (p != nullptr)
		{
			if (!p->m_forceRelayP2P)
				p->m_jitDirectP2PNeeded = true;

			if (error != nullptr)
			{
				*error = ErrorType_Ok;
			}

			return p->m_recentReliablePingMs;
		}

		// p2p group을 얻으려고 하는 경우 모든 멤버들의 평균 핑을 구한다.
		CP2PGroupPtr_C group = GetP2PGroupByHostID_Internal(peerHostID);

		if (group)
		{
			// touch jit p2p & get recent ping ave
			int cnt = 0;
			int total = 0;
			for (CP2PGroup_C::P2PGroupMembers::iterator iMember = group->m_members.begin(); iMember != group->m_members.end(); iMember++)
			{
				int ping = GetRecentReliablePingMs(iMember->GetFirst());//touch jit p2p
				if (ping >= 0)
				{
					cnt++;
					total += ping;
				}
			}

			if (cnt > 0)
			{
				if (error != nullptr)
					*error = ErrorType_Ok;
				return total / cnt;
			}
		}
		if (error != nullptr)
		{
			*error = ErrorType_ValueNotExist;
		}

		return -1;
	}


	ErrorType CNetClientImpl::StartRoundTripLatencyTest(HostID clientID, const StartRoundTripLatencyTestParameter& parameter)
	{
		if (parameter.pingIntervalMs <= 0)
		{
			throw Exception("Invalid pingIntervalMs value.");
		}

		if (parameter.testDurationMs <= 0)
		{
			throw Exception("Invalid testDurationMs value.");
		}

		// 측정 간격은 측정 기간보다 크면 안된다.
		if (parameter.testDurationMs <= parameter.pingIntervalMs)
		{
			throw Exception("testDurationMs is smaller than or equal to pingIntervalMs.");
		}

		// 자기 자신에 대해서는 측정 불가.
		if (clientID == GetLocalHostID())
		{
			return ErrorType_InvalidHostID;
		}

		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		shared_ptr<CHostBase> rc = nullptr;
		if (true == m_authedHostMap.TryGetValue(clientID, rc))
		{
			rc->m_rttStartTimeMs = GetPreciseCurrentTimeMs();
			rc->m_rttEndTimeMs = rc->m_rttStartTimeMs + (int64_t)parameter.testDurationMs;
			rc->m_rttIntervalMs = (int64_t)parameter.pingIntervalMs;

			// 카운터 초기화
			rc->m_rttTestCount = 0;

			// 누적량 초기화
			rc->m_rttTotalTimeMs = 0;

			rc->m_isTestingRtt = true;

			return ErrorType_Ok;
		}
		return ErrorType_InvalidHostID;
	}

	ErrorType CNetClientImpl::StopRoundTripLatencyTest(HostID clientID)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		shared_ptr<CHostBase> rc = nullptr;
		if (true == m_authedHostMap.TryGetValue(clientID, rc))
		{
			rc->m_isTestingRtt = false;

			return ErrorType_Ok;
		}
		return ErrorType_InvalidHostID;
	}

	ErrorType CNetClientImpl::GetRoundTripLatency(HostID clientID, /*out*/RoundTripLatencyTestResult& result)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		shared_ptr<CHostBase> rc = nullptr;
		if (!m_authedHostMap.TryGetValue(clientID, rc))
			return ErrorType_InvalidHostID;

		rc->m_isTestingRtt = false;

		// 0 나누기를 막는다
		if (0 == rc->m_rttTestCount)
		{
			result.totalTestCount = 0;
			return ErrorType_ValueNotExist;
		}

		// 최근 CNetConfig::RountTripLatencyTestMaxCount개에 대해서만 평균을 구합니다.
		result.latencyMs = (int32_t)(rc->m_rttTotalTimeMs / rc->m_latencies.size());
		result.totalTestCount = rc->m_rttTestCount;

		// 최근 CNetConfig::RountTripLatencyTestMaxCount개에 대한 표준편차를 계산합니다.
		int64_t sumOfDiff = 0;
		for(auto iter = rc->m_latencies.begin(); iter != rc->m_latencies.end(); ++iter)
		{
			sumOfDiff += (int64_t)pow((double)((*iter) - (int64_t)result.latencyMs), 2);
		}
		result.standardDeviationMs = (int64_t)sqrt((double)(sumOfDiff / (int64_t)rc->m_latencies.size()));

		return ErrorType_Ok;
	}

	ErrorType CNetClientImpl::GetUnreliableMessagingLossRatioPercent(HostID remotePeerID, int *outputPercent)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (remotePeerID == HostID_Server)
		{
			// 서버와의 UDP 통신이 활성화 중이면
			if (m_remoteServer->GetToServerUdpFallbackable() != nullptr && m_remoteServer->GetToServerUdpFallbackable()->IsRealUdpEnabled())
			{
				shared_ptr<CSuperSocket> pUdpSocket = m_remoteServer->m_ToServerUdp;
				if (pUdpSocket != nullptr)
				{
					int nPercent = pUdpSocket->GetUnreliableMessagingLossRatioPercent(m_remoteServer->GetToServerUdpFallbackable()->m_serverAddr);
					*outputPercent = nPercent;
					return ErrorType_Ok;
				}
			}
			else
			{
				// 서버와 unreliable 통신이 TCP로 되고 있을때는 로스율은 0입니다.
				*outputPercent = 0;
				return ErrorType_Ok;
			}
		}

		if (remotePeerID == GetVolatileLocalHostID())
		{
			// 루프백은 당삼 0이지.
			*outputPercent = 0;
			return ErrorType_Ok;
		}

		shared_ptr<CRemotePeer_C> pPeer = GetPeerByHostID_NOLOCK(remotePeerID);
		if (pPeer != nullptr)
		{
			shared_ptr<CSuperSocket> pUdpSocket = pPeer->m_udpSocket;
			if (pUdpSocket != nullptr && !pPeer->IsRelayedP2P())
			{
				int nPercent = pUdpSocket->GetUnreliableMessagingLossRatioPercent(pPeer->m_P2PHolepunchedRemoteToLocalAddr);
				*outputPercent = nPercent;
				return ErrorType_Ok;
			}
			else
			{
				shared_ptr<CSuperSocket> toServerUdpSocket = m_remoteServer->m_ToServerUdp;
				if (toServerUdpSocket != nullptr)
				{
					int percent = 0;
					GetUnreliableMessagingLossRatioPercent(HostID_Server, &percent);
					int stats = 100 - (100 - percent) * (100 - pPeer->m_CSPacketLossPercent) / 100;
					*outputPercent = stats;
					return ErrorType_Ok;
				}
				else
				{
					// 여기-서버와의 UDP가 안뚫린 경우에는 여기-서버와의 UDP는 0%입니다.
					*outputPercent = pPeer->m_CSPacketLossPercent;
					return ErrorType_Ok;
				}
			}
		}

		return ErrorType_InvalidHostID;
	}

	ErrorType CNetClientImpl::SetCoalesceIntervalMs(HostID remote, int interval)
	{
		if (interval < 0 || interval > 1000)
		{
			throw Exception("Coalesce interval out of range! Only 0~1000 is acceptable.");
		}

		CriticalSectionLock clk(GetCriticalSection(), true);

		if (remote == HostID_Server)
		{
			m_remoteServer->m_autoCoalesceInterval = false;
			m_remoteServer->SetManualOrAutoCoalesceInterval(interval);
			if (m_remoteServer->m_ToServerUdp != nullptr)
			{
				m_remoteServer->m_ToServerUdp->SetCoalesceInterval(m_remoteServer->GetServerUdpAddr(), m_remoteServer->GetCoalesceIntervalMs());
			}
			return ErrorType_Ok;
		}

		shared_ptr<CRemotePeer_C> rp = GetPeerByHostID_NOLOCK(remote);
		if (rp != nullptr)
		{
			rp->m_autoCoalesceInterval = false;
			rp->SetManualOrAutoCoalesceInterval(interval);
			if (rp->m_udpSocket != nullptr)
			{
				rp->m_udpSocket->SetCoalesceInteraval(rp->GetDirectP2PLocalToRemoteAddr(), rp->GetCoalesceIntervalMs());
			}

			return ErrorType_Ok;
		}

		return ErrorType_InvalidHostID;
	}

	ErrorType CNetClientImpl::SetCoalesceIntervalToAuto(HostID remote)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		if (remote == HostID_Server)
		{
			m_remoteServer->m_autoCoalesceInterval = true;
			m_remoteServer->SetManualOrAutoCoalesceInterval();
			if (m_remoteServer->m_ToServerUdp != nullptr)
			{
				m_remoteServer->m_ToServerUdp->SetCoalesceInterval(m_remoteServer->GetServerUdpAddr(), m_remoteServer->GetCoalesceIntervalMs());
			}
			return ErrorType_Ok;
		}

		shared_ptr<CRemotePeer_C> rp = GetPeerByHostID_NOLOCK(remote);
		if (rp != nullptr)
		{
			rp->m_autoCoalesceInterval = true;
			rp->SetManualOrAutoCoalesceInterval();
			if (rp->m_udpSocket != nullptr)
			{
				rp->m_udpSocket->SetCoalesceInteraval(rp->GetDirectP2PLocalToRemoteAddr(), rp->GetCoalesceIntervalMs());
			}

			return ErrorType_Ok;
		}

		return ErrorType_InvalidHostID;
	}

	void CNetClientImpl::OnCustomValueEvent(const ThreadPoolProcessParam& param, CWorkResult* workResult, CustomValueEvent customValue)
	{
		// timer tick or no coalesced send 상황에서 쌓이는 신호를 처리.
		// Moved to Proud.CThreadPoolImpl.EveryThreadReferrer_HeartbeatAndIssueSendOnNeed.
//		switch ((int)customValue)
//		{
//		case CustomValueEvent_Heartbeat:
//			/* CustomValueEvent_Heartbeat를 push할 때 인자로 net worker therad가 들어간다.
//			따라서 이 case를 처리하는 스레드 또한 net worker thread이다.
//
//			Heartbeat은 NS,NC,LS,LC 공통으로 필요로 합니다.
//			Heartbeat은 OnTick과 달리 PN 내부로직만을 처리합니다.
//			속도에 민감하므로 절대 블러킹 즉 CPU idle time이 없어야 하고요.
//
//			따라서 아래 Heartbeat 함수 중에서 ProcessCustomValueEvent에 옮겨야 할 루틴은 옮기시고,
//			나머지는 CNetServerImpl.Heartbeat에 그대로 남깁시다.
//			*/
//			if (AtomicCompareAndSwap32(0, 1, &m_isProcessingHeartbeat) == 0)
//			{
//				// 하나의 1-thread process 를 위하여 여기서 이 함수를 call 한다.
//				if (m_connectionParam.m_enableAutoConnectionRecovery == true)
//				{
//					CacheLocalIpAddress_MustGuaranteeOneThreadCalledByCaller();
//				}
//
//				Heartbeat();
//
//				AtomicCompareAndSwap32(1, 0, &m_isProcessingHeartbeat);
//			}
//
//			break;
//		}

		// 상위 클래스의 함수 콜
		CNetCoreImpl::ProcessCustomValueEvent(param, workResult, customValue);
	}

	// CNetCoreImpl method impl
	void CNetClientImpl::OnSocketGarbageCollected(const shared_ptr<CSuperSocket>& socket)
	{
		int udpPort = socket->GetLocalAddr().m_port;
		if (m_usedUdpPorts.ContainsKey((uint16_t)udpPort))
		{
			// 사용자 정의 UDP포트에 지정된 포트라면 반환합니다.
			m_unusedUdpPorts.Add((uint16_t)udpPort);
			m_usedUdpPorts.Remove((uint16_t)udpPort);
		}
	}

	void CNetClientImpl::OnAccepted(const shared_ptr<CSuperSocket>& /*socket*/, AddrPort /*localAddr*/, AddrPort /*remoteAddr*/)
	{
		// NC가 이걸 받을 일이 있나? 이 함수는 빈 상태로 둔다.
	}

	void CNetClientImpl::ProcessOneLocalEvent(LocalEvent& e, const shared_ptr<CHostBase>& /*subject*/, const PNTCHAR*& outFunctionName, CWorkResult* workResult)
	{
		LockMain_AssertIsNotLockedByCurrentThread();


		INetClientEvent* eventSink_NOCSLOCK = (INetClientEvent*)m_eventSink_NOCSLOCK;

		bool processed = true;
		switch ((int)e.m_type)
		{
		case LocalEventType_ConnectServerSuccess:
		{
			ByteArray tempByteArray(e.m_userData.GetData(), e.m_userData.GetCount());
			outFunctionName = _PNT("OnJoinServerComplete");

			if (OnJoinServerComplete.m_functionObject)
				OnJoinServerComplete.m_functionObject->Run(ErrorInfoPtr(new ErrorInfo()), tempByteArray);

			if (m_eventSink_NOCSLOCK != nullptr)
			{
				eventSink_NOCSLOCK->OnJoinServerComplete(ErrorInfoPtr(new ErrorInfo()), tempByteArray);
			}
		}
		break;
		case LocalEventType_ConnectServerFail:
		{
			ErrorInfoPtr ep = e.m_errorInfo;
			ep->m_remote = HostID_Server;
			ep->m_remoteAddr = e.m_remoteAddr;
			ep->m_socketError = e.m_socketErrorCode;

			ByteArray callbackArray;
			if (!e.m_userData.IsNull() && e.m_userData.GetCount() != 0)
			{
				callbackArray.SetCount(e.m_userData.GetCount());
				e.m_userData.CopyRangeTo(callbackArray, 0, e.m_userData.GetCount());
			}
			outFunctionName = _PNT("OnJoinServerComplete");

			if (OnJoinServerComplete.m_functionObject)
				OnJoinServerComplete.m_functionObject->Run(ep, callbackArray);

			if (m_eventSink_NOCSLOCK != nullptr)
			{
				eventSink_NOCSLOCK->OnJoinServerComplete(ep, callbackArray);
			}
		}
		break;
		case LocalEventType_ClientServerDisconnect:
#ifdef USE_HLA
			if (m_hlaSessionHostImpl != nullptr)
				m_hlaSessionHostImpl->Clear(); // 여기서 콜 하는 것이 좀 땜빵 같지만 그래도 나쁘지는 않음. no main lock이기도 하니까.
#endif // USE_HLA

			outFunctionName = _PNT("OnLeaveServer");
			if (OnLeaveServer.m_functionObject)
				OnLeaveServer.m_functionObject->Run(e.m_errorInfo);

			if (m_eventSink_NOCSLOCK != nullptr)
				eventSink_NOCSLOCK->OnLeaveServer(e.m_errorInfo);
			break;
		case LocalEventType_AddMember:
			outFunctionName = _PNT("OnP2PMemberJoin");

			if (OnP2PMemberJoin.m_functionObject)
				OnP2PMemberJoin.m_functionObject->Run(e.m_memberHostID, e.m_groupHostID, e.m_memberCount, e.m_customField);

			if (m_eventSink_NOCSLOCK != nullptr)
				eventSink_NOCSLOCK->OnP2PMemberJoin(e.m_memberHostID, e.m_groupHostID, e.m_memberCount, e.m_customField);
			break;
		case LocalEventType_DelMember:
			outFunctionName = _PNT("OnP2PMemberLeave");

			if (OnP2PMemberLeave.m_functionObject)
				OnP2PMemberLeave.m_functionObject->Run(e.m_memberHostID, e.m_groupHostID, e.m_memberCount);

			if (m_eventSink_NOCSLOCK != nullptr)
				eventSink_NOCSLOCK->OnP2PMemberLeave(e.m_memberHostID, e.m_groupHostID, e.m_memberCount);
			break;

		case LocalEventType_DirectP2PEnabled:
			outFunctionName = _PNT("OnChangeP2PRelayState");

			if (OnChangeP2PRelayState.m_functionObject)
				OnChangeP2PRelayState.m_functionObject->Run(e.m_remoteHostID, ErrorType_Ok);

			if (m_eventSink_NOCSLOCK != nullptr)
				eventSink_NOCSLOCK->OnChangeP2PRelayState(e.m_remoteHostID, ErrorType_Ok);
			break;

		case LocalEventType_RelayP2PEnabled:
			outFunctionName = _PNT("OnChangeP2PRelayState");

			if (OnChangeP2PRelayState.m_functionObject)
				OnChangeP2PRelayState.m_functionObject->Run(e.m_remoteHostID, e.m_errorInfo->m_errorType);

			if (m_eventSink_NOCSLOCK != nullptr)
				eventSink_NOCSLOCK->OnChangeP2PRelayState(e.m_remoteHostID, e.m_errorInfo->m_errorType);
			break;

		case LocalEventType_P2PMemberOffline:
		{
			CRemoteOfflineEventArgs args;
			args.m_remoteHostID = e.m_remoteHostID;

			if (OnP2PMemberOffline.m_functionObject)
				OnP2PMemberOffline.m_functionObject->Run(args);

			if (m_eventSink_NOCSLOCK != nullptr)
				eventSink_NOCSLOCK->OnP2PMemberOffline(args);
		}
		break;
		case LocalEventType_P2PMemberOnline:
		{
			CRemoteOnlineEventArgs args;
			args.m_remoteHostID = e.m_remoteHostID;

			if (OnP2PMemberOnline.m_functionObject)
				OnP2PMemberOnline.m_functionObject->Run(args);

			if (m_eventSink_NOCSLOCK != nullptr)
				eventSink_NOCSLOCK->OnP2PMemberOnline(args);
		}
		break;
		case LocalEventType_ServerOffline:
		{
			CRemoteOfflineEventArgs args;
			args.m_remoteHostID = e.m_remoteHostID;
			args.m_errorInfo = e.m_errorInfo;

			if (OnServerOffline.m_functionObject)
				OnServerOffline.m_functionObject->Run(args);

			if (m_eventSink_NOCSLOCK != nullptr)
				eventSink_NOCSLOCK->OnServerOffline(args);
		}
		break;
		case LocalEventType_ServerOnline:
		{
			CRemoteOnlineEventArgs args;
			args.m_remoteHostID = e.m_remoteHostID;

			if (OnServerOnline.m_functionObject)
				OnServerOnline.m_functionObject->Run(args);

			if (m_eventSink_NOCSLOCK != nullptr)
				eventSink_NOCSLOCK->OnServerOnline(args);
		}
		break;

		case LocalEventType_SynchronizeServerTime:
			outFunctionName = _PNT("OnSynchronizeServerTime");

			if (OnSynchronizeServerTime.m_functionObject)
				OnSynchronizeServerTime.m_functionObject->Run();

			if (m_eventSink_NOCSLOCK != nullptr)
				eventSink_NOCSLOCK->OnSynchronizeServerTime();
			break;
		case LocalEventType_Error:

			if (OnError.m_functionObject)
				OnError.m_functionObject->Run(ErrorInfo::From(e.m_errorInfo->m_errorType, e.m_remoteHostID, e.m_errorInfo->m_comment));

			if (m_eventSink_NOCSLOCK != nullptr)
				eventSink_NOCSLOCK->OnError(ErrorInfo::From(e.m_errorInfo->m_errorType, e.m_remoteHostID, e.m_errorInfo->m_comment));
			break;
		case LocalEventType_Warning:

			if (OnWarning.m_functionObject)
				OnWarning.m_functionObject->Run(ErrorInfo::From(e.m_errorInfo->m_errorType, e.m_remoteHostID, e.m_errorInfo->m_comment));

			if (m_eventSink_NOCSLOCK != nullptr)
				eventSink_NOCSLOCK->OnWarning(ErrorInfo::From(e.m_errorInfo->m_errorType, e.m_remoteHostID, e.m_errorInfo->m_comment));
			break;
		case LocalEventType_ServerUdpChanged:
		{
			outFunctionName = _PNT("OnChangeServerUdpState");

			if (OnChangeServerUdpState.m_functionObject)
				OnChangeServerUdpState.m_functionObject->Run(e.m_errorInfo->m_errorType);

			if (m_eventSink_NOCSLOCK != nullptr)
				eventSink_NOCSLOCK->OnChangeServerUdpState(e.m_errorInfo->m_errorType);
		}
		break;
		default:
		{
			processed = false;
		}
		return;
		}

		if (workResult != nullptr && processed)
			++workResult->m_processedEventCount;

	}

	void CNetClientImpl::OnHostGarbageCollected(const shared_ptr<CHostBase>& remote)
	{
		AssertIsLockedByCurrentThread();

		if (remote == m_loopbackHost)
		{
			// LoopbackHost는 별도의 Socket을 가지지 않기 때문에 여기서는 처리하지 않습니다.
		}

		shared_ptr<CRemoteServer_C> remoteServer = dynamic_pointer_cast<CRemoteServer_C>(remote);
		if (remoteServer)
		{
			// remote server가 가진 TCP socket을 garbage 처리 및 reference 해지한다.
			if (remoteServer->m_ToServerTcp)
			{
				GarbageSocket(remoteServer->m_ToServerTcp);
				remoteServer->m_ToServerTcp = shared_ptr<CSuperSocket>();
			}

			if (remoteServer->m_ToServerUdp)
			{
				GarbageSocket(remoteServer->m_ToServerUdp);
				remoteServer->m_ToServerUdp = shared_ptr<CSuperSocket>();
			}

			remoteServer->CleanupToServerUdpFallbackable();

		}

		shared_ptr<CRemotePeer_C> remotePeer = LeanDynamicCast_RemotePeer_C(remote);
		if (remotePeer)
		{
			// remote peer가 가진 UDP socket을 recycle bin에 넣고 reference 해지한다.

			assert(remotePeer->m_garbaged);
			assert(!m_userTaskQueue.DoesHostHaveUserWorkItem(remotePeer.get()));

			// remote peer는 재사용될때 Set_Active(false)로 될 뿐, remote peer와 소유한 UDP socket들은 모두 살아있다.
			// 그런데 remote peer가 garbage된다는 것은, inactive 상태에서의 시한 (수십초)도 끝났음을 의미한다.
			// 그러면 UDP socket도 그냥 다 없앤다.
			if (remotePeer->m_udpSocket)
			{
				GarbageSocket(remotePeer->m_udpSocket);
				remotePeer->m_udpSocket = shared_ptr<CSuperSocket>();
			}

			remotePeer->m_owner = nullptr;
			remotePeer->m_p2pConnectionTrialContext = CP2PConnectionTrialContextPtr();
			remotePeer->SetRelayedP2P(true);

			// recycle에 있었으면 거기서도 제거
			m_remotePeerRecycles.RemoveKey(remotePeer->m_HostID);
		}

		// 재접속용 host garbage인 경우도 있다. 목록에서 삭제.
		m_autoConnectionRecovery_temporaryRemoteServers.RemoveKey(remote.get());
	}

	shared_ptr<CHostBase> CNetClientImpl::GetTaskSubjectByHostID_NOLOCK(HostID hostID)
	{
		// NetServer,LanClient,LanServer에서만 사용되던 함수. 이 함수는 빈 상태로 둔다.
		// 역시 안되죠. 구현해 주시기 바랍니다.
		AssertIsLockedByCurrentThread();
		CHECK_CRITICALSECTION_DEADLOCK(this);

		if (hostID == HostID_None)
			return shared_ptr<CHostBase>();

		if (hostID == HostID_Server)
			return m_remoteServer;

		if (hostID == GetVolatileLocalHostID())
			return m_loopbackHost;

		return AuthedHostMap_Get(hostID);
	}

	bool CNetClientImpl::IsValidHostID_NOLOCK(HostID hostID)
	{
		// NetServer,LanClient,LanServer에서만 사용되던 함수. 이 함수는 빈 상태로 둔다.
		// 역시 안되죠. 구현해 주시기 바랍니다.
		AssertIsLockedByCurrentThread();
		CHECK_CRITICALSECTION_DEADLOCK(this);

		/* remoteServer와 loopbackhost는 HostID 발급받은 이후에 CandidateHost에서 AuthedHost로 옮겨집니다.
		즉, HostID를 발급받지 못한 상황이 발생하게되면 (예. ErrorType_TCPConnectFailure) LocalEvent가 정상적으로 발생되지가 않게됩니다.

		=> 이런 문제가 있었군요.
		그렇다면,
		loopback은 항상 존재해야 하고, 자기 자신의 hostID를 아는 이상 항상 valid하다고 리턴해야 합니다.
		remote server도 항상 존재하게 하고 HostID_Server라는 고정값을 갖고 있으므로 서버 연결 여부와 상관없이 항상 valid하다고 리턴하게 합시다.

		즉 remoteServer는 항상 authed에 있어야 하겠고, loopback도 항상 authed에 있게 합시다.
		단, LC,NC에서는 loopback은 자신의 hostID를 모를 때 즉 서버와의 연결이 안 되어 있을 때는 candidates에 있고
		그 외에는 항상 authed에 있게 합시다.

		LC,LS에서도 이렇게 수정해야 합니다. 당장에는 하지 마시고, 뭘 해야 하는지에 대한 메모만 추가해주세요.
		*/
		if (hostID == HostID_None)
			return false;

		if (hostID == HostID_Server)
			return true;

		if (hostID == GetVolatileLocalHostID())
			return true;

		if (AuthedHostMap_Get(hostID).get() != nullptr)
			return true;

		return false;
	}

	void CNetClientImpl::BadAllocException(Exception& ex)
	{
		FreePreventOutOfMemory();

		if (OnException.m_functionObject)
			OnException.m_functionObject->Run(ex);

		if (m_eventSink_NOCSLOCK != nullptr)
			m_eventSink_NOCSLOCK->OnException(ex);
		m_worker->SetState(CNetClientWorker::Disconnecting);
	}

	bool CNetClientImpl::IsSimplePacketMode()
	{
		return m_simplePacketMode;
	}

	// TCP 클라-연결이 위험한 상태인지, 즉 핑퐁을 포함해서 수신이 지나칠 정도로 지연되고 있는지?
	// NOTE: TCP는 20초 이상 패킷 로스가 심각하면 retransmission timeout 즉 10053 에러가 뜬다.
	// interval: 현재 시간 - 마지막으로 스트림 받은 시간
	bool CNetClientImpl::IsTcpUnstable(int64_t interval)
	{
		int64_t unstableTime = (int64_t)GetReliablePingTimerIntervalMs() + CNetConfig::TcpUnstableDetectionWaitTimeMs;
		if (interval > unstableTime)
			return true;

		return false;
	}

	bool CNetClientImpl::Stub_ProcessReceiveMessage(IRmiStub* stub, CReceivedMessage& receivedMessage, void* hostTag, CWorkResult* workResult)
	{
		bool processed = stub->ProcessReceivedMessage(receivedMessage, hostTag);

		if (workResult != nullptr)
		{
			if (processed && stub != ((IRmiStub*)&m_s2cStub) && ((IRmiStub*)&m_c2cStub))
			{
				++workResult->m_processedMessageCount;
			}
		}

		return processed;
	}

	// current thread가 user callback을 실행중인지, 즉 콜스택 어딘가에 user callback 이 있는지?
	bool CNetClientImpl::CurrentThreadIsRunningUserCallback()
	{
		if (m_userThreadPool && m_userThreadPool->ContainsCurrentThread())
			return true;
		if (Proud::GetCurrentThreadID() == m_zeroThreadPool_processingThreadID)
			return true;

		return false;
	}

	/* #TCP_KEEP_ALIVE
	모든 Socket들의 last received time이 out이면 SocketResult_TimeOut을,
	하나라도 time out이 아니면 SocketResult_NotTimeOut을,
	검사할 socket이 하나도 없으면 SocketResult_NoneToDo를 리턴한다.

	Q: C/S TCP socket만 검사해야 하는거 아닌가요?
	A: UDP 통신량이 과점을 해버려서 TCP 송수신이 우선순위에서 밀릴 수 있습니다.
	이때는 TCP congestion으로 인한 ECONNABORTED 즉 TCP 밑단에서의 retransmission timeout에 의존합시다.
	그리고 이 함수의 목적은, 총 통신 자체의 불량을 감지하는 목적입니다. */
	CNetClientImpl::SocketResult CNetClientImpl::GetAllSocketsLastReceivedTimeOutState()
	{
		CriticalSectionLock lock(m_critSec, true);

		// 단 하나라도 not timed out이면 false를 리턴한다.
		// remote가 가진 socket이 null인 경우, 검사를 건너뛴다.
		int64_t currTime = GetPreciseCurrentTimeMs();
		SocketResult result = NoneToDo;
		UpdateSocketLastReceivedTimeOutState(m_remoteServer->m_ToServerTcp, currTime, m_settings.m_defaultTimeoutTimeMs, &result);
		if (result == NotTimeOut)
		{
			// 소켓이 존재하며, time out이 아니니, 더 검사할 필요 없음. 아래도 다 마찬가지.
			return result;
		}
		UpdateSocketLastReceivedTimeOutState(m_remoteServer->m_ToServerUdp, currTime, m_settings.m_defaultTimeoutTimeMs, &result);
		if (result == NotTimeOut)
			return result;

		// 이제 각 remote peer에 대해 동일한 검사를.
		for (AuthedHostMap::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++)
		{
			shared_ptr<CRemotePeer_C> rp = dynamic_pointer_cast<CRemotePeer_C>(i->GetSecond());
			if (rp)
			{
				UpdateSocketLastReceivedTimeOutState(rp->m_udpSocket, currTime, m_settings.m_defaultTimeoutTimeMs, &result);
			}

			if (result == NotTimeOut)
				return result;
		}

		// 리턴값은 TimeOut or NoneToDo 중 하나일 것이다.
		return result;
	}

	ZeroThreadPoolUsageMarker::ZeroThreadPoolUsageMarker(CNetClientImpl* nc)
	{
		m_netClient = nc;
		if (nc->m_zeroThreadPool_processingThreadRecursionCount == 0)
		{
			nc->m_zeroThreadPool_processingThreadID = Proud::GetCurrentThreadID();
		}
		nc->m_zeroThreadPool_processingThreadRecursionCount++;
	}

	ZeroThreadPoolUsageMarker::~ZeroThreadPoolUsageMarker()
	{
		CNetClientImpl* nc = m_netClient;
		nc->m_zeroThreadPool_processingThreadRecursionCount--;
		if (nc->m_zeroThreadPool_processingThreadRecursionCount == 0)
			nc->m_zeroThreadPool_processingThreadID = 0;
	}
}
