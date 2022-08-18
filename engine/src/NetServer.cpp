/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <stack>
#include "../include/INetServerEvent.h"
#include "../include/Message.h"
#include "../include/NetConfig.h"
#include "../include/NetPeerInfo.h"
#include "../include/P2PGroup.h"
//#include "SendFragRefs.h"
#include "../include/sysutil.h"
//#include "../include/ErrorInfo.h"
#include "../include/MessageSummary.h"

#include "LeanDynamicCast.h"
#include "NetServer.h"
#include "P2PGroup_S.h"
#include "ReaderWriterMonitorTester.h"
#include "ReliableUDP.h"
#include "SendDest_S.h"
#include "ReliableUDPFrame.h"
#include "PooledObjects_C.h"
#include "ReliableUdpHelper.h"
#include "Functors.h"
#include "RemoteClient.h"
//#include "ServerSocketPool.h"
#include "TestUdpConnReset.h"
#include "UdpSocket_S.h"
//#include "networker_c.h"
#include "STLUtil.h"
#ifdef VIZAGENT
#include "VizAgent.h"
#endif
#include "RmiContextImpl.h"
#include "ReportError.h"
#include <typeinfo>

#include "pidl/NetC2S_stub.cpp"
#include "pidl/NetS2C_proxy.cpp"

#include "CollUtil.h"
//#include "LowContextSwitchingLoop.h"
#include "ReusableLocalVar.h"
#include "SendDestInfo.h"
#include "quicksort.h"

#ifndef PROUDSRCCOPY
#include "../PNLic/PNLicenseManager/PNLic.h"
#include "../PNLic/PNLicenseManager/Binary.inl"
#include "../PNLic/PNLicenseManager/PNLic.inl"

#include "../include/pnguid-win32.h"
#include "../PNLic/PNLicenseManager/AppExecuter.inl"

#endif

#include "RunOnScopeOut.h"
#include "ReceivedMessageList.h"
#include "CompactFieldMap.h"
#include "../include/numutil.h"

#define PARALLEL_RSA_DECRYPTION // 이걸 끄면 RSA 복호화 과정을 병렬화를 안한다.




namespace Proud
{
    StringA NotFirstRequestText = "!!";

#define ASSERT_OR_HACKED(x) { if(!(x)) { EnqueueHackSuspectEvent(rc,#x,HackType_PacketRig); } }

    int GetChecksum(ByteArray& array)
    {
        int ret = 0;
        for ( int i = 0; i < (int)array.GetCount(); i++ )
        {
            ret += array[i];
        }
        return ret;
    }

    DEFRMI_ProudC2S_NotifySendSpeed(CNetServerImpl::C2SStub)
    {
        _pn_unused(remote);
        _pn_unused(rmiContext);
        _pn_unused(fieldMap);

        CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(m_owner);

        // check validation
        shared_ptr<CRemoteClient_S> rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);
        if ( rc != nullptr )
        {
            rc->m_sendSpeed = speed;
        }
        return true;
    }


    DEFRMI_ProudC2S_NotifyLog(CNetServerImpl::C2SStub)
    {
        _pn_unused(remote);
        _pn_unused(rmiContext);
        _pn_unused(fieldMap);

        CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(m_owner);

        if ( m_owner->m_logWriter != nullptr )
        {
            m_owner->m_logWriter->WriteLine(logLevel, logCategory, logHostID, logMessage, logFunction, logLine);
        }

        return true;
    }

    DEFRMI_ProudC2S_NotifyLogHolepunchFreqFail(CNetServerImpl::C2SStub)
    {
        _pn_unused(rmiContext);
        _pn_unused(fieldMap);

        CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(m_owner);

        CNetServerStats stat;
        m_owner->GetStats(stat);

        if ( m_owner->m_logWriter != nullptr )
        {
            Tstringstream ss;
            ss << "[Client " << remote << "] Pair=" << stat.m_p2pDirectConnectionPairCount <<
                "/" << stat.m_p2pConnectionPairCount <<
                "##CCU=" << stat.m_clientCount << "##"<<text.GetString();

            m_owner->m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, ss.str().c_str());  // printf가 너무 길어지므로
        }

        // 가장 랭킹 높게 측정된 로그인지만 체크해서 저장한다. 아직 기록은 아님.
        shared_ptr<CRemoteClient_S> rc;
        if ( m_owner->m_freqFailLogMostRank < rank && (rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote)) != nullptr )
        {
            String txt2;
            txt2.Format(_PNT("[Client %d %s] Pair=%d/%d##CCU=%d"),
                        remote,
                        rc->m_tcpLayer->m_remoteAddr.ToString().GetString(),
                        stat.m_p2pDirectConnectionPairCount,
                        stat.m_p2pConnectionPairCount,
                        stat.m_clientCount);

            m_owner->m_freqFailLogMostRank = rank;
            m_owner->m_freqFailLogMostRankText = txt2 + text; // printf가 너무 길어지므로

            AtomicCompareAndSwap32(0, 1, &m_owner->m_fregFailNeed);
        }
        return true;
    }

    DEFRMI_ProudC2S_ReliablePing(CNetServerImpl::C2SStub)
    {
        _pn_unused(rmiContext);
        _pn_unused(fieldMap);

        CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(m_owner);

        shared_ptr<CRemoteClient_S> rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);
        if ( rc != nullptr )
        {
            //rc->m_safeTimes.Set_lastTcpStreamReceivedTime(m_owner->GetCachedServerTimeMs());
            // 사용자가 입력한 frameRate update

            int ackNum = 0;

            // ACR 이 켜져 있을때만 아래 부분을 처리하는것이 좋은데 문제는 Server 측에서 알 수 있는 방법이 없음
            // 일단 messageID 가 0 이 아니라면 ACR On 으로 간주하고 처리 하는데
            // 만약 0 값도 valid 한 값이라면 문제가 있다.
            if ( messageID != 0 )
            {
                // 수신 미보장 부분중 해당 부분을 제거
                rc->m_tcpLayer->AcrMessageRecovery_RemoveBeforeAckedMessageID(messageID);
                rc->m_tcpLayer->AcrMessageRecovery_PeekMessageIDToAck(&ackNum);
            }

            rc->m_lastReliablePingMs = lastReliablePingMs;
            rc->m_lastApplicationHint.m_recentFrameRate = recentFrameRate;
            m_owner->m_s2cProxy.ReliablePong(remote, g_ReliableSendForPN, localTimeMs, ackNum,
                CompactFieldMap());
        }
        return true;
    }

    DEFRMI_ProudC2S_ShutdownTcp(CNetServerImpl::C2SStub)
    {
        _pn_unused(rmiContext);
        _pn_unused(fieldMap);

        CriticalSectionLock clk(m_owner->GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(m_owner);

        shared_ptr<CRemoteClient_S> rc = m_owner->GetAuthedClientByHostID_NOLOCK(remote);
        if (rc != nullptr)
        {
            rc->m_shutdownComment = comment;
            rc->m_enableAutoConnectionRecovery = false;
            m_owner->m_s2cProxy.ShutdownTcpAck(remote, g_ReliableSendForPN,
                CompactFieldMap());
        }
        return true;
    }

    DEFRMI_ProudC2S_RoundTripLatencyPing(CNetServerImpl::C2SStub)
    {
        _pn_unused(remote);
        _pn_unused(rmiContext);

        // 클라이언트로부터 받은 것을 그대로 보냅니다
        m_owner->m_s2cProxy.RoundTripLatencyPong(remote, g_UnreliableSendForPN, pingTime);

        return true;
    }

    bool CNetServerImpl::GetJoinedP2PGroups(HostID clientHostID, HostIDArray &output)
    {
        output.Clear();

        if ( clientHostID == HostID_None )
            return false;

        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        shared_ptr<CHostBase> hb = AuthedHostMap_Get(clientHostID);
        if ( hb.get() == nullptr )
            return false;

        shared_ptr<CRemoteClient_S> c = LeanDynamicCast_RemoteClient_S(hb);
        if ( c == nullptr )
            return false;

        for ( JoinedP2PGroups_S::iterator ii = c->m_joinedP2PGroups.begin(); ii != c->m_joinedP2PGroups.end(); ii++ )
        {
            HostID i = ii->GetFirst();
            output.Add(i);
        }
        return true;
    }

    void CNetServerImpl::GetP2PGroups(CP2PGroups &ret)
    {
        ret.Clear();

        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);
        for ( P2PGroups_S::iterator ig = m_P2PGroups.begin(); ig != m_P2PGroups.end(); ig++ )
        {
            CP2PGroupPtr_S g = ig->GetSecond();
            CP2PGroupPtr gd = g->ToInfo();
            ret.Add(gd->m_groupHostID, gd);
        }
    }

    int CNetServerImpl::GetP2PGroupCount()
    {
        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);
        return (int)m_P2PGroups.GetCount();
    }

    shared_ptr<CRemoteClient_S> CNetServerImpl::GetRemoteClientBySocket_NOLOCK(const shared_ptr<CSuperSocket>& socket, const AddrPort& remoteAddr)
    {
        AssertIsLockedByCurrentThread();

        shared_ptr<CRemoteClient_S> hostBase;
        if (!TryGetRemoteClientBySocket_NOLOCK(socket, remoteAddr, hostBase))
            return shared_ptr<CRemoteClient_S>();
        else
            return hostBase;
    }

    // code profile 후 추가.
    bool CNetServerImpl::TryGetRemoteClientBySocket_NOLOCK(const shared_ptr<CSuperSocket>& socket, const AddrPort& remoteAddr, shared_ptr<CRemoteClient_S>& output)
    {
        AssertIsLockedByCurrentThread();
        bool isWildcard_IGNORE;
        shared_ptr<CHostBase> hostBase;
        if (!SocketToHostsMap_TryGet_NOLOCK(socket, remoteAddr, hostBase, isWildcard_IGNORE))
            return false;

        output = LeanDynamicCast_RemoteClient_S(hostBase);
        if (!output)
            return false;
        else
            return true;
    }

    void CNetServerImpl::PurgeTooOldAddMemberAckItem()
    {
        //try
        {
            CriticalSectionLock clk(GetCriticalSection(), true);
            CHECK_CRITICALSECTION_DEADLOCK(this);

            // clear ack-info if it is too old
            int64_t currTime = GetPreciseCurrentTimeMs();

            for ( P2PGroups_S::iterator ig = m_P2PGroups.begin(); ig != m_P2PGroups.end(); ig++ )
            {
                CP2PGroupPtr_S g = ig->GetSecond();
                for ( int im = 0; im < (int)g->m_addMemberAckWaiters.GetCount(); im++ )
                {
                    CP2PGroup_S::AddMemberAckWaiter& ack = g->m_addMemberAckWaiters[im];
                    if ( currTime - ack.m_eventTime > CNetConfig::PurgeTooOldAddMemberAckTimeoutMs )
                    {
                        HostID joiningPeerID = ack.m_joiningMemberHostID;
                        g->m_addMemberAckWaiters.RemoveAt(im); // 이거 하나만 지우고 리턴. 나머지는 다음 기회에서 지워도 충분하니까.

                        // 너무 오랫동안 ack가 안와서 제거되는 항목이 영구적 콜백 없음으로 이어져서는 안되므로 아래가 필요
                        if ( !g->m_addMemberAckWaiters.AckWaitingItemExists(joiningPeerID) )
                        {
                            EnqueAddMemberAckCompleteEvent(g->m_groupHostID, joiningPeerID, ErrorType_ConnectServerTimeout);
                        }
                        return;
                    }
                }
            }
        }
        // 		catch (...)	//사용자 정의 루틴을 콜 하는 곳이 없으므로 주석화
        // 		{
        // 			if(m_logWriter)
        // 			{
        // 				m_logWriter->WriteLine(TID_System, _PNT("FATAL ** Unhandled Exception at PurgeTooOldUnmatureClient!"));
        // 			}
        // 			throw;
        // 		}
    }


    // 너무 오랫동안 unmature 상태를 못 벗어나는 remote client들을 청소.]
    void CNetServerImpl::PurgeTooOldUnmatureClient()
    {
        CriticalSectionLock mainlock(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        intptr_t listCount = m_candidateHosts.GetCount();
        if ( listCount <= 0 )
            return;

        POOLED_LOCAL_VAR(CSuperSocketArray, timeoutSockets);

        int64_t currTime = GetPreciseCurrentTimeMs();

        // 중국의 경우 TcpSocketConnectTimeoutMs * 2 + 3 일때 63이 나오므로 상향조정함.
        assert(CNetConfig::ClientConnectServerTimeoutTimeMs < 70000);

        for ( auto i = m_candidateHosts.begin(); i != m_candidateHosts.end(); i++ )
        {
            // 아니 이런 무작정 다운캐스트를 하다니! 수정 바랍니다.
            // dynamic_cast를 사용하세요. dynamic_cast도 성능 문제로 가급적 피하는 것이 좋긴 합니다만.
            shared_ptr<CRemoteClient_S> rc = LeanDynamicCast_RemoteClient_S(i->GetSecond());
            if ( currTime - rc->m_createdTime > CNetConfig::ClientConnectServerTimeoutTimeMs && rc->m_purgeRequested == false )
            {
                rc->m_purgeRequested = true;

                // 조건을 만족하는 rc들만 list에 추가하자.
                if (rc->m_tcpLayer != nullptr)
                {
                    timeoutSockets.Add(rc->m_tcpLayer);
                }
            }
        }

        CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
        Message_Write(sendMsg, MessageType_ConnectServerTimedout);
        CSendFragRefs sd(sendMsg);

        // main unlock
        mainlock.Unlock();

        LowContextSwitchingLoop(timeoutSockets.GetData(), timeoutSockets.GetCount(),
            [](const shared_ptr<CSuperSocket>& object)
        {
            return &(object->GetSendQueueCriticalSection());
        },
            [&sd](const shared_ptr<CSuperSocket>& object, CriticalSectionLock& solock)->bool
        {
            object->AddToSendQueueWithSplitterAndSignal_Copy(object, sd, SendOpt());
            solock.Unlock();
            return true;
        });
    }

    void CNetServerImpl::OnHostGarbageCollected(const shared_ptr<CHostBase>& remote)
    {
        AssertIsLockedByCurrentThread();

        if ( remote == m_loopbackHost )
            return;

        // 목록에서 제거.
        m_HostIDFactory->Drop(remote->m_HostID);

        shared_ptr<CRemoteClient_S> rc = LeanDynamicCast_RemoteClient_S(remote);

        // OnHostGarbaged가 아니라 OnHostGarbageCollected에서 GarbageSocket을 호출해야 합니다.
        // 수정 바랍니다.
        // 단, socket to host map에서의 제거는 OnHostGarbaged에서 하는 것이 맞습니다.
        shared_ptr<CSuperSocket> tcpSocket = rc->m_tcpLayer;

        // TCP socket을 garbage 한다.

        if (rc->m_tcpLayer != nullptr )
        {
#ifndef PROUDSRCCOPY
            ((CPNLic*)m_lic)->POCL(rc->m_RT, rc->m_tcpLayer->m_remoteAddr);
#endif
            // TCP socket을 정리한다.
            GarbageSocket(rc->m_tcpLayer);
            SocketToHostsMap_RemoveForAnyAddr(rc->m_tcpLayer);

            // 참조 해제도 여기서 해 버려야.
            rc->m_tcpLayer.reset();
        }

        // remote client가 폐기되는 상황. 즉 ACR이 아니다. 따라서 UDP socket을 recycle이 아닌 garbage를 해 버리자.
        RemoteClient_CleanupUdpSocket(rc, true);

        // static assigned or per-remote UDP socket 중 하나를 참조하므로, 그냥 참조 해제만.
        rc->m_ToClientUdp_Fallbackable.m_udpSocket.reset();

        Unregister(rc);

        return ;
    }

    bool CNetServerImpl::TryGetCryptSessionKey(HostID remote, shared_ptr<CSessionKey>& output, String &errorOut, LogLevel& outLogLevel)
    {
        CriticalSectionLock clk(GetCriticalSection(), true);

        output = nullptr;

        if (remote == HostID_Server)
        {
            // loopback
            output = m_selfSessionKey;
        }
        else
        {
            // remote client
            shared_ptr<CRemoteClient_S> rc = GetAuthedClientByHostID_NOLOCK(remote);
            if (nullptr == rc || nullptr == rc->m_tcpLayer)
            {
                Tstringstream ss;
                ss << _PNT("Client ") << remote << _PNT(" does not exist!");
                errorOut = ss.str().c_str();
                outLogLevel = LogLevel_InvalidHostID;

                return false;
            }

            if (rc->m_tcpLayer && rc->m_tcpLayer->GetSocketType() == SocketType_WebSocket)
            {
                errorOut = _PNT("No need to encrypt Websocket message. Use wss protocol.");
                outLogLevel = LogLevel_Warning;
                return false;
        }

            output = rc->m_sessionKey;
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

    bool CNetServerImpl::CloseConnection(HostID clientHostID)
    {
        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);
        shared_ptr<CRemoteClient_S> rc = GetAuthedClientByHostID_NOLOCK(clientHostID);
        if (rc == nullptr)
            return false;

        if (m_logWriter)
        {
            Tstringstream ss;
            ss << "NetServer.CloseConnection(" << clientHostID << ") is called.";

            m_logWriter->WriteLine(0, LogCategory_System, HostID_Server, ss.str().c_str());
        }

        rc->m_enableAutoConnectionRecovery = false;

        // 종료할 소켓이 이미 offline 상태일 경우
        if (rc->m_enquedClientOfflineEvent == true)
        {
            //그냥 소켓 정리
            GarbageHost(rc, ErrorType_DisconnectFromLocal, ErrorType_Ok, ByteArray(), _PNT("CloseConnection"), SocketErrorCode_Ok);
        }
        else
        {
            //아닐 경우 우아한 연결 종료 시도
            RequestAutoPrune(rc);
        }

        return true;
    }

    void CNetServerImpl::CloseEveryConnection()
    {
        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        for ( auto i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++ )
        {
            shared_ptr<CHostBase> hb = i->GetSecond();

            // CNetServerImpl의 LoopbackHost는 제외
            if ( hb->GetHostID() == HostID_Server )
                continue;

            shared_ptr<CRemoteClient_S> rc = LeanDynamicCast_RemoteClient_S(hb);

            if ( rc != nullptr )
            {
                //dispose로 들어간 rc는 requestAutoPrune하지 않는다.
                if ( rc->m_disposeWaiter != nullptr )
                    continue;

                RequestAutoPrune(rc);
            }
        }
    }

    // 	shared_ptr<CRemoteClient_S> CNetServerImpl::GetRemoteClientByHostID_NOLOCK(HostID clientHostID)
    // 	{
    // 		AssertIsLockedByCurrentThread();
    // 		//CriticalSectionLock clk(GetCriticalSection(), true);
    // 		shared_ptr<CRemoteClient_S> rc = nullptr;
    // 		m_authedHosts.TryGetValue(clientHostID, rc);
    // 		return rc;
    // 	}


    int CNetServerImpl::GetLastUnreliablePingMs(HostID peerHostID, ErrorType* /*error*/)
    {
        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        shared_ptr<CRemoteClient_S> peer = GetAuthedClientByHostID_NOLOCK(peerHostID);  //여기에 일단 백업
        if ( peer == nullptr )
            return -1;
        else
        {
            return peer->m_lastPingMs;
        }
    }

    int  CNetServerImpl::GetRecentUnreliablePingMs(HostID peerHostID, ErrorType* /*error*/)
    {
        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        shared_ptr<CRemoteClient_S> peer = GetAuthedClientByHostID_NOLOCK(peerHostID);  //여기에 일단 백업
        if ( peer == nullptr )
            return -1;
        else
        {
            return peer->m_recentPingMs;
        }
    }

    shared_ptr<CSuperSocket> CNetServerImpl::GetAnyUdpSocket()
    {
        if ( m_staticAssignedUdpSockets.GetCount() <= 0 )
            return shared_ptr<CSuperSocket>();

        int randomIndex = m_random.Next((int)m_staticAssignedUdpSockets.GetCount() - 1);
        return m_staticAssignedUdpSockets[randomIndex];
    }

    void CNetServerImpl::EnqueueClientLeaveEvent(
        const shared_ptr<CRemoteClient_S>& rc,
        ErrorType errorType,
        ErrorType detailtype,
        const ByteArray& comment,
        SocketErrorCode /*socketErrorCode*/)
    {
        AssertIsLockedByCurrentThread();
        LocalEvent e;
        e.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
        e.m_errorInfo->m_errorType = errorType;
        e.m_errorInfo->m_detailType = detailtype;
        e.m_netClientInfo = rc->ToNetClientInfo();
        e.m_type = LocalEventType_ClientLeaveAfterDispose;
        e.m_byteArrayComment = comment;
        assert(rc->m_HostID);
        e.m_remoteHostID = rc->m_HostID;

        //modify by rekfkno1 - per-remote event로 변경.
        //EnqueLocalEvent(e);
        EnqueLocalEvent(e, rc);

        // 			// 121 에러 감지를 위함
        // 			if(socketErrorCode == 121)
        // 			{
        // 				String txt;
        // 				txt.Format(_PNT("SocketErrorCode=121! Source={%s,%d},ClientInfo={%s}, CCU = %d, SrvConfig={%s}"),
        // 					rc->GetDisposeCaller() ? rc->GetDisposeCaller() : _PNT(""),
        // 					rc->m_disposeWaiter ? rc->m_disposeWaiter->m_reason : 0,
        // 					(LPCWSTR) e.m_netPeerInfo->ToString(true),
        // 					m_authedHosts.GetCount(),
        // 					(LPCWSTR)GetConfigString() );
        //
        // 				CErrorReporter_Indeed::Report(txt);
        // 			}

        /*if(m_vizAgent)
        {
        CriticalSectionLock vizlock(m_vizAgent->m_cs,true);
        m_vizAgent->m_c2sProxy.NotifySrv_Clients_Remove(HostID_Server, g_ReliableSendForPN, rc->m_HostID);
        }*/

    }

    INetCoreEvent * CNetServerImpl::GetEventSink_NOCSLOCK()
    {
        AssertIsNotLockedByCurrentThread();
        return m_eventSink_NOCSLOCK;
    }

    CNetServerImpl::CNetServerImpl()
        : //m_userTaskQueue(this),
        m_listening(false),
        m_PurgeTooOldUnmatureClient_Timer(CNetConfig::PurgeTooOldAddMemberAckTimeoutMs),
        m_PurgeTooOldAddMemberAckItem_Timer(CNetConfig::PurgeTooOldAddMemberAckTimeoutMs),
        //m_disposeGarbagedHosts_Timer(CNetConfig::DisposeGarbagedHostsTimeoutMs),
        m_electSuperPeer_Timer(CNetConfig::ElectSuperPeerIntervalMs),
        m_removeTooOldUdpSendPacketQueue_Timer(CNetConfig::LongIntervalMs),
        //m_DisconnectRemoteClientOnTimeout_Timer(CNetConfig::UnreliablePingIntervalMs),
        m_removeTooOldRecyclePair_Timer(CNetConfig::RecyclePairReuseTimeMs / 2)
#ifdef SUPPORTS_WEBGL
        , m_webSocketServer(nullptr), m_webSocketServerSecured(nullptr)
#endif
    {
        m_globalTimer = CGlobalTimerThread::GetSharedPtr();

        m_HostIDRecycleAllowTimeMs = CNetConfig::HostIDRecycleAllowTimeMs;

#ifndef PROUDSRCCOPY
        BINARYreference();
        m_lic = new CPNLic;
#endif
        m_forceDenyTcpConnection_TEST = false;

        m_internalVersion = CNetConfig::InternalNetVersion;
#ifdef _WIN32
        EnableLowFragmentationHeap();
#endif

        m_selfSessionKey = shared_ptr<CSessionKey>(new CSessionKey);

        //m_shutdowning = false;
        m_eventSink_NOCSLOCK = nullptr;

        //m_timer.Start();

        m_s2cProxy.m_internalUse = true;
        m_c2sStub.m_internalUse = true;

        AttachProxy(&m_s2cProxy);
        AttachStub(&m_c2sStub);

        m_c2sStub.m_owner = this;

        //		m_userTaskRunning = false;

        m_SpeedHackDetectorReckRatio = 100;

        m_selfSessionKey = shared_ptr<CSessionKey>(new CSessionKey);

        m_allowEmptyP2PGroup = true;

        m_s2cProxy.m_internalUse = true;
        m_c2sStub.m_internalUse = true;

        // 용량을 미리 잡아놓는다.
        m_routerIndexList.SetCapacity(CNetConfig::OrdinaryHeavyS2CMulticastCount);

        m_joinP2PGroupKeyGen = 0;

        m_timerCallbackInterval = 0;
        m_timerCallbackParallelMaxCount = 1;
        m_timerCallbackContext = nullptr;
        m_startCreateP2PGroup = false;

        m_loopbackHost = nullptr;

    }


    int CNetServerImpl::GetClientHostIDs(HostID* ret, int count)
    {
        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);
        int c = 0;
        for ( auto i = m_authedHostMap.begin(); c < count && i != m_authedHostMap.end(); i++ )
        {
            HostID hostID = i->GetFirst();

            // CNetServerImpl의 LoopbackHost는 제외
            if ( hostID == HostID_Server )
                continue;

            ret[c++] = hostID;
        }
        return c;
    }

    bool CNetServerImpl::GetClientInfo(HostID clientHostID, CNetClientInfo &ret)
    {
        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        if ( clientHostID == HostID_None )
            return false;

        if ( clientHostID == HostID_Server )
            return false;

        shared_ptr<CHostBase> hb = AuthedHostMap_Get(clientHostID);
        if ( hb.get() == nullptr )
            return false;

        CRemoteClient_S *rc = dynamic_cast<CRemoteClient_S*>(hb.get());
        if ( rc == nullptr )
            return false;

        rc->ToNetClientInfo(ret);

        return true;
    }

    // 	bool CNetServerImpl::IsFinalReceiveQueueEmpty()
    // 	{
    // 		AssertIsLockedByCurrentThread();
    //
    // 		return m_finalUserWorkItemList.GetCount() == 0;
    // 	}
    //
    // NS에서 remote client에 대한 first dispose issue에 대한 추가 처리
    void CNetServerImpl::OnHostGarbaged(
        const shared_ptr<CHostBase>& remote,
        ErrorType errorType,
        ErrorType detailType,
        const ByteArray& comment,
        SocketErrorCode socketErrorCode)
    {
        LockMain_AssertIsLockedByCurrentThread();

        // NOTE: LoopbackHost는 별도의 Socket을 가지지 않기 때문에 여기서는 처리하지 않습니다.
        if (remote == m_loopbackHost)
            return;

        shared_ptr<CRemoteClient_S> rc = LeanDynamicCast_RemoteClient_S(remote);

        // 일부 고객사에서 OnClientLeave(X)를 스레드가 오래 잡고 있으면 뒷북으로 OnClientOffline(X)이 콜백된다고.
        // 그래서 이게 추가됐다.
        rc->m_enableAutoConnectionRecovery = false;

        // 이벤트 노티
        // 원래는 remoteClient에 enquelocalevent를 처리 하려 했으나.
        // delete되기 전에 enque하는 것이므로 다른 쓰레드에서 이 remoteclient에 대한 이벤트가
        // 나오지 않을 것이므로, 이렇게 처리해도 무방하다.
        if (rc->m_HostID != HostID_None)
        {
            if (rc->m_disposeWaiter != nullptr)
            {
                EnqueueClientLeaveEvent(rc,
                    rc->m_disposeWaiter->m_reason,
                    rc->m_disposeWaiter->m_detail,
                    rc->m_disposeWaiter->m_comment,
                    rc->m_disposeWaiter->m_socketErrorCode);
            }
            else
            {
                EnqueueClientLeaveEvent(rc,
                    ErrorType_DisconnectFromLocal,
                    ErrorType_ConnectServerTimeout,
                    comment,
                    SocketErrorCode_Ok);
            }
        }

        /* 내부 절차는 아래와 같다.
        먼저 TCP socket을 close한다. (UDP는 공용되므로 불필요)
        그리고 issue중이던 것들은 에러가 발생하게 된다. 혹은 issuerecv/send에서 이벤트 없이 에러가 발생한다.
        이때 recv on progress, send on progress가 중지될 것이다. 물론 TCP에 한해서 말이다.
        양쪽 모두 중지 확인되면 즉시 dispose를 한다.
        group info 등을 파괴하는 것은 즉시 하지 않고 이 객체가 파괴되는 즉시 하는게 맞다. */
#ifdef USE_HLA
        if (m_hlaSessionHostImpl != nullptr)
            m_hlaSessionHostImpl->DestroyViewer(rc);
#endif
        // 이렇게 소켓을 닫으면 issue중이던 것들이 모두 종료한다. 그리고 나서 재 시도를 하려고 해도
        // m_disposeWaiter가 있으므로 되지 않을 것이다. 그러면 안전하게 객체를 파괴 가능.
        // 소켓을 닫기전에 이 리모트에게 보냈던 총량을 기록한다.
        if (m_logWriter && rc->m_tcpLayer != nullptr) // ACR 연결을 본 연결에 던져주고 시체가 된 to-client의 TCP는 null.
        {
            Tstringstream ss;
            ss << "Remote client total send bytes=" << rc->m_tcpLayer->GetTotalIssueSendBytes();
            m_logWriter->WriteLine(0, LogCategory_System, rc->m_HostID, ss.str().c_str());
        }

        // TCP socket에서의 수신 처리에 대해서 더 이상 이 host가 처리하지 않는다.
        // 단, 댕글링 위험 때문에, TCP and UDP socket을 garbage는 안하고,
        // 추후 OnHostGarbageCollected에서 수행한다.
        if (rc->m_tcpLayer != nullptr) // ACR 연결을 본 연결에 던져주고 시체가 된 remote client가 가진 TCP는 null이므로, null check를 해야.
        {
            SocketToHostsMap_RemoveForAnyAddr(rc->m_tcpLayer);
        }

        // UDP socket에 대해서도 위에와 마찬가지로.
        CFallbackableUdpLayer_S& udp = rc->m_ToClientUdp_Fallbackable;
        if (udp.m_udpSocket)
        {
            SocketToHostsMap_RemoveForAddr(udp.m_udpSocket, udp.GetUdpAddrFromHere());
            udp.m_udpSocket->ReceivedAddrPortToVolatileHostIDMap_Remove(udp.GetUdpAddrFromHere());
        }
        if (rc->m_ownedUdpSocket)
        {
            SocketToHostsMap_RemoveForAnyAddr(rc->m_ownedUdpSocket);
        }

        rc->WarnTooShortDisposal(_PNT("NS.OHG"));

        // P2P 그룹 관계를 모두 청산해버린다.
        if (rc->m_HostID != HostID_None)
        {
            for (auto groupIt = rc->m_joinedP2PGroups.begin(); groupIt != rc->m_joinedP2PGroups.end(); ++groupIt)
            {
                // remove from P2PGroup_Add ack info
                CP2PGroupPtr_S gp = groupIt->GetSecond().m_groupPtr;

                // memberHostID이 들어있는 AddMemberAckWaiter 객체를 목록에서 찾아서 모두 제거한다.
                AddMemberAckWaiters_RemoveRelated_MayTriggerJoinP2PMemberCompleteEvent(
                    gp, rc->m_HostID, ErrorType_DisconnectFromRemote);

                // notify member leave to related group members
                // modify by ulelio : Server는 P2PGroup_MemberLeave를 받을 필요가 없다.
                for (auto memberIt = gp->m_members.begin(); memberIt != gp->m_members.end(); ++memberIt)
                {
                    if (memberIt->first != HostID_Server)
                    {
                        m_s2cProxy.P2PGroup_MemberLeave(
                            memberIt->first,
                            g_ReliableSendForPN,
                            rc->m_HostID,
                            gp->m_groupHostID,
                            CompactFieldMap());
                    }
                }

                // P2P그룹에서 제명
                gp->m_members.erase(rc->m_HostID);

                // 제명한 후 P2P그룹이 잔존해야 한다면...
                if (gp->m_members.size() > 0 || m_allowEmptyP2PGroup)
                {
                    P2PGroup_RefreshMostSuperPeerSuitableClientID(gp);
                }
                else
                {
                    // P2P그룹이 파괴되어야 한다면...
                    HostID bak = gp->m_groupHostID;
                    if (m_P2PGroups.Remove(bak))
                    {
                        m_HostIDFactory->Drop(bak);
                        EnqueueP2PGroupRemoveEvent(bak);
                    }
                }

                // 클라 나감 이벤트에서 '나가기 처리 직전까지 잔존했던 종속 P2P그룹'을 줘야 하므로 여기서 백업.
                rc->m_hadJoinedP2PGroups.Add(gp->m_groupHostID);
            }
            rc->m_joinedP2PGroups.Clear();
        }

        // AddMemberAckWaiters_RemoveRelated_MayTriggerJoinP2PMemberCompleteEvent 내에서
        // rc->m_disposeWaiter가 nullptr이 아니면 EnqueLocalEvent를 할 수 없게 된다.
        // 순서를 바꿔서 해결.
        if (rc->m_disposeWaiter == nullptr)
        {
            rc->m_disposeWaiter.Free();
            rc->m_disposeWaiter.Attach(new CRemoteClient_S::CDisposeWaiter());
            rc->m_disposeWaiter->m_reason = errorType;
            rc->m_disposeWaiter->m_detail = detailType;
            rc->m_disposeWaiter->m_comment = comment;
            rc->m_disposeWaiter->m_socketErrorCode = socketErrorCode;

            if (m_logWriter)
            {
                Tstringstream ss;
                ss << "IssueDispose call. Error=" << ErrorInfo::TypeToPlainString(errorType) << ",Detail=" << ErrorInfo::TypeToPlainString(detailType);
                m_logWriter->WriteLine(0, LogCategory_System, rc->m_HostID, ss.str().c_str());
            }
        }

        // 클라가 P2P 연결이 되어 있는 상대방 목록을 찾아서 모두 제거
        if (rc->m_HostID != HostID_None)
        {
            m_p2pConnectionPairList.RemovePairOfAnySide(rc);
#ifdef TRACE_NEW_DISPOSE_ROUTINE
            NTTNTRACE("%d RemovePair Call!!\n", (int)rc->m_HostID);
#endif // TRACE_NEW_DISPOSE_ROUTINE
        }

        //return true;
        //PNTRACE(TID_System,"%s(this=%p,WSAGetLastError=%d)\n",__FUNCTION__,this,WSAGetLastError());

        // 인덱스 정리. 없애기로 한 remote는 더 이상 UDP 데이터를 받을 자격이 없다.
        //m_UdpAddrPortToRemoteClientIndex.RemoveKey(rc->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere());
    }


    // TCP 연결이 장애가 생긴 상황을 처리한다.
    // TCP receive timeout 혹은 TCP connection lost 상태에서 호출됨.
    // ACR을 발동하거나 디스 시킨다.
    void CNetServerImpl::RemoteClient_GarbageOrGoOffline(
        const shared_ptr<CRemoteClient_S>& rc,
        ErrorType errorType,
        ErrorType detailType,
        const ByteArray& shutdownCommentFromClient,
        const String& commentFromErrorInfo,
        const PNTCHAR* where,
        SocketErrorCode socketErrorCode)
    {
        AssertIsLockedByCurrentThread();

        if (m_enableAutoConnectionRecoveryOnServer == true
            && rc->m_enableAutoConnectionRecovery)
        {
            RemoteClient_ChangeToAcrWaitingMode(
                rc,
                errorType,
                detailType,
                shutdownCommentFromClient,
                commentFromErrorInfo,
                where,
                socketErrorCode);
        }
        else
        {
            // ACR 안함. 그냥 님 디스요.
            GarbageHost(rc, errorType, detailType, shutdownCommentFromClient, where, socketErrorCode);
        }
    }

    int64_t CNetServerImpl::GetTimeMs()
    {
        //if(m_timer != nullptr)
        return GetPreciseCurrentTimeMs();

        //return -1;
    }

    int CNetServerImpl::GetClientCount()
    {
        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        // CNetServerImpl의 LoopbackHost는 제외
        return m_authedHostMap.GetCount() - 1;
    }

    CNetServerImpl::~CNetServerImpl()
    {
        Stop_Internal(true);
#ifdef USE_HLA

        m_hlaSessionHostImpl.Free();
#endif
        CriticalSectionLock lock(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);
        // PN 내부에서도 쓰는 RMI까지 더 이상 참조되지 않음을 확인해야 하므로 여기서 시행해야 한다.
        CleanupEveryProxyAndStub(); // 꼭 이걸 호출해서 미리 청소해놔야 한다.

#ifndef PROUDSRCCOPY
        delete ((CPNLic*)m_lic);
#endif
    }

    // 사용자 API를 구현.
    void CNetServerImpl::GetRemoteIdentifiableLocalAddrs(CFastArray<NamedAddrPort> &output)
    {
        output.Clear();

        CFastArray<Proud::String> localAddresses;
        CNetUtil::GetLocalIPAddresses(localAddresses);

        for ( int i = 0; i < (int)m_TcpListenSockets.GetCount(); i++ )
        {
            shared_ptr<CSuperSocket> listenSocket = m_TcpListenSockets[i];
            if ( listenSocket != nullptr )
            {
                AddrPort addr = listenSocket->GetSocketName();
                NamedAddrPort namedAddr = NamedAddrPort::From(addr);

                // local NIC이 지정된 바 있으면 그것을 넣자.
                namedAddr.OverwriteHostNameIfExists(m_localNicAddr);
                // external 주소가 지정된 바 있으면 그것을 넣자.
                namedAddr.OverwriteHostNameIfExists(m_serverAddrAlias);

                // 아직까지도 비어있다고? 그렇다면 모든 NIC에 대해서 잔뜩 채워야 한다.
                if (CNetUtil::IsAddressUnspecified(namedAddr.m_addr))
                {
                    for (int j = 0; j < (int)localAddresses.GetCount(); ++j)
                    {
                        NamedAddrPort a2;
                        a2.m_port = namedAddr.m_port;
                        a2.m_addr = localAddresses[j];
                        output.Add(a2);
                    }
                }
                output.Add(namedAddr);
            }
        }
    }

    // *주의* 이 함수는 reentrant하게 하지 말 것. 멤버 변수를 로컬변수처럼 쓰는 데가 있어서다.
    // => 동적 크기의 로컬 변수를 obj-pool 방식으로 고친 것 같은데, 그렇다면 이 주의사항은 사라져도 됨.
    void CNetServerImpl::DoForLongInterval()
    {
        EveryRemote_HardDisconnect_AutoPruneGoesTooLongClient();

        CriticalSectionLock mainlock(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        /* 각 remote에 대해서는 main lock 건 상태에서 즉시 처리하고
        각 remote가 가진 socket들 포함 모든 socket에 대해서는 LowContextSwitchingLoop를 통해
        main lock 없이 처리하도록 하자.
        per-socket lock은 다른 곳에서 syscall을 하기 때문에 contention이 잦기 때문. */
        int64_t currentTime = GetCachedServerTimeMs();

        POOLED_LOCAL_VAR(CSuperSocketArray, socketList);

        for ( int i = 0; i < m_staticAssignedUdpSockets.GetCount(); i++ )
        {
            auto &us = m_staticAssignedUdpSockets[i];
            socketList.Add(us);
        }

        // 각 remote에 대해서 처리를. 이때 remote가 가진 socket들도 목록에 추가된다.
        for ( AuthedHostMap::iterator irc = m_authedHostMap.begin(); irc != m_authedHostMap.end(); irc++ )
        {
            shared_ptr<CHostBase> hb = irc->GetSecond();

            if ( hb->GetHostID() == HostID_Server )
                continue;

            shared_ptr<CRemoteClient_S> rc = LeanDynamicCast_RemoteClient_S(hb);
            rc->DoForLongInterval(currentTime);

            if (false == IsSimplePacketMode() && rc->m_tcpLayer != nullptr)
            {
                rc->m_tcpLayer->SendDummyOnTcpOnTooLongUnsending(currentTime);
            }

            if ( rc->m_ownedUdpSocket != nullptr )
            {
                socketList.Add(rc->m_ownedUdpSocket);
            }

            // GarbageHost를 통하여 m_tcpLayer의 연결을 끊기 때문에 nullptr 확인을 해야한다.
            if ( rc->m_tcpLayer != nullptr )
            {
                socketList.Add(rc->m_tcpLayer);
            }
        }

        for ( auto irc = m_candidateHosts.begin(); irc != m_candidateHosts.end(); irc++ )
        {
            CRemoteClient_S* rc = (CRemoteClient_S*)irc->GetSecond().get();
            rc->DoForLongInterval(currentTime);

            if (false == IsSimplePacketMode() && rc->m_tcpLayer != nullptr)
            {
                rc->m_tcpLayer->SendDummyOnTcpOnTooLongUnsending(currentTime);
            }

            if ( rc->m_ownedUdpSocket )
            {
                socketList.Add(rc->m_ownedUdpSocket);
            }

            if (rc->m_tcpLayer != nullptr)
            {
                socketList.Add(rc->m_tcpLayer);
            }
        }

        // main unlock
        mainlock.Unlock();

        LowContextSwitchingLoop(socketList.GetData(), socketList.GetCount(),
            [](const shared_ptr<CSuperSocket>& object)
        {
            return &object->GetCriticalSection();
        },
            [currentTime](const shared_ptr<CSuperSocket>& object, CriticalSectionLock& solock)->bool
        {
            uint32_t queueLength;
            object->DoForLongInterval(currentTime, queueLength);
            solock.Unlock();

            return true;
        });
    }


    void CNetServerImpl::EnableLog(const PNTCHAR* logFileName)
    {
        CriticalSectionLock lock(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        if ( m_logWriter )
            return;

        m_logWriter = shared_ptr<CLogWriter>(CLogWriter::New(logFileName));

        // 모든 클라이언트들에게 로그를 보내라는 명령을 한다.
        for ( auto i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++ )
        {
            shared_ptr<CHostBase> hb = i->GetSecond();

            // CNetServerImpl의 LoopbackHost는 제외
            if ( hb->GetHostID() == HostID_Server )
                continue;

            CRemoteClient_S* rc = dynamic_cast<CRemoteClient_S*>(hb.get());
            if ( rc->m_disposeWaiter != nullptr )
                continue;

            m_s2cProxy.EnableLog(i->GetFirst(), g_ReliableSendForPN,
                CompactFieldMap());
        }
    }

    void CNetServerImpl::DisableLog()
    {
        CriticalSectionLock lock(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        m_logWriter = nullptr;

        // 모든 클라이언트들에게 로그를 보내지 말라는 명령을 한다.
        for ( auto i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++ )
        {
            shared_ptr<CHostBase> rc = i->GetSecond();

            // CNetServerImpl의 LoopbackHost는 제외
            if ( rc->GetHostID() == HostID_Server )
                continue;

            if ( rc->m_disposeWaiter != nullptr )
                continue;

            m_s2cProxy.DisableLog(i->GetFirst(), g_ReliableSendForPN,
                CompactFieldMap());
        }
    }

    /* 각 remote 객체에 대한 heartbeat 즉 10~100 ms 시간 단위의 짧은 처리를 합니다.
    하는 일은 다양하며, 계속 늘어납니다. reliable udp resend 여부를 판단한다던지, 오랫동안 무응답인 연결을 뒷정리 한다던지 등.
    너무 짧으면 서버에 CPU 사용량이 증가합니다. 너무 길면 reliable udp resend 여부 판단이 더디거나 합니다.
    */
    void CNetServerImpl::Heartbeat_EveryRemoteClient()
    {
        CriticalSectionLock mainLock(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        int64_t currTime = GetCachedServerTimeMs();

        for (auto i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++)
        {
            shared_ptr<CHostBase> hb = i->GetSecond();

            // CNetServerImpl의 LoopbackHost는 제외
            if (hb->GetHostID() == HostID_Server)
                continue;

            if (hb->m_garbaged)
                continue;

            shared_ptr<CRemoteClient_S> rc = LeanDynamicCast_RemoteClient_S(hb);
            if (rc->m_tcpLayer												// m_tcpLayer가 ACR에 의해 null일 수 있는 경우도 고려해야 하므로... NDN 이슈 4120
                && rc->m_tcpLayer->GetSocketType() != SocketType_WebSocket)  // 이 기능은 websocket에서는 안 하는 기능이다.
            {
                // 너무 오랫동안 TCP 수신을 못한 경우 디스 처리 한다.
                // 심플 패킷 모드가 false 일때만 한다.
                // websocket은 비해당이다. websocket 자체는 TCP 기반이므로.
                if (rc->m_closeNoPingPongTcpConnections &&
                    IsSimplePacketMode() == false &&
                    rc->m_tcpLayer != nullptr &&
                    rc->m_autoConnectionRecoveryWaitBeginTime == 0 &&
                    GetAllSocketsLastReceivedTimeOutState(currTime, rc) == TimeOut)
                {
                    if ( m_logWriter )
                    {
                        Tstringstream ss;
                        ss << "Server: Client " << rc->GetHostID() << " TCP connection timed out.";
                        m_logWriter->WriteLine(0, LogCategory_System, HostID_Server, ss.str().c_str());
                    }

                    RemoteClient_GarbageOrGoOffline(rc,
                                                    ErrorType_DisconnectFromRemote,
                                                    ErrorType_ConnectServerTimeout,
                                                    rc->m_shutdownComment,
                                                    String(_PNT("")),
                                                    _PNT("RC_GOGO"),
                                                    SocketErrorCode_Ok);
                }

                }

            // TODO: JS or WebGL 클라에 대해서 재접속 기능이 들어가게 되면, 위 if 구문을 수정해서 이것도 유효하게 해야 할 것이다.
            Heartbeat_AutoConnectionRecovery_GiveupWaitOnNeed(rc);

            // UDP에만 해당
            FallbackServerUdpToTcpOnNeed(rc, currTime);

            // UDP에만 해당
            ArbitraryUdpTouchOnNeed(rc, currTime);

                // tcp가 위험에 빠졌는지 체크 한다.
                // 			if(CNetConfig::EnableSendBrake)
                // 			{
                // 				rc->CheckTcpInDanger(currTime);
                // 			}

                // 매 2초마다 next expected Message ID(마지막 받은 것+1)를 ack 전송
                //Heartbeat_AcrSendMessageIDAckOnNeed(rc);

                // ProudNet level의 graceful disconnect 과정이 진행중인 클라가 아직도 더디고 있으면 강제로 TCP 연결을 닫아버림
                // NOTE: graceful disconnect가 별도로 있는 이유는, 연결 끊기 함수를 사용자가 호출한 직전에도 보내려던 RMI를 모두 전송하기 위함.
                HardDisconnect_AutoPruneGoesTooLongClient(rc);


            RefreshSendQueuedAmountStat(rc);

            // MessageOverload Warning 추가
            if ( rc->MessageOverloadChecking(GetPreciseCurrentTimeMs()) )
            {
                EnqueWarning(ErrorInfo::From(ErrorType_MessageOverload, rc->GetHostID()));
            }
        }
    }

    void CNetServerImpl::SetSpeedHackDetectorReckRatioPercent(int newValue)
    {
        if ( newValue <= 0 || newValue > 100 )
            ThrowInvalidArgumentException();

        m_SpeedHackDetectorReckRatio = newValue;
    }

    void CNetServerImpl::SetMessageMaxLength(int value_s, int value_c)
    {
        CNetConfig::ThrowExceptionIfMessageLengthOutOfRange(value_s);
        CNetConfig::ThrowExceptionIfMessageLengthOutOfRange(value_c);

        //{
        //CriticalSectionLock l(CNetConfig::GetWriteCriticalSection(),true);
        //CNetConfig::MessageMaxLength = max(CNetConfig::MessageMaxLength ,value); // 아예 전역 값도 수정하도록 한다.
        //}
        // 		int compareValue =  max(value_s, value_c);
        // 		CNetConfig::MessageMaxLength = max(CNetConfig::MessageMaxLength , compareValue); // 아예 전역 값도 수정하도록 한다.

        m_settings.m_serverMessageMaxLength = value_s;
        m_settings.m_clientMessageMaxLength = value_c;
    }

    void CNetServerImpl::SetDefaultTimeoutTimeMs(int newValInMs)
    {
        AssertIsNotLockedByCurrentThread();
        // 핑퐁 시간인 약 3010ms보다 DefaultTimeoutTime이 짧을 때 로미오-줄리엣 버그가 발생하여서 더 늘릴려고 하였는 데,
        // 4000Ms 이상이 적당하다고 하셔서 1000에서 4000으로 올림.
        if (newValInMs < 4000)
        {
            ShowUserMisuseError(_PNT("Too short timeout value. It may cause false-positive disconnection."));
            return;
        }

        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        if (m_listening == true)
        {
            clk.Unlock();
            AssertIsNotLockedByCurrentThread();
            ShowUserMisuseError(_PNT("Can only be used before calling CNetServer.Start."));
            return;
        }

        m_settings.m_defaultTimeoutTimeMs = newValInMs;
    }

    void CNetServerImpl::SetTimeoutTimeMs(HostID host, int newValInMs)
    {
        AssertIsNotLockedByCurrentThread();

        // 핑퐁 시간인 약 3010ms보다 DefaultTimeoutTime이 짧을 때 로미오-줄리엣 버그가 발생하여서 더 늘릴려고 하였는 데,
        // 사장님께서 4000Ms 이상이 적당하다고 하셔서 1000에서 6000으로 올림.
        if (newValInMs < 4000)
        {
            ShowUserMisuseError(_PNT("Too short timeout value. It may cause false-positive disconnection."));
            return;
        }

        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        if (m_listening == false)
        {
            clk.Unlock();
            AssertIsNotLockedByCurrentThread();
            ShowUserMisuseError(_PNT("Can only be used after calling CNetServer.Start."));
            return;
        }

        shared_ptr<CHostBase> hb;
        if (m_authedHostMap.TryGetValue(host, hb) == true)
        {
            if (hb->GetHostID() == HostID_Server)
            {
                return;
            }

            shared_ptr<CRemoteClient_S> rc = LeanDynamicCast_RemoteClient_S(hb);
            if (rc->m_garbaged)
            {
                return;
            }

            // 실제 리모트 클라이언트 객체에 적용
            rc->m_timeoutTimeMs = newValInMs;

            // 클라에게도 알림
            m_s2cProxy.NotifyChangedTimeoutTime(host, g_ReliableSendForPN, newValInMs);
        }
    }

    /* #TCP_KEEP_ALIVE
    한 remote에 대해, 모든 Socket들의 last received time이 out이면 SocketResult_TimeOut을,
    하나라도 time out이 아니면 SocketResult_NotTimeOut을,
    검사할 socket이 하나도 없으면 SocketResult_NoneToDo를 리턴한다.

    Q: C/S TCP socket만 검사해야 하는거 아닌가요?
    A: UDP 통신량이 과점을 해버려서 TCP 송수신이 우선순위에서 밀릴 수 있습니다.
    이때는 TCP congestion으로 인한 ECONNABORTED 즉 TCP 밑단에서의 retransmission timeout에 의존합시다.
    그리고 이 함수의 목적은, 총 통신 자체의 불량을 감지하는 목적입니다. */
    CNetCoreImpl::SocketResult CNetServerImpl::GetAllSocketsLastReceivedTimeOutState(int64_t currTime, const shared_ptr<CRemoteClient_S>& rc)
    {
        SocketResult result = NoneToDo;
        // remote client가 가진 모든 socket에 대해 검사를.
        UpdateSocketLastReceivedTimeOutState(rc->m_tcpLayer, currTime, rc->m_timeoutTimeMs, &result);
        if (result == NotTimeOut)
        {
            // 소켓이 존재하며, time out이 아니니, 더 검사할 필요 없음.
            return result;
        }
        UpdateSocketLastReceivedTimeOutState(rc->m_ToClientUdp_Fallbackable.m_udpSocket, currTime, rc->m_timeoutTimeMs, &result);
        if (result == NotTimeOut)
        {
            // 소켓이 존재하며, time out이 아니니, 더 검사할 필요 없음.
            return result;
        }

        // 리턴값은 TimeOut or NoneToDo 중 하나일 것이다.
        return result;
    }

    void CNetServerImpl::GetStats(CNetServerStats &outVal)
    {
        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        CriticalSectionLock statsLock(m_statsCritSec, true);
        outVal = m_stats;

        // 통계 정보는 좀 부정확해도 된다.
        // 하지만 P2P 카운트 정보는 정확해야 한다.
        // 그래서 여기서 즉시 계산한다.

        // CNetServerImpl의 LoopbackHost는 제외
        outVal.m_clientCount = (uint32_t)m_authedHostMap.GetCount() - 1;

        outVal.m_realUdpEnabledClientCount = 0;

        outVal.m_occupiedUdpPortCount = (int)m_staticAssignedUdpSockets.GetCount();

        for ( auto i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++ )
        {
            shared_ptr<CHostBase> hb = i->GetSecond();

            // CNetServerImpl의 LoopbackHost는 제외
            if ( hb->m_HostID == HostID_Server )
                continue;

            CRemoteClient_S* rc = dynamic_cast<CRemoteClient_S*>(hb.get());
            if ( rc->m_ToClientUdp_Fallbackable.m_realUdpEnabled )
                outVal.m_realUdpEnabledClientCount++;
        }

        for ( RCPairMap::iterator i = m_p2pConnectionPairList.m_activePairs.begin(); i != m_p2pConnectionPairList.m_activePairs.end(); i++ )
        {
            shared_ptr<P2PConnectionState> pair = (i->GetSecond());
            if ( pair->m_jitDirectP2PRequested )
            {
                outVal.m_p2pConnectionPairCount++;
                if ( !pair->IsRelayed() )
                    outVal.m_p2pDirectConnectionPairCount++;
            }
        }
    }

    void CNetServerImpl::GetTcpListenerLocalAddrs(CFastArray<AddrPort> &output)
    {
        CriticalSectionLock lock(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        output.Clear();
        if ( (int)m_TcpListenSockets.GetCount() == 0 )
        {
            output.Add(AddrPort::Unassigned);
            return;
        }

        m_TcpListenSockets[0]->GetSocketName();

        for ( int i = 0; i < (int)m_TcpListenSockets.GetCount(); i++ )
        {
            if ( m_TcpListenSockets[i] != nullptr )
            {
                output.Add(m_TcpListenSockets[i]->GetSocketName());
            }
        }
    }

    bool CNetServerImpl::IsValidHostID(HostID id)
    {
        CriticalSectionLock clk(GetCriticalSection(), true);
        return IsValidHostID_NOLOCK(id);

    }

    String CNetServerImpl::DumpGroupStatus()
    {
        return String();
    }

    bool CNetServerImpl::NextEncryptCount(HostID remote, CryptCount &output)
    {
        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);
        shared_ptr<CRemoteClient_S> rc = GetAuthedClientByHostID_NOLOCK(remote);
        if ( rc != nullptr )
        {
            output = rc->m_encryptCount;
            rc->m_encryptCount++;
            return true;
        }
        else if ( remote == HostID_Server )
        {
            output = m_selfEncryptCount;
            m_selfEncryptCount++;
            return true;
        }
        return false;
    }

    void CNetServerImpl::PrevEncryptCount(HostID remote)
    {
        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);
        shared_ptr<CRemoteClient_S> rc = GetAuthedClientByHostID_NOLOCK(remote);
        if ( rc != nullptr )
        {
            rc->m_encryptCount--;
        }
        else if ( remote == HostID_Server )
        {
            m_selfEncryptCount--;
        }
    }


    bool CNetServerImpl::GetExpectedDecryptCount(HostID remote, CryptCount &output)
    {
        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);
        shared_ptr<CRemoteClient_S> rc = GetAuthedClientByHostID_NOLOCK(remote);
        if ( rc != nullptr )
        {
            output = rc->m_decryptCount;
            return true;
        }
        else if ( remote == HostID_Server )
        {
            output = m_selfDecryptCount;
            return true;
        }
        return false;
    }

    bool CNetServerImpl::NextDecryptCount(HostID remote)
    {
        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);
        shared_ptr<CRemoteClient_S> rc = GetAuthedClientByHostID_NOLOCK(remote);
        if ( rc != nullptr )
        {
            rc->m_decryptCount++;
            return true;
        }
        else if ( remote == HostID_Server )
        {
            m_selfDecryptCount++;
            return true;
        }
        return false;
    }

    bool CNetServerImpl::IsConnectedClient(HostID clientHostID)
    {
        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);
        shared_ptr<CRemoteClient_S> rc = GetAuthedClientByHostID_NOLOCK(clientHostID);

        if ( rc == nullptr )
            return false;
        else
            return true;
    }

    bool CNetServerImpl::EnableSpeedHackDetector(HostID clientID, bool enable)
    {
        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        shared_ptr<CRemoteClient_S> rc = GetAuthedClientByHostID_NOLOCK(clientID);
        if (rc == nullptr)
            return false;

        if (enable && IsSimplePacketMode() == false)
        {
            if (rc->m_speedHackDetector == nullptr)
            {
                rc->m_speedHackDetector.Attach(new CSpeedHackDetector);
                m_s2cProxy.NotifySpeedHackDetectorEnabled(clientID, g_ReliableSendForPN, true);
            }
        }
        else
        {
            if (rc->m_speedHackDetector != nullptr)
            {
                rc->m_speedHackDetector.Free();
                m_s2cProxy.NotifySpeedHackDetectorEnabled(clientID, g_ReliableSendForPN, false);
            }
        }

        return true;
    }

    void CNetServerImpl::TEST_SomeUnitTests()
    {
        // IsSameSubnet24 를 임시 제거
        //TestAddrPort();

        // 서버를 실행한 상태이어야 한다. test 중 발생하는 에러는 event로 콜백될 것이다.
        //TestReliableUdpFrame();
        TestSendBrake();
        TestFastArray();
        TestStringA();
        TestStringW();
#if defined (_WIN32)
        // $$$; == > 본 작업이 완료되면 반드시 ifdef를 제거해주시기 바랍니다.
        // CReaderWriterMonitor_NORECURSE에서 Proud::Event를 사용해서 현재는 사용 불가능
        TestReaderWriterMonitor();
#endif
        TestUdpConnReset();
    }

    void CNetServerImpl::TestSendBrake()
    {
        // 		CSendBrake b;
        // 		// 일단 속도 지정
        // 		b.SetMaxSendSpeed(3000);
        // 		// 최초 콜
        // 		b.MoveTimeToCurrent(10000);
        // 		// 송신한 거 있음
        // 		b.Accumulate(5000, 10000);
        // 		if (b.BrakeNeeded(10000))
        // 		{
        // 			EnqueueUnitTestFailEvent(_PNT("Not the Time to Break!"));
        // 		}
        // 		if (b.BrakeNeeded(10001))
        // 		{
        // 			EnqueueUnitTestFailEvent(_PNT("Not the Time to Break!"));
        // 		}
        // 		if (!b.BrakeNeeded(10002))
        // 		{
        // 			EnqueueUnitTestFailEvent(_PNT("Time to Break!"));
        // 		}
        //
        // 		b.Accumulate(5000, 10003);
        //
        // 		if (b.BrakeNeeded(10003))
        // 		{
        // 			EnqueueUnitTestFailEvent(_PNT("Not the Time to Break!"));
        // 		}
        // 		if (b.BrakeNeeded(10004))
        // 		{
        // 			EnqueueUnitTestFailEvent(_PNT("Not the Time to Break!"));
        // 		}
        // 		if (!b.BrakeNeeded(10005))
        // 		{
        // 			EnqueueUnitTestFailEvent(_PNT("Time to Break!"));
        // 		}

    }

    // 	void CNetServerImpl::TestReliableUdpFrame()
    // 	{
    // 		CriticalSectionLock lock(GetCriticalSection(),true);
    // 		CHECK_CRITICALSECTION_DEADLOCK(this);
    //
    // 		CHeldPtr<CFastHeap> heap(CFastHeap::New());
    //
    // 		CUncompressedFrameNumberArray v(heap);
    // 		v.Add(5);
    // 		v.Add(1);
    // 		v.Add(2);
    // 		v.Add(4);
    //
    // 		QuickSort(v.GetData(),4);
    // 		if(v[0]!=1 || v[1]!=2 || v[2]!=4 || v[3] !=5)
    // 			EnqueueUnitTestFailEvent(_PNT("Proud.QuickSort() failed!"));
    //
    // 		ReliableUdpFrame a(heap);
    // 		a.m_ackedFrameNumbers=CCompressedFrameNumbers::NewInstance(heap);
    // 		for(int i=0;i<(int)v.GetCount();i++)
    // 		{
    // 			a.m_ackedFrameNumbers->AddSortedNumber(v[i]);
    // 		}
    //
    // 		CMessage msg;
    // 		msg.UseInternalBuffer();
    // 		a.m_ackedFrameNumbers->WriteTo(msg);
    //
    // 		msg.SetReadOffset(0);
    //
    // 		ReliableUdpFrame b(heap);
    // 		b.m_ackedFrameNumbers=CCompressedFrameNumbers::NewInstance(heap);
    // 		b.m_ackedFrameNumbers->ReadFrom(msg);
    //
    // 		CUncompressedFrameNumberArray w(heap);
    // 		b.m_ackedFrameNumbers->Uncompress(w);
    //
    // 		if(!w.Equals(v))
    // 			EnqueueUnitTestFailEvent(_PNT("FrameNumberList pack/unpack failed!"));
    // 	}

    void CNetServerImpl::EnqueueUnitTestFailEvent(String msg)
    {
        CriticalSectionLock lock(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        LocalEvent e;
        e.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
        e.m_type = LocalEventType_UnitTestFail;
        e.m_errorInfo->m_comment = msg;
        EnqueLocalEvent(e, m_loopbackHost);
    }

    void CNetServerImpl::TestFastArray()
    {
        CFastArray<int> v;
        v.Add(5);
        v.Add(1);
        v.Add(2);
        v.Add(4);

        CFastArray<int> w;
#if (_MSC_VER >= 1700)
        for(int i:v)
        {
            w.Add(i);
        }
#else
        for ( CFastArray<int>::iterator i = v.begin(); i != v.end(); i++ )
        {
            w.Add(*i);
        }
#endif
        if ( !w.Equals(v) )
            EnqueueUnitTestFailEvent(_PNT("CFastArray iterator failed!"));
    }

    void CNetServerImpl::TestStringA()
    {
        StringA a = "123가나다";
        StringA b;
        CMessage msg;
        msg.UseInternalBuffer();
        msg << a;
        msg.SetReadOffset(0);
        msg >> b;

        if ( a != b )
        {
            EnqueueUnitTestFailEvent(_PNT("Proud.StringA serialization failed!"));
        }
    }

    void CNetServerImpl::TestStringW()
    {
        StringA a = "123가나다";
        StringA b;
        CMessage msg;
        msg.UseInternalBuffer();
        msg << a;
        msg.SetReadOffset(0);
        msg >> b;

        if ( a != b )
        {
            EnqueueUnitTestFailEvent(_PNT("Proud.String serialization failed!"));
        }
    }

#ifdef USE_HLA
    void CNetServerImpl::HlaFrameMove()
    {
        if (m_hlaSessionHostImpl != nullptr)
            return m_hlaSessionHostImpl->HlaFrameMove();
        else
            throw Exception(HlaUninitErrorText);
    }

    void CNetServerImpl::HlaAttachEntityTypes(CHlaEntityManagerBase_S* entityManager)
    {
        if (m_hlaSessionHostImpl != nullptr)
            m_hlaSessionHostImpl->HlaAttachEntityTypes(entityManager);
        else
            throw Exception(HlaUninitErrorText);
    }

    void CNetServerImpl::HlaSetDelegate(IHlaDelegate_S* dg)
    {
        if (m_hlaSessionHostImpl == nullptr)
        {
            m_hlaSessionHostImpl.Attach(new CHlaSessionHostImpl_S(this));
        }
        m_hlaSessionHostImpl->HlaSetDelegate(dg);
    }

    CHlaSpace_S* CNetServerImpl::HlaCreateSpace()
    {
        if (m_hlaSessionHostImpl != nullptr)
            return m_hlaSessionHostImpl->HlaCreateSpace();
        else
            throw Exception(HlaUninitErrorText);

    }

    void CNetServerImpl::HlaDestroySpace(CHlaSpace_S* space)
    {
        if (m_hlaSessionHostImpl != nullptr)
            m_hlaSessionHostImpl->HlaDestroySpace(space);
        else
            throw Exception(HlaUninitErrorText);
    }

    CHlaEntity_S* CNetServerImpl::HlaCreateEntity(HlaClassID classID)
    {
        if (m_hlaSessionHostImpl != nullptr)
            return m_hlaSessionHostImpl->HlaCreateEntity(classID);
        else
            throw Exception(HlaUninitErrorText);
    }

    void CNetServerImpl::HlaDestroyEntity(CHlaEntity_S* Entity)
    {
        if (m_hlaSessionHostImpl != nullptr)
            m_hlaSessionHostImpl->HlaDestroyEntity(Entity);
        else
            throw Exception(HlaUninitErrorText);
    }

    void CNetServerImpl::HlaViewSpace(HostID viewerID, CHlaSpace_S* space, double levelOfDetail)
    {
        if (m_hlaSessionHostImpl != nullptr)
            m_hlaSessionHostImpl->HlaViewSpace(viewerID, space, levelOfDetail);
        else
            throw Exception(HlaUninitErrorText);
    }

    void CNetServerImpl::HlaUnviewSpace(HostID viewerID, CHlaSpace_S* space)
    {
        if (m_hlaSessionHostImpl != nullptr)
            m_hlaSessionHostImpl->HlaUnviewSpace(viewerID, space);
        else
            throw Exception(HlaUninitErrorText);
    }
#endif

    void CNetServerImpl::OnSocketWarning(shared_ptr<CSuperSocket> s, String msg)
    {
        //여기는 socket가 접근하므로, 메인락을 걸면 데드락 위험이 있다.
        //logwriter의 포인터를 보호할 크리티컬 섹션을 따로 걸어야 하나??ㅠㅠ
        //CriticalSectionLock lock(GetCriticalSection(),true);
        if ( m_logWriter )
        {
            m_logWriter->WriteLine(0, LogCategory_System, HostID_Server, msg);
        }
    }

    int CNetServerImpl::GetInternalVersion()
    {
        return m_internalVersion;
    }

    void CNetServerImpl::ShowWarning_NOCSLOCK(ErrorInfoPtr errorInfo)
    {
        String txt;
        txt.Format(_PNT("WARNING: %s"), errorInfo->ToString().GetString());
        if (m_logWriter)
        {
            m_logWriter->WriteLine(0, LogCategory_System, HostID_Server, txt);
        }

        if (OnWarning.m_functionObject)
            OnWarning.m_functionObject->Run(errorInfo);

        if (m_eventSink_NOCSLOCK)
        {
            m_eventSink_NOCSLOCK->OnWarning(errorInfo);
        }
    }

    void CNetServerImpl::ShowError_NOCSLOCK(ErrorInfoPtr errorInfo)
    {
        if (m_logWriter)
        {
            m_logWriter->WriteLine(0, LogCategory_System, HostID_Server, errorInfo->ToString());
        }

        CNetCoreImpl::ShowError_NOCSLOCK(errorInfo);
    }

    void CNetServerImpl::ShowNotImplementedRmiWarning(const PNTCHAR* RMIName)
    {
        CNetCoreImpl::ShowNotImplementedRmiWarning(RMIName);
    }

    void CNetServerImpl::PostCheckReadMessage(CMessage &msg, const PNTCHAR* RMIName)
    {
        CNetCoreImpl::PostCheckReadMessage(msg, RMIName);
    }

#if defined (_WIN32)
    // $$$; == > 본 작업이 완료되면 반드시 ifdef를 제거해주시기 바랍니다.
    void CNetServerImpl::TestReaderWriterMonitor()
    {
        CReaderWriterMonitorTest test;
        test.m_main = this;
        test.RunTest();
    }
#endif

    void CNetServerImpl::SetEventSink(INetServerEvent *eventSink)
    {
        if ( AsyncCallbackMayOccur() )
            throw Exception(AsyncCallbackMayOccurErrorText);

        AssertIsNotLockedByCurrentThread();
        m_eventSink_NOCSLOCK = eventSink;
    }

    void CNetServerImpl::EnqueError(ErrorInfoPtr e)
    {
        CriticalSectionLock lock(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        LocalEvent e2;
        e2.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
        e2.m_type = LocalEventType_Error;

        e2.m_errorInfo->m_errorType = e->m_errorType;
        e2.m_errorInfo->m_comment = e->m_comment;
        e2.m_remoteHostID = e->m_remote;
        e2.m_remoteAddr = e->m_remoteAddr;

        EnqueLocalEvent(e2, m_loopbackHost);
    }

    void CNetServerImpl::EnqueWarning(ErrorInfoPtr e)
    {
        CriticalSectionLock lock(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

#ifdef SUPPORTS_WEBGL
        // wss에 Secure이면 경고 미출력
        if (e->m_errorType == ErrorType_NoNeedWebSocketEncryption && m_webSocketParam.webSocketType == WebSocket_Wss)
            return;
#endif

        LocalEvent e2;
        e2.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
        e2.m_type = LocalEventType_Warning;

        e2.m_errorInfo->m_errorType = e->m_errorType;
        e2.m_errorInfo->m_comment = e->m_comment;
        e2.m_remoteHostID = e->m_remote;
        e2.m_remoteAddr = e->m_remoteAddr;

        EnqueLocalEvent(e2, m_loopbackHost);
    }

    void CNetServerImpl::EnquePacketDefragWarning(shared_ptr<CSuperSocket> superSocket, AddrPort sender, const String& text)
    {
        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        shared_ptr<CRemoteClient_S> rc = GetRemoteClientBySocket_NOLOCK(superSocket, sender);

        if ( CNetConfig::EnablePacketDefragWarning )
            EnqueWarning(ErrorInfo::From(ErrorType_InvalidPacketFormat, rc ? rc->m_HostID : HostID_None, text));
    }

    // 클라이언트로부터 도착한 user defined request data와 클라의 IP 주소 등을 인자로 받는 콜백을 일으킨다.
    // 사용자는 그 콜백에서 클라의 연결 수락 혹은 거부를 판단할 것이다.
    // 그리고 그 콜백의 결과에서 나머지 연결 수락 처리(HostID 발급 등)을 마무리한다.
    void CNetServerImpl::EnqueClientJoinApproveDetermine(const shared_ptr<CRemoteClient_S>& rc, const ByteArray& request)
    {
        AssertIsLockedByCurrentThread();

        if(rc->m_tcpLayer->GetSocketType() != SocketType_WebSocket)
            assert(rc->m_tcpLayer->m_remoteAddr.IsUnicastEndpoint());

        LocalEvent e2;
        e2.m_type = LocalEventType_ClientJoinCandidate;
        e2.m_remoteAddr = rc->m_tcpLayer->m_remoteAddr;
        e2.m_connectionRequest = request;

        EnqueLocalEvent(e2, rc);
    }

    void CNetServerImpl::ProcessOnClientJoinRejected(const shared_ptr<CRemoteClient_S>& rc, const ByteArray &response)
    {
        AssertIsLockedByCurrentThread();
        //CriticalSectionLock lock(GetCriticalSection(),true);

        CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
        Message_Write(sendMsg, MessageType_NotifyServerDeniedConnection);
        sendMsg.Write(response);

        rc->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(
            rc->m_tcpLayer,
            CSendFragRefs(sendMsg), SendOpt(), m_simplePacketMode);
    }

    // C/S 첫 핑퐁 작업 : OnClientJoin에서도 Ping 얻는 함수를 사용할 수 있도록 하는 작업
    // recent ping 등을 모두 계산하여 갖고 있는다.
    void CNetServerImpl::SetClientPingTimeMs(const shared_ptr<CRemoteClient_S>& rc)
    {
        if (rc == nullptr)
            throw Exception("Unexpected at candidate remote client removal!");

        rc->m_lastPingMs = max(rc->m_lastPingMs, 1);

        if (rc->m_recentPingMs > 0)
            rc->m_recentPingMs = LerpInt(rc->m_recentPingMs, rc->m_lastPingMs, CNetConfig::LagLinearProgrammingFactorPercent, 100);
        else
            rc->m_recentPingMs = rc->m_lastPingMs;

        rc->m_recentPingMs = max(rc->m_recentPingMs, 1);
    }

    // 클라이언트-서버 인증 과정이 다 끝나면
    void CNetServerImpl::ProcessOnClientJoinApproved(const shared_ptr<CRemoteClient_S>& rc, const ByteArray &response)
    {
        AssertIsLockedByCurrentThread();

        if ( rc == nullptr )
            throw Exception("Unexpected at candidate remote client removal!");

        // C/S 첫 핑퐁 작업 : OnClientJoin에서도 Ping 얻는 함수를 사용할 수 있도록 하는 작업
        SetClientPingTimeMs(rc);

        // NDN issue 4138 해결책. 
        // 서버를 시작하자마자 접속 들어오는 클라에 대한 핸들링.
        // 이미 Start 함수에서 HostIDFactory를 먼저 생성 후 TCP listen을 하도록 수정했으나, 긴급 라이브 서비스 업체에 대한 패치이므로, 일단 이것을 넣는다.
        // 이후 이 if구문이 필요없어질 날이 올테니,그때는 이 if구문을 제거해도 된다.
        if (nullptr == m_HostIDFactory)
        {
            GarbageHost(rc, ErrorType_DisconnectFromLocal, ErrorType_TCPConnectFailure, ByteArray(), _PNT("POCJA"), SocketErrorCode_Ok);
            return;
        }

        // promote unmature client to mature, give new HostID
        HostID newHostID = m_HostIDFactory->Create();
        rc->m_HostID = newHostID;

        m_candidateHosts.Remove(rc.get());
        m_authedHostMap.Add(newHostID, rc);

        //// create magic number
        //rc->m_ToClientUdp_Fallbackable.m_holePunchMagicNumber = Win32Guid::NewGuid();

        //// send magic number and any UDP address via TCP
        //{
        //	CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
        //	Message_Write(sendMsg, MessageType_RequestStartServerHolepunch);
        //	sendMsg.Write(rc->m_ToClientUdp_Fallbackable.m_holePunchMagicNumber);
        //	rc->m_ToClientTcp->AddToSendQueueWithSplitterAndSignal_Copy(CSendFragRefs(sendMsg), SendOpt());
        //}
#ifdef USE_HLA

        if ( m_hlaSessionHostImpl != nullptr )
            m_hlaSessionHostImpl->CreateViewer(rc);
#endif
        // enqueue new client event
        {
            LocalEvent e;
            e.m_type = LocalEventType_ClientJoinApproved;
            e.m_netClientInfo = rc->ToNetClientInfo();
            e.m_remoteHostID = rc->m_HostID;
            EnqueLocalEvent(e, rc);
        }

        // 테스트 차원에서 강제로 TCP를 연결해제하기로 했다면
        if ( m_forceDenyTcpConnection_TEST )
        {
            /* HostID를 할당, 즉 OnClientJoin 조건을 만족하는 순간 바로 TCP 연결을 끊어버린다.
            이렇게 해야 유저가 OnClientJoin에서 SetHostTag를 콜 하기 전에
            OnClientLeave를 위한 local event가 enque되게 한다.
            이렇게 되어야
            OnClientJoin(X)->내부에서 X디스->SetHostTag(X,Y)->OnClientLeave(X)->GetHostTag(X)가 Y리턴하는지?
            를 테스트 할 수 있다.
            */
            GarbageHost(rc, ErrorType_DisconnectFromLocal, ErrorType_TCPConnectFailure, ByteArray(), _PNT("POCJA"), SocketErrorCode_Ok);
        }
        else // send welcome with HostID
        {
            if(rc->m_tcpLayer->GetSocketType() != SocketType_WebSocket)
                assert(rc->m_tcpLayer->m_remoteAddr.IsUnicastEndpoint());

            CMessage sendMsg;
            sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
            Message_Write(sendMsg, MessageType_NotifyServerConnectSuccess);
            sendMsg.Write(newHostID);
            sendMsg.Write(response);
            sendMsg.Write(NamedAddrPort::From(rc->m_tcpLayer->m_remoteAddr));
            if (!IsSimplePacketMode())
            {
                sendMsg.Write(m_instanceGuid);

                int c2s = 0;
                int s2c = 0;

                // ACR이 활성화되어 있는 경우, rc의 AMR 객체를 생성해 넣도록 한다.
                if (rc->m_enableAutoConnectionRecovery)
                {
                    c2s = CRandom::StaticGetInt();
                    if ( c2s > 0 )
                        c2s = -c2s;  // C2S는 음수, S2C는 양수. 디버깅을 편하게 하기 위함.
                    s2c = CRandom::StaticGetInt();
                    if ( s2c < 0 )
                        s2c = -s2c;

                    rc->m_tcpLayer->AcrMessageRecovery_Init(s2c, c2s);
                }
                sendMsg.Write(c2s);
                sendMsg.Write(s2c);

                // C/S 첫 핑퐁 작업
                sendMsg.Write(rc->m_lastReliablePingMs);

                //#GetServerTime 서버 로컬 타임을 추가로 보내자. 클라에서 이것을 갖고 클라-서버간 시간 차이를 구한다. 측정된 레이턴시와 함께.
                CompactFieldMap fieldMap;
                fieldMap.SetField(FieldType_LocalTime, (int64_t)GetPreciseCurrentTimeMs());

                // 서버의 외부 주소를 클라에게 보낸다. 클라는 이 외부 주소가 지정되었던 경우, ACR 재접속시 이 주소를 이용할 것이다.
                fieldMap.SetField(FieldType_ServerAddrAtClient, NetVariant(m_serverAddrAlias.c_str()));  

                Message_Write(sendMsg, fieldMap);
            }
            //NTTNTRACE("RemoteClient(%d)'s instance GUID = %s\n", rc->m_HostID, StringW2A(m_instanceGuid.ToString()).GetString());

            {
                // send queue에 넣는 것과 AMR set은 atomic해야 하므로 이 lock이 필요
                CriticalSectionLock lock(rc->m_tcpLayer->GetSendQueueCriticalSection(), true);

                rc->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(
                    rc->m_tcpLayer,
                    CSendFragRefs(sendMsg), SendOpt(), m_simplePacketMode);
            }
        }
        /*	if(m_vizAgent)
        {
        CriticalSectionLock vizlock(m_vizAgent->m_cs,true);
        m_vizAgent->m_c2sProxy.NotifySrv_Clients_AddOrEdit(HostID_Server, g_ReliableSendForPN, rc->m_HostID);
        }*/
    }

    bool CNetServerImpl::AsyncCallbackMayOccur()
    {
        if ( m_listening )
            return true;
        else
            return false;
    }

    void CNetServerImpl::ProcessOneLocalEvent(LocalEvent& e, const shared_ptr<CHostBase>& subject, const PNTCHAR*& outFunctionName, CWorkResult*)
    {
        AssertIsNotLockedByCurrentThread();

        INetServerEvent* eventSink_NOCSLOCK = (INetServerEvent*)m_eventSink_NOCSLOCK;

        switch ((int)e.m_type)
        {
        case LocalEventType_ClientLeaveAfterDispose:
        {
            outFunctionName = _PNT("OnClientLeave");
            if (eventSink_NOCSLOCK)
            {
                eventSink_NOCSLOCK->OnClientLeave(e.m_netClientInfo.get(), e.m_errorInfo, e.m_byteArrayComment);
            }

            if (OnClientLeave.m_functionObject)
            {
                OnClientLeave.m_functionObject->Run(e.m_netClientInfo.get(), e.m_errorInfo, e.m_byteArrayComment);
            }
        }
        break;
        case LocalEventType_ClientOffline:
        {
            CRemoteOfflineEventArgs args;
            args.m_remoteHostID = e.m_remoteHostID;
            args.m_errorInfo = e.m_errorInfo;

            if (eventSink_NOCSLOCK)
            {
                eventSink_NOCSLOCK->OnClientOffline(args);
            }

            if (OnClientOffline.m_functionObject)
            {
                OnClientOffline.m_functionObject->Run(args);
            }
        }
        break;
        case LocalEventType_ClientOnline:
        {
            CRemoteOnlineEventArgs args;
            args.m_remoteHostID = e.m_remoteHostID;
            if (eventSink_NOCSLOCK)
            {
                eventSink_NOCSLOCK->OnClientOnline(args);
            }

            if (OnClientOnline.m_functionObject)
            {
                OnClientOnline.m_functionObject->Run(args);
            }

            CRemoteClient_S* rc = dynamic_cast<CRemoteClient_S*>(subject.get());
            if (rc != nullptr)
            {
                // 초기화. 즉, ONClientOffline event를 enque를 할 수 있게 허락을.
                rc->m_enquedClientOfflineEvent = false;
            }
            else
            {
                // rc가 null 인데 요게 콜 되었으면 막장
                // (기반 로직이 잘못 되었다는 뜻 acrimpl_s.cpp 분석 요망)
                throw Exception("Fatal error: POLE");
            }
        }
        break;

        case LocalEventType_ClientJoinApproved:
            outFunctionName = _PNT("OnClientJoin");
            if (eventSink_NOCSLOCK)
                eventSink_NOCSLOCK->OnClientJoin(e.m_netClientInfo.get());

            if (OnClientJoin.m_functionObject)
                OnClientJoin.m_functionObject->Run(e.m_netClientInfo.get());

            break;

        case LocalEventType_AddMemberAckComplete:
            outFunctionName = _PNT("OnP2PGroupJoinMemberAckComplete");
            if (eventSink_NOCSLOCK)
                eventSink_NOCSLOCK->OnP2PGroupJoinMemberAckComplete(e.m_groupHostID, e.m_memberHostID, e.m_errorInfo->m_errorType);

            if (OnP2PGroupJoinMemberAckComplete.m_functionObject)
                OnP2PGroupJoinMemberAckComplete.m_functionObject->Run(e.m_groupHostID, e.m_memberHostID, e.m_errorInfo->m_errorType);

            break;

        case LocalEventType_HackSuspected:
            outFunctionName = _PNT("OnClientHackSuspected");
            if (eventSink_NOCSLOCK)
                eventSink_NOCSLOCK->OnClientHackSuspected(e.m_remoteHostID, e.m_hackType);

            if (OnClientHackSuspected.m_functionObject)
                OnClientHackSuspected.m_functionObject->Run(e.m_remoteHostID, e.m_hackType);

            break;

        case LocalEventType_P2PGroupRemoved:
            outFunctionName = _PNT("OnP2PGroupRemoved");
            if (eventSink_NOCSLOCK)
            eventSink_NOCSLOCK->OnP2PGroupRemoved(e.m_remoteHostID);

            if (OnP2PGroupRemoved.m_functionObject)
                OnP2PGroupRemoved.m_functionObject->Run(e.m_remoteHostID);

            break;

        case LocalEventType_TcpListenFail:
            ShowError_NOCSLOCK(ErrorInfo::From(ErrorType_ServerPortListenFailure, HostID_Server));
            break;

        case LocalEventType_UnitTestFail:
            ShowError_NOCSLOCK(ErrorInfo::From(ErrorType_UnitTestFailed, HostID_Server, e.m_errorInfo->m_comment));
            break;

        case LocalEventType_Error:
            ShowError_NOCSLOCK(ErrorInfo::From(e.m_errorInfo->m_errorType, e.m_remoteHostID, e.m_errorInfo->m_comment));
            break;

        case LocalEventType_Warning:
            ShowWarning_NOCSLOCK(ErrorInfo::From(e.m_errorInfo->m_errorType, e.m_remoteHostID, e.m_errorInfo->m_comment));
            break;

        case LocalEventType_ClientJoinCandidate:
        {
            ByteArray response;
            outFunctionName = _PNT("OnConnectionRequest");

            // 사용자에게 접속 승인을 묻기
            bool approved1 = false;
            if (eventSink_NOCSLOCK)
            {
                approved1 = eventSink_NOCSLOCK->OnConnectionRequest(e.m_remoteAddr, e.m_connectionRequest, response);
            }
            bool approved2 = false;

            if (OnConnectionRequest.m_functionObject)
                OnConnectionRequest.m_functionObject->Run(e.m_remoteAddr, e.m_connectionRequest, response, &approved2);

            bool approved = approved1 || approved2;

            CriticalSectionLock lock(GetCriticalSection(), true);
            CHECK_CRITICALSECTION_DEADLOCK(this);

            shared_ptr<CRemoteClient_S> rc = LeanDynamicCast_RemoteClient_S(subject);
            if (rc != nullptr)
            {
                if (approved)
                {
                    // 승인
                    ProcessOnClientJoinApproved(rc, response);
                }
                else
                {
                    // 거절
                    ProcessOnClientJoinRejected(rc, response);
                }
            }
        }
        break;
        default:
            return;

        }
    }

    String CNetServerImpl::GetConfigString()
    {
        String ret;
        ret.Format(_PNT("Listen=%s"), m_localNicAddr.GetString());

        return ret;
    }

    int CNetServerImpl::GetMessageMaxLength()
    {
        return m_settings.m_serverMessageMaxLength;
    }

    // 	void CNetServerImpl::PruneTooOldDefragBoardOnNeed()
    // 	{
    // 		CriticalSectionLock clk(GetCriticalSection(), true);
    //
    // 		if(GetCachedServerTime() - m_lastPruneTooOldDefragBoardTime > CNetConfig::AssembleFraggedPacketTimeout / 2)
    // 		{
    // 			m_lastPruneTooOldDefragBoardTime = GetCachedServerTime();
    //
    // 			m_udpPacketDefragBoard->PruneTooOldDefragBoard();
    // 		}
    // 	}

    void CNetServerImpl::LogFreqFailOnNeed()
    {
        //CriticalSectionLock mainlock(GetCriticalSection(), true);
        //이거 쓰기싫어 m_fregFailNeed를 만듬.

        if ( m_fregFailNeed > 0 && GetCachedServerTimeMs() - m_lastHolepunchFreqFailLoggedTime > 30000 )
        {
            LogFreqFailNow();
        }
    }

    void CNetServerImpl::LogFreqFailNow()
    {
        if ( !m_freqFailLogMostRankText.IsEmpty() )
        {
            CErrorReporter_Indeed::Report(m_freqFailLogMostRankText);
        }
        m_lastHolepunchFreqFailLoggedTime = GetCachedServerTimeMs();
        m_freqFailLogMostRank = 0;
        m_freqFailLogMostRankText = _PNT("");

        AtomicCompareAndSwap32(1, 0, &m_fregFailNeed);
    }

    Proud::HostID CNetServerImpl::GetVolatileLocalHostID() const
    {
        return HostID_Server;
    }

    Proud::HostID CNetServerImpl::GetLocalHostID()
    {
        return HostID_Server;
    }

    void CNetServerImpl::RequestAutoPrune(const shared_ptr<CRemoteClient_S>& rc)
    {
        // 클라 추방시 소켓을 바로 닫으면 직전에 보낸 RMI가 안갈 수 있다. 따라서 클라에게 자진 탈퇴를 종용하되 시한을 둔다.
        if ( !rc->m_safeTimes.Get_requestAutoPruneStartTime() )
        {
            rc->m_safeTimes.Set_requestAutoPruneStartTime(GetCachedServerTimeMs());
            m_s2cProxy.RequestAutoPrune(rc->m_HostID, g_ReliableSendForPN,
                CompactFieldMap());
        }
    }

    // PN 레벨 shutdown handshake 과정이 덜된 상태로 오래 방치된 것은 그냥 중단하고 TCP 디스
    void CNetServerImpl::HardDisconnect_AutoPruneGoesTooLongClient(const shared_ptr<CRemoteClient_S>& rc)
    {
        // 5초로 잡아야 서버가 클라에게 많이 보내고 있던 중에도 직전 RMI의 확실한 송신이 되므로
        if ( rc->m_safeTimes.Get_requestAutoPruneStartTime() > 0
            && GetCachedServerTimeMs() - rc->m_safeTimes.Get_requestAutoPruneStartTime() > 5000 )
        {
            rc->m_safeTimes.Set_requestAutoPruneStartTime(0);
            // caller에서는 Proud.CNetServerImpl.m_authedHosts를 iter중이 아니므로 안전
            GarbageHost(
                rc,
                ErrorType_DisconnectFromLocal,
                ErrorType_ConnectServerTimeout,
                ByteArray(),
                _PNT("APGTL"),
                SocketErrorCode_Ok);
        }
    }

    void CNetServerImpl::TEST_SetOverSendSuspectingThresholdInBytes(int newVal)
    {
        newVal = PNMIN(newVal, CNetConfig::DefaultOverSendSuspectingThresholdInBytes);
        newVal = PNMAX(newVal, 0);
        m_settings.m_overSendSuspectingThresholdInBytes = newVal;
    }

    void CNetServerImpl::TEST_SetTestping(HostID hostID0, HostID hostID1, int ping)
    {
        P2PConnectionStatePtr stat;
        stat = m_p2pConnectionPairList.GetPair(hostID0, hostID1);
        if ( stat != nullptr )
        {
            stat->m_recentPingMs = ping;
        }
    }

    void CNetServerImpl::TEST_ForceDenyTcpConnection()
    {
        m_forceDenyTcpConnection_TEST = true;

    }

    bool CNetServerImpl::IsNagleAlgorithmEnabled()
    {
        return m_settings.m_enableNagleAlgorithm;
    }

    bool CNetServerImpl::IsValidHostID_NOLOCK(HostID id)
    {
        AssertIsLockedByCurrentThread();
        CHECK_CRITICALSECTION_DEADLOCK(this);

        if ( id == HostID_None )
            return false;

        if ( id == HostID_Server )
            return true;

        if ( AuthedHostMap_Get(id).get() != nullptr )
            return true;

        if ( GetP2PGroupByHostID_NOLOCK(id) )
            return true;

        return false;
    }

    void CNetServerImpl::RefreshSendQueuedAmountStat(const shared_ptr<CRemoteClient_S>& rc)
    {
        AssertIsLockedByCurrentThread();

        // 송신큐 잔여 총량을 산출한다.
        int tcpQueuedAmount = 0;
        int udpQueuedAmount = 0;

        if ( rc->m_tcpLayer != nullptr )
        {
            //CriticalSectionLock tcplock(rc->m_tcpLayer->GetSendQueueCriticalSection(),true);<= 내부에서 이미 잠그므로 불필요
            if(rc->m_tcpLayer->GetSocketType() == SocketType_Tcp)
                tcpQueuedAmount = rc->m_tcpLayer->GetSendQueueLength();
#ifdef SUPPORTS_WEBGL
            else if (rc->m_tcpLayer->GetSocketType() == SocketType_WebSocket)
            {
                // WebSocket도 TCP
                shared_ptr<CWebSocket> webSocket = dynamic_pointer_cast<CWebSocket>(rc->m_tcpLayer);
                if (m_webSocketParam.webSocketType == WebSocket_Ws)
                    tcpQueuedAmount = webSocket->m_webSocket->get_send_queue_length();
                else if (m_webSocketParam.webSocketType == WebSocket_Wss)
                    tcpQueuedAmount = webSocket->m_webSocketSecured->get_send_queue_length();
            }
#endif
        }

        if ( rc->m_ToClientUdp_Fallbackable.m_udpSocket && rc->m_ToClientUdp_Fallbackable.m_realUdpEnabled )
        {
            udpQueuedAmount = rc->m_ToClientUdp_Fallbackable.m_udpSocket->GetUdpSendQueueLength(rc->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere());
        }
        rc->m_sendQueuedAmountInBytes = tcpQueuedAmount + udpQueuedAmount;

        //add by rekfkno1 - sendqueue의 용량을 검사한다.
        int64_t curtime = GetCachedServerTimeMs();
        if ( rc->m_safeTimes.Get_sendQueueWarningStartTime() != 0 )
        {
            if ( rc->m_sendQueuedAmountInBytes > CNetConfig::SendQueueHeavyWarningCapacity )
            {
                if ( curtime - rc->m_safeTimes.Get_sendQueueWarningStartTime() > CNetConfig::SendQueueHeavyWarningTimeMs )
                {
                    String str;
                    str.Format(_PNT("sendQueue %dBytes"), rc->m_sendQueuedAmountInBytes);
                    EnqueWarning(ErrorInfo::From(ErrorType_SendQueueIsHeavy, rc->GetHostID(), str));

                    rc->m_safeTimes.Set_sendQueueWarningStartTime(curtime);
                }
            }
            else
            {
                rc->m_safeTimes.Set_sendQueueWarningStartTime(0);
            }
        }
        else if ( rc->m_sendQueuedAmountInBytes > CNetConfig::SendQueueHeavyWarningCapacity )
        {
            rc->m_safeTimes.Set_sendQueueWarningStartTime(curtime);
        }
    }

    void CNetServerImpl::TestUdpConnReset()
    {
        CTestUdpConnReset tester;
        tester.m_owner = this;
        tester.DoTest();
    }

#ifdef VIZAGENT
    void CNetServerImpl::EnableVizAgent(const PNTCHAR* vizServerAddr, int vizServerPort, const String &loginKey)
    {
        if ( !m_vizAgent )
        {
            m_vizAgent.Attach(new CVizAgent(this, vizServerAddr, vizServerPort, loginKey));
        }
}
#endif

#ifdef VIZAGENT
    void CNetServerImpl::Viz_NotifySendByProxy(const HostID* remotes, int remoteCount, const MessageSummary &summary, const RmiContext& rmiContext)
    {
        if ( m_vizAgent )
        {
            CFastArray<HostID> tgt;
            tgt.SetCount(remoteCount);
            UnsafeFastMemcpy(tgt.GetData(), remotes, sizeof(HostID) * remoteCount);
            m_vizAgent->m_c2sProxy.NotifyCommon_SendRmi(HostID_Server, g_ReliableSendForPN, tgt, summary);
        }
    }

    void CNetServerImpl::Viz_NotifyRecvToStub(HostID sender, RmiID RmiID, const PNTCHAR* RmiName, const PNTCHAR* paramString)
    {
        if ( m_vizAgent )
        {
            m_vizAgent->m_c2sProxy.NotifyCommon_ReceiveRmi(HostID_Server, g_ReliableSendForPN, sender, RmiName, RmiID);
        }
    }
#endif

    void CNetServerImpl::GetUserWorkerThreadInfo(CFastArray<CThreadInfo> &output)
    {
        //CriticalSectionLock lock(GetCriticalSection(),true);
        //CHECK_CRITICALSECTION_DEADLOCK(this);

        if ( m_userThreadPool )
        {
            m_userThreadPool->GetThreadInfos(output);
        }
    }

    void CNetServerImpl::GetNetWorkerThreadInfo(CFastArray<CThreadInfo> &output)
    {
        if ( m_netThreadPool )
        {
            m_netThreadPool->GetThreadInfos(output);
        }
    }

    // 	void CNetServerImpl::RemoteClient_RemoveFromCollections(const shared_ptr<CRemoteClient_S>& rc)
    // 	{
    // 		// 목록에서 제거.
    // 		m_candidateHosts.Remove(rc);
    // 		m_authedHosts.RemoveKey(rc->m_HostID);
    // 		m_HostIDFactory->Drop(rc->m_HostID);
    // 		//m_UdpAddrPortToRemoteClientIndex.Remove(rc->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere());
    // 		if (m_udpAssignMode == ServerUdpAssignMode_Static)
    // 			SocketToHostsMap_RemoveForAddr(rc->m_ToClientUdp_Fallbackable.m_udpSocket, rc->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere());
    // 		else
    // 			SocketToHostsMap_RemoveForAnyAddr(rc->m_ToClientUdp_Fallbackable.m_udpSocket);
    // 	}

    void CNetServerImpl::EnqueAddMemberAckCompleteEvent(HostID groupHostID, HostID addedMemberHostID, ErrorType result)
    {
        // enqueue 'completed join' event
        LocalEvent le;
        le.m_type = LocalEventType_AddMemberAckComplete;
        le.m_groupHostID = groupHostID;
        le.m_memberHostID = addedMemberHostID;
        le.m_remoteHostID = addedMemberHostID;
        le.m_errorInfo = ErrorInfo::From(result);

        // local event는 해당 host에 넣어주자.
        // OnP2PMemberJoinAck는 local host에 대한 것을 처리하는 것으로도 충분하겠지만
        // 혹시 모르니.
        if ( addedMemberHostID == HostID_Server )
            EnqueLocalEvent(le, m_loopbackHost);
        else
            EnqueLocalEvent(le, GetAuthedClientByHostID_NOLOCK(addedMemberHostID));
    }

    bool CNetServerImpl::SetHostTag(HostID hostID, void* hostTag)
    {
        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        shared_ptr<CHostBase> subject = GetTaskSubjectByHostID_NOLOCK(hostID);
        if ( subject.get() == nullptr )
            return false;

        subject->m_hostTag = hostTag;
        return true;
    }

    // 	CNetServerImpl::CUdpAddrPortToRemoteClientIndex::CUdpAddrPortToRemoteClientIndex()
    // 	{
    // 		//InitHashTable(CNetConfig::ClientListHashTableValue);
    //
    // 	}

    CNetServer *CNetServer::Create_Internal()
    {
        // 여기서 PROUDNET_H_LIB_SIGNATURE를 건드린다. 이렇게 함으로 이 전역 변수가 컴파일러에 의해 사라지지 않게 한다.
        PROUDNET_H_LIB_SIGNATURE++;

        return new CNetServerImpl();
    }

    shared_ptr<CHostBase> CNetServerImpl::GetTaskSubjectByHostID_NOLOCK(HostID subjectHostID)
    {
        AssertIsLockedByCurrentThread();
        CHECK_CRITICALSECTION_DEADLOCK(this);
        //CriticalSectionLock clk(m_owner->GetCriticalSection(), true);

        if ( subjectHostID == HostID_None )
            return shared_ptr<CHostBase>();

        if ( subjectHostID == HostID_Server )
            return m_loopbackHost;

        return AuthedHostMap_Get(subjectHostID);
    }

#ifdef _DEBUG // 이 함수는 매우 느리다. 디버깅 용도로만 써야 하므로.
    void CNetServerImpl::CheckCriticalsectionDeadLock_INTERNAL(const PNTCHAR* functionname)
    {
        AssertIsLockedByCurrentThread();

        for ( AuthedHostMap::iterator itr = m_authedHostMap.begin(); itr != m_authedHostMap.end(); ++itr )
        {
            CRemoteClient_S* rc = dynamic_cast<CRemoteClient_S*>(itr->GetSecond().get());
            if ( rc != nullptr )
            {
                if ( rc->IsLockedByCurrentThread() == true )
                {
                    String strfmt;
                    strfmt.Format(_PNT("RemoteClient DeadLock - %s"), functionname);
                    ShowUserMisuseError(strfmt.GetString());
                }

                if ( m_udpAssignMode == ServerUdpAssignMode_PerClient )
                {
                    if ( rc->m_ownedUdpSocket != nullptr )
                    {
#ifdef PN_LOCK_OWNER_SHOWN
                        if ( Proud::IsCriticalSectionLockedByCurrentThread(rc->m_ownedUdpSocket->m_cs) )
                        {
                            String strfmt;
                            strfmt.Format(_PNT("UDPSocket DeadLock - %s"), functionname);
                            ShowUserMisuseError(strfmt.GetString());
                        }
#endif
                    }
                }
            }
        }

        if ( m_udpAssignMode == ServerUdpAssignMode_Static )
        {
            int count = m_staticAssignedUdpSockets.GetCount();
            for (int i = 0; i < count; ++i )
            {
                if (m_staticAssignedUdpSockets[i]->IsLockedByCurrentThread() == true)
                {
                    String strfmt;
                    strfmt.Format(_PNT("UDPSocket DeadLock - %s"), functionname);
                    ShowUserMisuseError(strfmt.GetString());
                }
            }
        }
    }
#endif // _DEBUG

    // 받은 메시지를 networker thread에서 직접 처리하거나, user work로 넣어 버려 user worker thread에서 처리되게 한다.
    void CNetServerImpl::ProcessMessageOrMoveToFinalRecvQueue(
        const shared_ptr<CSuperSocket>& socket,
        CReceivedMessageList &extractedMessages)
    {
        SocketType socketType = socket->GetSocketType();
        AssertIsNotLockedByCurrentThread();

        // main lock을 걸었으니 송신자의 HostID를 이제 찾을 수 있다.
        // 		CHostBase* pFrom = SocketToHostMap_Get(socket);
        // 		HostID fromID = HostID_None;
        // 		if (pFrom != nullptr)
        // 			fromID = pFrom->GetHostID();
        
        POOLED_LOCAL_VAR(RemoteArray, extractedMessagesRemotes);
        extractedMessagesRemotes.SetCount(extractedMessages.GetCount()); // 크기 미리 조정. 메모리 파편화 예방.

        // TCP or per-remote UDP socket이면 대응 host가 모두 동일하다.
        // 이때는 socket to hosts map에는 socket에 대한 wildcard host가 들어있다.
        // 이때는 두번 이상 찾아봤자 다 똑같다. 따라서 처음 한번만 찾고 나머지는 wildcard host로 대체하는게 경제적이다.
        // code profile 결과 이 구간은 main lock 구간이므로 성능에 민감하므로 이러한 최적화가 필요하다.
        shared_ptr<CHostBase> wildCardHost;
        bool useWildcardHost = false; // 위의 값이 비록 찾았지만 null일 수 있으므로 이 변수를 따로 쓰자.
        {
            CriticalSectionLock mainLock(GetCriticalSection(), true);

            // 모든 수신 메시지를 처리하되 1회의 main lock만을 걸도록 하자.
            // 그리고 각 메시지 처리에서는 main lock 없이 가도록 하자.

            //////////////////////////////////////////////////////////////////////////
            // 각 extracted message들을 처리할 remote 객체들. main unlock 후에도 액세스하기 위해 shared_ptr take를 한다.

            // 아직 MessageType_NotifyServerConnectSuccess가 미처리 remote는 hostID=none이다.
            // 따라서 messageList의 hostID를 믿지 말고, socket으로부터 remote를 찾도록 하자.

            int c = 0;

            for (CReceivedMessageList::iterator i = extractedMessages.begin();
            i != extractedMessages.end() && !useWildcardHost;  // wildcard host를 이미 찾았으면 루프 돌 필요 없다.
                i++)
            {
                CReceivedMessage& receivedMessage = *i;

                bool isWildcardHost = false;
                // super socket이 연관된 remote 객체를 찾자.
                // code profile 결과, 여기서 remote client로 dynamic cast하며, 가뜩이나 main lock 구간인데, 병렬 병목이 된다.
                // 따라서 dynamic cast는 main unlock 후에 한다.
                if (!SocketToHostsMap_TryGet_NOLOCK(socket, receivedMessage.m_remoteAddr_onlyUdp, extractedMessagesRemotes[c], isWildcardHost))
                {
                    // 위 TryGet함수가 false를 리턴했을 때 rc는 invalid이다. 즉 always null임을 신뢰할 수 없다.
                    extractedMessagesRemotes[c] = nullptr;
                }
                else
                {
                    // wild card host이면 즉 socket이 가리키는 host가 유일한 경우
                    // 두번째부터는 찾을 필요가 없다.
                    if (isWildcardHost)
                    {
                        wildCardHost = MOVE_OR_COPY(extractedMessagesRemotes[0]);
                        useWildcardHost = true;
                    }
                }

                // next turn
                c++;
            }
            // main unlock.
        }

        AssertIsNotLockedByCurrentThread();

        //////////////////////////////////////////////////////////////////////////

        // main unlock 후 느린 dynamic cast를 한다.
        shared_ptr<CRemoteClient_S> wildcardRemoteClient;
        if (useWildcardHost)
            wildcardRemoteClient = LeanDynamicCast_RemoteClient_S(wildCardHost);

        // 프넷 레벨 메시지면 여기서 바로 처리를, 아니면 user work queue로 옮긴다.
        int c = 0;
        for (CReceivedMessageList::iterator i = extractedMessages.begin(); i != extractedMessages.end(); i++)
        {
            CReceivedMessage& receivedMessage = *i;

            bool move_rc_to_wildcardHost; // true이면 아래 move를 일 마친 후 롤백한다.
            shared_ptr<CRemoteClient_S> rc;
            if (useWildcardHost)
            {
                rc = MOVE_OR_COPY(wildcardRemoteClient);
                move_rc_to_wildcardHost = true;
            }
            else
            {
                // 느린 dynamic cast를 수행.
                rc = LeanDynamicCast_RemoteClient_S(extractedMessagesRemotes[c++]);
                move_rc_to_wildcardHost = false;
            }

            if (rc == nullptr) // 메시지는 받았지만 대응 remote가 없으면
            {
                receivedMessage.m_remoteHostID = HostID_None;

                if (socketType == SocketType_Udp)
                {
                    receivedMessage.GetReadOnlyMessage().SetReadOffset(0);
                    ProcessMessage_FromUnknownClient(receivedMessage.m_remoteAddr_onlyUdp,
                        receivedMessage.GetReadOnlyMessage(),
                        socket);
                }
                else
                {
                    // 연결유지기능이 들어가기전까지는 Remote가 nullptr인 경우가 발생하는지 확인차 추가한 코드입니다.

                    // 해당 경우가 발생
                    //assert(0);
                }
            }
            else // 대응 remote가 있음
            {
                // messageList 각 항목의 m_remoteHostID 채우기
                receivedMessage.m_remoteHostID = rc->m_HostID;

                // NOTE: 추출된 message들은 모두 같은 endpoint로부터 왔다.
                // 따라서 모두 같은 hostID이므로 아래 if 구문은 safe.
                ASSERT_OR_HACKED(receivedMessage.m_unsafeMessage.GetReadOffset() == 0);

                // 수신 미보장 부분임에도 불구하고 이미 처리한 메시지면 false로 세팅된다.
                // 즉 무시하고 버려진다.
                bool useMessage;
                if (!receivedMessage.m_hasMessageID || socketType != SocketType_Tcp)
                {
                    useMessage = true;
                }
                else
                {
                    if (rc)
                    {
                        // 고객사에서 디버그 브레이크를 오래 잡고 있을때 아래 assert가 발생한다는 제보가 있었다.
                        // 무시해도 되는 현상이므로 주석처리한다.
                        // assert(rc->m_tcpLayer == socket);
                        useMessage = socket->AcrMessageRecovery_ProcessReceivedMessageID(receivedMessage.m_messageID);
                    }
                    else
                    {
                        useMessage = true;
                    }
                }

                if (useMessage)
                {
                    ProcessMessage_ProudNetLayer(
                        receivedMessage,
                        rc,
                        socket);

                    // 주의: RequestServerConnection는 내부적으로 main unlock을 한번 한다.
                    // 이 때문에, 이 줄 이하에서는 상태가 다른 스레드에 변해 있을 수 있다.
                    // 따라서, 루틴을 추가할 때 주의하도록 하자.
                }
            }

            if (move_rc_to_wildcardHost)
            {
                wildcardRemoteClient = MOVE_OR_COPY(rc);
            }
        }
    }

    bool CNetServerImpl::ProcessMessage_ProudNetLayer(
        CReceivedMessage& receivedInfo,
        const shared_ptr<CRemoteClient_S>& rc,
        const shared_ptr<CSuperSocket>& socket)
    {
        CMessage &recvedMsg = receivedInfo.m_unsafeMessage;
        int orgReadOffset = recvedMsg.GetReadOffset();

        // _mm_prefetch((char*)(recvedMsg.GetData() + orgReadOffset), _MM_HINT_NTA);이거 적용해봐도 성능 차이 거의 없는데?

        MessageType type;
        if (!Message_Read(recvedMsg, type))
        {
            recvedMsg.SetReadOffset(orgReadOffset);
            return false;
        }

        //		NTTNTRACE("[ACR] Processing message %d in server~\n", type);

        bool messageProcessed = false;

        switch ( type )
        {
            case MessageType_ServerHolepunch:
            {
                // 이 메시지는 아직 RemoteClient가 배정되지 않은 클라이언트에 대해서도 작동한다.
                bool unicastEndpoint = receivedInfo.GetRemoteAddr().IsUnicastEndpoint();
                ASSERT_OR_HACKED(unicastEndpoint);
                if (unicastEndpoint)
                {
                    IoCompletion_ProcessMessage_ServerHolepunch(
                        receivedInfo.GetReadOnlyMessage(),
                        receivedInfo.GetRemoteAddr(), socket);
                }
                messageProcessed = true;
                break;
            }
            case MessageType_PeerUdp_ServerHolepunch:
            {
                // 이 메시지는 아직 RemoteClient가 배정되지 않은 클라이언트에 대해서도 작동한다.
                bool unicastEndpoint = receivedInfo.GetRemoteAddr().IsUnicastEndpoint();
                ASSERT_OR_HACKED(unicastEndpoint);
                if (unicastEndpoint)
                {
                    IoCompletion_ProcessMessage_PeerUdp_ServerHolepunch(
                        receivedInfo.GetReadOnlyMessage(),
                        receivedInfo.GetRemoteAddr(), socket);
                }
                messageProcessed = true;
                break;
            }
            case MessageType_RequestServerConnection:
                // isRealUdp를 멤버로 넣고 그 이유가 AssertIsZeroUseCount 때문인데요, 차라리 그냥
                // 이 함수 윗단에서 AssertIsZeroUseCount를 검사하게 하고 아래 각 함수 안에서는
                // 그걸 검사하지 말게 합시다.
                IoCompletion_ProcessMessage_RequestServerConnection_HAS_INTERIM_UNLOCK(recvedMsg, rc);
                messageProcessed = true;
                break;

            case MessageType_RequestAutoConnectionRecovery:
                // 서버측에 ACR 로직 처리 기능이 On된 경우만 클라이언트로부터 ACR메세지를 받았을 때 처리합니다.
                if (m_enableAutoConnectionRecoveryOnServer)
                {
                    IoCompletion_ProcessMessage_RequestAutoConnectionRecovery(recvedMsg, rc, socket);
                }
                messageProcessed = true;
                break;
            case MessageType_UnreliablePing:
                IoCompletion_ProcessMessage_UnreliablePing(recvedMsg, rc);
                messageProcessed = true;
                break;
            case MessageType_SpeedHackDetectorPing:
                IoCompletion_ProcessMessage_SpeedHackDetectorPing(recvedMsg, rc);
                messageProcessed = true;
                break;
            case MessageType_UnreliableRelay1:
                IoCompletion_ProcessMessage_UnreliableRelay1(recvedMsg, rc);
                messageProcessed = true;
                break;
            case MessageType_ReliableRelay1:
                IoCompletion_ProcessMessage_ReliableRelay1(recvedMsg, rc);
                messageProcessed = true;
                break;
            case MessageType_UnreliableRelay1_RelayDestListCompressed:
                IoCompletion_ProcessMessage_UnreliableRelay1_RelayDestListCompressed(recvedMsg, rc);
                messageProcessed = true;
                break;
            case MessageType_LingerDataFrame1:
                IoCompletion_ProcessMessage_LingerDataFrame1(recvedMsg, rc);
                messageProcessed = true;
                break;
            case MessageType_NotifyHolepunchSuccess:
                IoCompletion_ProcessMessage_NotifyHolepunchSuccess(recvedMsg, rc);
                messageProcessed = true;
                break;
            case MessageType_PeerUdp_NotifyHolepunchSuccess:
                IoCompletion_ProcessMessage_PeerUdp_NotifyHolepunchSuccess(recvedMsg, rc);
                messageProcessed = true;
                break;
                //case MessageType_ReliableUdp_P2PDisconnectLinger1:
                //    NetWorkerThread_ReliableUdp_P2PDisconnectLinger1(recvedMsg, rc);
                //    messageProcessed = true;
                //    break;
            case MessageType_Rmi:
                IoCompletion_ProcessMessage_Rmi(receivedInfo, messageProcessed, rc, socket);
                break;

            case MessageType_UserMessage:
                IoCompletion_ProcessMessage_UserOrHlaMessage(receivedInfo, UWI_UserMessage, messageProcessed, rc, socket);
                break;
            case MessageType_Hla:
                IoCompletion_ProcessMessage_UserOrHlaMessage(receivedInfo, UWI_Hla, messageProcessed, rc, socket);
                break;

            case MessageType_Encrypted_Reliable_EM_Secure:
            case MessageType_Encrypted_Reliable_EM_Fast:
            case MessageType_Encrypted_UnReliable_EM_Secure:
            case MessageType_Encrypted_UnReliable_EM_Fast:
            {
                CReceivedMessage decryptedReceivedMessage;
                if (ProcessMessage_Encrypted(type, receivedInfo, decryptedReceivedMessage.m_unsafeMessage))
                {
                    decryptedReceivedMessage.m_relayed = receivedInfo.m_relayed;
                    decryptedReceivedMessage.m_remoteAddr_onlyUdp = receivedInfo.m_remoteAddr_onlyUdp;
                    decryptedReceivedMessage.m_remoteHostID = receivedInfo.m_remoteHostID;
                    decryptedReceivedMessage.m_encryptMode = receivedInfo.m_encryptMode;

                    messageProcessed |= ProcessMessage_ProudNetLayer(decryptedReceivedMessage,
                        rc, socket);
                }
            }
            break;

            case MessageType_Compressed:
            {
                CReceivedMessage uncompressedReceivedMessage;
                if ( ProcessMessage_Compressed(receivedInfo, uncompressedReceivedMessage.m_unsafeMessage) )
                {
                    uncompressedReceivedMessage.m_relayed = receivedInfo.m_relayed;
                    uncompressedReceivedMessage.m_remoteAddr_onlyUdp = receivedInfo.m_remoteAddr_onlyUdp;
                    uncompressedReceivedMessage.m_remoteHostID = receivedInfo.m_remoteHostID;
                    uncompressedReceivedMessage.m_compressMode = receivedInfo.m_compressMode;
                    // Send 할 때 압축 후 암호화를 한다. Recv 시에 복호화 후 압축을 푼다. 따라서 여기서 m_encryptMode 값을 셋팅해 주어야 압축정보를 제대로 받을 수 있다.
                    uncompressedReceivedMessage.m_encryptMode = receivedInfo.m_encryptMode;

                    messageProcessed |= ProcessMessage_ProudNetLayer(uncompressedReceivedMessage,
                        rc, socket);
                }
                break;
            }

            case MessageType_PolicyRequest:
                //IoCompletion_ProcessMessage_PolicyRequest(rc);
                messageProcessed = true;
            default:
                break;
        }

        // 만약 잘못된 메시지가 도착한 것이면 이미 ProudNet 계층에서 처리한 것으로 간주하고
        // 메시지를 폐기한다. 그리고 예외 발생 이벤트를 던진다.
        // 단, C++ 예외를 발생시키지 말자. 디버깅시 혼란도 생기며 fail over 처리에도 애매해진다.
        if ( messageProcessed == true &&
            receivedInfo.GetReadOnlyMessage().GetLength() != receivedInfo.GetReadOnlyMessage().GetReadOffset()
            && type != MessageType_Encrypted_Reliable_EM_Secure
            && type != MessageType_Encrypted_Reliable_EM_Fast
            && type != MessageType_Encrypted_UnReliable_EM_Secure
            && type != MessageType_Encrypted_UnReliable_EM_Fast ) // 암호화된 메시지는 별도 버퍼에서 복호화된 후 처리되므로
        {
            messageProcessed = true;
            // 2009.11.26 add by ulelio : 에러시에 마지막 메세지를 저장한다.
            String comment;
            comment.Format(_PNT("%s, type[%d]"), _PNT("PMPNL"), (int)type);
            ByteArray lastreceivedMessage(recvedMsg.GetData(), recvedMsg.GetLength());
            EnqueError(ErrorInfo::From(ErrorType_InvalidPacketFormat, receivedInfo.m_remoteHostID, comment, lastreceivedMessage));
        }
        //AssureMessageReadOkToEnd(messageProcessed, type,receivedInfo,rc);
        if ( messageProcessed == false )
        {
            recvedMsg.SetReadOffset(orgReadOffset);
            return false;
        }

        return true;
    }

    void CNetServerImpl::IoCompletion_ProcessMessage_ServerHolepunch(CMessage &msg, AddrPort from, const shared_ptr<CSuperSocket>& udpSocket)
    {
        // caller가 성능에 민감하다. 그래서 여기서 main lock을 한다.
        AssertIsNotLockedByCurrentThread();
        CriticalSectionLock mainLock(GetCriticalSection(), true);

        if (m_simplePacketMode)
            return;

        // 이 메시지를 TCP로 의도적으로 해킹 전송을 할 경우, 그냥 버려야
        if (udpSocket == nullptr || udpSocket->GetSocketType() != SocketType_Udp)
        {
            assert(false);
            return;
        }

        NTTNTRACE("C2S Holepunch from %s\n", from.ToString().GetString());

        Guid magicNumber;
        if ( msg.Read(magicNumber) == false )
            return;

        // C/S 홀펀칭에서 RTT 처리
        int cliTime, rtt;
        if (msg.Read(cliTime) == false)
            return;

        if (msg.Read(rtt) == false)
            return;

        if (rtt) {
            // 클라이언트에서 측정된 UDP RTT를 서버도 공유받았다. 갖고 있자.
            shared_ptr<CRemoteClient_S> rc = GetRemoteClientBySocket_NOLOCK(udpSocket, from);

            if (rc) {
                rc->m_lastPingMs = rtt;
                SetClientPingTimeMs(rc);
            }
        }

        // ack magic number and remote UDP address via UDP once
        CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
        sendMsg.Write((int8_t)MessageType_ServerHolepunchAck);
        sendMsg.Write(magicNumber);
        sendMsg.Write(from);

        // C/S 홀펀징에서 RTT 처리
        sendMsg.Write(cliTime);

        //위에서 usecount +1하고 와야함.

        udpSocket->AddToSendQueueWithSplitterAndSignal_Copy(
            udpSocket,
            HostID_None,
            FilterTag::CreateFilterTag(HostID_Server, HostID_None),
            from,
            CSendFragRefs(sendMsg),
            GetPreciseCurrentTimeMs(),
            SendOpt(MessagePriority_Holepunch, true));

        //이하는 메인 lock하에 진행.
        AssertIsLockedByCurrentThread();
        // 		CriticalSectionLock clk(GetCriticalSection(), true);
        // 		CHECK_CRITICALSECTION_DEADLOCK(this);

        if ( m_logWriter )
        {
            Tstringstream ss;
            ss << "MessageType_ServerHolepunch from " << from.ToString().GetString();
            m_logWriter->WriteLine(0, LogCategory_P2P, HostID_Server, ss.str().c_str());
        }
    }

    // 클라로부터 온 최초의 메시지이다.
    // 주의: 중간에 main unlock을 한다. 따라서 이 함수의 caller에서는 상태가 변한다는 전제의 코딩이 필수.
    void CNetServerImpl::IoCompletion_ProcessMessage_RequestServerConnection_HAS_INTERIM_UNLOCK(
        CMessage &msg,
        const shared_ptr<CRemoteClient_S>& rc)
    {
        // remote client가 없으면, 처리 불가능.
        if (!rc)
        {
            assert(false); // 서버를 debug build로 실행중일 때는, 해커의 공격이 아닌 여기 오지 말아야 한다.
            return;
        }

        rc->AssertIsSocketNotLockedByCurrentThread();

        AssertIsNotLockedByCurrentThread();

        // 참고: 하단에서 잠시 unlock을 하는 경우가 있다.
        CriticalSectionLock mainLock(GetCriticalSection(), true);

        // 미인증 클라인지 확인.
        // 해킹된 클라에서 이게 반복적으로 오는 것일 수 있다. 그런 경우 무시하도록 하자.
        if (!m_candidateHosts.ContainsKey(rc.get()))
        {
            NotifyProtocolVersionMismatch(rc);
            return;
        }

        ByteArray aesKeyBlob; // AES key 데이터 
        ByteArray fastKeyBlob; // XOR 단순 암호화 알고리즘용 키 데이터
        ByteArray encryptedAESKeyBlob; // 공개키로 암호화된 상태
        ByteArray encryptedFastKeyBlob; // 공개키로 암호화된 상태
        ByteArray encryptedCredential; // 공개키로 암호화된 상태
        ByteArray credentialBlock; // ACR 재접속 때 사용되는 credential 값.

        ByteArray userData;
        Guid protocolVersion;
        int internalVersion;

        bool enableAutoConnectionRecovery = false;
        bool CloseNoPingPongTcpConnections = false;

        int runtimePlatform = 0;

        // simple packet mode가 아닌 경우 복잡한 처리를 수행.
        if (!m_simplePacketMode)
        {
            // 라이선스 유효 검사
            if (!msg.Read(runtimePlatform))
            {
                NotifyProtocolVersionMismatch(rc);
                return;
            }

            // 웹소켓은 ACR과 암호화, 압축을 일단 배제한다.
            if (rc->m_tcpLayer->GetSocketType() != SocketType_WebSocket)
            {
                // ACR 설정을 켠 클라인지?
                if (!msg.Read(enableAutoConnectionRecovery))
                {
                    NotifyProtocolVersionMismatch(rc);
                    return;
                }

                if (msg.Read(CloseNoPingPongTcpConnections) == false)
                {
                    NotifyProtocolVersionMismatch(rc);
                    return;
                }

                if (m_settings.m_enableEncryptedMessaging == true) {
                    // AES key를 얻는다.
                    if (msg.Read(encryptedAESKeyBlob) == false)
                    {
                        NotifyProtocolVersionMismatch(rc);
                        return;
                    }

                    // Fast key를 얻는다.
                    if (msg.Read(encryptedFastKeyBlob) == false)
                    {
                        NotifyProtocolVersionMismatch(rc);
                        return;
                    }

                    // Credential Key를 얻는다.
                    if (msg.Read(encryptedCredential) == false)
                    {
                        NotifyProtocolVersionMismatch(rc);
                        return;
                    }
                }

            }

            // C/S 첫 핑퐁 작업
            int pingPongTime;
            if (msg.Read(pingPongTime) == false)
            {
                NotifyProtocolVersionMismatch(rc);
                return;
            }

            pingPongTime = ((int)GetPreciseCurrentTimeMs() - pingPongTime) / 2;
            rc->m_lastReliablePingMs = max(pingPongTime, 1);
            rc->m_lastPingMs = rc->m_lastReliablePingMs;
            rc->m_recentPingMs = rc->m_lastReliablePingMs;
        }

        if (msg.Read(userData) == false)
        {
            NotifyProtocolVersionMismatch(rc);
            return;
        }

        if ( msg.Read(protocolVersion) == false )
        {
            NotifyProtocolVersionMismatch(rc);
            return;
        }
        if (msg.Read(internalVersion) == false)
        {
            NotifyProtocolVersionMismatch(rc);
            return;
        }

        if ( !m_simplePacketMode )
        {

#ifndef PROUDSRCCOPY
            if ( !((CPNLic*)m_lic)->POCJ(runtimePlatform, rc->m_tcpLayer->m_remoteAddr, GetClientCount()) )
            {
                NotifyLicenseMismatch(rc);
                return;
            }
            else
            {
                rc->m_RT = runtimePlatform;
            }
#endif

            if (m_settings.m_enableEncryptedMessaging == true && rc->m_tcpLayer->GetSocketType() != SocketType_WebSocket) {
                // main unlock을 한다. 키 복호화는 많은 시간을 차지하기 때문에, 병렬 병목을 피하기 위해서다.
                // 다시 main lock 후에는 상태가 변해있을 수 있음에 주의해서 코딩할 것.

                // rc가 돌발 delete되면 안되므로
                shared_ptr<CSessionKey> rc_sessionKey = shared_ptr<CSessionKey>(new CSessionKey);

                // 비파괴 보장 상태로 만든다.
                // main lock 상태에서 rc는 파괴되지 않으므로 댕글링 걱정 뚝.
#ifdef PARALLEL_RSA_DECRYPTION
                mainLock.Unlock(); // main unlock을 했지만, rc가 smart ptr이므로, 변수 존재 자체가 비파괴 보장이다.
                AssertIsNotLockedByCurrentThread(); // 이걸 검사해야 병렬 병목을 예방.
#endif

                // 암호화 실패나 성공시 아래가 반드시 실행되게 한다.
                RunOnScopeOut rollback([&mainLock, rc](){
#ifdef PARALLEL_RSA_DECRYPTION
                    mainLock.Lock();
#endif
                });

                // 암호화된 세션키를 복호화해서 클라이언트 세션키에 넣는다.
                // m_selfXchgKey는 서버 시작시에 한번만 만드므로 락을 안걸어도 괜찮을것 같다.
                if (ErrorInfoPtr err = CCryptoRsa::DecryptSessionKeyByPrivateKey(aesKeyBlob, encryptedAESKeyBlob, m_selfXchgKey))
                {
                    NotifyProtocolVersionMismatch(rc);
                    return;
                }

                // 실패시 실행할 마무리 루틴
                auto failCase = [this, rc](){
                    EnqueError(ErrorInfo::From(ErrorType_EncryptFail, HostID_None, _PNT("Failed to create SessionKey.")));

                    CMessage sendMsg;
                    sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
                    Message_Write(sendMsg, MessageType_NotifyServerDeniedConnection);
                    sendMsg.Write(ByteArray());   // user-defined reply data

                    rc->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(
                        rc->m_tcpLayer,
                        CSendFragRefs(sendMsg), SendOpt());
                };

                // AES 키 세팅 / Fast 키 세팅
                if (CCryptoAes::ExpandFrom(rc_sessionKey->m_aesKey, aesKeyBlob.GetData(), m_settings.m_encryptedMessageKeyLength / 8) != true)
                {
                    rollback.Run();
                    failCase();
                    return;
                }
                if (CCryptoAes::DecryptByteArray(rc_sessionKey->m_aesKey, encryptedFastKeyBlob, fastKeyBlob) != true)
                {
                    rollback.Run();
                    failCase();
                    return;
                }

                if (CCryptoFast::ExpandFrom(rc_sessionKey->m_fastKey, fastKeyBlob.GetData(), m_settings.m_fastEncryptedMessageKeyLength / 8) != true)
                {
                    rollback.Run();
                    failCase();
                    return;
                }

                // ACR에서 사용될 재접속용 credential.
                if (ErrorInfoPtr err = CCryptoRsa::DecryptSessionKeyByPrivateKey(credentialBlock, encryptedCredential, m_selfXchgKey))
                {
                    NotifyProtocolVersionMismatch(rc);
                    return;
                }

                rollback.Run();

                // 복호화 성공
                rc->m_sessionKey = rc_sessionKey;
                rc->m_sessionKey->m_aesKeyBlock = aesKeyBlob;	// copy
                rc->m_sessionKey->m_fastKeyBlock = fastKeyBlob;	// copy
                rc->m_credentialBlock = credentialBlock;	// copy
            }
        }

        // protocol version match and assign received session key
        if ( protocolVersion != m_protocolVersion || internalVersion != m_internalVersion )
        {
            NotifyProtocolVersionMismatch(rc);
            return;
        }

        rc->m_enableAutoConnectionRecovery = enableAutoConnectionRecovery;
        rc->m_closeNoPingPongTcpConnections = CloseNoPingPongTcpConnections;

        // 접속 허가를 해도 되는지 유저에게 묻는다.
        // 이 스레드는 이미 CS locked이므로 여기서 하지 말고 스레드 풀에서 콜백받은 후 후반전으로 ㄱㄱ
        if (m_eventSink_NOCSLOCK != nullptr || OnConnectionRequest.m_functionObject != nullptr)
        {
            // 콜백을 일으킴. 자세한 것은 이 함수의 주석을 참고.
            EnqueClientJoinApproveDetermine(rc, userData);
        }
        else
        {
            // 사용자 정의 콜백 함수가 없음.
            // 무조건 성공 처리.
            ProcessOnClientJoinApproved(rc, ByteArray());
        }
    }

    void CNetServerImpl::IoCompletion_ProcessMessage_UnreliablePing(CMessage &msg, const shared_ptr<CRemoteClient_S>& rc)
    {
        // remote client가 없으면, 처리 불가능.
        if (!rc)
        {
            assert(false); // 서버를 debug build로 실행중일 때는, 해커의 공격이 아닌 여기 오지 말아야 한다.
            return;
        }

        AssertIsNotLockedByCurrentThread();

        // 메시지 읽기.
        int64_t clientLocalTime;
        int measuredLastLag;
        int64_t speed = 0;
        int CSLossPercent = 0;

        if (!msg.Read(clientLocalTime))
            return;

        if (!msg.Read(measuredLastLag))
            return;

        if (!msg.ReadScalar(speed))
            return;

        if (!msg.Read(CSLossPercent))
            return;

        CMessage sendMsg; // ping 응답 즉 pong

            // rc 비파괴 보장이 아니라, rc 자체를 액세스하는거라면, main lock 필요.
            // caller가 성능에 민감하다. 그래서 여기서 main lock을 한다.
        CriticalSectionLock mainLock(GetCriticalSection(), true);

        rc->AssertIsSocketNotLockedByCurrentThread();
        // 		CriticalSectionLock clk(GetCriticalSection(), true);
        // 		CHECK_CRITICALSECTION_DEADLOCK(this);

        int64_t currTime = GetPreciseCurrentTimeMs();

        // 가장 마지막에 unreliable ping을 받은 시간을 체크해둔다.
        rc->m_safeTimes.Set_lastUdpPingReceivedTime(GetCachedServerTimeMs());

        // 측정된 랙도 저장해둔다.
        rc->m_lastPingMs = max(measuredLastLag, 1); //측정된 값이 1보다 작을경우 1로 측정값을 바꾼다.
        rc->m_recentPingMs = max(LerpInt(rc->m_recentPingMs, measuredLastLag, 1, 2), 1);
        rc->m_unreliableRecentReceivedSpeed = speed;

        if (rc->m_ToClientUdp_Fallbackable.m_udpSocket != nullptr)
        {
            CriticalSectionLock lock(rc->m_ToClientUdp_Fallbackable.m_udpSocket->GetSendQueueCriticalSection(), true);
            rc->m_ToClientUdp_Fallbackable.m_udpSocket->SetReceiveSpeedAtReceiverSide(rc->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere(), speed, CSLossPercent, GetPreciseCurrentTimeMs());
        }

        // 에코를 unreliable하게 보낸다.
        sendMsg.UseInternalBuffer();
        Message_Write(sendMsg, MessageType_UnreliablePong);
        sendMsg.Write(clientLocalTime);
        sendMsg.Write(currTime);
        speed = 0;
        if (rc->m_ToClientUdp_Fallbackable.m_udpSocket != nullptr)
        {
            CriticalSectionLock lock(rc->m_ToClientUdp_Fallbackable.m_udpSocket->GetSendQueueCriticalSection(), true);
            speed = rc->m_ToClientUdp_Fallbackable.m_udpSocket->GetRecentReceiveSpeed(rc->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere());
        }
        GetUnreliableMessagingLossRatioPercent(rc->m_HostID, &CSLossPercent);
        sendMsg.WriteScalar(speed);
        sendMsg.Write(CSLossPercent);

        // 여기는 성능 안 민감.
        // 그리고 아래 함수는 rc의 각종 멤버를 액세스함. 즉 main lock 필요.
        // 그래서 여기서 main unlock 안함.

        rc->m_ToClientUdp_Fallbackable.AddToSendQueueWithSplitterAndSignal_Copy(rc->GetHostID(), CSendFragRefs(sendMsg), SendOpt(MessagePriority_Ring0, true));
    }

    void CNetServerImpl::IoCompletion_ProcessMessage_SpeedHackDetectorPing(CMessage& /*msg*/, const shared_ptr<CRemoteClient_S>& rc)
    {
        // remote client가 없으면, 처리 불가능.
        if (rc == nullptr)
        {
            assert(false); // 서버를 debug build로 실행중일 때는, 해커의 공격이 아닌 여기 오지 말아야 한다.
            return;
        }

        // caller가 성능에 민감하다. 그래서 여기서 main lock을 한다.
        AssertIsNotLockedByCurrentThread();
        CriticalSectionLock mainLock(GetCriticalSection(), true);

        rc->AssertIsSocketNotLockedByCurrentThread();

        // 		if (!isReadlUdp)
        // 			rc->m_tcpLayer->AssertIsZeroUseCount();
        // 		else
        // 			rc->m_ownedUdpSocket->AssertIsZeroUseCount();

        AssertIsLockedByCurrentThread();
        // 		CriticalSectionLock clk(GetCriticalSection(), true);
        // 		CHECK_CRITICALSECTION_DEADLOCK(this);

        // 스피드핵 체크를 한다.
        rc->DetectSpeedHack();
    }

    void CNetServerImpl::IoCompletion_ProcessMessage_Rmi(
        CReceivedMessage &receivedInfo,
        bool messageProcessed,
        const shared_ptr<CRemoteClient_S>& rc,
        const shared_ptr<CSuperSocket>& socket)
    {
        // remote client가 없으면, 처리 불가능.
        if (!rc)
        {
            assert(false); // 서버를 debug build로 실행중일 때는, 해커의 공격이 아닌 여기 오지 말아야 한다.
            return;
        }

        AssertIsNotLockedByCurrentThread();

        shared_ptr<CReferrerHeart> acq;
        TryGetReferrerHeart(acq);
        if(!acq)
        {
            // 이게 없어서, 서버 종료시 크래시가 나는 경우가 있었다.
            return;
        }

        // ProudNet layer의 RMI이면 아래 구문에서 true가 리턴되고 결국 user thread로 넘어가지 않는다.
        // 따라서 아래처럼 하면 된다.
        {
            // 이 값은 MessageType_Rmi 다음의 offset이다.
            CMessage& payload = receivedInfo.GetReadOnlyMessage();
            int rmiReadOffset = payload.GetReadOffset();

            // 프넷 내부 RMI를 수행한다.
            messageProcessed |= m_c2sStub.ProcessReceivedMessage(receivedInfo, rc->m_hostTag);
            if ( !messageProcessed ) // 프넷 내부용 RMI가 아니라면
            {
                // 유저 스레드에서 RMI를 처리하도록 enque한다.
                payload.SetReadOffset(rmiReadOffset);

                CFinalUserWorkItem workItem;
                CReceivedMessage& receivedInfo2 = workItem.Internal().m_unsafeMessage;
                receivedInfo2.m_unsafeMessage.UseInternalBuffer();
                receivedInfo2.m_unsafeMessage.AppendByteArray(
                    payload.GetData() + payload.GetReadOffset(),
                    payload.GetLength() - payload.GetReadOffset());
                receivedInfo2.m_unsafeMessage.SetSimplePacketMode(payload.IsSimplePacketMode());
                receivedInfo2.m_relayed = receivedInfo.m_relayed;
                receivedInfo2.m_remoteAddr_onlyUdp = receivedInfo.m_remoteAddr_onlyUdp;
                receivedInfo2.m_remoteHostID = receivedInfo.m_remoteHostID;
                receivedInfo2.m_encryptMode = receivedInfo.m_encryptMode;
                receivedInfo2.m_compressMode = receivedInfo.m_compressMode;
                workItem.Internal().m_netCoreReferrerHeart = MOVE_OR_COPY(acq);
                workItem.Internal().m_type = UWI_RMI;

                if (rc != nullptr && socket->GetSocketType() == SocketType_Udp)
                {
                    // stale 문제가 있지만 이정도는 감수하자.
                    // 지금은 main lock cost가 훨씬 문제가 되니까.
                    rc->m_toServerSendUdpMessageSuccessCount++;
                }

                if (workItem.Internal().m_netCoreReferrerHeart)
                    m_userTaskQueue.Push(rc, workItem);
            }
        }
    }

    void CNetServerImpl::IoCompletion_ProcessMessage_UserOrHlaMessage(
        CReceivedMessage &receivedInfo,
        FinalUserWorkItemType UWIType,
        bool /*messageProcessed*/,
        const shared_ptr<CRemoteClient_S>& rc,
        const shared_ptr<CSuperSocket>& socket)
    {
        // remote client가 없으면, 처리 불가능.
        if (!rc)
        {
            assert(false); // 서버를 debug build로 실행중일 때는, 해커의 공격이 아닌 여기 오지 말아야 한다.
            return;
        }

        AssertIsNotLockedByCurrentThread();

        // main unlock 상태에서, user worker thread에서 처리할 수 있게 user task queue로 옮긴다.

        rc->AssertIsSocketNotLockedByCurrentThread();

        CMessage& payload = receivedInfo.GetReadOnlyMessage();

        // 유저 스레드에서 사용자 정의 메시지를 처리하도록 enque한다.
        CFinalUserWorkItem workItem;
        TryGetReferrerHeart(workItem.Internal().m_netCoreReferrerHeart);
        CReceivedMessage& receivedInfo2 = workItem.Internal().m_unsafeMessage;
        receivedInfo2.m_unsafeMessage.UseInternalBuffer();
        receivedInfo2.m_unsafeMessage.AppendByteArray(
            payload.GetData() + payload.GetReadOffset(),
            payload.GetLength() - payload.GetReadOffset());
        receivedInfo2.m_unsafeMessage.SetSimplePacketMode(payload.IsSimplePacketMode());
        receivedInfo2.m_relayed = receivedInfo.m_relayed;
        receivedInfo2.m_remoteAddr_onlyUdp = receivedInfo.m_remoteAddr_onlyUdp;
        receivedInfo2.m_remoteHostID = receivedInfo.m_remoteHostID;
        receivedInfo2.m_encryptMode = receivedInfo.m_encryptMode;
        receivedInfo2.m_compressMode = receivedInfo.m_compressMode;
        workItem.Internal().m_type = UWIType;

        if (rc != nullptr && socket->GetSocketType() == SocketType_Udp)
        {
            // stale 문제가 있지만 이정도는 감수하자.
            // 지금은 main lock cost가 훨씬 문제가 되니까.
            rc->m_toServerSendUdpMessageSuccessCount++;
        }

        if (workItem.Internal().m_netCoreReferrerHeart)
            m_userTaskQueue.Push(rc, workItem);
    }

    /*
    송신자는 Relay를 요청할 때 암호화 플래그를 집어넣는다.
    이 함수에서는 해당 플래그를 보고 복호화를 해줄 필요가 있을때 복호화를 해준다.
    보낸 클라가 WebGL인지 Non-WebGL인지에 따라 암호/복호화 여부가 달라진다.
    WebGL에서 보낸 경우 암호화 되어있지 않다.
    1. 수신자가 WebGL : 그대로 전달.
    2. 수신자가 Non-WebGL : 셋팅된 플래그(EM_Fast or EM_Secure)대로 서버에서 S2C키로 암호화해서 전달하라는 요청.
    Non-WebGL에서 보낸 경우 암호화 되어있을 수 있다.
    1. 수신자가 WebGL : C2S키로 암호화가 되어있으니 복호화를 해서 전달하라는 요청.
    2. 수신자가 Non-WebGL : C2C키로 암호화가 되어있으니 그대로 전달.

    **** 주의 **** relay1 송신자 및 relay2 수신자가 모두 non WebGL인 경우, decryptedOutput에는 C2C 세션키로 암호화되어있던 payload가 그대로 들어간다.
    */
    DecryptResult CNetServerImpl::GetDecryptedPayloadByRemoteClient(
        const shared_ptr<CRemoteClient_S>& rc,
        const EncryptMode& encMode,
        CMessage& msg,
        CMessage& decryptedOutput)
    {
        HostID remoteHostID;

        {
            CriticalSectionLock lock(GetCriticalSection(), true);

            // WebSocket은 암호화를 하지 않는다.
            if (rc->m_tcpLayer->GetSocketType() == SocketType_WebSocket)
                return DecryptResult_NoCare;

            // 웹소켓이 아닌데 EM_None이면 암호화를 하지 않았거나 C2C로 암호화 되어있어서 서버가 관여할 필요가 없다.
            if (encMode == EM_None && rc->m_tcpLayer->GetSocketType() != SocketType_WebSocket)
                return DecryptResult_NoCare;

            remoteHostID = rc->GetHostID(); // Remote client의 hostID 값은 몇 분간은 재사용 안된다. 따라서 ABA problem 걱정 뚝.
        }

        // 이 아래부터는 WebSocket이 아닌 송신자가 EM_Fast/EM_Secure로 서버로 송신했을 때이다. C2S키로 복호화한다.

        decryptedOutput.UseInternalBuffer();

        // 복호화를 한다.
        // 병렬 병목을 피하기 위해 main lock 없이 한다.
        String errorOut;
        LogLevel outLogLevel = LogLevel_Ok;
        shared_ptr<CSessionKey> sessionKey;
        if (!TryGetCryptSessionKey(remoteHostID, sessionKey, errorOut, outLogLevel)) // C/S session key 가져오기
            sessionKey = shared_ptr<CSessionKey>();

        bool decryptedResult = false;
        if (sessionKey.get() == nullptr)
        {
            // 암호화된 메시지를 받았으나 그것을 처리할 remote 가 없는 경우.
            // 비암호화 메시지의 경우도 remote 객체가 이미 사라졌는데 받는 경우가 있을 수 있다. 그때는 무시하고 있다.
            // sesstionKey 가 nullptr 인 경우는 2가지가 있따. remote 가 없는경우. 서버 시작시 암호화 사용 셋팅을 하지 않은경우.
            // remote 가 없는 경우는 무시하면 되지만, 셋팅을 하지 않은 경우는 error 를 날려주어야 한다.
            if (outLogLevel == LogLevel_Error)
            {
                if (errorOut.IsEmpty())
                    errorOut = _PNT("Make sure that enableP2PEncryptedMessaging is true.");

                EnqueError(ErrorInfo::From(ErrorType_DecryptFail, remoteHostID, errorOut));
            }

            return DecryptResult_Fail;
        }

        assert(sessionKey.get() != nullptr); // 뻔한 방어 코딩이지만 넣어두자. 유지보수하다 실수하기도.

        MessageType msgType;
        if (!Message_Read(msg, msgType))
            return DecryptResult_Fail;

        if (msgType < MessageType_Encrypted_Reliable_EM_Secure || msgType > MessageType_Encrypted_UnReliable_EM_Fast)
            return DecryptResult_Fail;

        ErrorInfoPtr errorInfo;
        if (msgType == MessageType_Encrypted_Reliable_EM_Secure || msgType == MessageType_Encrypted_UnReliable_EM_Secure)
        {
            // 일반 암호화
            decryptedOutput.UseInternalBuffer();
            decryptedResult = CCryptoAes::DecryptMessage(sessionKey->m_aesKey, msg, decryptedOutput, msg.GetReadOffset());
            if (decryptedResult == false)
                errorInfo = ErrorInfo::From(ErrorType_DecryptFail, remoteHostID, _PNT("decryption failure 1"));
        }
        else if (msgType == MessageType_Encrypted_Reliable_EM_Fast || msgType == MessageType_Encrypted_UnReliable_EM_Fast)
        {
            // 빠른 암호화
            decryptedOutput.UseInternalBuffer();
            decryptedResult = CCryptoFast::DecryptMessage(sessionKey->m_fastKey, msg, decryptedOutput, msg.GetReadOffset(), errorInfo);
        }

        if (decryptedResult == false)
        {
            CriticalSectionLock clk(GetCriticalSection(), true);
            CHECK_CRITICALSECTION_DEADLOCK(this);

            errorInfo->m_remote = remoteHostID;
            EnqueError(errorInfo);

            // 암호화가 실패하면 원상복구시키고, 그냥 원본 메시지(깨진 암호화된 메시지?)를 output한다.
            // 그리고 false를 리턴한다. caller가 이를 처리할 것이다.

            return DecryptResult_Fail;
        }

        if (msgType == MessageType_Encrypted_Reliable_EM_Secure || msgType == MessageType_Encrypted_Reliable_EM_Fast)
        {
            // 시리얼 값의 정상 여부를 체크한다.
            CryptCount decryptCount1;
            if (!decryptedOutput.Read(decryptCount1))
            {
                CriticalSectionLock clk(GetCriticalSection(), true);
                CHECK_CRITICALSECTION_DEADLOCK(this);

                EnqueError(ErrorInfo::From(ErrorType_DecryptFail, remoteHostID, _PNT("decryptCount1 read failed!!")));

                return DecryptResult_Fail;
            }
        }

        return DecryptResult_Success;
    }

    void CNetServerImpl::NotifyProtocolVersionMismatch(const shared_ptr<CRemoteClient_S>& rc)
    {
        AssertIsLockedByCurrentThread();

        CMessage sendMsg;
        sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
        Message_Write(sendMsg, MessageType_NotifyProtocolVersionMismatch);

        rc->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(
            rc->m_tcpLayer,
            CSendFragRefs(sendMsg), SendOpt(), m_simplePacketMode);
    }

    void CNetServerImpl::NotifyLicenseMismatch(const shared_ptr<CRemoteClient_S>& rc)
    {
        AssertIsLockedByCurrentThread();

        CMessage sendMsg;
        sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
        Message_Write(sendMsg, MessageType_NotifyLicenseMismatch);

        rc->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(
            rc->m_tcpLayer,
            CSendFragRefs(sendMsg), SendOpt(), m_simplePacketMode);
    }

    bool CNetServerImpl::ProcessMessage_FromUnknownClient(AddrPort from, CMessage& recvedMsg, const shared_ptr<CSuperSocket>& udpSocket)
    {
        int orgReadOffset = recvedMsg.GetReadOffset();
        MessageType type;
        if ( Message_Read(recvedMsg, type) == false )
        {
            recvedMsg.SetReadOffset(orgReadOffset);
            return false;
        }

        bool messageProcessed = false;
        switch ( (int)type )
        {
            case MessageType_ServerHolepunch:
                IoCompletion_ProcessMessage_ServerHolepunch(recvedMsg, from, udpSocket);
                messageProcessed = true;
                break;
            case MessageType_PeerUdp_ServerHolepunch:
                IoCompletion_ProcessMessage_PeerUdp_ServerHolepunch(
                    recvedMsg,
                    from,
                    udpSocket);
                messageProcessed = true;
                break;
        }

        if ( !messageProcessed )
        {
            recvedMsg.SetReadOffset(orgReadOffset);
            return false;
        }

        return true;
    }

    // 이미 릴레이 타겟을 모두 찾아낸 상태에서, 각 릴레이 타겟에 대한 relay2 메시지를 멀티캐스트한다.
    // mainLock: AddToSendQueue를 하기 전에 main unlock을 하려면 여기를 세팅할 것. null을 넣으면 그런거 안한다.
    void CNetServerImpl::IoCompletion_MulticastUnreliableRelay2(
        CriticalSectionLock *mainLock,
        const HostIDArray &relayDest,
        HostID relaySenderHostID,
        EncryptMode& encMode,
        CMessage &payload,
        MessagePriority priority,
        int64_t uniqueID,
        bool fragmentOnNeed)
    {
        if(!mainLock)
            AssertIsNotLockedByCurrentThread(); // code profile 결과 main lock도 병목이라서.

        // 수신자가 없으면 성능 낭비하지 말자.
        intptr_t relayCount = relayDest.GetCount();
        if ( relayCount <= 0 )
            return;

        // 인자로 넘어온 CMessage &payload는 복호화 되어있는 메시지
        int payloadLength = payload.GetLength();

        // 수신자 목록이 저장될 곳
        
        POOLED_LOCAL_VAR(UnreliableDestInfoArray, relayDestlist);

        // 보낼 메시지를 조립.
        CSmallStackAllocMessage header;
        Message_Write(header, MessageType_UnreliableRelay2);
        header.Write(relaySenderHostID);		// sender
                                                // (unreliable은 frame number가 불필요)
#ifdef _DEBUG
        CMessageTestSplitterTemporaryDisabler dd(header);
#endif
        header.WriteScalar(payloadLength);	// payload length

        CSendFragRefs fragList;
        fragList.Add(header);
        fragList.Add(payload);

        SendOpt sendOpt = SendOpt::CreateFromRmiContextAndClearRmiContextSendFailedRemotes(g_UnreliableSendForPN);
        sendOpt.m_priority = priority;
        sendOpt.m_uniqueID.m_value = uniqueID;
        sendOpt.m_fragmentOnNeed = fragmentOnNeed;

        {
            // 수신대상 연산 등을 하므로 main lock 필요
            CriticalSectionLock mainLock_(GetCriticalSection(), true);

            // 각 dest들을 리스트에 추가한다.
            for (int i = 0; i < relayCount; i++)
            {
                HostID dest = relayDest[i];

                // remote를 찾고
                shared_ptr<CRemoteClient_S> destRC;
                if (TryGetAuthedClientByHostID(dest, destRC))
                {
                    // code profile 결과 Add()가 많이 먹는다. 게다가 AddrPort가 멤버로 있어서 IPv6 복사도 크다.
                    // 따라서 Add()대신 불편해도 이렇게 한다.
                    int c = relayDestlist.GetCount();
                    relayDestlist.SetCount(c + 1); // 새 항목 공간을 미리 만들고
                    UnreliableDestInfo &destinfo = relayDestlist[c]; // 그것의 리퍼런스 변수

                    destinfo.sendTo = dest;

                    // JIT UDP socket creation.
                    RemoteClient_NewLocalUdpSocketAndRequestNewRemoteUdpSocketOnNeed(destRC);

                    // UDP or TCP socket을 선택한다. Relay2를 위해.
                    if (destRC->m_ToClientUdp_Fallbackable.m_realUdpEnabled)
                    {
                        destinfo.m_socket = destRC->m_ToClientUdp_Fallbackable.m_udpSocket;
                        destinfo.destAddr = destRC->m_ToClientUdp_Fallbackable.GetUdpAddrFromHere();
                        destinfo.useUdpSocket = true;
                    }
                    else
                    {
                        destinfo.m_socket = destRC->m_tcpLayer;
                        destinfo.useUdpSocket = false;
                    }
                    
                    // 수신자가 WebGL이면 payload(복호화됨)을 그대로 사용.
                    if (destRC->m_runtimePlatform == RuntimePlatform_UWebGL)
                    {
                        destinfo.payloadToBeSent = &fragList;
                    }
                    else // 수신자가 Non-WebGL이면 S2C키로 암호화
                    {
                        if (encMode == EM_None)
                        {
                            destinfo.payloadToBeSent = &fragList;
                        }
                        else
                        {
                            // 세션키가 다르기 때문에 하나씩 공간을 생성해준다.
                            // 어차피 여기 왔으면 wss나 AES로 암복호화되어 릴레이되고 있는거다. new,delete 부하보다 엄청 상회한다.
                            // 따라서 이 CSendFragRefs에 대해 new,delete하는 것의 부하에 대해 너무 걱정하지 말자. 정 문제가 되면 추후 code profile 후 Proud.CObjectPool를 적용하거나 하도록 하자.
                            CSendFragRefs* sendData = new CSendFragRefs(); // delete 하는 곳이 있다. #payloadToBeSent 쪽에 참고할 것.

                            CMessage encryptedPayload;
                            encryptedPayload.UseInternalBuffer();

                            MessageType msgType;

                            bool encryptOK = false;
                            ErrorInfoPtr errorInfo;
                            if (encMode == EM_Secure)
                            {
                                msgType = MessageType_Encrypted_UnReliable_EM_Secure;
                                encryptOK = CCryptoAes::EncryptMessage(destRC->m_sessionKey->m_aesKey, payload, encryptedPayload, 0);

                                if (!encryptOK)
                                    errorInfo = ErrorInfo::From(ErrorType_EncryptFail, destRC->GetHostID(),
                                        _PNT("Please Check Encrypt Error CStartServerParameter"));
                            }
                            else 
                            {
                                if (encMode != EM_Fast) // EM_Secure 아니면 EM_Fast이어야 한다.
                                {
                                    EnqueError(ErrorInfo::From(ErrorType_Unexpected, HostID_None, _PNT("Unexpected encrypt mode!")));
                                }
                                // EM_Fast에 대한 처리
                                msgType = MessageType_Encrypted_UnReliable_EM_Fast;
                                encryptOK = CCryptoFast::EncryptMessage(destRC->m_sessionKey->m_fastKey, payload, encryptedPayload, 0, errorInfo);
                            }

                            if (encryptOK)
                            {
                                CSmallStackAllocMessage encHeader;
                                Message_Write(encHeader, MessageType_UnreliableRelay2);
                                encHeader.Write(relaySenderHostID);		// sender
                                encHeader.WriteScalar(sizeof(int8_t) + encryptedPayload.GetLength());	// encrypted payload length
                                encHeader.Write((int8_t)msgType);	// messageType

                                sendData->Add(encHeader);
                                sendData->Add(encryptedPayload);

                                destinfo.payloadToBeSent = sendData;		// #payloadToBeSent 여기서 delete를 안 해도, 다른 #payloadToBeSent 표시 영역에서 delete한다.
                            }
                            else
                            {
                                errorInfo->m_remote = dest;
                                EnqueError(errorInfo);
                                destinfo.payloadToBeSent = nullptr;

                                delete sendData;
                                sendData = nullptr;
                            }
                        }
                    }
                }
            }
        } // main unlock

        // NOTE: relay1 메시지 자체가 MTU를 넘는 경우가 있을 수 있다. 따라서 relay2에서도 MTU 단위로 쪼개는 것이 필요할 수 있다. false로 세팅하지 말자.
        //sendOpt.m_INTERNAL_USE_fraggingOnNeed = false;

        // caller가 MessageType_UnreliableRelay1_RelayDestListCompressed를 처리하고 있었다면
        // caller는 main lock 상태에서 여기를 호출한다. 따라서 unlock을 해주어야
        // 병목 예방하면서 하기 LowContextSwitchingLoop를 할 수 있다.
        if (mainLock)
        {
            mainLock->Unlock();
        }
        else
        {
            // 이 스레드는 잠그고 있지 않음이 보장되어야 아래 멀티코어 멀티캐스트가 효력을 낸다.
            AssertIsNotLockedByCurrentThread();
        }

        int64_t currTime = GetPreciseCurrentTimeMs();
        LowContextSwitchingLoop(relayDestlist.GetData(), relayDestlist.GetCount(),
            [](UnreliableDestInfo& dest)
        {
            return &(dest.m_socket->GetSendQueueCriticalSection());
        },
            [/*&fragList, */encMode, &sendOpt, currTime](UnreliableDestInfo& dest, CriticalSectionLock& sendQueueLock)->bool
        {
            if (dest.payloadToBeSent != nullptr)
            {
                if (dest.useUdpSocket)
                {
                    dest.m_socket->AddToSendQueueWithSplitterAndSignal_Copy(
                        dest.m_socket,
                        dest.sendTo,
                        FilterTag::CreateFilterTag(HostID_Server, dest.sendTo),
                        dest.destAddr,
                        *dest.payloadToBeSent,//fragList,
                        currTime, //성능에 민감합니다. 미리 SendToUnreliableDestInfo의 멤버 변수로 시간을 넣은 후 여기서 그것을 사용하게 하세요.
                        sendOpt);
                }
                else
                {
                    dest.m_socket->AddToSendQueueWithSplitterAndSignal_Copy(
                        dest.m_socket,
                        *dest.payloadToBeSent,
                        sendOpt);
                }
                sendQueueLock.Unlock();

                // #payloadToBeSent 암호화를 했다는건 각자의 공간을 생성했다는 것이니 지워줘야함. 
                if (dest.m_socket->GetSocketType() != SocketType_WebSocket && encMode != EM_None)
                {
                    delete dest.payloadToBeSent;
                    dest.payloadToBeSent = nullptr;
                }
            }
            return true;
        });
    }

    void CNetServerImpl::ShowThreadExceptionAndPurgeClient(shared_ptr<CRemoteClient_S>  client, const PNTCHAR* where, const PNTCHAR* reason)
    {
        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        if ( client )
        {
            String txt;
            txt.Format(_PNT("Expelling the Client %d from %s due to %s."), client->m_HostID, where, reason);
            EnqueError(ErrorInfo::From(ErrorType_ExceptionFromUserFunction, client->m_HostID, txt));
            GarbageHost(client, ErrorType_ExceptionFromUserFunction, ErrorType_TCPConnectFailure, ByteArray(), _PNT("STEXAX"), SocketErrorCode_Ok);
        }
        else
        {
            String txt;
            txt.Format(_PNT("%s Error Occurred from %s but Clients Could Not Be Discerned."), reason, where);
            EnqueError(ErrorInfo::From(ErrorType_ExceptionFromUserFunction, HostID_None, txt));
        }
    }

    // 이 객체는 IThreadReferrer를 상속받는다.
    // 이 객체에 대해 CThreadPoolImpl.RegisterReferrer를 호출한 전제하에
    // 이 객체에 대해 post custom event를 한 경우 이것이 호출된다.
    void CNetServerImpl::OnCustomValueEvent(const ThreadPoolProcessParam& param, CWorkResult* workResult, CustomValueEvent customValue)
    {
        AssertIsNotLockedByCurrentThread();

        switch ((int)customValue)
        {

            /*이제는 listner thread와 networker thread가 동일합니다.
                따라서 CustomValueEvent_NewClient를 제거해 버립시다.
                그리고 CustomValueEvent_NewClient를 post하는 곳에서 직접 CustomValueEvent_NewClient를
                받아 처리하는 루틴이 직접 실행되게 수정합시다.

                LS에서는 ProcessNewClients에 대응하는 것이 CLanServerImpl::IoCompletion_NewClientCase 이므로 그걸 손대시길.
                */
            // 		case CustomValueEvent_NewClient:      // 처음 시작 조건
            // 			// 서버 종료 조건이 아니면 클라를 IOCP에 엮고 다음 송신 절차를 시작한다.
            // 			if (m_netThreadPool != nullptr)
            // 			{
            // 				ProcessNewClients();
            // 			}
            // 			break;

// #if 0
// 			case CustomValueEvent_Heartbeat:
// 				/*Heartbeat은 NS,NC,LS,LC 공통으로 필요로 합니다.
// 					Heartbeat은 OnTick과 달리 PN 내부로직만을 처리합니다.
// 					속도에 민감하므로 절대 블러킹 즉 CPU idle time이 없어야 하고요.
//
// 					따라서 아래 Heartbeat 함수 중에서 ProcessCustomValueEvent에 옮겨야 할 루틴은 옮기시고,
// 					나머지는 CNetServerImpl.Heartbeat에 그대로 남깁시다.
// 					*/
// 				Heartbeat();
// 				break;
// #endif
                // 		case CustomValueEvent_OnTick:
                // 			/*OnTick을 호출하는 로직도 NetCore의 공통분모로 옮겨버립시다.
                // 			   즉 ProcessCustomValueEvent르 모두 옮겨버립시다.
                // 			   함수만 옮길게 아니라 관련 변수도 NetCore로 모두 옮겨야 하고 그변수들의 참조루틴도 필요한 것들을
                // 			   모두 NetCore로 옮겨야 합니다.
                // 			   */
                //
                // 			// 여기 왔을때, 이 스레드는 user worker thread이다.
                // 			OnTick();
                // 			break;
                // 		case CustomValueEvent_DoUserTask:
                // 			/*DoUserTask을 호출하는 로직도 NetCore의 공통분모로 옮겨버립시다.
                // 				즉 ProcessCustomValueEvent르 모두 옮겨버립시다.
                // 				함수만 옮길게 아니라 관련 변수도 NetCore로 모두 옮겨야 하고 그변수들의 참조루틴도 필요한 것들을
                // 				모두 NetCore로 옮겨야 합니다.
                // 				*/
                //
                // 			// 여기 왔을때, 이 스레드는 user worker thread이다.
                // 			DoUserTask();
                // 			break;
#if defined (_WIN32)
            case CustomValueEvent_IssueFirstRecv:
                // Start or Connect를 호출한 스레드에서 직접 socket을 생성할 때에 대한 처리입니다.
                // NS,NC,LS,LC의 중복코드 방지를 위해
                // NetCore.m_issueFirstRecvNeededSockets라는 큐를 만들어 거기 넣기만 하면 알아서 first recv가
                // 걸리게 만드는 것이 좋아 보입니다. 수정합시다.
                TcpListenSockets_IssueFirstAccept();
                StaticAssignedUdpSockets_IssueFirstRecv();
                return;

#endif // _WIN32
                // 		case CustomValueEvent_End:
                // 			/* NetCore의 공통분모로 옮겨버립시다.
                // 				즉 ProcessCustomValueEvent르 모두 옮겨버립시다.
                // 				함수만 옮길게 아니라 관련 변수도 NetCore로 모두 옮겨야 하고 그변수들의 참조루틴도 필요한 것들을
                // 				모두 NetCore로 옮겨야 합니다.
                // 				*/
                // 			EndCompletion();
                // 			break;
                // 		default:
                // 			assert(0);
                // 			break;
        }

        // NetCore도 custom value event를 처리해야 한다.
        CNetCoreImpl::ProcessCustomValueEvent(param, workResult, customValue);
    }

    void CNetServerImpl::ProcessHeartbeat()
    {
        Heartbeat();
    }

    void CNetServerImpl::Heartbeat()
    {
#ifdef EXPOSE_CRASH_THAN_CONCEAL_EXCEPTION // 사용자가 핸들링하지 않은 에러는 차라리 unhandled exception으로 이어지게 해서 미니덤프를 남기는 것이 conceal보다 훨씬 문제해결을 빨리 해낸다.
        try
        {
#endif // EXPOSE_CRASH_THAN_CONCEAL_EXCEPTION

            Heartbeat_One();

#ifndef PROUDSRCCOPY
                ((CPNLic*)m_lic)->POSH(GetPreciseCurrentTimeMs());
#endif


#ifdef EXPOSE_CRASH_THAN_CONCEAL_EXCEPTION // 사용자가 핸들링하지 않은 에러는 차라리 unhandled exception으로 이어지게 해서 미니덤프를 남기는 것이 conceal보다 훨씬 문제해결을 빨리 해낸다.
        }
        catch ( std::bad_alloc &ex )
        {
            BadAllocException(ex);
        }
        catch ( std::exception &e )
        {
            String txt;
            txt.Format(_PNT("std.exception(%s)"), StringA2T(e.what()));
            ShowThreadUnexpectedExit(_PNT("HB1"), txt);
        }
        catch ( _com_error &e )
        {
            String txt;
            txt.Format(_PNT("_com_error(%s)"), (const PNTCHAR*)e.Description());
            ShowThreadUnexpectedExit(_PNT("HB2"), txt);
        }
        catch ( void* )
        {
            ShowThreadUnexpectedExit(_PNT("HB3"), _PNT("void*"), true);
        }
#endif // EXPOSE_CRASH_THAN_CONCEAL_EXCEPTION
    }

    void CNetServerImpl::Heartbeat_One()
    {
        // m_sendReadyList을 지역변수에서 쥐고 있도록 합니다. 아래 나머지 루틴들이 작동하는 동안 이게 중도 사라지는 일이 없도록 하기 위해서 입니다.
        shared_ptr<CSendReadySockets> sendReadyList = m_sendReadyList;

        // change 35143에서 서버 크래시 수정때문에 아래 if구문이 있다고 수정되어 있으나, 왜 있어야 하는지 모르겠다. 일단 삭제.
// 		if (!sendReadyList)
// 		{
// 			return;
// 		}

        int64_t currentTimeMs = GetPreciseCurrentTimeMs();

        if ( m_PurgeTooOldUnmatureClient_Timer.IsTimeToDo(currentTimeMs) == true )
            PurgeTooOldUnmatureClient();

        if ( m_PurgeTooOldAddMemberAckItem_Timer.IsTimeToDo(currentTimeMs) == true )
            PurgeTooOldAddMemberAckItem();

        // 통신량에 의해 적합 수퍼피어가 달라질 수 있기 때문에
        if ( m_electSuperPeer_Timer.IsTimeToDo(currentTimeMs) == true )
            ElectSuperPeer();

        if ( m_removeTooOldUdpSendPacketQueue_Timer.IsTimeToDo(currentTimeMs) == true )
        {
            DoForLongInterval();

            PooledObjects_ShrinkOnNeed_ClientDll();
            PooledObjects_ShrinkOnNeed_ServerDll();
        }

        if ( m_DisconnectRemoteClientOnTimeout_Timer.IsTimeToDo(currentTimeMs) == true )
        {
            Heartbeat_EveryRemoteClient();
        }

        if ( m_removeTooOldRecyclePair_Timer.IsTimeToDo(currentTimeMs) == true )
        {
            CriticalSectionLock mainlock(GetCriticalSection(), true);
            CHECK_CRITICALSECTION_DEADLOCK(this);
            m_p2pConnectionPairList.RecyclePair_RemoveTooOld(GetCachedServerTimeMs());
        }
        //DoGarbageCollect(); OnCustomValueEvent에서 처리하므로 여기는 불필요

        GarbageTooOldRecyclableUdpSockets();

        LogFreqFailOnNeed();

    }

    void CNetServerImpl::OnThreadBegin()
    {
        if (OnUserWorkerThreadBegin.m_functionObject)
            OnUserWorkerThreadBegin.m_functionObject->Run();
        if ( m_eventSink_NOCSLOCK != nullptr )
            ((INetServerEvent*)m_eventSink_NOCSLOCK)->OnUserWorkerThreadBegin();
    }

    void CNetServerImpl::OnThreadEnd()
    {
        if (OnUserWorkerThreadEnd.m_functionObject)
            OnUserWorkerThreadEnd.m_functionObject->Run();
        if ( m_eventSink_NOCSLOCK != nullptr )
            ((INetServerEvent*)m_eventSink_NOCSLOCK)->OnUserWorkerThreadEnd();
    }

    // remote obj를 파괴하기 전에 콜 하는 정리 함수
    // C# 버전으로 간다면 이 함수가 왜 존재하는지 깨달을 것임
    void CNetServerImpl::Unregister(const shared_ptr<CRemoteClient_S>& rc)
    {
        AssertIsLockedByCurrentThread();

        if (m_userTaskQueue.DoesHostHaveUserWorkItem(rc.get()))
        {
            throw Exception("Cannot delete remote object whose task is still pending!");
        }

        //rc->m_finalUserWorkItemList.Clear();
        rc->ClearFinalUserWorkItemList();
    }

    // 	void CNetServerImpl::SetTimerCallbackIntervalMs(int newVal)
    // 	{
    // 		m_periodicPoster_Tick->SetPostInterval((newVal));
    // 	}

    // 패킷양이 많은지 판단
    // 	bool CNetServerImpl::CheckMessageOverloadAmount()
    // 	{
    // 		if (m_finalUserWorkItemList.GetCount() >= CNetConfig::MessageOverloadWarningLimit)
    // 		{
    // 			return true;
    // 		}
    // 		return false;
    // 	}

    ErrorType CNetServerImpl::SetCoalesceIntervalMs(HostID remote, int intervalMs)
    {
        if ( intervalMs < 0 || intervalMs > 1000 )
        {
            throw Exception("Set out of interval range! Can set interval at 0 ~ 1000.");
        }

        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);
        shared_ptr<CRemoteClient_S> rc = GetAuthedClientByHostID_NOLOCK(remote);

        if ( rc != nullptr )
        {
            rc->m_autoCoalesceInterval = false;
            rc->SetManualOrAutoCoalesceInterval(intervalMs);

            return ErrorType_Ok;
        }

        return ErrorType_InvalidHostID;
    }

    ErrorType CNetServerImpl::SetCoalesceIntervalToAuto(HostID remote)
    {
        CriticalSectionLock clk(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);
        shared_ptr<CRemoteClient_S> rc = GetAuthedClientByHostID_NOLOCK(remote);

        if ( rc )
        {
            rc->m_autoCoalesceInterval = true;
            rc->SetManualOrAutoCoalesceInterval();

            return ErrorType_Ok;
        }

        return ErrorType_InvalidHostID;
    }
    //
    // 	// 이 함수를 지우고 이 기능을 CNetCoreImpl.SendReadyList_Add로 옮기자.
    // 	void CNetServerImpl::SendReadyList_Add(const shared_ptr<CSuperSocket>& socket)
    // 	{
    // 		CriticalSectionLock lock(m_cs, true);
    // 		// send ready list에도 추가한다. main lock 상태가 아닐 수 있음에 주의.
    // 		m_sendReadyList->AddOrSet(sender);
    // 		m_netThreadPool->PostCompletionStatus(this, CustomValueEvent_SendEnqueued);
    //
    // 		// 그리고 tag도 지워버리고(NC에서는 remote peer의 host ID를 미리 알 수 있지만 서버는 안그럼) tag에는 remote object ptr을 가리키되
    // 		// map에서 찾아 validation을 하게 해야 하겠다. 아니면, remote 객체들을 모두 smart ptr, weak ptr로 만들어 안전하게 건드려지는 tag로 바꾸던가.
    // 	}

    void CNetServerImpl::OnMessageSent(int doneBytes, SocketType socketType/*, const shared_ptr<CSuperSocket>& socket*/)
    {
        AssertIsNotLockedByCurrentThread();

        CriticalSectionLock statsLock(m_statsCritSec, true); // 통계 정보만 갱신
        // 주의: 여기서 main lock 하면 데드락 생긴다! 잠금 순서 지키자.
        if ( socketType == SocketType_Tcp )
        {
            m_stats.m_totalTcpSendCount++;
            m_stats.m_totalTcpSendBytes += doneBytes;
        }
        else if(socketType == SocketType_Udp)
        {
            m_stats.m_totalUdpSendCount++;
            m_stats.m_totalUdpSendBytes += doneBytes;
        }
        else if (socketType == SocketType_WebSocket)
        {
            m_stats.m_totalWebSocketSendCount++;
            m_stats.m_totalWebSocketSendBytes += doneBytes;
        }
    }

    // overrided function
    void CNetServerImpl::OnMessageReceived(int doneBytes, CReceivedMessageList& messageList, const shared_ptr<CSuperSocket>& socket)
    {
        AssertIsNotLockedByCurrentThread();

        // ndn 4120에서 socket may be null. main unlock 구간이 있는 동안 해킹된 클라가 ACR 시도를 하게 해서 socket is null이 되게 만드는 경우가 있는지도?
        // 아무튼 널체크 해주어야 한다.
        // socket is null이면 ACR 처리에 의해(있을 수 없지만) 무효화된 remote client에 대한 것이므로, 이 메시지는 그냥 씹어버려도 된다.
        if (socket == nullptr)
        {
            return;
        }

        SocketType socketType = socket->GetSocketType();

        // socket의 critsec은 unlock 상태이다. 따라서 언제든지 다른 스레드에 의해
        // 송신 버퍼는 변경될 수 있다. 하지만 수신 버퍼는 바뀌지 않는다.
        // 왜냐하면 next recv issue를 이 스레드가 추후 걸기 전에는 안오며,
        // 이 소켓에 대한 epoll recv avail event도 이 스레드에서만 발생하기 때문이다.
        // 따라서 이 함수에서 수신버퍼를 건드리는 로직들은 모두 안전하다.
        {
            CriticalSectionLock statsLock(m_statsCritSec, true); // 통계 정보만 갱신

            if ( socketType == SocketType_Tcp )
            {
                m_stats.m_totalTcpReceiveCount++;
                m_stats.m_totalTcpReceiveBytes += doneBytes;
            }
            else if ( socketType == SocketType_Udp )
            {
                m_stats.m_totalUdpReceiveCount++;
                m_stats.m_totalUdpReceiveBytes += doneBytes;
            }
            else if (socketType == SocketType_WebSocket)
            {
                m_stats.m_totalWebSocketReceiveCount++;
                m_stats.m_totalWebSocketReceiveBytes += doneBytes;
            }
        }


/*		// 3003 오류 검사용
        // 서버가 3003을 받을 일 없다.
        for (auto i : messageList)
        {
            MessageInternalLayer match;
            match.m_simplePacketMode = m_simplePacketMode;
            match.Analyze(i.m_unsafeMessage);
            if (match.m_rmiLayer.m_rmiID == 3003)
            {
                puts("---3003!!!---");
                for (auto i : messageList)
                {
                    MessageInternalLayer match;
                    match.m_simplePacketMode = m_simplePacketMode;
                    match.Analyze(i.m_unsafeMessage);
                    match.Dump();
                }
                puts("------");
                assert(0);
            }
        }*/



        ProcessMessageOrMoveToFinalRecvQueue(socket, messageList);
    }

    void CNetServerImpl::ProcessDisconnecting(const shared_ptr<CSuperSocket>& socket, const ErrorInfo& errorInfo)
    {
        Proud::AssertIsNotLockedByCurrentThread(socket->m_cs);

        CriticalSectionLock mainLock(GetCriticalSection(), true);
        CHECK_CRITICALSECTION_DEADLOCK(this);

        shared_ptr<CRemoteClient_S> rc = GetRemoteClientBySocket_NOLOCK(socket, AddrPort::Unassigned);
        if ( rc != nullptr )
        {
            // TCP socket에서 연결이 실패했으므로 연결 해제 처리를 한다.
            // NOTE: GarbageHost 함수 내부의 OnHostGarbaged에서 이미 처리하도록 되어 있습니다.
            RemoteClient_GarbageOrGoOffline(
                rc,
                errorInfo.m_errorType,				// ExtractMessage가 준 것이 아니면, ErrorType_DisconnectFromLocal 혹은 ErrorType_DisconnectFromRemote가 들어있어야 한다.
                errorInfo.m_detailType,
                rc->m_shutdownComment,
                errorInfo.m_comment,
                _PNT("PD"),
                errorInfo.m_socketError);

            // 혹시 이거 해야 하나? 상관없지만. rc->IssueDispose(ErrorType_DisconnectFromRemote, ErrorType_TCPConnectFailure, ByteArray(), _PNT("PD"), r);
            rc->WarnTooShortDisposal(_PNT("PD"));

            /*  == > 여기에 도착했을 때는 remoteAddr이 없어서 오류가 발생하여 임시로 주석처리
                => rc->m_tcpLayer->m_remoteAddr 쓰지 말고,
                socket->m_remoteAddr을 쓰면 될 것 같아요.
                */
            if ( m_logWriter != nullptr ) // 사용자가 EnableLog를 켰다면
            {
                Tstringstream ss;
                ss << "Server: client " << rc->m_HostID <<
                    " is disconnected. Addr=" << socket->m_remoteAddr.ToString().GetString() <<
                    ", error code=" << errorInfo.m_socketError << ", error type=" << ErrorInfo::TypeToString(errorInfo.m_errorType) << ", error comment=" << errorInfo.m_comment.GetString();
                m_logWriter->WriteLine(0, LogCategory_System, HostID_Server, ss.str().c_str());
            }
        }
        else
        {
            // 대응하는 socket이 없는 경우 그냥 무시
            // listen socket에 대해서는 다른 곳에서 핸들링함
            if ( socket->m_isListeningSocket && m_listening )
            {
                // 에러를 토한다.
                EnqueErrorOnIssueAcceptFailed(errorInfo.m_socketError);
            }

            /* Q: ACR 연결 후보는 처리 안 하나요?
            A: 그건 NC에서 하는 일이에요 -_-
            */
        }
    }

    void CNetServerImpl::EnqueErrorOnIssueAcceptFailed(SocketErrorCode e)
    {
        if ( e != SocketErrorCode_NotSocket && e != SocketErrorCode_Ok )
        {
            String cmt;
            cmt.Format(_PNT("FATAL: AcceptEx failed with error code %d! No more accept will be possible."), e);
            EnqueError(ErrorInfo::From(ErrorType_Unexpected, HostID_Server, cmt));
        }
    }

    void CNetServerImpl::EveryRemote_HardDisconnect_AutoPruneGoesTooLongClient()
    {
        CriticalSectionLock lock(GetCriticalSection(), true);
        for ( auto i = m_garbagedHosts.begin(); i != m_garbagedHosts.end(); ++i )
        {
            shared_ptr<CRemoteClient_S> rc = LeanDynamicCast_RemoteClient_S(i->GetSecond());

            if ( rc )
            {
                HardDisconnect_AutoPruneGoesTooLongClient(rc);
            }
        }
    }

    // remote client의, 외부 인터넷에서도 인식 가능한 TCP 연결 주소를 리턴한다.
    Proud::NamedAddrPort CNetServerImpl::GetRemoteIdentifiableLocalAddr(const shared_ptr<CRemoteClient_S>& rc)
    {
        AssertIsNotLockedByCurrentThread();//mainlock만으로 진행이 가능할듯.

        AddrPort sockName = rc->m_tcpLayer->GetSocketName();
        NamedAddrPort ret = NamedAddrPort::From(sockName);
        ret.OverwriteHostNameIfExists(rc->m_tcpLayer->GetSocketName().IPToString());  // TCP 연결을 수용한 소켓의 주소. 가장 연결 성공률이 낮다. NIC가 두개 이상 있어도 TCP 연결을 받은 주소가 UDP 연결도 받을 수 있는 확률이 크므로 OK.
        ret.OverwriteHostNameIfExists(m_localNicAddr); // 만약 NIC가 두개 이상 있는 서버이며 NIC 주소가 지정되어 있다면 지정된 NIC 주소를 우선 선택한다.
        ret.OverwriteHostNameIfExists(m_serverAddrAlias); // 만약 서버용 공인 주소가 따로 있으면 그것을 우선해서 쓰도록 한다.

        if ( !ret.IsUnicastEndpoint() )
        {
            assert(0); // 드물지만 있더라. 어쨌거나 어설션 즐
            // GetOneLocalAddress 퇴역에 따라 다시 어셜션
            //ret.m_addr = CNetUtil::GetOneLocalAddress();
        }

        return ret;
    }

    void CNetServerImpl::OnSocketGarbageCollected(const shared_ptr<CSuperSocket>& /*socket*/)
    {

    }

    // 메모리 고갈 상황에서, 잔여 공간 더 확보 후 네트웍 연결 끊기를 수행한다.
    void CNetServerImpl::BadAllocException(Exception& ex)
    {
        FreePreventOutOfMemory();

        if (OnException.m_functionObject)
            OnException.m_functionObject->Run(ex);

        if ( m_eventSink_NOCSLOCK != nullptr )
        {
            m_eventSink_NOCSLOCK->OnException(ex);
        }

        // caller가 user worker thread이면 deadlock 가능.
        // 따라서 Stop()이 아님.
        StopAsync();
    }

    bool CNetServerImpl::Stub_ProcessReceiveMessage(IRmiStub* stub, CReceivedMessage& receivedMessage, void* hostTag, CWorkResult*)
    {
        return stub->ProcessReceivedMessage(receivedMessage, hostTag);
    }



}

