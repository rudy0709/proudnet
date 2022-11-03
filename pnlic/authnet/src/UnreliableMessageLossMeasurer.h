/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/
#pragma once

#include "../include/pnstdint.h"
#include "../include/sysutil.h"
#include "FastMap2.h"

namespace Proud 
{
	/* 패킷 로스율을 얻는 역할을 수행한다.
	덤으로, UDP 패킷 복제 해킹을 감지하기 위해 '최근 수신한 패킷 ID들'을 보관하는 역할도 한다.
	*/
	class CUnreliableMessageLossMeasurer
	{
		// 최근동안 조립중인 패킷 ID들. 패킷 복제 공격을 막는 역할을 한다.
		CFastMap2<int32_t,int,int> m_recentAssemblyingPacketIDs;	// 최근 받은 패킷ID들. 오래되면 비워진다.

		int32_t m_nMinPacketIDValue;		// 패킷 아이디 최소값(signed 로 바뀌었음/RO)
		int32_t m_nMaxPacketIDValue;		// 패킷 아이디 최대값(signed 로 바뀌었음/R0)
		bool m_hasReceivedUnreliablePacket;		// 패킷을 받은 적이 없으면 false, 받은 적이 있으면 true/ 주기(NetConfig)마다도 다시 false 로 셋팅한다.

	public:
		CUnreliableMessageLossMeasurer() : m_nMinPacketIDValue(INT32_MAX),
			m_nMaxPacketIDValue(INT32_MIN),
			m_hasReceivedUnreliablePacket(false)
		{
		}

		~CUnreliableMessageLossMeasurer()
		{
		}

		bool AddPacketID(int32_t nPacketID);
		void UpdateUnreliableMessagingLossRatioVars(int32_t nPacketID);
		int	GetUnreliableMessagingLossRatioPercent() const;
		void ResetUnreliableMessagingLossRatioVars();
	};
}