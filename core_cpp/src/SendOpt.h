#pragma once

#include "../include/BasicTypes.h"
#include "../include/Enums.h"
#include "../include/EncryptEnum.h"
#include "../include/CompressEnum.h"
#include "../include/NetConfig.h"
#include "../include/RMIContext.h"
#include "UniqueID.h"
#include "FastMap2.h"

namespace Proud 
{
    /* RmiContext와 거의 비슷하게 생겼으나, 송신 함수들의 내부 용도로 사용되는 구조체.

    Q: RmiContext를 그냥 쓰지, 왜 SendOpt를 따로 만들어서 복사 생성자 노가다 코딩을 하나요?
    A: 몇 멤버는 SendOpt에만, 몇 멤버는 RmiContext에만 있다. 쓰지도 않는 변수를 허락함으로 인해 생기는 실수를 compile error로 노출시킬 수 있는 장점이 있다.
    SendOpt와 RmiContext의 m_uniqueID 타입이 다르고 의미가 다르다. SendOpt가 별도로 있으면, 코딩 중 실수를 compile error로 노출시키는 효과가 있다. */
    struct SendOpt
    {
        MessageReliability m_reliability;           // Unreliable인 경우 UDP 혹은 fallback TCP로 전송한다. Unreliable인 경우 ACR 재접속시 AMR 재전송 과정을 하지 않는다.
        int m_unreliableS2CRoutedMulticastMaxCount;
        int m_unreliableS2CRoutedMulticastMaxPingMs;	
        int m_maxDirectP2PMulticastCount;
        
        // 통신량 스로틀링용.
        // TCP,UDP에 적용됨.
        // 0이 아닌 경우 같은 값을 가진 메시지가 queue에 있으면 선 제거한다.
        UniqueID m_uniqueID;	
        
        MessagePriority m_priority;		// UDP
        EncryptMode m_encryptMode;
        CompressMode m_compressMode;
        bool m_enableP2PJitTrigger;
        bool m_enableLoopback;	// TCP,UDP
        bool m_allowRelaySend;
        int m_ttl;		// UDP
        double m_forceRelayThresholdRatio;
        bool m_fragmentOnNeed;		// UDP. false이면 defragboard를 안타고 MTU 제한 무시하고 바로 소켓으로 들어간다. 릴레이 류는 이것을 쓸 것.
        bool m_INTERNAL_USE_isProudNetSpecificRmi;	 // ProudNet 내부 전용 메시지이면 true로 세팅된다. 가령 유저 메시지 카운팅 합산에 포함 안되게 한다.
        bool m_useUniqueID;
        RmiContext* m_pSendWorkOutput; // 송신 처리를 한 결과가 여기다 출력된다. 송신 실패한 것들...

        // 중복 코딩이지만, 성능에 민감하므로, a=x 대신 a(x)를 썼다.
        inline SendOpt(MessagePriority priority = MessagePriority_LAST, bool isProudNetSpecificRmi = false)
            : m_reliability(MessageReliability_LAST)
            , m_unreliableS2CRoutedMulticastMaxCount(0)
            , m_unreliableS2CRoutedMulticastMaxPingMs(CNetConfig::UnreliableS2CRoutedMulticastMaxPingDefaultMs)
            , m_maxDirectP2PMulticastCount(0)
            , m_uniqueID(UniqueID())
            , m_priority(priority)
            , m_encryptMode(EM_LAST)
            , m_compressMode(CM_None)
            , m_enableP2PJitTrigger(true)
            , m_enableLoopback(true)
            , m_allowRelaySend(true)
            , m_ttl(-1)
            , m_forceRelayThresholdRatio(0)
            , m_fragmentOnNeed(CNetConfig::FraggingOnNeedByDefault)
            , m_INTERNAL_USE_isProudNetSpecificRmi(isProudNetSpecificRmi)
            , m_useUniqueID(false)
            , m_pSendWorkOutput(nullptr)
        {
        }

        // rmiContext로부터 복사받고 rmiContext의 sendFailedRemotes를 청소한다. 
        // 청소는...사용자가 rmiContext를 여러번 재사용하는 경우의 문제를 예방한다.
        // 걱정말자. return value optimization에 의해 복사 연산자는 실행 안된다.
        static inline SendOpt CreateFromRmiContextAndClearRmiContextSendFailedRemotes(RmiContext& rmiContext)
        {
            SendOpt ret; 

            ret.m_reliability = rmiContext.m_reliability;
            ret.m_unreliableS2CRoutedMulticastMaxCount = rmiContext.m_unreliableS2CRoutedMulticastMaxCount;
            ret.m_unreliableS2CRoutedMulticastMaxPingMs = rmiContext.m_unreliableS2CRoutedMulticastMaxPingMs;
            ret.m_maxDirectP2PMulticastCount = rmiContext.m_maxDirectP2PMulticastCount;
            ret.m_priority = rmiContext.m_priority;
            ret.m_encryptMode = rmiContext.m_encryptMode;
            ret.m_compressMode = rmiContext.m_compressMode;
            ret.m_enableP2PJitTrigger = rmiContext.m_enableP2PJitTrigger;
            ret.m_enableLoopback = rmiContext.m_enableLoopback;
            ret.m_allowRelaySend = rmiContext.m_allowRelaySend;
            ret.m_forceRelayThresholdRatio = rmiContext.m_forceRelayThresholdRatio;
            ret.m_fragmentOnNeed = rmiContext.m_fragmentOnNeed;
            ret.m_INTERNAL_USE_isProudNetSpecificRmi = rmiContext.m_INTERNAL_USE_isProudNetSpecificRmi;
            ret.m_useUniqueID = rmiContext.m_uniqueID != 0;
            ret.m_pSendWorkOutput = &rmiContext; // RmiContext의 주소를 채운다. 출력용이므로.

            ret.m_uniqueID.m_value = rmiContext.m_uniqueID;
            ret.m_uniqueID.m_relayerSeparator = (int8_t)UniqueID_RelayerSeparator_None;

            // rmiContext를 변경
            if(rmiContext.m_fillSendFailedRemotes)
                rmiContext.m_sendFailedRemotes.Clear();

            return ret;
        }
    };


}
