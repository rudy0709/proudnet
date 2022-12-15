/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once

#include "../include/BasicTypes.h"
#include "../include/FastList.h"
#include "../include/Message.h"
#include "FallbackableUdpLayer_S.h"
//#include "IOCP.h"
#include "DeviationDetector.h"
//#include "ISendDest_C.h"
#include "TcpLayer_S.h"
#include "SuperSocket.h"
//#include "FinalUserWorkItem.h"
#include "P2PGroup_S.h"
#include "P2PPair.h"
//#include "UseCount.h"
#include "Crypto.h"
#include "../include/CryptoRsa.h"
#include "HlaSessionHostImpl_S.h"
#include "RemoteBase.h"
#include "LeanType.h"

namespace Proud
{
    class P2PConnectionState;

    class CRemoteClient_S;
    typedef CFastMap2<HostID, CHostBase*,int> RemoteClients_S;

    // 스피드핵 감지 기능
    class CSpeedHackDetector
    {
    public:
        bool m_hackSuspected;					// true이면 해킹이 의심된다는거다.
        int64_t m_lastPingReceivedTimeMs;           // 마지막에 MessageType_SpeedHackDetectorPing을 받은 시간. 0이면 아직 안받는거다.
        int64_t m_firstPingReceivedTimeMs;          // 처음 MessageType_SpeedHackDetectorPing을 받은 시간. 0이면 아직 안받는거다.
        int64_t m_recentPingIntervalMs;     // MessageType_SpeedHackDetectorPing를 받은 평균 주기(밀리초). 이거가 비정상이면 스피드핵으로 간주한다.

        CSpeedHackDetector()
        {
            m_recentPingIntervalMs = CNetConfig::SpeedHackDetectorPingIntervalMs; // 정상 수치
            m_firstPingReceivedTimeMs = 0;
            m_lastPingReceivedTimeMs = 0;
            m_hackSuspected = false;
        }
    };

    // 서버에 접속된 클라 객체. HostID를 가진다.
    class CRemoteClient_S 
        : public CHostBase
        , public CP2PGroupMemberBase_S
        , public enable_shared_from_this<CRemoteClient_S>
#ifdef USE_HLA
        , public IHlaViewer_S
#endif
    {
    private:
        const PNTCHAR* m_disposeCaller;
    public:
        inline const PNTCHAR* GetDisposeCaller() { return m_disposeCaller; }

    public:
        // volatile 키워드에만 의존했던 루틴 모두 수정하기.pptx 에 의하면,
        // 아래 변수들은 멀티 코어 CPU 활용을 하다 보니 no lock 상태에서 접근될 가능성이 있다.
        // 프로그램 유지보수 중 절대 그럴 가능성이 없다고 장담하지 못할 경우 찾기 힘든 버그로 이어질 위험이 있다.
        // 따라서 아래와 같이 처리했다. 어차피 성능에 거의 영향을 안 주므로 이렇게 해도 괜찮다.
        class CSafeTimes
        {
            // 아래 변수들을 보호함. remote cs와 main cs 중 어디에 속하는 것도 애매. 그래서 별도로 또 뒀다.
            CriticalSection m_cs;

            int64_t m_lastRequestMeasureSendSpeedTime;

            // MessageType_ArbitraryTouch를 마지막에 보낸 시간.
            int64_t m_arbitraryUdpTouchedTime;

            /* 가장 마지막에 클라로부터 ping을 받은 시간
            real UDP enabled mode에서만 유효한 값이다.
            클라로부터 패킷은 받지만 클라가 보내는 양이 상당히 많아서 
            서버에서 퐁을 보내지 못해서 펄백하면 안습이다. 따라서 이러한 경우를 막기 위해 이게 쓰인다. */
            int64_t m_lastUdpPingReceivedTime;

            // 자진탈퇴 요청을 건 시간
            int64_t m_requestAutoPruneStartTime;

            // 이 리모트 클라에 대해 sendqueue가 일정이상을 찍었을때에 시작되는 타임이다.
            int64_t m_sendQueueWarningStartTime;

        public:
// 			inline void Set_lastTcpStreamReceivedTime(int64_t newValue) { CriticalSectionLock lock(m_cs, true); m_lastTcpStreamReceivedTime = newValue; } 
// 			inline int64_t Get_lastTcpStreamReceivedTime() { CriticalSectionLock lock(m_cs, true); return m_lastTcpStreamReceivedTime; }
            inline void Set_lastRequestMeasureSendSpeedTime(int64_t newValue) { CriticalSectionLock lock(m_cs, true); m_lastRequestMeasureSendSpeedTime = newValue; } 
            inline int64_t Get_lastRequestMeasureSendSpeedTime() { CriticalSectionLock lock(m_cs, true); return m_lastRequestMeasureSendSpeedTime; }
// 			inline void Set_lastUdpPacketReceivedTime(int64_t newValue) { CriticalSectionLock lock(m_cs, true); m_lastUdpPacketReceivedTime = newValue; } 
// 			inline int64_t Get_lastUdpPacketReceivedTime() { CriticalSectionLock lock(m_cs, true); return m_lastUdpPacketReceivedTime; }
            inline void Set_arbitraryUdpTouchedTime(int64_t newValue) { CriticalSectionLock lock(m_cs, true); m_arbitraryUdpTouchedTime = newValue; } 
            inline int64_t Get_arbitraryUdpTouchedTime() { CriticalSectionLock lock(m_cs, true); return m_arbitraryUdpTouchedTime; }
            inline void Set_lastUdpPingReceivedTime(int64_t newValue) { CriticalSectionLock lock(m_cs, true); m_lastUdpPingReceivedTime = newValue; } 
            inline int64_t Get_lastUdpPingReceivedTime() { CriticalSectionLock lock(m_cs, true); return m_lastUdpPingReceivedTime; }
            inline void Set_requestAutoPruneStartTime(int64_t newValue) { CriticalSectionLock lock(m_cs, true); m_requestAutoPruneStartTime = newValue; } 
            inline int64_t Get_requestAutoPruneStartTime() { CriticalSectionLock lock(m_cs, true); return m_requestAutoPruneStartTime; }
            inline void Set_sendQueueWarningStartTime(int64_t newValue) { CriticalSectionLock lock(m_cs, true); m_sendQueueWarningStartTime = newValue; } 
            inline int64_t Get_sendQueueWarningStartTime() { CriticalSectionLock lock(m_cs, true); return m_sendQueueWarningStartTime; }
        };
        CSafeTimes m_safeTimes;

        double m_sendSpeed;

        // 이 클라이언트의 타임아웃 시간
        int m_timeoutTimeMs;

        //이 클라이언트의 ACR timeout(타임아웃) 시간, NetSettings::m_autoConnectionRecoveryTimeoutTimeMs와 대응함
        int m_autoConnectionRecoveryTimeoutMs;

        // 한 remote client가 direct P2P 연결을 맺을 수 있는 최대 갯수
        // 기본값: unlimit
        // 공유기나 방화벽의 과부하 때문에 실질적으로는 한계를 두는 것을 권장(예: 30)
        int m_maxDirectP2PConnectionCount;

        // 수퍼 피어로서 이 클라가 얼마나 역할을 잘 해낼까
        // 높을수록 1등이 될 가능성 오름
        double m_SuperPeerRating;

        // 측정된 클라-서버 랙
        int m_lastPingMs;

        // C/S 첫 핑퐁 작업 : 클라와의 reliable 핑의 마지막 시간
        int m_lastReliablePingMs;
        
        // local에서 보낸 UDP 패킷을 remote에서 실제로 수신할 수 있었던 평균 속도
        // 수신자 측에서 보고해주어야 알 수 있는 값.
        // 이 값은 측정용일 뿐이고 이 값이 세팅되면서 packet frag board에도 동시에 넣고 있다.
        int64_t m_unreliableRecentReceivedSpeed;

        // C2C 클라끼리 보내고 받은 real udp packet count  
        uint32_t m_toRemotePeerSendUdpMessageSuccessCount;
        uint32_t m_toRemotePeerSendUdpMessageTrialCount;	
        
        // C2S 서버에게 보내고 받은 real UDP message count
        uint32_t m_toServerSendUdpMessageTrialCount;
        uint32_t m_toServerSendUdpMessageSuccessCount;

        // 사용자가 입력한 마지막 프레임레이트
        CApplicationHint m_lastApplicationHint;

        // 측정된 송신대기량(바이트단위,TCP+UDP)
        int m_sendQueuedAmountInBytes;


        // 공유기 이름
        String m_natDeviceName;

        // 클라와 graceful 디스 과정에서만 채워진다.
        ByteArray m_shutdownComment;

        CNetServerImpl *m_owner;
        virtual CriticalSection& GetOwnerCriticalSection() PN_OVERRIDE;

        
        // 이 클라가 디스될 때 그룹들이 파괴되는데, 이를 클라 디스 이벤트 콜백에서 전달되게 하기 위해 여기에 백업.
        CFastArray<HostID> m_hadJoinedP2PGroups;
        
        shared_ptr<CSessionKey> m_sessionKey;
        CryptCount m_encryptCount, m_decryptCount;

        // 이 객체가 생성된 시간. 그러나 이것이 접속 성공 시간을 말하지는 않는다. ACR이 실패하거나 초기 key 인증이 실패해서 HostID 발급이 실패해도 이 값은 당시 시간을 가진다.
        int64_t m_createdTime;

//		bool m_purgeRequested;

        // 예전에는 CFastSet이었으나, key로 smart ptr을 쓰면서 괜히 traits까지 만들어야 하고, 고객사 중 이거에서 크래시 제보가 있어서,
        // 일단 잠재요인은 하나라도 제거하는게 좋다. 따라서 key로 plain ptr을 쓴다.
        typedef CFastMap2<P2PConnectionState*, P2PConnectionStatePtr, intptr_t> P2PConnectionPairs;
        P2PConnectionPairs m_p2pConnectionPairs;
#ifdef CRYPT_OLD
        CCryptoKeyPtr m_xchgKey;
        ByteArray m_publicKeyBlob;
#endif
        //HostID m_HostID;

        // 서버 소켓 중 랜덤으로 하나 선택된, 연계된 UDP 소켓
        CFallbackableUdpLayer_S m_ToClientUdp_Fallbackable;

        // 클라이언트와 통신할 TCP 소켓
        // ACR 연결을 본 연결에 던져주고 시체가 된 to-client의 TCP는 null.
        shared_ptr<CSuperSocket> m_tcpLayer;

        // not null인 경우 스피드핵 감지 기능이 켜져있는거다.
        CHeldPtr<CSpeedHackDetector> m_speedHackDetector;


        // per-peer RC인 경우에만 설정된다.
        shared_ptr<CSuperSocket> m_ownedUdpSocket;
#ifdef USE_HLA

        CFastSet<CHlaSpace_S*> m_hlaViewingSpaces;
#endif
#ifndef PROUDSRCCOPY
        unsigned long m_RT; // 디버깅 어렵게 하려고 의도적으로 DWORD. 인증된 경우에만 값이 채워짐.
#endif

        CRemoteClient_S(CNetServerImpl *owner,
            const shared_ptr<CSuperSocket>& newSocket,
            AddrPort tcpRemoteAddr,
            int defaultTimeoutTimeMs,
            int autoConnectionRecoveryTimeoutMs);

#ifdef SUPPORTS_WEBGL
        CRemoteClient_S(CNetServerImpl *owner, const shared_ptr<CSuperSocket>& newSocket);
#endif

        ~CRemoteClient_S();

        bool IsAuthed()
        {
            return m_HostID != HostID_None;
        }

        void ToNetClientInfo(CNetClientInfo &ret);
        CNetClientInfoPtr ToNetClientInfo();
        
        NamedAddrPort GetRemoteIdentifiableLocalAddr();
        bool IsBehindNat();

        virtual HostID GetHostID() PN_OVERRIDE
        {
            return m_HostID;
        }

        virtual LeanType GetLeanType() PN_OVERRIDE
        {
            return LeanType_CRemoteClient_S;
        }

//		bool IsFinalReceiveQueueEmpty();

        void DetectSpeedHack();
        
        int GetP2PGroupTotalRecentPing(HostID groupID);

        void WarnTooShortDisposal(const PNTCHAR* funcWhere);

        bool IsDispose();
        //void IssueDispose(ErrorType errorType, ErrorType detailType, const ByteArray& comment, const PNTCHAR* where, SocketErrorCode socketErrorCode);
        bool IsValidEnd();

        //void CheckTcpInDanger(int64_t currTime);

        // remote 객체는 직접 tcp socket을 가지지 않는다. 
        // garbage는 될 지언정. 
// 		CriticalSection& GetSendCriticalSection();
// 		CriticalSection& GetCriticalSection();

        //void IssueSendOnNeed(int64_t currTime);
        // IHostObject를 상속받았지만 TCP만을 위한 것이라서
        inline void AssertIsSocketLockedByCurrentThread()
        {
            assert(IsLockedByCurrentThread() == true);
        }
        inline void AssertIsSocketNotLockedByCurrentThread()
        {
            assert(IsLockedByCurrentThread() == false);
        }
        
        bool IsLockedByCurrentThread();
        
        void DoForLongInterval(int64_t currTime);
        
        //void ResetSpeedHackDetector();
#pragma push_macro("new")
#undef new
        // 이 클래스는 ProudNet DLL 경우를 위해 커스텀 할당자를 쓰되 fast heap을 쓰지 않는다.
        DECLARE_NEW_AND_DELETE
#pragma pop_macro("new")

#ifdef USE_HLA
        virtual CFastSet<CHlaSpace_S*>& GetViewingSpaces();
#endif

        //////////////////////////////////////////////////////////////////////////
        // ACR 연결유지기능

        // ACR 기능을 켜거나 끈다. 넷클라의 서버 연결 정보에 담겨있다. 단, 자신이 새 ACR 시도 연결인 경우 이 값은 false이다.
        // 처음에는 true였다가도, NetClient.Disconnect or NetServer.CloseConnection에 의해 false가 되기도 한다.
        bool m_enableAutoConnectionRecovery;

        // true이면 last received timeout 처리를 한다.
        bool m_closeNoPingPongTcpConnections;

        /* 클라가 돌연 TCP connection lost를 한 시간.
        이때부터 클라측의 ACR 완료를 대기하기 시작한다. 
        너무 오랫동안 ACR 완료가 안되면 그때야 비로소 remote client를 제거한다.
        0: 메인 TCP가 정상임을 의미.
        >0: TCP connection lost가 되어, ACR의 완료를 기다리는 중. */
        int64_t m_autoConnectionRecoveryWaitBeginTime;

        // ClientOffline 이벤트가 Enque 되었는지 체크 하는 변수
        // 해당 변수가 존재 하는 이유는 서버와 클라의 타임아웃이 같을 때 ClientOffline 이 두번 호출 되는 문제가 있다.
        // 이는 서버에서 TimeOut 을 감지하고 Enque Offline 한 뒤, ACR 작업 중 RequestAutoConnectionRecovery 메시지에서 한번 더 Enque 해버린다.
        bool m_enquedClientOfflineEvent;

        // ACR 사용 시 재접속에 인증키
        ByteArray m_credentialBlock;

        // ACR 관련. 원래의 TCP 연결이 메롱했을 때의 오류 상황. ACR 과정도 실패할 경우 이 값이 사용된다.
        ErrorType m_errorBeforeAutoConnectionRecoveryErrorType;
        ErrorType m_errorBeforeAutoConnectionRecoveryDetailType;
        ByteArray m_errorBeforeAutoConnectionRecoveryComment;
        String m_errorBeforeAutoConnectionRecoveryWhere;
        SocketErrorCode m_errorBeforeAutoConnectionRecoverySocketErrorCode;
    };

}
