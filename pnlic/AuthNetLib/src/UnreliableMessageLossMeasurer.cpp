/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/
#include "stdafx.h"
//#include "../include/NetConfig.h"
#include "../include/numutil.h"
//#include "../include/errorinfo.h"
#include "UnreliableMessageLossMeasurer.h"

namespace Proud 
{
	int CUnreliableMessageLossMeasurer::GetUnreliableMessagingLossRatioPercent() const
	{
		// 패킷 로스율은, 최근 받은 패킷들의 packet ID들의 범위 중에서 이빨이 빠진 것들을 체크하면 된다.
		
		if (!m_hasReceivedUnreliablePacket)
			return 0;

		int nMinMaxDiffPacketIDValue = m_nMaxPacketIDValue - m_nMinPacketIDValue + 1;
		if (nMinMaxDiffPacketIDValue < 0)
			nMinMaxDiffPacketIDValue = m_nMinPacketIDValue - m_nMaxPacketIDValue + 1;

		// 캐릭터 위치를 초당 5회 미만으로 보내는 저용량 게임에서는 30도 크다.
		// 그리고 로스율이 조금이라도 있으면 TFRC 불문율상 트래픽을 확 줄이는 것이 통례.
		// 따라서 10으로 하향했다.
		if (nMinMaxDiffPacketIDValue < 10)	
		{
			// 너무 적은 샘플링으로는 로스율을 얻지 못하므로 무조건 0
			return 0;
		}

		int x = 100 - ((100 * m_recentAssemblyingPacketIDs.GetCount()) / nMinMaxDiffPacketIDValue);
		
		x = min(x,100);
		x = max(x,0);

		return x;
	}

	void CUnreliableMessageLossMeasurer::ResetUnreliableMessagingLossRatioVars()
	{
		m_recentAssemblyingPacketIDs.Clear();

		m_nMinPacketIDValue = INT32_MAX;
		m_nMaxPacketIDValue = INT32_MIN;

		m_hasReceivedUnreliablePacket = false;
	}

	void CUnreliableMessageLossMeasurer::UpdateUnreliableMessagingLossRatioVars(int32_t nPacketID)
	{
		if (nPacketID < m_nMinPacketIDValue)
			m_nMinPacketIDValue = nPacketID;
		if (nPacketID > m_nMaxPacketIDValue)
			m_nMaxPacketIDValue = nPacketID;

		m_hasReceivedUnreliablePacket = true;
	}

	// 잘 추가됐으면 true,이미 같은 값이 있어서 추가가 무시되면 false.
	// 패킷복제 해킹을 감지하기 위해서 리턴값이 활용된다.
	bool CUnreliableMessageLossMeasurer::AddPacketID(int32_t nPacketID)
	{
		return m_recentAssemblyingPacketIDs.Add(nPacketID, 0);
	}

}