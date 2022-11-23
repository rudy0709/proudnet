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
	struct SendOpt
	{
		MessageReliability m_reliability;
		int m_unreliableS2CRoutedMulticastMaxCount;
		int m_unreliableS2CRoutedMulticastMaxPingMs;	
		int m_maxDirectBroadcastCount;
		
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
		bool m_INTERNAL_USE_fraggingOnNeed;		// UDP. false이면 defragboard를 안타고 MTU 제한 무시하고 바로 소켓으로 들어간다. 릴레이 류는 이것을 쓸 것.
		bool m_INTERNAL_USE_isProudNetSpecificRmi;	 // ProudNet 내부 전용 메시지이면 true로 세팅된다. 가령 유저 메시지 카운팅 합산에 포함 안되게 한다.
		bool m_useUniqueID;
	private:
		inline void Init() 
		{
			m_reliability = MessageReliability_LAST;
			m_unreliableS2CRoutedMulticastMaxCount = 0;
			m_unreliableS2CRoutedMulticastMaxPingMs = CNetConfig::UnreliableS2CRoutedMulticastMaxPingDefaultMs;
			m_maxDirectBroadcastCount = 0;
			m_uniqueID = UniqueID();
			m_priority = MessagePriority_LAST;
			m_encryptMode = EM_LAST;
			m_enableP2PJitTrigger = true;
			m_enableLoopback = true;
			m_allowRelaySend = true;
			m_ttl = -1;
			m_forceRelayThresholdRatio = 0;
			m_INTERNAL_USE_isProudNetSpecificRmi = false;
			m_INTERNAL_USE_fraggingOnNeed = CNetConfig::FraggingOnNeedByDefault;
			m_compressMode = CM_None;
			m_useUniqueID = false;
		}

	public:
		inline SendOpt()
		{
			Init();
		}

		inline SendOpt(MessagePriority priority, bool isProudNetSpecificRmi)
		{
			Init();

			m_priority = priority;
			m_INTERNAL_USE_isProudNetSpecificRmi = isProudNetSpecificRmi;
		}

		inline SendOpt(const RmiContext& rmiContext)
		{
			m_reliability = rmiContext.m_reliability;
			m_unreliableS2CRoutedMulticastMaxCount = rmiContext.m_unreliableS2CRoutedMulticastMaxCount;
			m_unreliableS2CRoutedMulticastMaxPingMs = rmiContext.m_unreliableS2CRoutedMulticastMaxPingMs;
			m_maxDirectBroadcastCount = rmiContext.m_maxDirectP2PMulticastCount;
			m_uniqueID.m_value = rmiContext.m_uniqueID;
			m_uniqueID.m_relayerSeparator = (char)UniqueID_RelayerSeparator_None;
			m_priority = rmiContext.m_priority;
			m_enableLoopback = rmiContext.m_enableLoopback;
			m_enableP2PJitTrigger = rmiContext.m_enableP2PJitTrigger;
			m_allowRelaySend = rmiContext.m_allowRelaySend;
			m_encryptMode = rmiContext.m_encryptMode;
			m_ttl = -1;
			m_forceRelayThresholdRatio = rmiContext.m_forceRelayThresholdRatio;
			m_INTERNAL_USE_isProudNetSpecificRmi = rmiContext.m_INTERNAL_USE_isProudNetSpecificRmi;
			m_INTERNAL_USE_fraggingOnNeed = CNetConfig::FraggingOnNeedByDefault;
			m_compressMode = rmiContext.m_compressMode;
			m_useUniqueID = (rmiContext.m_uniqueID!=0);
		}
	};


}