#include "stdafx.h"
#include "UdpSocket_S.h"
#include "FallbackableUdpLayer_S.h"
//#include "../include/NetConfig.h"
//#include <conio.h>
//#include "NetClient.h"
//#include "TcpLayer.h"
#include "SendFragRefs.h"
//#include "RemotePeer.h"
#include "P2PGroup_S.h"
#include "UdpSocket_S.h"
#include "RemoteClient.h"
#include "NetServer.h"

namespace Proud
{
	void CFallbackableUdpLayer_S::SetUdpAddrFromHere( AddrPort val )
	{
		if (m_UdpAddrFromHere_USE_OTHER != val)
		{
			if (m_owner->m_owner->m_udpAssignMode == ServerUdpAssignMode_Static)
			{
				m_owner->m_owner->SocketToHostsMap_RemoveForAddr(m_udpSocket, m_UdpAddrFromHere_USE_OTHER);
			}
			else
			{
				m_owner->m_owner->SocketToHostsMap_RemoveForAnyAddr(m_udpSocket);
			}

			//m_owner->m_owner->m_UdpAddrPortToRemoteClientIndex.RemoveKey(m_UdpAddrFromHere_USE_OTHER);
			m_udpSocket->ReceivedAddrPortToVolatileHostIDMap_Remove(m_UdpAddrFromHere_USE_OTHER);

			m_UdpAddrFromHere_USE_OTHER = val;

			if (m_UdpAddrFromHere_USE_OTHER != AddrPort::Unassigned)
			{
				if (m_owner->m_owner->m_udpAssignMode == ServerUdpAssignMode_Static)
				{
					m_owner->m_owner->SocketToHostsMap_SetForAddr(m_udpSocket, m_UdpAddrFromHere_USE_OTHER, m_owner);
				}
				else
				{
					m_owner->m_owner->SocketToHostsMap_SetForAnyAddr(m_udpSocket, m_owner);
				}

				//m_owner->m_owner->m_UdpAddrPortToRemoteClientIndex[m_UdpAddrFromHere_USE_OTHER] = m_owner;
				assert(m_owner->GetHostID() != HostID_None);
				m_udpSocket->ReceivedAddrPortToVolatileHostIDMap_Set(m_UdpAddrFromHere_USE_OTHER, m_owner->GetHostID());
			}
		}
	}


	CFallbackableUdpLayer_S::CFallbackableUdpLayer_S( CRemoteClient_S* owner)
	{
		m_realUdpEnabled = false;
		m_clientUdpReadyWaiting = false;
		
		m_createUdpSocketHasBeenFailed = false;

		m_UdpAddrFromHere_USE_OTHER = AddrPort::Unassigned;
		m_UdpAddrInternal = AddrPort::Unassigned;

		m_owner = owner;
		m_holePunchMagicNumber = Guid(); // null guid
	}

	// remote client에게 UDP로 보내되 여의치 않으면 TCP로 보낸다.
	// 이함수 호출 전에 main lock이 걸려있어야함.  
	void CFallbackableUdpLayer_S::AddToSendQueueWithSplitterAndSignal_Copy(HostID sendHostID, const CSendFragRefs& sendData,const SendOpt &sendOpt)
	{
		m_owner->LockMain_AssertIsLockedByCurrentThread();
		if (m_realUdpEnabled)
		{
			if (m_udpSocket != NULL)
			{
				// UDP로 송신.
				m_udpSocket->AddToSendQueueWithSplitterAndSignal_Copy(sendHostID,
					FilterTag::CreateFilterTag(HostID_Server, m_owner->GetHostID()),
					GetUdpAddrFromHere(),
					sendData,
					GetPreciseCurrentTimeMs(),
					sendOpt);
			}
		}
		else
		{
			// fallback mode
			//CriticalSectionLock lock(m_owner->m_tcpLayer->GetSendQueueCriticalSection(),true); <= 내부에서 이미 잠그므로 불필요
			if (m_owner->m_tcpLayer != NULL)
			{
				m_owner->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(sendData, sendOpt, m_owner->m_owner->m_simplePacketMode);
			}			
		}
	}

// 	CTcpLayer_S & CFallbackableUdpLayer_S::GetFallbackTcpLayer()
// 	{
// 		return *(m_owner->m_tcpLayer);
// 	}

	/* UDP 소켓이 재사용될 경우 앞서 사용했던 packet frag 버퍼링이 새 UDP peer에게 갈 경우 문제가 있을 수 있다.
	따라서 그것들을 모두 초기화해버린다. */
	void CFallbackableUdpLayer_S::ResetPacketFragState()
	{
		if(m_udpSocket && m_UdpAddrFromHere_USE_OTHER.IsUnicastEndpoint())
		{
			//static assigned이면 어쩌려고;;; 이렇게 없는거 막 넣지 마세요.
			// 이거 모르고 넘어갔으면 "동접 많아지니 UDP가 툭하면 안된다는 제보 발생"이 마구 생겼을텐데, 그리고 이런 거는 쉽게 발견도 안될텐데, 아찔하네요.
			// 아래 지운 소스가 과거에는 왜 m_UdpAddrFromHere_USE_OTHER를 사용했는지 정확히 파악하시기 바랍니다.
			// 미리 리뷰해서 잘못된거 수정 안하면 디버깅할때 훨씬 많은 시간 들여 버그 잡아야 함. 그런 재앙 오지 않게 하세요.
			// 다른 소스도 싹 찾아서 잘못 짠 곳 있으면 수정 바랍니다.
			// 우리가 하는 일은 서버입니다. 실수하면 피해가 큽니다. 
			
			Proud::AssertIsLockedByCurrentThread(m_udpSocket->m_cs);
	
			{
				Proud::AssertIsLockedByCurrentThread(m_udpSocket->GetSendQueueCriticalSection());
				// 주의! 여기서 FragBoard lock을 걸게되면 dispose에서 순서가 꼬이게 됨. (미리 걸고들어오는걸로 변경)
				//CriticalSectionLock lock(m_udpSocket->GetFragBoardCriticalSection(),true);
				m_udpSocket->UdpPacketFragBoard_Remove(m_UdpAddrFromHere_USE_OTHER);
			}

			m_udpSocket->UdpPacketDefragBoard_Remove(m_UdpAddrFromHere_USE_OTHER);
		}
	}

	CFallbackableUdpLayer_S::~CFallbackableUdpLayer_S()
	{
	}
}
