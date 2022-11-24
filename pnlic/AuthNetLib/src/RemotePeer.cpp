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

#include "../include/NetConfig.h"
#include "../include/NetPeerInfo.h"
#include "../include/NetClientInterface.h"

#include "RmiContextImpl.h"

#include "NetClient.h"
//#include "FallbackableUDPLayer_C.h"
//#include "networker_c.h"
#include "RemotePeer.h"
//#include "NetClientManager.h"
#include "NetUtil.h"
#include "SendFragRefs.h"

//extern bool g_MustNotRelay;






namespace Proud
{
	// return: 수명이 다됐으면 true 리턴
	bool CP2PConnectionTrialContext::Heartbeat()
	{
		// 너무 오랫동안 시도중이면 즐
		if ( GetPreciseCurrentTimeMs() - m_startTime > m_owner->m_owner->m_P2PConnectionTrialEndTime )
		{
			NTTNTRACE("P2P connection trial expired.\n");
			return false;
		}

		switch ( m_state->m_state )
		{
			case S_ServerHolepunch:
			{
				ServerHolepunchState* state = (ServerHolepunchState*)m_state.m_p;

				// 필요시 서버에 홀펀칭을 시도한다.
				if ( state->m_sendTimeToDo - GetPreciseCurrentTimeMs() < 0 )
				{
					state->m_sendTimeToDo = GetPreciseCurrentTimeMs() + CNetConfig::ServerHolepunchIntervalMs;

					if (m_owner->m_udpSocket)
					{
						CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
						sendMsg.Write((char)MessageType_PeerUdp_ServerHolepunch);
						sendMsg.Write(state->m_holepunchMagicNumber);
						sendMsg.Write(m_owner->m_HostID);
						m_owner->m_udpSocket->AddToSendQueueWithSplitterAndSignal_Copy(HostID_Server,
							FilterTag::CreateFilterTag(m_owner->m_owner->GetVolatileLocalHostID(), HostID_Server),
							GetServerUdpAddr(),
							sendMsg,
							GetPreciseCurrentTimeMs(),
							SendOpt(MessagePriority_Holepunch, true));
					}
				}
			}
			break;
			case S_PeerHolepunch:
			{
				// P2PShotgunStartTurn가 지나치게 짧으면 너무 섣불리 샷건을 쏘게 되고 과량 홀펀칭 매핑으로 이어지므로
				assert(CNetConfig::P2PShotgunStartTurn * 3 >= CNetConfig::P2PHolepunchMaxTurnCount);

				PeerHolepunchState* state = (PeerHolepunchState*)m_state.m_p;

				// 필요시 P2P 홀펀칭 쏘기
				if ( state->m_sendTimeToDo - GetPreciseCurrentTimeMs() < 0 )
				{
					state->m_sendTimeToDo = GetPreciseCurrentTimeMs() + GetClient()->m_P2PHolepunchInterval;
					state->m_sendTurn++;

					/* send magic number and each of ABSendAddrs via address ABSendAddrs, repeat:
					for every one seconds until 10 seconds

					ABSendAddrs are:
					B's internal address
					B's punched address by server */

					assert(m_owner->m_UdpAddrFromServer.IsUnicastEndpoint());
					assert(m_owner->m_UdpAddrInternal.IsUnicastEndpoint());

					SendPeerHolepunch(m_owner->m_UdpAddrFromServer, m_owner->m_magicNumber);

					/* 헤어핀이 없는 공유기를 위해 내부랜끼리 홀펀칭.
					IsSameLan 만으로는 부족하다. 외부 IP가 서로 같은 경우에만 해야 한다.
					안그러면 어떤 공유기(혹은 피어의 시스템)는 알수없는 호스트에 패킷 송신을 감행하고 있다는 이유만으로 공유기 선에서 호스트를 막아버릴 수 있을테니까.
					*/
					assert(m_owner->m_udpSocket->GetLocalAddr().IsUnicastEndpoint());
					if ( m_owner->m_udpSocket &&
						m_owner->m_udpSocket->m_localAddrAtServer.IsSameHost(m_owner->m_UdpAddrFromServer)
						// 같은 외부IP를 가지고 있음에도 내부IP가 서로 다르게 셋팅되어있는경우, 가령 ( 한쪽은 192.168.77.xxx 다른쪽은 192.168.66.xxx ) 이런식에 대응하기위해 같은 LAN인지 검사하는 로직을 뺀다.
						/*&& m_owner->m_UdpAddrInternal.IsSameSubnet24(m_owner->Get_ToPeerUdpSocket()->GetLocalAddr())*/ )
					{
						SendPeerHolepunch(m_owner->m_UdpAddrInternal, m_owner->m_magicNumber);
					}

					// 포트 예측
					// IPTIME 때문에 바로 쏘지 말자
					if ( state->m_sendTurn > CNetConfig::P2PShotgunStartTurn )
					{
						state->m_offsetShotgunCountdown--;
						if ( state->m_offsetShotgunCountdown < 0 )
						{
							state->m_offsetShotgunCountdown = CNetConfig::ShotgunTrialCount;
							state->m_shotgunMinPortNum += (uint16_t)CNetConfig::ShotgunRange;
							state->m_shotgunMinPortNum = AdjustUdpPortNumber(state->m_shotgunMinPortNum);
						}

						AddrPort shotgunTo = m_owner->m_UdpAddrFromServer;
						shotgunTo.m_port = state->m_shotgunMinPortNum;
						for ( int i=0; i < CNetConfig::ShotgunRange; i++ )
						{
							SendPeerHolepunch(shotgunTo, m_owner->m_magicNumber);
							shotgunTo.m_port++;
							shotgunTo.m_port = AdjustUdpPortNumber(shotgunTo.m_port);
						}
					}
				}
			}
			break;
		}

		return true;
	}

	CP2PConnectionTrialContext::CP2PConnectionTrialContext(CRemotePeer_C *owner)
	{
		m_startTime = GetPreciseCurrentTimeMs();
		m_owner = owner;

		// 유효성
		assert(owner->m_HostID != m_owner->m_owner->GetVolatileLocalHostID());

		m_state.Attach(new ServerHolepunchState);
	}

#define LOG_ERROR { if(main->m_enableLog || main->m_settings.m_emergencyLogLineCount > 0) { main->Log(0, LogCategory_P2P, _PNT("Failed to Holepunch1"), StringA2T(__FILE__), __LINE__); } }

	void CP2PConnectionTrialContext::ProcessPeerHolepunch(CNetClientImpl *main, CReceivedMessage &ri)
	{
		// 이쪽이 B측이다. 저쪽은 A고.
		CMessage &msg = ri.GetReadOnlyMessage();

		Guid p2pMagicNumber, serverInstanceGuid;
		AddrPort ABSendAddr; // 저쪽에서 이쪽으로 송신할 때 쓴 주소
		HostID AHostID;

		if ( msg.Read(AHostID) == false )
		{
			LOG_ERROR;
			return;
		}
		if ( msg.Read(p2pMagicNumber) == false )
		{
			LOG_ERROR;
			return;
		}
		if ( msg.Read(serverInstanceGuid) == false )
		{
			LOG_ERROR;
			return;
		}
		if ( msg.Read(ABSendAddr) == false )
		{
			LOG_ERROR;
			return;
		}

		// 서버가 1개 호스트에서 여럿 작동중일때 같은 HostID지만 서로 다른 클라로부터 holepunch가 올 수 있다.
		// 이런 경우 서버 인스턴스 guid 및 group member join 여부도 검사해서 불필요한 에코를 하지 않아야 저쪽에서 잘못된 홀펀칭 성사를
		// 막을 수 있다.
		if ( serverInstanceGuid != main->m_serverInstanceGuid )
		{
			//LOG_ERROR;
			return;
		}

		AddrPort ABRecvAddr = ri.m_remoteAddr_onlyUdp; // 저쪽으로부터 받을 때 이 주소로 받으면 된다.

		CRemotePeer_C *rp = main->GetPeerByHostID_NOLOCK(AHostID);
		if ( !rp || rp->m_garbaged || !rp->m_p2pConnectionTrialContext )
		{
			//LOG_ERROR;
			return;
		}

		// remote peer를 찾았지만 그 remote peer와 사전에 합의된 (서버를 통해 받은) connection magic number와 다르면
		// 에코를 무시한다. 이렇게 해야 잘못된 3rd remote의 hole punch 요청에 대해 엉뚱한 hole punch success를 막을 수 있다.
		assert(rp->m_magicNumber != Guid());
		assert(p2pMagicNumber != Guid());
		if ( p2pMagicNumber != rp->m_magicNumber )
		{
			//LOG_ERROR;
			return;
		}

		if ( main->m_enableLog || main->m_settings.m_emergencyLogLineCount > 0 )
		{
			main->Log(0, LogCategory_P2P, String::NewFormat(_PNT("Received P2P Holepunch. ABS=%s ABR=%s"), ABSendAddr.ToString().GetString(), ABRecvAddr.ToString().GetString()));
		}

		/* maybe B receives A's magic number via address ABRecvAddr
		* we can conclude that
		* A can send to B via selected one of ABSendAddrs
		* B can choose A's messages if it is received from ABRecvAddr

		THUS; send magic number ack / selected ABSendAddr / ABRecvAddr / each one of BASendAddrs via address BASendAddrs, once

		BASendAddrs are:
		A's internal address
		A's punched address for server
		ABRecvAddr
		*/

		// #ifdef PER_PEER_SOCKET_DEBUG
		// 	if(rp->m_UdpAddrInternal==AddrPort::Unassigned)
		// 	{
		// 		ShowErrorBox(_PNT("rp->m_UdpAddrInternal!=AddrPort::Unassigned"));
		// 	}
		// 	if(rp->m_UdpAddrFromServer==AddrPort::Unassigned)
		// 	{
		// 		ShowErrorBox(_PNT("rp->m_UdpAddrFromServer==AddrPort::Unassigned"));
		// 	}
		// 	if(ri.m_remoteAddr_onlyUdp==AddrPort::Unassigned)
		// 	{
		// 		ShowErrorBox(_PNT("ri.m_remoteAddr_onlyUdp"));
		// 	}
		// #endif

		//		assert(rp->m_udpSocket->m_socket->GetSockName().IsUnicastEndpoint());

		if ( !rp->m_UdpAddrFromServer.IsUnicastEndpoint() )
		{
			NTTNTRACE("to-peer holepunch before to-server holepunch yet? (addr=%s)\n",
				StringT2A(rp->m_UdpAddrFromServer.ToString()).GetString());
		}
		//assert(rp->m_UdpAddrFromServer.IsUnicastEndpoint());// 무조건 fail하면 문제지만, UDP의 특성상 뒷북이 있을 수 있으므로 가끔 fail하면 무시해도 된다.

		if ( rp->m_UdpAddrInternal.IsUnicastEndpoint() &&  // 아직 서버로부터 이걸 갱신하기 위한 값이 안왔을 수 있으므로 이 조건문 필요
			rp->m_udpSocket->GetLocalAddr().IsUnicastEndpoint() &&
			rp->m_udpSocket->m_localAddrAtServer.IsSameHost(rp->m_UdpAddrFromServer)
			// 같은 외부IP를 가지고 있음에도 내부IP가 서로 다르게 셋팅되어있는경우, 가령 ( 한쪽은 192.168.77.xxx 다른쪽은 192.168.66.xxx ) 이런식에 대응하기위해 같은 LAN인지 검사하는 로직을 뺀다.
			/*&& rp->m_UdpAddrInternal.IsSameSubnet24(rp->Get_ToPeerUdpSocket()->GetLocalAddr())*/ )
		{
			rp->m_p2pConnectionTrialContext->SendPeerHolepunchAck(rp->m_UdpAddrInternal, p2pMagicNumber, ABSendAddr, ABRecvAddr);
		}
		if ( rp->m_UdpAddrFromServer.IsUnicastEndpoint() ) // 아직 서버로부터 이걸 갱신하기 위한 값이 안왔을 수 있으므로 이 조건문 필요
			rp->m_p2pConnectionTrialContext->SendPeerHolepunchAck(rp->m_UdpAddrFromServer, p2pMagicNumber, ABSendAddr, ABRecvAddr);
		rp->m_p2pConnectionTrialContext->SendPeerHolepunchAck(ri.m_remoteAddr_onlyUdp, p2pMagicNumber, ABSendAddr, ABRecvAddr);
	}

	void CP2PConnectionTrialContext::ProcessPeerHolepunchAck(CNetClientImpl *main, CReceivedMessage &ri)
	{
		CMessage &msg = ri.GetReadOnlyMessage();

		// ** 이쪽이 A측 입장이고 저쪽은 B측 입장이다.

		// maybe A receives the ack via address BARecvAddr
		Guid p2pMagicNumber;
		HostID remoteHostID;
		AddrPort ABSendAddr, ABRecvAddr, BASendAddr;
		if ( msg.Read(p2pMagicNumber) == false )
			return;
		if ( msg.Read(remoteHostID) == false )
			return;
		if ( msg.Read(ABSendAddr) == false )
			return;
		if ( msg.Read(ABRecvAddr) == false )
			return;
		if ( msg.Read(BASendAddr) == false )
			return;

		AddrPort BARecvAddr = ri.m_remoteAddr_onlyUdp;

		CRemotePeer_C *rp = main->GetPeerByHostID_NOLOCK(remoteHostID);
		if ( !rp || rp->m_garbaged || !rp->m_p2pConnectionTrialContext )
			return;

		// symmetric NAT 포트 예측을 하는 경우 hostID까지는 매치되지만 정작 magic number가 안맞을 수 있다.
		// 한 컴퓨터에 여러 서버가 띄워져있으면 hostID가 중복될테니까.
		// 이것까지 체크해야 한다.
		assert(rp->m_magicNumber != Guid());

		if ( rp->m_magicNumber != p2pMagicNumber )
			return;

		if ( !rp->m_p2pConnectionTrialContext->m_state ||
			rp->m_p2pConnectionTrialContext->m_state->m_state != S_PeerHolepunch )
			return;

		// 2009.12.15  사용되지 않는 지역변수 주석
		//PeerHolepunchState* state = (PeerHolepunchState* ) rp->m_p2pConnectionTrialContext->m_state.m_p;

		/* we can conclude that
		B can send to A via selected one of BASendAddr
		A can choose B's message if it is received from BARecvAddr
		*
		* send 'P2P holepunching ok' to server

		* now we are interested to:
		selected one of ABSendAddrs
		ABRecvAddr
		selected one of BASendAddr
		BARecvAddr
		...with the interested values below */

		// symmetric NAT 포트 예측을 하는 경우 중복해서 대량으로 holepunch 2 success가 올 수 있으므로
		// 더 이상 받지 않도록 무시 처리해야 한다.
		// 또한 다른 P2P 연결 시도를 모두 막아야 한다. 안그러면 NAT 장치에서 낮은 확률로
		// 같은 endpoint에 대한 서로다른 port mapping이 race condition이 되어버리기 때문이다.
		rp->m_p2pConnectionTrialContext = CP2PConnectionTrialContextPtr();

		// 양측 모두 이걸 해야 한다. 따라서 상대측에서 못하게 하는 명령을 보내도록 한다.
		// 강제로 릴레이를 해야 한다. 그것도 reliable send로.
		RmiContext forceRelaySend = g_ReliableSendForPN;
		forceRelaySend.m_maxDirectP2PMulticastCount = 0; // 강제 relay
		main->m_c2cProxy.HolsterP2PHolepunchTrial(rp->m_HostID, forceRelaySend);

		// P2P verify 과정을 거치지 말고, 바로 P2P 홀펀칭 성공을 노티한다.
		main->m_c2sProxy.NotifyP2PHolepunchSuccess(
			HostID_Server, g_ReliableSendForPN,
			main->GetVolatileLocalHostID(), rp->m_HostID,
			ABSendAddr, ABRecvAddr, BASendAddr, BARecvAddr);

		if ( main->m_enableLog || main->m_settings.m_emergencyLogLineCount > 0 )
		{
			main->Log(0, LogCategory_P2P, String::NewFormat(_PNT("HolepunchAck OK. ABS=%s ABR=%s BAS=%s BAR=%s"),
				ABSendAddr.ToString().GetString(), ABRecvAddr.ToString().GetString(),
				BASendAddr.ToString().GetString(), BARecvAddr.ToString().GetString()));
		}
	}

	CNetClientImpl * CP2PConnectionTrialContext::GetClient()
	{
		return m_owner->m_owner;
	}

	Proud::AddrPort CP2PConnectionTrialContext::GetExternalAddr()
	{
		return m_owner->m_UdpAddrFromServer;
	}

	Proud::AddrPort CP2PConnectionTrialContext::GetInternalAddr()
	{
		return m_owner->m_UdpAddrInternal;
	}

	uint16_t CP2PConnectionTrialContext::AdjustUdpPortNumber(uint16_t portNum)
	{
		if ( portNum < 1023 || portNum>65534 )
			portNum = 1023;

		return portNum;
	}

	Proud::AddrPort CP2PConnectionTrialContext::GetServerUdpAddr()
	{
		if ( m_owner->m_owner->m_remoteServer->GetToServerUdpFallbackable() )
			return m_owner->m_owner->m_remoteServer->GetServerUdpAddr();
		else
			return AddrPort::Unassigned;
	}

	void CP2PConnectionTrialContext::ProcessMessage_PeerUdp_ServerHolepunchAck(CReceivedMessage &ri, Guid magicNumber, AddrPort addrOfHereAtServer, HostID peerID)
	{
		if ( !m_state || m_state->m_state != S_ServerHolepunch )
			return;

		ServerHolepunchState* state = (ServerHolepunchState*)m_state.m_p;

		if ( magicNumber != state->m_holepunchMagicNumber )
			return;

		if ( state->m_ackReceiveCount > 0 )
			return;

		if ( GetServerUdpAddr() != ri.m_remoteAddr_onlyUdp )
			return;

		if ( !m_owner->m_udpSocket )
			return;

		assert(m_owner->m_udpSocket->GetLocalAddr().m_port != 0);
		assert(m_owner->m_udpSocket->GetLocalAddr().m_port != 0xffff);

		// send my magic number & client local UDP address via TCP
		CMessage header; header.UseInternalBuffer();
		header.Write((char)MessageType_PeerUdp_NotifyHolepunchSuccess);
		header.Write(m_owner->m_udpSocket->GetLocalAddr());

		// 클라와 서버가 같은 NAT에 있고 NAT가 헤어핀을 지원하는 경우
		// 서버로부터 받는 addrOfHereAtServer가 클라의 내부 주소인 경우가 있다.
		// 이러한 경우 클라의 외부 주소를 알 수 없다.
		// 그러면 다른 클라와 P2P 홀펀칭할 때 상대 클라가 Symmetric NAT이면 홀펀칭이 안된다.
		// 이 문제를 해결하려면 클라와 서버가 같은 NAT에 있는 경우 클라의 외부 주소를 강제로 넣어야 한다.
		// m_clientAddrAtServer는 그러한 목적으로 쓰인다.
		String clientAddrAtServer = m_owner->m_owner->m_connectionParam.m_clientAddrAtServer;
		if ( clientAddrAtServer.IsEmpty() == false )
		{
			addrOfHereAtServer = AddrPort::FromHostNamePort(clientAddrAtServer, addrOfHereAtServer.m_port);
		}
		header.Write(addrOfHereAtServer);

		header.Write(m_owner->m_HostID);

		assert(m_owner->m_udpSocket->GetLocalAddr().IsUnicastEndpoint());
		assert(addrOfHereAtServer.IsUnicastEndpoint());

		CSendFragRefs sendData;
		sendData.Add(header);

		m_owner->m_udpSocket->m_localAddrAtServer = addrOfHereAtServer;

		GetClient()->m_remoteServer->m_ToServerTcp->AddToSendQueueWithSplitterAndSignal_Copy(sendData, SendOpt(), m_owner->m_owner->m_simplePacketMode);

		if ( GetClient()->m_enableLog || GetClient()->m_settings.m_emergencyLogLineCount > 0 )
		{
			GetClient()->Log(0, LogCategory_P2P, String::NewFormat(_PNT("Message_PeerUdp_ServerHolepunchAck. AddrOfHereAtServer=%s"), addrOfHereAtServer.ToString().GetString()));
		}

		state->m_ackReceiveCount++;
	}

	void CP2PConnectionTrialContext::SendPeerHolepunch(AddrPort ABSendAddr, Guid magicNumber)
	{
		if (m_owner->m_udpSocket)
		{
			CMessage holepunch1Msg; holepunch1Msg.UseInternalBuffer();
			holepunch1Msg.Write((char)MessageType_PeerUdp_PeerHolepunch);
			holepunch1Msg.Write(GetClient()->GetVolatileLocalHostID());
			holepunch1Msg.Write(magicNumber);
			holepunch1Msg.Write(GetClient()->m_serverInstanceGuid);
			holepunch1Msg.Write(ABSendAddr);

			assert(m_owner->m_garbaged == false);
			m_owner->m_udpSocket->AddToSendQueueWithSplitterAndSignal_Copy(m_owner->m_HostID,
				FilterTag::CreateFilterTag(m_owner->m_owner->GetVolatileLocalHostID(), m_owner->m_HostID),
				ABSendAddr,
				holepunch1Msg,
				GetPreciseCurrentTimeMs(),
				SendOpt(MessagePriority_Holepunch, true));
		}

		// 	if (GetClient()->m_enableLog)
		// 	{
		// 		GetClient()->Log(TID_Holepunch, _PNT("Client %d(%s)에게 PeerHolePunch 쏨"), m_owner->m_HostID, ABSendAddr.ToString().GetString());
		// 	}
	}

	void CP2PConnectionTrialContext::SendPeerHolepunchAck(AddrPort BASendAddr, Guid magicNumber, AddrPort ABSendAddr, AddrPort ABRecvAddr)
	{
		CMessage sendMsg; sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
		sendMsg.Write((char)MessageType_PeerUdp_PeerHolepunchAck);
		sendMsg.Write(magicNumber); // 그대로 echo한다.
		sendMsg.Write(GetClient()->GetVolatileLocalHostID());
		sendMsg.Write(ABSendAddr); // ABSendAddr
		sendMsg.Write(ABRecvAddr); // ABRecvAddr
		sendMsg.Write(BASendAddr); // BASendAddr

		assert(m_owner->m_garbaged == false);

		if (!m_owner->m_udpSocket)
			return;

		m_owner->m_udpSocket->AddToSendQueueWithSplitterAndSignal_Copy(m_owner->m_HostID,
																	   FilterTag::CreateFilterTag(m_owner->m_owner->GetVolatileLocalHostID(), m_owner->m_HostID),
																	   BASendAddr,
																	   sendMsg,
																	   GetPreciseCurrentTimeMs(),
																	   SendOpt(MessagePriority_Holepunch, true));

		if ( GetClient()->m_enableLog || GetClient()->m_settings.m_emergencyLogLineCount > 0 )
		{
			GetClient()->Log(0, LogCategory_P2P, String::NewFormat(_PNT("Try to PeerHolepunchAck. ABS=%s ABR=%s BAS=%s"),
				ABSendAddr.ToString().GetString(), ABRecvAddr.ToString().GetString(), BASendAddr.ToString().GetString()));
		}
	}

	void CRemotePeer_C::ToNetPeerInfo(CNetPeerInfo &ret)
	{
		ret.m_HostID = m_HostID;
		ret.m_UdpAddrFromServer = m_UdpAddrFromServer;
		ret.m_UdpAddrInternal = m_UdpAddrInternal;
		ret.m_recentPingMs = m_recentPingMs;
		ret.m_sendQueuedAmountInBytes = m_sendQueuedAmountInBytes;

		for ( P2PGroups_C::iterator ig = m_joinedP2PGroups.begin(); ig != m_joinedP2PGroups.end(); ig++ )
		{
			HostID groupID = ig->GetFirst();
			ret.m_joinedP2PGroups.insert(groupID);
		}
		ret.m_RelayedP2P = IsRelayedP2P();
		ret.m_isBehindNat = IsBehindNat();
		ret.m_hostTag = m_hostTag;
		ret.m_directP2PPeerFrameRate = m_recentFrameRate;
		ret.m_toRemotePeerSendUdpMessageTrialCount = m_toRemotePeerSendUdpMessageTrialCount;
		ret.m_toRemotePeerSendUdpMessageSuccessCount = m_toRemotePeerSendUdpMessageSuccessCount;

		if (m_udpSocket)
		{
			ret.m_udpSendDataTotalBytes = m_udpSocket->GetTotalIssueSendBytes();
		}
		else
		{
			ret.m_udpSendDataTotalBytes = 0;
		}
	}

	CRemotePeer_C::CRemotePeer_C(CNetClientImpl *owner)
		: m_ToPeerReliableUdp(this)
		, m_ToPeerUdp(this)
	{
		UngarbageAndInit(owner);

		m_HostID = HostID_None;
		m_recycled = false;
	}

	CRemotePeer_C::~CRemotePeer_C()
	{
		if ( m_udpSocket != NULL )
		{
			//소켓은 이미 닫힌 상태이어야 한다.
			assert(m_udpSocket && m_udpSocket->IsSocketClosed());
			m_owner->SocketToHostsMap_RemoveForAnyAddr(m_udpSocket);
		}

		assert(IsDisposeSafe());
	}

	// garbaged remote peer를 다시 사용 가능 상태로 바꾼다.
	void CRemotePeer_C::UngarbageAndInit(CNetClientImpl *owner)
	{
		int64_t curtime = GetPreciseCurrentTimeMs();
		m_forceRelayP2P = !CP2PGroupOption::Default.m_enableDirectP2P;

		m_jitDirectP2PNeeded = (owner->m_settings.m_directP2PStartCondition == DirectP2PStartCondition_Always);
		m_jitDirectP2PTriggered = false;

		// 	m_sendStopAcked = false;
		// 	m_recvStopAcked = false;

		m_p2pConnectionTrialContext = CP2PConnectionTrialContextPtr();
		
		m_newP2PConnectionNeeded = false;

		m_lastPingMs = 0;
		m_lastReliablePingMs = 0;
		m_p2pPingChecked = false;
		m_p2pReliablePingChecked = false;
		m_recentFrameRate = 0;
		m_peerToServerPingMs = 0;
		m_CSPacketLossPercent = 0;

		m_P2PHolepunchedLocalToRemoteAddr = AddrPort::Unassigned;
		m_P2PHolepunchedRemoteToLocalAddr = AddrPort::Unassigned;
		m_UdpAddrFromServer = AddrPort::Unassigned;
		m_UdpAddrInternal = AddrPort::Unassigned;
		m_owner = owner;
		m_RelayedP2P_USE_FUNCTION = true;
		m_RelayedP2PDisabledTimeMs = 0;

		m_ReliablePingDiffTimeMs = curtime + LerpInt(CNetConfig::ReliablePingIntervalMs / 2,
													 CNetConfig::ReliablePingIntervalMs,
													 (int64_t)m_owner->m_random.Next(256),
													 (int64_t)256);
// 		owner->Log(0,
// 				   LogCategory_P2P,
// 				   String::NewFormat(_PNT("Setting ReliablePingDiffTime %d.\n"),
// 				   m_ReliablePingDiffTimeMs));

		m_UnreliablePingDiffTimeMs = curtime + LerpInt(CNetConfig::UnreliablePingIntervalMs / 2,
													   CNetConfig::UnreliablePingIntervalMs,
													   (int64_t)m_owner->m_random.Next(256),
													   (int64_t)256);

		m_indirectServerTimeDiff = 0; // 이 값은 정상이 아니다. GetIndirectServerTime에서 교정될 것이다.

		m_lastDirectUdpPacketReceivedTimeMs = curtime;
		m_directUdpPacketReceiveCount = 0;
		m_lastUdpPacketReceivedInterval = -1;

		m_recentReliablePingMs = 0;
		m_sendQueuedAmountInBytes = 0;

		m_encryptCount = 0;
		m_decryptCount = 0;

		m_repunchCount = 0;
		m_repunchStartTimeMs = 0;

		m_toRemotePeerSendUdpMessageTrialCount = 0;
		m_toRemotePeerSendUdpMessageSuccessCount = 0;
		m_receiveudpMessageSuccessCount = 0;

		m_lastCheckSendQueueTimeMs = 0;
		m_sendQueueHeavyStartTimeMs = 0;

		m_hostTag = NULL;

		m_udpSocketCreationTimeMs = GetRenewalSocketCreationTimeMs();

		m_ToPeerReliableUdpHeartbeatLastTimeMs = 0;

		// 처음엔 relay이므로 true
		m_setToRelayedButLastPingIsNotCalulcatedYet = true;
	}

	// 홀펀칭 과정을 다루는 상태 기계를 생성한다.
	void CRemotePeer_C::CreateP2PConnectionTrialContext()
	{
		m_p2pConnectionTrialContext = CP2PConnectionTrialContextPtr(new CP2PConnectionTrialContext(this));
	}

	void CRemotePeer_C::Heartbeat(int64_t currTime)
	{
		// reliable UDP에 대한 heartbeat 처리
		if ( currTime - m_ToPeerReliableUdpHeartbeatLastTimeMs > m_owner->m_ReliableUdpHeartbeatInterval_USE )
		{
			m_ToPeerReliableUdp.Heartbeat();
			m_ToPeerReliableUdpHeartbeatLastTimeMs = currTime;
		}

		// direct P2P 가 허락되며, P2P 홀펀칭 재사용 여부의 판독 결과를 받은 상태이면
		FallbackMethod fm = m_owner->m_settings.m_fallbackMethod;
		if (fm != FallbackMethod_PeersUdpToTcp
			&& fm != FallbackMethod_CloseUdpSocket
			&& fm != FallbackMethod_ServerUdpToTcp
			&& !m_forceRelayP2P)
		{
			if (m_jitDirectP2PNeeded && !m_jitDirectP2PTriggered &&
				m_udpSocket == NULL && GetPreciseCurrentTimeMs() > m_udpSocketCreationTimeMs)
			{
				// 유저가 로컬에서 피어로의 통신을 시도했었다.
				// 서버에게 딱 1회만 "저쪽 피어와 홀펀칭 하겠다"를 요청한다.
				m_jitDirectP2PTriggered = true;
				m_owner->m_c2sProxy.NotifyJitDirectP2PTriggered(HostID_Server, g_ReliableSendForPN, m_HostID);
			}

			// 서버에서 '둘다 홀펀칭 하라'는 명령이 있으면 홀펀칭 과정을 시작한다.
			// (그래서 이름 뒤에 OnNeed다.)
			NewUdpSocketAndStartP2PHolepunch_OnNeed();
		}

		if ( m_p2pConnectionTrialContext != NULL )
		{
			bool alive = m_p2pConnectionTrialContext->Heartbeat();
			if ( alive == false )
			{
				m_p2pConnectionTrialContext = CP2PConnectionTrialContextPtr(); // kill it
			}
		}

		// add by ulelio : 일정 시간 마다 클라간에 서버 시간 및 FrameRate 및 Ping을 동기화 하자.
		// 		if(currTime - m_ToPeerReportServerTimeAndPingLastTimeMs > CNetConfig::ReportServerTimeAndPingIntervalMs)
		// 		{
		// 			CApplicationHint hint;
		// 			m_owner->GetApplicationHint(hint);
		// 			m_owner->m_c2cProxy.ReportServerTimeAndFrameRateAndPing(m_HostID, g_ReliableSendForPN, GetPreciseCurrentTimeMs(), hint.m_recentFrameRate);
		// 			m_ToPeerReportServerTimeAndPingLastTimeMs = currTime;
		// 		}

		// 일정 시간내로 UDP 패킷을 전혀 받지 못하면 디스로 간주한다. 
		// P2P unreliable ping이 있기 때문에 UDP 패킷을 너무 오래 받는 것이 false positive일리는 없음.
		if ( IsRelayConditionByUdpFailure(currTime) )
		{
			FallbackP2PToRelay(true, ErrorType_P2PUdpFailed);
		}
		else if ( IsRelayConditionByReliableUdpFailure() )
		{
			FallbackP2PToRelay(true, ErrorType_ReliableUdpFailed);
		}

		// p2p repunch를 필요시 한다.	
		if ( IsRelayedP2P() &&
			m_repunchStartTimeMs > 0 &&
			currTime > m_repunchStartTimeMs &&
			m_udpSocket &&
			!m_udpSocket->IsSocketClosed() )
		{
			m_repunchStartTimeMs = 0; // 더 이상 할 필요가 없다. 또 fallback을 한 상황이 발생하기 전까지는.
			CreateP2PConnectionTrialContext();
		}

		if ( m_udpSocket != NULL && currTime - m_lastCheckSendQueueTimeMs > CNetConfig::SendQueueHeavyWarningCheckCoolTimeMs )
		{
			int length = m_udpSocket->GetUdpSendQueueLength(m_P2PHolepunchedLocalToRemoteAddr);

			if ( m_sendQueueHeavyStartTimeMs != 0 )
			{
				if ( length > CNetConfig::SendQueueHeavyWarningCapacity )
				{
					if ( currTime - m_sendQueueHeavyStartTimeMs > CNetConfig::SendQueueHeavyWarningTimeMs )
					{
						m_sendQueueHeavyStartTimeMs = currTime;

						String str;
						str.Format(_PNT("sendQueue %dBytes"), length);
						m_owner->EnqueWarning(ErrorInfo::From(ErrorType_SendQueueIsHeavy, m_HostID, str));
					}
				}
				else
					m_sendQueueHeavyStartTimeMs = 0;
			}
			else if ( length > CNetConfig::SendQueueHeavyWarningCapacity )
				m_sendQueueHeavyStartTimeMs = currTime;

			m_lastCheckSendQueueTimeMs = currTime;
		}
	}

	int64_t CRemotePeer_C::GetIndirectServerTimeDiff()
	{
		// modiby by ulelio : 이젠 udp가 안되도 일정시간마다 이값을 갱신한다.
		return m_indirectServerTimeDiff;
	}

	// firstChance=true이면 서버에게도 알리고 로그도 남긴다.
	void CRemotePeer_C::FallbackP2PToRelay(bool firstChance, ErrorType reason)
	{
		if ( !IsRelayedP2P() )
		{
			if ( firstChance )
			{
#ifndef _DEBUG // 디버그 빌드에서는 의레 있는 일이므로
				if(reason != ErrorType_NoP2PGroupRelation && (m_owner->m_enableLog || m_owner->m_settings.m_emergencyLogLineCount > 0))
				{
					bool LBN = false, RBN = false;
					const PNTCHAR* LBNStr = _PNT("N/A");
					if(m_owner->IsLocalHostBehindNat(LBN))
					{
						LBNStr = LBN ? _PNT("Yes") : _PNT("No");
					}
					RBN = IsBehindNat();
					String TrafficStat = m_owner->GetTrafficStatText();
					int64_t UdpDuration = GetPreciseCurrentTimeMs() - m_RelayedP2PDisabledTimeMs;
					int64_t LastUdpRecvIssueDuration = GetPreciseCurrentTimeMs() - m_owner->m_remoteServer->m_ToServerUdp->m_lastReceivedTime;

					int rank = 1;
					if(!RBN)
						rank++;
					if(!m_owner->GetNatDeviceName().IsEmpty())
						rank++;

					String txt;
					txt.Format(_PNT("(first chance) to-peer client %d UDP punch lost##Reason:%d##CliInstCount=%d##DisconnedCount=%d##recv count=%d##last ok recv interval=%3.31f##Recurred:%d##LocalIP:%s##Remote peer behind NAT:%d##UDP kept time:%3.31f##Time diff since last RecvIssue:%3.31f##%s##Process=%s"),
							   m_HostID , 
							   reason,
							   m_owner->m_manager->m_instanceCount,
							   m_owner->m_manager->m_disconnectInvokeCount,
							   m_directUdpPacketReceiveCount,
							   double(m_lastUdpPacketReceivedInterval) / 1000,
							   m_repunchCount,
							   m_owner->Get_ToServerUdpSocketLocalAddr().ToString().GetString(),
							   RBN,
							   double(UdpDuration) / 1000,
							   double(LastUdpRecvIssueDuration) / 1000,
							   TrafficStat.GetString(),
							   GetProcessName().GetString());

					m_owner->LogHolepunchFreqFail(rank, _PNT("%s"), txt.GetString());

					//MessageBox(NULL,txt,_PNT("AA"),MB_OK);//TEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMP
					//	CErrorReporter_Indeed::Report(txt);  // TEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMPTEMP
				}
#endif
			}

			SetRelayedP2P(true);

			if ( firstChance )
			{
				// 상대측에도 P2P 직빵 연결이 끊어졌으니 relay mode로 전환하라고 알려야 한다.
				m_owner->m_c2sProxy.P2P_NotifyDirectP2PDisconnected(HostID_Server, g_ReliableSendForPN, m_HostID, reason);
			}

			// P2P 홀펀칭 시도중이던 것이 있으면 중지시킨다.
			m_p2pConnectionTrialContext = CP2PConnectionTrialContextPtr();

			/* 이벤트 넣기
			(디스 이유가, 상대측이 더 이상 통신할 이유가 없기 때문일 수 있는데
			이런 경우 이벤트 노티를 하지 말자. 어차피 조만간 서버로부터 P2P 멤버 나감 노티를 받을테니까.) */
			if ( reason != ErrorType_NoP2PGroupRelation )
				m_owner->EnqueFallbackP2PToRelayEvent(m_HostID, reason);

			// repunch를 예비해둔다.
			ReserveRepunch();
		}

		// 아래는 불필요. 이미 이것이 호출될 때는 c/s TCP fallback이 필요하다면 이미 된 상태이니.
		// 	if (firstChance && m_owner->m_ToServerUdp_fallbackable->m_realUdpEnabled == true)
		// 	{
		// 		if(reason == ErrorType_ServerUdpFailed)
		// 			m_owner->FallbackServerUdpToTcp(false, ErrorType_Ok);
		// // 		else if(reason == ErrorType_P2PUdpFailed) 과잉진압이다. 불필요.
		// // 			m_owner->FallbackServerUdpToTcp();
		// 	}
	}

	void CRemotePeer_C::CUdpLayer::SendWithSplitter_Copy(const CSendFragRefs &sendData, const SendOpt &sendOpt)
	{
		if ( m_owner->m_udpSocket )
		{
			assert(m_owner->m_garbaged == false);
			m_owner->m_udpSocket->AddToSendQueueWithSplitterAndSignal_Copy(m_owner->m_HostID,
																		   FilterTag::CreateFilterTag(m_owner->m_owner->GetVolatileLocalHostID(), m_owner->m_HostID),
																		   m_owner->m_P2PHolepunchedLocalToRemoteAddr,
																		   sendData,
																		   GetPreciseCurrentTimeMs(), sendOpt);
		}
		else
			assert(0); // must not be here
	}

	int CRemotePeer_C::CUdpLayer::GetUdpSendBufferPacketFilledCount()
	{
		if ( !m_owner->m_udpSocket )
			return 0;

		return m_owner->m_udpSocket->GetUdpSendQueuePacketCount(m_owner->m_P2PHolepunchedLocalToRemoteAddr);
	}

	bool CRemotePeer_C::CUdpLayer::IsUdpSendBufferPacketEmpty()
	{
		if ( !m_owner->m_udpSocket )
			return true;

		return m_owner->m_udpSocket->IsUdpSendBufferPacketEmpty(m_owner->m_P2PHolepunchedLocalToRemoteAddr);
	}

	bool CRemotePeer_C::IsRelayConditionByUdpFailure(int64_t currTime)
	{
		if ( IsRelayedP2P() == false 
			&& currTime - m_lastDirectUdpPacketReceivedTimeMs > CNetConfig::GetFallbackP2PUdpToTcpTimeoutMs() )
		{
			return true;
		}
		else
			return false;
	}

	bool CRemotePeer_C::IsRelayConditionByReliableUdpFailure()
	{
		/* Reliable UDP 계층에서 sender window 재송신이 지나치게 오래 걸리면 릴레이 모드로 전환한다.
		V회사 케이스에서 필요성이 붙었다. ProudNet의 Reliable UDP만의 성능으로 모든 것을 신뢰할 수는 없기 때문에 최악의 상황에서는
		TCP의 성능에 의존해야 할 터이니. */
		if ( !IsRelayedP2P() && m_ToPeerReliableUdp.m_host &&
			m_ToPeerReliableUdp.m_host->GetMaxResendElapsedTimeMs() > CNetConfig::P2PFallbackTcpRelayResendTimeIntervalMs )	// 어차피 피어,클섭간 연결 살아있는지를 위한 ping-pong이 따로 존재한다. 따라서 Windows TCP처럼 20초 이상 first send가 실패하는 것과 동일하게 처리하는 것이 안전. 안그러면 false positive가 우려됨.
		{
			return true;
		}
		else
			return false;
	}

	void CRemotePeer_C::SetRelayedP2P(bool flag)
	{
		if ( flag )
		{
//			assert(!g_MustNotRelay);




			if ( !m_RelayedP2P_USE_FUNCTION )
				m_setToRelayedButLastPingIsNotCalulcatedYet = true;
			m_RelayedP2P_USE_FUNCTION = true;
		}
		else
		{
			assert(m_udpSocket != NULL);
			m_RelayedP2P_USE_FUNCTION = false;
			m_RelayedP2PDisabledTimeMs = GetPreciseCurrentTimeMs();
			m_lastDirectUdpPacketReceivedTimeMs = GetPreciseCurrentTimeMs(); // 아주 따끈따끈한 상태를 만든다.
			m_directUdpPacketReceiveCount = 0;
			m_lastUdpPacketReceivedInterval = -1;
		}
	}

	// 릴레이로 주고받는게 D-P2P보다 훨씬 빠르면 true. 단, 릴레이 핑 및 직접P2P 핑이 측정된 경우에만.
	bool CRemotePeer_C::IsRelayMuchFasterThanDirectP2P(int serverUdpRecentPingMs, double forceRelayThresholdRatio) const
	{
		if ( forceRelayThresholdRatio <= 0 )
			return false;

		// 하나라도 미측정 핑이 있다면 비교할 릴레이가 훨씬 빠르다는 가정을 해서는 안된다. caller가 D-P2P를 써야 하니까.
		// NOTE: 미측정 핑은 음수로 처리하는데, 이것도 고려되었다. 핑을 측정은 했지만 <0ms라서 0이 나오는 경우도 있는데 이건 미측정이라고 간주하면 안된다.
		if ( serverUdpRecentPingMs <= 0 || m_peerToServerPingMs <= 0 || m_recentPingMs <= 0 )
			return false;

		// 피어간 핑이 충분히 작은 값이면 릴레이 핑이 훨씬 짧건 말건 릴레이가 더 빠름을 무시한다.
		// 계산 오차도 충분할 뿐더러 피어간으로 보내는 것이 어쨌거나 더 나으니까.
		if ( m_recentPingMs <= 20 )
			return false;

		int nRelayPingMs = (serverUdpRecentPingMs + m_peerToServerPingMs);

		return double(nRelayPingMs)*forceRelayThresholdRatio < double(m_recentPingMs);
	}

	void CRemotePeer_C::ReserveRepunch()
	{
		if ( m_repunchCount < CNetConfig::ServerUdpRepunchMaxTrialCount )
		{
			m_repunchCount++;
			m_repunchStartTimeMs = GetPreciseCurrentTimeMs() + CNetConfig::ServerUdpRepunchIntervalMs;
		}
	}

	bool CRemotePeer_C::IsBehindNat()
	{
		return m_UdpAddrInternal.m_binaryAddress != m_UdpAddrFromServer.m_binaryAddress;
	}

	int64_t CRemotePeer_C::GetRenewalSocketCreationTimeMs()
	{
		// 최소한 1초보다는 길어야 한다.
		// 안그러면 P2P member join 노티를 받은 직후 너무 짧은 속도로 P2P RMI를 주고 받을 경우 상대측에서 P2P member join 노티를 받기도 전에 대응하는
		// 피어의 RMI를 받는 사태가 생길 수 있으므로.
		CriticalSectionLock lock(m_owner->GetCriticalSection(), true);

		return GetPreciseCurrentTimeMs() + m_owner->m_random.Next(2000) + 1000;
	}

	// to-peer UDP socket을 생성(혹은 기존 재사용 허가된 UDP socket을 그대로 사용)하고 홀펀칭을 시작한다.
	// 단, 시작 조건을 만족 안하면 둘다 안한다.
	void CRemotePeer_C::NewUdpSocketAndStartP2PHolepunch_OnNeed()
	{
		if (m_owner->m_remoteServer->m_ToServerUdp == NULL)
		{
			// 아직 클라-서버간 UDP가 없으면 시작 못한다. 
			// 하지만 서버로부터 UDP 홀펀칭하라는 요청이 왔으니, 곧 만들어질 것이다.
			// 이 함수는 매 틱마다 호출되므로 곧 다시 올 것이다.
			return;
		}

		m_owner->LockMain_AssertIsLockedByCurrentThread();

		// NOTE: 얼마전 leave했던 remote peer가 다시 join한 경우, 그것이 썼던 UDP socket이 재사용될 수도 있다.
		if ( m_newP2PConnectionNeeded)
		{
			m_newP2PConnectionNeeded = false;

			if (m_udpSocket==NULL)
			{
				// 재사용된 UDP socket도 없으니,
				// UDP 소켓을 생성하되, TCP에서 사용중인 NIC과 같은 것을 쓰도록 한다.
				// 재수없게도 다른 NIC를 통해 엉뚱한 데이터를 UDP socket이 수신해 버리면 
				// 그 UDP socket은 이후부터 보낼 때 NIC B가 송신 주소가 되어서 전송되어 버리기 때문이다.
				AddrPort udpLocalAddr = m_owner->m_remoteServer->m_ToServerTcp->GetLocalAddr();
				if (udpLocalAddr.m_binaryAddress == 0 || udpLocalAddr.m_binaryAddress == 0xffffffff)
				{
#if defined(_WIN32)
					String text;
					text.Format(_PNT("FATAL: Cannot get TCP NIC address! %s cannot be used."), udpLocalAddr.ToString());
					CErrorReporter_Indeed::Report(text);
#endif
					m_owner->EnqueWarning(ErrorInfo::From(ErrorType_LocalSocketCreationFailed, m_owner->GetVolatileLocalHostID(), _PNT("UDP socket for peer connection")));
					return;
				}

				// UDP socket 생성 성공
				m_udpSocket = CSuperSocketPtr(CSuperSocket::New(m_owner, SocketType_Udp));


/*
//3003!!!
			m_udpSocket->m_tagName = _PNT("to-peer UDP");*/

				// bind를 한다. bind는 항상 성공하지만, 사용자가 지정한 UDP port pool이 쓰일수도 있고 아닐수도 있다.
				m_owner->BindUdpSocketToAddrAndAnyUnusedPort(m_udpSocket, udpLocalAddr);
				assert(m_owner->IsCalledByWorkerThread());

				m_owner->AssociateSocket(m_udpSocket->GetSocket());
			}

			// 재사용이건 초기 생성이건 이걸 세팅해주어야.
			m_owner->ValidSendReayList_Add(m_udpSocket);
			m_owner->SocketToHostsMap_SetForAnyAddr(m_udpSocket, this);

#if defined(_WIN32)
			// 이미 IOCP assoc까지 끝난 상태. 이제 first issue를 건다.
			m_udpSocket->IssueRecv(true); // first issue
#endif
			// P2P 홀펀칭을 개시한다.
			CreateP2PConnectionTrialContext();
		}
	}

	// 인자로 받은 hostID에 대한 UDP소켓을 재활용하기 위해 꺼내온다.
	// hostID는 새로 생성되었거나, 연결이 끊어져 garbage처리가 진행 중인 호스트이다.
	bool CRemotePeer_C::RecycleUdpSocketByHostID(HostID hostID)
	{
		if ( m_udpSocket == NULL )
		{
			CFastMap2<HostID, CSuperSocketPtr, int>::iterator it;
			it = m_owner->m_recycles.find(hostID);
			if ( it == m_owner->m_recycles.end() )
			{
				// 재활용할 소켓이 없는 경우.
				return false;
			}
			
			// 재활용 가능한 소켓이 있는 경우
			m_udpSocket = it.GetSecond();



/*//3003!!
			m_udpSocket->m_tagName = _PNT("to-peer UDP recycled");

			assert(m_udpSocket->m_tagName != _PNT("to-server UDP"));*/




			m_owner->m_recycles.erase(it);

			// 꺼내온 것은 살아있어야 한다.
			assert(!m_udpSocket->IsSocketClosed());
		}
		else
		{
			// 이 상황에 왔다는건 P2P Member leave 인 Work Item 이 처리 되기 전에
			// 바로 P2P Member Join 즉, Update 되어서 온 상황이다.
			// 그대로 재사용한다.
			
			// 끊어지기 전에 보낸 Unreliable 메시지들을 다시 받을 수 있다.
		}

		assert(m_udpSocket != NULL);
		assert(m_owner->m_recycles.ContainsKey(hostID) == false);

		m_lastCheckSendQueueTimeMs = 0;
		m_sendQueueHeavyStartTimeMs = 0;

		m_udpSocket->m_timeToGarbage = 0;
		m_udpSocket->m_dropSendAndReceive = false;

		// 재사용하기 위해 꺼내왔지만 아직은 수신을 허락하지 않는다.
		// 수신 허락은 정식으로 홀펀칭을 위해 UDP socket을 쓰기 시작할 때
		// 즉 NewUdpSocketAndStartP2PHolepunch_OnNeed 에서 세팅한다.

		return true;
	}

	// DirectP2P이건 아니건 일단 채우기는 한다.
	void CRemotePeer_C::GetDirectP2PInfo(CDirectP2PInfo& output)
	{
		output.m_localToRemoteAddr = m_P2PHolepunchedLocalToRemoteAddr;
		output.m_remoteToLocalAddr = m_P2PHolepunchedRemoteToLocalAddr;
		if ( m_udpSocket )
			output.m_localUdpSocketAddr = m_udpSocket->GetLocalAddr();
		else
			output.m_localUdpSocketAddr = AddrPort::Unassigned;
	}

	// 이 remote가 local과 같은 LAN에 있는가? 
	// 홀펀칭 안되어있으면 false를 리턴함.
	bool CRemotePeer_C::IsSameLanToLocal()
	{
		if ( !m_udpSocket )
			return false;

		return m_owner->Get_ToServerUdpSocketAddrAtServer().IsSameHost(m_UdpAddrFromServer)
			// 같은 외부IP를 가지고 있음에도 내부IP가 서로 다르게 셋팅되어있는경우, 가령 ( 한쪽은 192.168.77.xxx 다른쪽은 192.168.66.xxx ) 이런식에 대응하기위해 같은 LAN인지 검사하는 로직을 뺀다.
			/*&& m_owner->Get_ToServerUdpSocketLocalAddr().IsSameSubnet24(m_UdpAddrInternal)*/;
	}


	/*	bool CRemotePeer_C::IsDirectP2PInfoDifferent()
	{
		bool different = false;

		if (m_UdpAddrInternal_old != m_UdpAddrInternal)
			different = true;
		if (m_UdpAddrFromServer_old != m_UdpAddrFromServer)
			different = true;
		if (m_P2PHolepunchedLocalToRemoteAddr_old != m_P2PHolepunchedLocalToRemoteAddr)
			different = true;
		if (m_P2PHolepunchedRemoteToLocalAddr_old != m_P2PHolepunchedRemoteToLocalAddr)
			different = true;

		return different;
	}

	void CRemotePeer_C::BackupDirectP2PInfo()
	{
		m_UdpAddrInternal_old = m_UdpAddrInternal;
		m_UdpAddrFromServer_old = m_UdpAddrFromServer;
		m_P2PHolepunchedLocalToRemoteAddr_old = m_P2PHolepunchedLocalToRemoteAddr;
		m_P2PHolepunchedRemoteToLocalAddr_old = m_P2PHolepunchedRemoteToLocalAddr;
	}*/

	CRemotePeer_C::CDisposeWaiter_LeaveEventCount::CDisposeWaiter_LeaveEventCount(CNetClientImpl* owner)
	{
		m_owner = owner;
		m_leaveEventCount = 0;
	}

	bool CRemotePeer_C::CDisposeWaiter_LeaveEventCount::IsTimeout(int64_t currentTimeMs) const
	{
		// 이것은 로컬이벤트이므로 무조건 성공한다고 가정한다.
		return false;
	}

	bool CRemotePeer_C::CDisposeWaiter_LeaveEventCount::CanDispose() const
	{
		return m_leaveEventCount == 0;
	}

	void CRemotePeer_C::CDisposeWaiter_LeaveEventCount::Increase()
	{
		AssertIsLockedByCurrentThread(m_owner->GetCriticalSection());
		++m_leaveEventCount;
		assert(m_leaveEventCount > 0);
	}
	void CRemotePeer_C::CDisposeWaiter_LeaveEventCount::Decrease()
	{
		AssertIsLockedByCurrentThread(m_owner->GetCriticalSection());
		--m_leaveEventCount;
		assert(m_leaveEventCount >= 0);
	}

	CRemotePeer_C::CDisposeWaiter_JoinProcessCount::CDisposeWaiter_JoinProcessCount(CNetClientImpl* owner)
	{
		m_owner = owner;
		m_count = 0;
		m_lastStartTime = 0;
	}
	bool CRemotePeer_C::CDisposeWaiter_JoinProcessCount::IsTimeout(int64_t currentTimeMs) const
	{
		if ( m_lastStartTime != 0 ) {
			if ( currentTimeMs - m_lastStartTime > 10 * 1000 )
				return true;
		}
		return false;
	}
	bool CRemotePeer_C::CDisposeWaiter_JoinProcessCount::CanDispose() const
	{
		return (m_count == 0)
			|| (m_owner->m_worker->GetState() != CNetClientWorker::Connected);
	}
	void CRemotePeer_C::CDisposeWaiter_JoinProcessCount::Increase()
	{
		AssertIsLockedByCurrentThread(m_owner->GetCriticalSection());
		m_lastStartTime = GetPreciseCurrentTimeMs();
		++m_count;
		assert(m_count > 0);
	}
	void CRemotePeer_C::CDisposeWaiter_JoinProcessCount::Decrease()
	{
		AssertIsLockedByCurrentThread(m_owner->GetCriticalSection());
		--m_count;
		assert(m_count >= 0);
	}
}