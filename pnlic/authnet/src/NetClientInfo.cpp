﻿/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "../include/NetClientInfo.h"

namespace Proud
{

	CNetClientInfo::CNetClientInfo(void)
	{
		m_realUdpEnabled = false;
		m_RelayedP2P = true;
		m_HostID = HostID_None;
		m_TcpAddrFromServer = m_UdpAddrFromServer = AddrPort::Unassigned;
		m_UdpAddrInternal = AddrPort::Unassigned;
		m_recentPingMs = 0;
		m_sendQueuedAmountInBytes = 0;
		m_isBehindNat = false;
		m_hostTag = NULL;
		m_recentFrameRate = 0;
		m_hostIDRecycleCount = 0;
        m_toServerSendUdpMessageTrialCount = 0;
        m_toServerSendUdpMessageSuccessCount = 0;
		m_unreliableMessageRecentReceiveSpeed = 0;
	}


	String CNetClientInfo::ToString( bool atServer )
	{
		String ret;

		if(atServer)
		{
			ret.Format(_PNT("HostID=%d,TcpAddrFromServer=%s,JoinedP2PGroupCount=%d,IsBehindNat=%d,RealUdpEnabled=%d,NatDeviceName=%s,HostIDRecyclecount:%u,toServerSendUdpMessageTrialCount:%u,toServerSendUdpMessageSuccessCount:%u, unreliableMessageRecentReceiveSpeed:%I64d "),
				m_HostID,
				m_TcpAddrFromServer.ToString().GetString(),
				m_joinedP2PGroups.GetCount(),
				m_isBehindNat,
				m_realUdpEnabled,
				m_natDeviceName.GetString(),
				m_hostIDRecycleCount,
                m_toServerSendUdpMessageTrialCount,
				m_toServerSendUdpMessageSuccessCount,
				m_unreliableMessageRecentReceiveSpeed);
			return ret;
		}
		else
		{
			ret.Format(_PNT("HostID=%d,RelayedP2P=%d,JoinedP2PGroupCount=%d,IsBehindNat=%d,RealUdpEnabled=%d,NatDeviceName=%s, unreliableMessageRecentReceiveSpeed:%I64d "),
				m_HostID,
				m_RelayedP2P,
				m_joinedP2PGroups.GetCount(),
				m_isBehindNat,
				m_realUdpEnabled,
				m_natDeviceName.GetString(),
				m_unreliableMessageRecentReceiveSpeed);
			return ret;
		}
	}
}
