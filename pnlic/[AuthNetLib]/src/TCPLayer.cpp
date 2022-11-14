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
#include "../include/ReceivedMessage.h"
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
	extern StringA policyFileText;// = "<?xml version=\"1.0\"?><cross-domain-policy><allow-access-from domain=\"*\" to-ports=\"*\" /></cross-domain-policy>";

	/* header는 heap access를 줄이기 위해 이 함수를 호출하는데서 
	stack alloc을 해둔 초기화된 객체이어야 한다. 
	함수 내부에서 임시로 쓰는 변수이므로 사용 후 폐기 가능. */
	void CTcpLayer_Common::AddSplitterButShareBuffer(
		const CSendFragRefs &payload,
		CSendFragRefs &ret,
		CMessage& header,
		bool simplePacketMode)
	{
		// splitter와 data length를 header로 추가한 후 보낸다.
		// splitter가 두 종류인 이유는, messageID를 보낼 필요가 없는 UDP에서는 트래픽 낭비를 하지 않기 위해.
		if(simplePacketMode)
		{
			header.Write((short)SimpleSplitter);
			header.Write((int)payload.GetTotalLength());
		}
		else
		{
		header.Write((short)Splitter);
		header.WriteScalar(payload.GetTotalLength());
		}

		//clear을 할 필요가 없다.괜한 cost만 먹는다.
		//이 함수를 사용하는 곳에서는 ret를 모두 생성해서 넣기 때문인데...
		//만일 ret를 clear을 하도록 외부에서 사용한다면 이 주석을 풀자.
		//ret.Clear();
		ret.Add(header.GetData(), header.GetLength());
		ret.Add(payload);
	}

	// 위 함수와 같되, messageID 인자를 추가로 받는다.
	// Splitter는 HasMessageID라는 다른 것이 들어가게 된다.
	// 연결 유지 기능에서 auto message recovery 기능에 쓰임.
	void CTcpLayer_Common::AddSplitterButShareBuffer(
		int messageID,
		const CSendFragRefs &payload,
		CSendFragRefs &ret,
		CMessage& header,
		bool simplePacketMode)
	{
		// splitter와 data length를 header로 추가한 후 보낸다.
		// splitter가 두 종류인 이유는, messageID를 보낼 필요가 없는 UDP에서는 트래픽 낭비를 하지 않기 위해.
		if (simplePacketMode)
		{
			header.Write((short)SimpleSplitter);
			header.Write(payload.GetTotalLength());
		}
		else
		{
			header.Write((short)HasMessageID);
			header.Write(messageID);
			header.WriteScalar(payload.GetTotalLength());
		}

		//clear을 할 필요가 없다.괜한 cost만 먹는다.
		//이 함수를 사용하는 곳에서는 ret를 모두 생성해서 넣기 때문인데...
		//만일 ret를 clear을 하도록 외부에서 사용한다면 이 주석을 풀자.
		//ret.Clear();
		ret.Add(header);
		ret.Add(payload);
	}

	// 리턴값: 추출되어 추가된 항목의 갯수. 만약 잘못된 스트림 데이터가 입력된 것이라면 -1가 리턴된다.
	// 예외 발생가능 함수
	int CTcpLayer_Common::ExtractMessagesFromStreamAndRemoveFlushedStream(
		CStreamQueue &recvStream,
		CReceivedMessageList &extractedMessageAddTarget,
		HostID senderHostID,
		int messageMaxLength,
		ErrorType& outError,
		bool simplePacketMode)
	{
		CTcpLayerMessageExtractor extractor;	

		extractor.m_recvStream = recvStream.GetData();
		extractor.m_recvStreamCount = (int)recvStream.GetLength();
		extractor.m_extractedMessageAddTarget = &extractedMessageAddTarget;
		extractor.m_senderHostID = senderHostID;
		extractor.m_messageMaxLength = messageMaxLength;
		//extractor.m_unsafeHeap = unsafeHeap;

		int addedCount = extractor.Extract(outError, simplePacketMode);

		recvStream.PopFront(extractor.m_outLastSuccessOffset);

		return addedCount;
	}

	/* 스트림으로부터 메시지들을 추려낸다.
	추려낸 결과는 m_extractedMessageAddTarget 에 채워진다.
	- 메서드 호출 전에 멤버 변수들을 다 채워야 한다.
	- policy text는 skip하고 메시지를 추출한다.
	\return 추출한 메시지 갯수. -1이면 잘못된 스트림 데이터가 있음을 의미한다. */
	int CTcpLayerMessageExtractor::Extract( ErrorType &outError, bool simplePacketMode )
	{
		//ATLASSERT(m_unsafeHeap);
		assert(m_messageMaxLength>0);
		outError = ErrorType_Ok;

		if (m_recvStreamCount == 0)
			return 0;

		int retCount = 0;
		// 메시지 크기, 인식자 등을 파악한다.
		CMessage msg;
		msg.UseExternalBuffer((uint8_t*)m_recvStream, m_recvStreamCount);
		msg.SetLength(m_recvStreamCount);
		msg.SetReadOffset(0);

		int lastSuccessOffset = 0;

		// 더 이상 없을때까지 계속 추가한다.
		while (true)
		{
			short splitter;
			ByteArrayPtr content;
			// 처음에는 splitter로 시작한다.
			if (!msg.Read(splitter))
			{
				m_outLastSuccessOffset = lastSuccessOffset;
				return retCount;
			}

			// 잘못된 패킷이면 이를 예외 이벤트로 전달하고(C++ 예외 발생을 하지 말자. 디버깅시 혼란만 가중하고
			// fail over에 약하다) 폐기한다. 그리고 스트림을 끝까지 다 처리한 것처럼 간주한다.
			// TCP인 경우라면 상대와의 연결을 끊고, UDP인 경우라면 제3자 해커가 쓰레기 메시지를 보낸
			// 것일 수도 있으므로 무시한다.
			if (
				(	   splitter != CTcpLayer_Common::Splitter 
					&& splitter != CTcpLayer_Common::TinySplitter 
					&& splitter != CTcpLayer_Common::HasMessageID 
					&& !simplePacketMode) 
				||
				(
					splitter != CTcpLayer_Common::SimpleSplitter && simplePacketMode)
				)
			{
				// splitter가 아니면, plain text로 간주해서 sz가 나올때까지 잘 받아 처리한 것으로 간주하자. 
				// sz가 안나오면 하나도 처리되지 않은 것으로 간주하자. 성능상 위험해 보이지만, PN 자체가 처음 policy text 주고받을 때 빼고는 plain text가 끼어들 일이 없으므로
				// 성능상 문제 없다.
				msg.SetReadOffset(lastSuccessOffset);

				uint8_t* msgData = msg.GetData();
				int textReadOffset = lastSuccessOffset;
				int msgLength = msg.GetLength();

				for(int textIndex = textReadOffset; textIndex<msgLength;textIndex++)
				{
					if(msgData[textIndex] == '\0') // end of text가 감지되었다. 다 읽은 것으로 간주하고 루프를 계속 돈다.
					{
						lastSuccessOffset = textIndex+1;
						msg.SetReadOffset(textIndex+1);  // 이것도 바꿔줘야지.
						goto LoopAgain;	// 루프를 여기서 끝내면 안된다. 받은 message가 추가로 있는지 뒤도 검사해야 하니까.
					}
				}

				// 끝까지 sz가 안나왔다. 아무것도 처리 안된 것으로 간주하고 루프를 끝낸다.
				m_outLastSuccessOffset = lastSuccessOffset;
				return retCount;
			}

			// 메시지 ID
			int messageID; // hasMessageID = true일 때만 유효
			bool hasMessageID;
			hasMessageID = (splitter == CTcpLayer_Common::HasMessageID);
			if (hasMessageID)
			{
				if (!msg.Read(messageID))
				{
					m_outLastSuccessOffset = lastSuccessOffset;
					return retCount;
				}
			}

			// 메시지의 크기를 읽는다.
			int payloadLength;
			if(simplePacketMode)
			{
				if(msg.Read(payloadLength) == false)
				{
					m_outLastSuccessOffset = lastSuccessOffset;
					return retCount;
				}
			}
			else
			{
				if(splitter == CTcpLayer_Common::Splitter || splitter == CTcpLayer_Common::HasMessageID)
				{
					if(msg.ReadScalar(payloadLength) == false)
					{
						m_outLastSuccessOffset = lastSuccessOffset;
						return retCount;
					}
				}
				else // TinySplitter이면
				{
					// 단 1개의 메시지만이 들어있는 UDP packet이다. 따라서 payload length는 그냥 산출하자.				
					payloadLength = m_recvStreamCount - msg.GetReadOffset();
				}
			}

			// 메시지의 크기가 잘못된 크기이면 오류 처리를 한다.
			if(payloadLength < 0)
			{
				m_outLastSuccessOffset = msg.GetLength();
				outError = ErrorType_InvalidPacketFormat;
				return -1;
				/*throw new CBadStreamException(); */
			}
			else if(payloadLength > m_messageMaxLength)
			{
				m_outLastSuccessOffset = msg.GetLength();
				outError = ErrorType_TooLargeMessageDetected;
				return -1;
				/*throw new CBadStreamException(); */
			}

			// 메시지 내용을 읽을 공간이 없으면 애시당초 버퍼를 준비하지 않도록 한다. 여기의 실행지점 충돌 횟수가 워낙 많기 때문에
			// 이런것도 최적화 해야 한다.
			if(msg.CanRead(payloadLength) == false)
			{
				m_outLastSuccessOffset = lastSuccessOffset;
				return retCount;
			}

			// 받을 메시지 버퍼를 준비한다.
			// (여기서 Fast heap access! 따라서 느리다.)
			content.UseInternalBuffer();
			content.SetCount(payloadLength);

			// 메시지 내용을 받는다.
			if (msg.Read(content.GetData(),payloadLength) == false)
			{
				m_outLastSuccessOffset = lastSuccessOffset;
				return retCount;
			}

			{
				CReceivedMessage ri;
				ri.m_unsafeMessage.ShareFromAndResetReadOffset(content);
				ri.m_hasMessageID = hasMessageID;
				if (ri.m_hasMessageID)
					ri.m_messageID = messageID;

				// 각 ri에 대해 ri.m_remoteAddr_onlyUdp에 채움
				ri.m_remoteHostID = m_senderHostID;
				ri.m_remoteAddr_onlyUdp = m_remoteAddr_onlyUdp;
				m_extractedMessageAddTarget->Add(ri);

				/* 
					splitter 가 SimpleSplitter 일때에도 아래 조건문을 수행하여야 한다.
					그렇지 않으면 SimplePacKetMode 에서 하나의 메시지 처리를 계속 하게 된다.
				*/
				if(splitter == CTcpLayer_Common::Splitter || splitter == CTcpLayer_Common::SimpleSplitter || splitter == CTcpLayer_Common::HasMessageID)
				{
					retCount++;

					// 메시지를 성공적으로 읽었다. last success offset을 다음 msg read offset으로 바꾸자.
					// (last success offset이라는 변수명 자체가 틀렸다. offset to read first 식의 의미로 바꿔야 할 듯.)
					lastSuccessOffset = msg.GetReadOffset();
				}
				else
				{
					return ++retCount;
				}
			}				

LoopAgain:
			;
		}
	}

	CTcpLayerMessageExtractor::CTcpLayerMessageExtractor()
	{
		//m_unsafeHeap = NULL;
		m_recvStream = NULL;
		m_recvStreamCount = 0;
		m_extractedMessageAddTarget = NULL;
		m_senderHostID = HostID_None;
		m_messageMaxLength = 0;
		m_outLastSuccessOffset = 0;
	}

	void CTcpSendQueue::PushBack_Copy(const CSendFragRefs& sendData, const SendOpt& sendOpt)
	{

		TcpPacketCtx *packet = m_packetPool.NewOrRecycle();
		packet->FromSendOpt(sendOpt);
		packet->m_enquedTime = GetPreciseCurrentTimeMs();

		sendData.ToAssembledByteArray(packet->m_packet);

		// non thinned queue 에 담기
		m_nonThinnedQueue.AddTail(packet);

		m_nonThinnedQueueTotalLength += packet->m_packet.GetCount();

		// 같은 uniqueID를 가지는 것을 찾아서 중복 처리한다. 단 0은 제외!
		// #ifdef ALLOW_UNIQUEID_FOR_TCP
		// 		if(packet.m_uniqueID != 0)
		// 		{
		// 			for(QueueType::iterator i=m_queue.begin();i!=m_queue.end();i++)
		// 			{
		// 				TcpPacketCtx& e = *i;
		// 				if(e.m_uniqueID == packet.m_uniqueID)
		// 				{
		// 					m_totalLength -= e.m_packet.GetCount();
		// 					e = packet;
		// 					m_totalLength += e.m_packet.GetCount();
		// 					goto L1;
		// 				}
		// 			}
		// 		}
		// #endif // ALLOW_UNIQUEID_FOR_TCP
		// 
		// 		// 비중복. 그냥 끝에 넣기.
		// 		m_queue.AddTail(packet);
		// 		m_totalLength += (int)packet->m_packet.GetCount();
		// #ifdef ALLOW_UNIQUEID_FOR_TCP
		// L1:
		// #endif // ALLOW_UNIQUEID_FOR_TCP
		//		CheckConsist();
	}

	// length만큼 보낼 데이터들을 fragmented send buffer(WSABUF)에 포인터 리스트로서 채운다.
	// fragCount는 output에 집어넣을 fragment의 개수이다.
	void CTcpSendQueue::PeekSendBuf(CFragmentedBuffer& output, int fragCount/*=INT32_MAX*/)
	{
		output.Clear();

		NormalizePacketQueue();

		// 보내다 만것을 우선으로!
		if (m_partialSentPacket != NULL)
		{
			uint8_t* ptr = m_partialSentPacket->m_packet.GetData() + m_partialSentLength;

			int len = (int)m_partialSentPacket->m_packet.GetCount() - m_partialSentLength;
			assert(len > 0);

			output.Add(ptr, len);
			--fragCount;
		}

		// 나머지를 채우기
		for (QueueType::iterator i = m_thinnedQueue.begin(); i != m_thinnedQueue.end(); ++i)
		{
			if ( fragCount <= 0 )
				break;
			TcpPacketCtx* e = *i;
			output.Add(e->m_packet.GetData(), (int)e->m_packet.GetCount());
			--fragCount;
		}
	}

	CTcpSendQueue::CTcpSendQueue()
	{
		m_totalLength = 0;
		m_nonThinnedQueueTotalLength = 0;

		m_partialSentLength = 0;
		//m_queue.UseMemoryPool();CFastList2 자체가 node pool 기능이 有

		m_nextNormalizeWorkTime = 0;

		m_partialSentPacket = NULL;
	}

	// length만큼 패킷 큐에서 제거한다.
	// 최우선: 보내다 만거, 차우선: 패킷 큐 상단
	// 패킷 큐에서 제거 후 남은건 partial sent packet으로 옮긴다. 그리고 offset도 변경.
	// 최종 처리 후 m_totalLength로 변경됨.
	void CTcpSendQueue::PopFront( int length )
	{
		if(length < 0)
			ThrowInvalidArgumentException();

		if(length == 0)
			return;

		if(m_partialSentPacket != NULL)
		{
			// 지우고 나서도 남아있으면
			if(m_partialSentLength + length < (int)m_partialSentPacket->m_packet.GetCount())
			{
				m_partialSentLength += length;
				m_totalLength -= length;
				return;
			}

			// partial sent packet 청소
			int remainder = (int)m_partialSentPacket->m_packet.GetCount() - m_partialSentLength;
			m_totalLength -= remainder;
			length -= remainder;
			
			//m_partialSentPacket->m_packet.ClearAndKeepCapacity();
			m_packetPool.Drop(m_partialSentPacket);
			m_partialSentPacket = NULL;
			m_partialSentLength = 0;
		}

		// queue에서 length만큼 다 지우되 남은 것들은 partial sent packet으로 이송.
		while(length > 0 && m_thinnedQueue.GetCount() > 0)
		{
			TcpPacketCtx *headPacket = m_thinnedQueue.RemoveHead();
			
			if (headPacket->m_uniqueID.m_value != 0)
			{
				m_uniqueIDToPacketMap.RemoveKey(headPacket->m_uniqueID);
			}
			
			if((int)headPacket->m_packet.GetCount() <= length)
			{
				
				m_totalLength -= (int)headPacket->m_packet.GetCount();
				length -= (int)headPacket->m_packet.GetCount();
				//headPacket->m_packet.ClearAndKeepCapacity();
				m_packetPool.Drop(headPacket);
			}
			else 
			{
				m_partialSentPacket = headPacket;
				m_partialSentLength = length;
				
				m_totalLength -= length;
				length = 0;
			}
		}

		CheckConsist();
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

	void CTcpSendQueue::DoForLongInterval()
	{
		m_packetPool.ShrinkOnNeed();
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
				if (e->m_reliability == MessageReliability_Unreliable &&
					e->m_priority != MessagePriority_Ring0 && e->m_priority != MessagePriority_Ring1 &&
					e->m_enquedTime - curTime > CNetConfig::CleanUpOldPacketIntervalMs)
				{
					Position iOld = i;
					m_thinnedQueue.GetNext(i);

					m_thinnedQueue.RemoveAt(iOld);

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
// 				else
// 				{
// 						m_uniqueIDToPacketMap.Add(headPacket->m_uniqueID, m_thinnedQueue.GetTailPosition() + 1);
// 				}
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

	void SetSocketSendAndRecvBufferLength(CFastSocket* socket, int sendBufferLength, int recvBufferLength)
	{
		socket->SetSendBufferSize(sendBufferLength);
		socket->SetRecvBufferSize(recvBufferLength);

#if defined(SO_SNDLOWAT) && !defined(_WIN32)
		// 소켓의 LowWaterMark 설정한다.
		// 이래야 io event가 충분히 빈 상태에서만 오게 함으로, event<->iocall 횟수를 줄이니까.
		socket->SetSendLowWatermark(sendBufferLength - 100);
#endif
	}

	void SetTcpDefaultBehavior_Client( CFastSocket *socket )
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

	void SetTcpDefaultBehavior_Server( CFastSocket *socket )
	{
		// 아직까지는 서버,클라 달라야 할 이유가 없다.
		SetTcpDefaultBehavior_Client(socket);
	}

	void SetUdpDefaultBehavior_Client( CFastSocket *socket )
	{
		SetSocketSendAndRecvBufferLength(socket,
			CNetConfig::UdpSendBufferLength_Client,
			CNetConfig::UdpRecvBufferLength_Client);
	}

	void SetUdpDefaultBehavior_Server( CFastSocket *socket )
	{
		SetSocketSendAndRecvBufferLength(socket,
			CNetConfig::UdpSendBufferLength_Server,
			CNetConfig::UdpRecvBufferLength_Server);
	}

	void SetUdpDefaultBehavior_ServerStaticAssigned( CFastSocket *socket )
	{
		SetSocketSendAndRecvBufferLength(socket,
			CNetConfig::UdpSendBufferLength_ServerStaticAssigned,
			CNetConfig::UdpRecvBufferLength_ServerStaticAssigned);
	}
}