//////////////////////////////////////////////////////////////////////////
// 연결유지기능의 클라,서버 공통 모듈

#include "stdafx.h"
#include "SendFragRefs.h"
#include "acrimpl.h"
#include "SuperSocket.h"
#include "../include/MilisecTimer.h"
#include "SendFragRefs.h"
#include "MessagePrivateImpl.h"

namespace Proud
{
	void ByteArray_IncreaseEveryByte(ByteArray &data)
	{
		for (int i = 0; i < data.GetCount(); i++)
		{
			data[i] += static_cast<uint8_t>(i);
		}
	}

	CAcrMessageRecovery::CAcrMessageRecovery(int nextMessageIDToSend, int nextMessageIDToReceive)
	{
		m_lastAckSentTime = 0;
		m_nextMessageIDToReceive = nextMessageIDToReceive;
		m_nextMessageIDToSend = nextMessageIDToSend;
	}

	// messageID가 가리키는 값 까지의 수신미보장 영역을 제거한다.
	// Message 보낸 것에 대한 ack가 오면 처리됨
	// *주의* ackMessageID는 next expected number임. 일반 네트워크 프로토콜의 ack가 의미하는 바를 그대로 준수했음!
	void CAcrMessageRecovery::Unguarantees_RemoveUntil(int ackMessageID)
	{
		while (!m_unguarantees_NOLOCK.IsEmpty())
		{
			CUnguarantee& r = m_unguarantees_NOLOCK.GetHead();

			// 같지 않으면서, 더 과거의 것이면 제거. 그렇지 않으면 쫑.
			if (r.m_messageID - ackMessageID < 0) // 주의: minus 연산 후 0과 비교해야 함. 그냥 값 대소로 비교할게 아니라.
			{
				m_unguarantees_NOLOCK.RemoveHeadNoReturn();
			}
			else
			{
				break;
			}
		}
	}

	int CAcrMessageRecovery::PopNextMessageIDToSend()
	{
		int ret = m_nextMessageIDToSend;
		m_nextMessageIDToSend++;
		return ret;
	}

	int CAcrMessageRecovery::PeekNextMessageIDToSend()
	{
		return m_nextMessageIDToSend;
	}

	// 가장 마지막에 수신한 message id의 다음 값을 리턴한다. ack 전송용.
	int CAcrMessageRecovery::PeekNextMessageIDToReceive()
	{
		return m_nextMessageIDToReceive; // 여러 스레드에서 다루는 값이더라도 어차피 4byte align이고 4byte이므로 no cs lock이더라도 값을 찢어먹지는 않음
	}

	// N=0이 아니면 N+1 처리. 매 2초마다 next expected MID를 ack하는 트리거. 만약 이미 처리했으므로 버려야 하는 패킷이면 false 리턴.
	bool CAcrMessageRecovery::ProcessReceivedMessageID(int messageID)
	{
		// 이미 처리한 메시지면 false를 리턴
		if (messageID - m_nextMessageIDToReceive < 0)
		{
			//		NTTNTRACE("[ACR] Too old frame %d is dropped. Diff=%d\n", messageID, messageID - m_nextMessageIDToReceive);
			return false;
		}

		//assert(m_nextMessageIDToReceive == messageID); // 보내는 쪽이 더 앞설 수 있으나 정상. RmiContext.m_uniqueID를 넣은 경우 message id가 skip될 수 있으니까.
		m_nextMessageIDToReceive = messageID + 1;

		//NTTNTRACE("[ACR] %p: Message %d is received.\n", this, messageID);

		return true;
	}

	// 수신미보장 부분에 개체 하나를 추가한다.
	// messageWithSplitter: 이미 splitter가 추가된, 즉 tcp stream으로 바로 사용가능한 payload
	// 사용상 주의: 이 함수는 GetNextMessageIDToSend를 호출한 후 얻은 messageID만 사용해서 유저가 호출해야 한다
	void CAcrMessageRecovery::Unguarantee_Add(int messageID, const CSendFragRefs &messageWithSplitter)
	{
		CUnguarantee newItem;
		messageWithSplitter.ToAssembledMessage(newItem.m_message);
		newItem.m_messageID = messageID;

		m_unguarantees_NOLOCK.AddTail(newItem);
	}

	// old socket이 갖고 있던 AMR (수신미보장, 마지막 받은 message ID 등)을 new socket으로 옮긴다.
	// 이 과정은 main 및 두 socket의 lock을 한 상태에서 수행된다.
	void CSuperSocket::AcrMessageRecovery_MoveSocketToSocket(shared_ptr<CSuperSocket> oldSocket, const shared_ptr<CSuperSocket>& newSocket)
	{
		// lock을 할때 ptr 값 순으로 하자. 데드락 예방차.
		CriticalSection* s1 = &oldSocket->m_sendQueueCS;
		CriticalSection* s2 = &newSocket->m_sendQueueCS;
		if (s1 > s2)
		{
			swap(s1, s2);
		}

		s1->Lock();
		s2->Lock();

		// 이제, 교체를 한다.
		newSocket->m_acrMessageRecovery = oldSocket->m_acrMessageRecovery;
		oldSocket->m_acrMessageRecovery = RefCount<CAcrMessageRecovery>();

		// 잠금 해제는 순서 무관함.
		s1->Unlock();
		s2->Unlock();
	}

	// 수신미보장 부분에 쌓여있는 메시지들을 재송신한다.
	// 재송신하더라도, 지우지는 않는다. 어차피 수신한 쪽에서 ack를 보내올테고 그때서야 지우면 되니까.
	// 괜히 먼저 지웠다가 미증유의 버그 만들지 말자.
	void CSuperSocket::AcrMessageRecovery_ResendUnguarantee(
		const shared_ptr<CSuperSocket>& param_shared_from_this)
	{
		CriticalSectionLock sendQueueLock(m_sendQueueCS, true);
		if (m_acrMessageRecovery != nullptr)
		{
			CFastList2<CUnguarantee, int>& u = m_acrMessageRecovery->m_unguarantees_NOLOCK;
			NTTNTRACE("Resent %d messages after ACR.\n", u.GetCount());
			for (CFastList2<CUnguarantee, int>::iterator i = u.begin(); i != u.end(); i++)
			{
				CUnguarantee& ri = *i;
				// ACR 복원 메시지는 이미 splitter가 둘러싸여 있다. 게다가 unguarantee에 들어가면 안되니까 'WITHOUT'
				AddToSendQueueWithoutSplitterAndSignal_Copy(param_shared_from_this, ri.m_message);
			}
		}
	}

	// 마지막까지 받은 message들의 message ID의 다음 값 즉 expected message ID to receive를 출력한다.
	// 출력에 성공했으면 true를 리턴한다.
	// 아직 ack를 보낼 타이밍이 아니거나, 받은 메시지가 없거나, AMR이 없으면 false를 리턴한다.
	bool CSuperSocket::AcrMessageRecovery_PeekMessageIDToAck(int* output)
	{
		CriticalSectionLock sendQueueLock(m_sendQueueCS, true);

		if (!m_acrMessageRecovery)
			return false;

		// 2초마다 보내는 기능은 없애버린다.
		// 해당 messageID 는 c-s ping-pong 에 piggyback 되어 보내지므로...
//		int64_t currTime = GetPreciseCurrentTimeMs();
//		if (currTime - m_acrMessageRecovery->m_lastAckSentTime <= CNetConfig::MessageRecovery_MessageIDAckIntervalMs)
//			return false;

		*output = m_acrMessageRecovery->PeekNextMessageIDToReceive();

		m_acrMessageRecovery->m_lastAckSentTime = GetPreciseCurrentTimeMs();

		return true;
	}

	void CSuperSocket::AcrMessageRecovery_RemoveBeforeAckedMessageID(int messageID)
	{
		CriticalSectionLock sendQueueLock(m_sendQueueCS, true);

		if (m_acrMessageRecovery != NULL)
			m_acrMessageRecovery->Unguarantees_RemoveUntil(messageID);
	}

	// 받은 메시지의 message ID를 처리한다.
	// next expected message ID를 갱신하고, caller는 이 메시지를 버릴 것인지 process할지를
	// 판단하기 위한 값(true:처리해라 false:버려라)를 리턴한다.
	bool CSuperSocket::AcrMessageRecovery_ProcessReceivedMessageID(int messageID)
	{
		CriticalSectionLock sendQueueLock(m_sendQueueCS, true);

		if (m_acrMessageRecovery)
			return m_acrMessageRecovery->ProcessReceivedMessageID(messageID);
		else
			return true;
	}


	// AMR을 초기화한다 기존에 있던 것은 폐기한다.
	// 인자로, 양방향의 message ID를 받는다 (각각 보내기, 받기)
	void CSuperSocket::AcrMessageRecovery_Init(int LocalToRemoteNextMessageID, int RemoteToLocalNextMessageID)
	{
		assert(m_acrMessageRecovery == NULL);

		CriticalSectionLock sendQueueLock(m_sendQueueCS, true);
		m_acrMessageRecovery = RefCount<CAcrMessageRecovery>(new CAcrMessageRecovery(LocalToRemoteNextMessageID, RemoteToLocalNextMessageID));
	}
}
