/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "../include/NetClientInfo.h"
#include "NetClient.h"

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
		m_TcpLocalAddrFromServer = AddrPort::Unassigned;
	}


	// NetClientInfo가 서버에 존재하는데 atServer는 왜있는지 모르겠지만 혹시 고객사에서 사용중일수 있으므로 유지..
	String CNetClientInfo::ToString( bool atServer )
	{
		String ret;
		Tstringstream retStream;

		if(atServer)
		{
			retStream << _PNT("HostID=") << m_HostID;
			retStream << _PNT(", TcpAddrFromServer=") << m_TcpAddrFromServer.ToString().GetString();
			retStream << _PNT(", JoinedP2PGroupCount=") << m_joinedP2PGroups.GetCount();
			retStream << _PNT(", IsBehindNat=") << m_isBehindNat;
			retStream << _PNT(", RealUdpEnabled=") << m_realUdpEnabled;
			retStream << _PNT(", NatDeviceName=") << m_natDeviceName.GetString();
			retStream << _PNT(", HostIDRecyclecount=") << m_hostIDRecycleCount;
			retStream << _PNT(", toServerSendUdpMessageTrialCount=") << m_toServerSendUdpMessageTrialCount;
			retStream << _PNT(", toServerSendUdpMessageSuccessCount=") << m_toServerSendUdpMessageSuccessCount;
			retStream << _PNT(", unreliableMessageRecentReceiveSpeed=") << m_unreliableMessageRecentReceiveSpeed;
			retStream << _PNT(", serverAddr=") << m_TcpLocalAddrFromServer.ToString().GetString() << _PNT(" ");
		}
		else
		{
			retStream << _PNT("HostID=") << m_HostID;
			retStream << _PNT(", RelayedP2P=") << m_RelayedP2P;
			retStream << _PNT(", JoinedP2PGroupCount=") << m_joinedP2PGroups.GetCount();
			retStream << _PNT(", IsBehindNat=") << m_isBehindNat;
			retStream << _PNT(", RealUdpEnabled=") << m_realUdpEnabled;
			retStream << _PNT(", NatDeviceName=") << m_natDeviceName.GetString();
			retStream << _PNT(", unreliableMessageRecentReceiveSpeed=") << m_unreliableMessageRecentReceiveSpeed;
			retStream << _PNT(", serverAddr=") << m_TcpLocalAddrFromServer.ToString().GetString()<< _PNT(" ");
		}

		ret = retStream.str().c_str();

		return ret;
	}

	// 서버 연결이 잘 안되고 있으면, 내부 진척 상태 디버그 정보를 출력한다.
	// PNTEST에서 다룬다.
	void CNetClientImpl::PrintServerConnectDebugInfo()
	{
		if (!m_remoteServer)
		{
			cout << "no RS\n";
			return;
		}

		if (!m_remoteServer->m_ToServerTcp)
		{
			cout << " no RS.TCP\n";
			return;
		}

		if(m_remoteServer->m_ToServerTcp->m_isConnectingSocket)
		{
			// issue connect를 걸었지만 아직 결과가 안 나왔다.
			cout << "No RS.TCP conn. \n";
#ifdef _WIN32
			cout << "  SI flag = " << m_remoteServer->m_ToServerTcp->m_sendIssued << endl;
#endif
		}
		else
		{
			// 만약 아래 addr2가 안나오고 여기로 온다면, 애당초 issue connect 자체조차 안한 듯? 왜?
		}

		cout << "RS.TCP addr1=" << StringT2A(m_remoteServer->m_ToServerTcp->GetLocalAddr().ToString()).GetString() << endl;
		cout << "RS.TCP addr2=" << StringT2A(m_remoteServer->m_ToServerTcp->m_remoteAddr.ToString()).GetString() << endl;

		// 이게 잘 나온다면, TCP 송수신은 되고는 있다.
		// 그렇다면 Message 오가는건 된건데 문제는 그 다음이라는?
		cout << "RS.TCP S count=" << m_remoteServer->m_ToServerTcp->m_totalSendEventCount << endl;
		cout << "RS.TCP R count=" << m_remoteServer->m_ToServerTcp->m_totalReceiveEventCount << endl;

	}


}
