/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "../include/Message.h"
#include "../include/NetConfig.h"
#include "ReceivedMessageList.h"
#include "SendFragRefs.h"
#include "../include/MilisecTimer.h"
//#include "NetClient.h"
//#include "NetServer.h"
//#include "Networker_C.h"
//#include "P2PGroup_S.h"
//#include "RemoteClient.h"
//#include "RemotePeer.h"
#include "FastSocket.h"
#include "TCPLayer.h"
#include "SendFragRefs.h"

namespace Proud
{
	StringA firstRequestText = "<policy-file-request/>";


	CTcpSendQueue::CTcpSendQueue()
	{
		m_totalLength = 0;
		m_nonThinnedQueueTotalLength = 0;

		m_partialSentLength = 0;
		//m_queue.UseMemoryPool();CFastList2 자체가 node pool 기능이 有

		m_nextNormalizeWorkTime = 0;

		m_partialSentPacket = NULL;
	}

	void CTcpSendQueue::CheckConsist()
	{
#ifdef _DEBUG
		int totalLength = 0;

		if(m_partialSentPacket != NULL)
		{
			assert(m_partialSentLength < (int)m_partialSentPacket->m_packet.GetCount() );
			assert(m_partialSentLength> 0);
			totalLength += (int)m_partialSentPacket->m_packet.GetCount() - m_partialSentLength ;
		}

		for(QueueType::iterator i=m_thinnedQueue.begin();i!=m_thinnedQueue.end();i++)
		{
			TcpPacketCtx *packet = *i;
			totalLength += (int)packet->m_packet.GetCount();
		}

		assert(m_totalLength >= 0);
		assert(totalLength == m_totalLength);
#endif
	}

	CTcpSendQueue::~CTcpSendQueue()
	{
		for(QueueType::iterator i=m_nonThinnedQueue.begin();i!= m_nonThinnedQueue.end();i++)
		{
			m_packetPool.Drop(*i);
		}
		for(QueueType::iterator i=m_thinnedQueue.begin();i!=m_thinnedQueue.end();i++)
		{
			m_packetPool.Drop(*i);
		}

		if(m_partialSentPacket != NULL)
		{
			m_packetPool.Drop(m_partialSentPacket);
			m_partialSentPacket = NULL;
		}

		m_uniqueIDToPacketMap.Clear();
	}

	void CTcpSendQueue::NormalizePacketQueue()
	{
		// 오래된 메시지 솎아내기
		// unique ID 솎아내기
		// 옮기기

		int64_t curTime = GetPreciseCurrentTimeMs();

		if (m_nextNormalizeWorkTime == 0)
			m_nextNormalizeWorkTime = curTime;

		// UDP 통신이 안되는 경우 Unreliable 패킷은 TCP를 통해서 전송됩니다,
		// ring 0 or 1가 아닌 Unreliable 패킷에 대해서 오래된 패킷을 버립니다. 이 검사는 5초마다 시행합니다.
		if (curTime > m_nextNormalizeWorkTime)
		{
			for (Position i = m_thinnedQueue.GetHeadPosition(); i != NULL;)
			{
				TcpPacketCtx* e = m_thinnedQueue.GetAt(i);
				// ikpil.choi 2017-03-29 (N3785) : 시간 체크가 현재 시간 - 넣은 시간 이여야 하는데, 버그 수정
				if (e->m_reliability == MessageReliability_Unreliable &&
					e->m_priority != MessagePriority_Ring0 && e->m_priority != MessagePriority_Ring1 &&
					curTime - e->m_enquedTime > CNetConfig::CleanUpOldPacketIntervalMs)
				{
					Position iOld = i;
					m_thinnedQueue.GetNext(i);

					m_thinnedQueue.RemoveAt(iOld);

					// imays 2017-04-02 (N3785) : 태그 추가, 안전히 삭제 하는 로직 추가
					if (e->m_uniqueID.m_value != 0)
						m_uniqueIDToPacketMap.RemoveKey(e->m_uniqueID);

					m_totalLength -= (int)e->m_packet.GetCount();

					m_packetPool.Drop(e);
				}
				else
				{
					m_thinnedQueue.GetNext(i);
				}
			}

			m_nextNormalizeWorkTime = curTime + CNetConfig::NormalizePacketIntervalMs;
		}

		// non thinned queue로부터 가져오되 unique ID가 겹치면 교체
		while (m_nonThinnedQueue.GetCount() > 0)
		{
			TcpPacketCtx* headPacket = m_nonThinnedQueue.RemoveHead();

			m_nonThinnedQueueTotalLength -= headPacket->m_packet.GetCount();

			bool alreadyExist = false;

			if (headPacket->m_uniqueID.m_value != 0)
			{
				Position findPacketPos = NULL;
				if (m_uniqueIDToPacketMap.TryGetValue(headPacket->m_uniqueID, findPacketPos))
				{
					// 같은 unique id 를 가진 값이 잇다면 해당 값이 가리키는 패킷을 삭제하고 새 패킷으로 교체
					TcpPacketCtx* pk = m_thinnedQueue.GetAt(findPacketPos);
					m_totalLength -= (int)pk->m_packet.GetCount();

					m_thinnedQueue.SetAt(findPacketPos, headPacket);
					m_totalLength += (int)headPacket->m_packet.GetCount();

					m_packetPool.Drop(pk);

					alreadyExist = true;
				}
//				else
//				{
//						m_uniqueIDToPacketMap.Add(headPacket->m_uniqueID, m_thinnedQueue.GetTailPosition() + 1);
//				}
			}

			if (!alreadyExist)
			{
				m_thinnedQueue.AddTail(headPacket);
				m_totalLength += (int)headPacket->m_packet.GetCount();
				if (headPacket->m_uniqueID.m_value != 0)
					m_uniqueIDToPacketMap.Add(headPacket->m_uniqueID, m_thinnedQueue.GetTailPosition());
			}
		}

		CheckConsist();
	}

	void SetSocketSendAndRecvBufferLength(const shared_ptr<CFastSocket>& socket, int sendBufferLength, int recvBufferLength)
	{
		socket->SetSendBufferSize(sendBufferLength);
		socket->SetRecvBufferSize(recvBufferLength);

#if defined(SO_SNDLOWAT) && !defined(_WIN32)
		// 소켓의 LowWaterMark 설정한다.
		// 이래야 io event가 충분히 빈 상태에서만 오게 함으로, event<->iocall 횟수를 줄이니까.
		socket->SetSendLowWatermark(sendBufferLength - 100);
#endif
	}

	void SetTcpDefaultBehavior_Client( const shared_ptr<CFastSocket>& socket )
	{
		SetSocketSendAndRecvBufferLength(socket,
			CNetConfig::TcpSendBufferLength,
			CNetConfig::TcpRecvBufferLength);

		if (CNetConfig::EnableSocketTcpKeepAliveOption)
		{
			bool value = 1;
			setsockopt(socket->m_socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&value, sizeof(value));
		}
	}

	void SetTcpDefaultBehavior_Server( const shared_ptr<CFastSocket>& socket )
	{
		// 아직까지는 서버,클라 달라야 할 이유가 없다.
		SetTcpDefaultBehavior_Client(socket);
	}

	void SetUdpDefaultBehavior_Client(const shared_ptr<CFastSocket>& socket)
	{
		SetSocketSendAndRecvBufferLength(socket,
			CNetConfig::UdpSendBufferLength_Client,
			CNetConfig::UdpRecvBufferLength_Client);
	}

	void SetUdpDefaultBehavior_Server( const shared_ptr<CFastSocket>& socket )
	{
		SetSocketSendAndRecvBufferLength(socket,
			CNetConfig::UdpSendBufferLength_Server,
			CNetConfig::UdpRecvBufferLength_Server);
	}

	void SetUdpDefaultBehavior_ServerStaticAssigned( const shared_ptr<CFastSocket>& socket )
	{
		SetSocketSendAndRecvBufferLength(socket,
			CNetConfig::UdpSendBufferLength_ServerStaticAssigned,
			CNetConfig::UdpRecvBufferLength_ServerStaticAssigned);
	}

	volatile uint8_t CTcpLayer_Common::m_randomNumber = 25;
}
