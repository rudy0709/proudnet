#pragma once

#include "../include/CriticalSect.h"
#include "../include/ReceivedMessage.h"
#include "../include/Enums.h"
#include "FastList2.h"

//////////////////////////////////////////////////////////////////////////
// 연결 유지 기능(ACR)의 서버,클라 공통 모듈

namespace Proud
{
	PROUD_API void ByteArray_IncreaseEveryByte(ByteArray &data);

	class CSendFragRefs;

	// 수신 미보장 부분의 데이터
	class CUnguarantee
	{
	public:
		// 아래 message에 이미 포함되어 있는 message ID
		int m_messageID;
		
		// 이미 splitter, message ID가 모두 포함된 상태의 메시지 데이터
		// TcpPacketCtx.m_message와 share를 하기 위해 객체 타입이 이거다.
		// NOTE: CMessage는 ByteArrayPtr을 내장하며 ByteArrayPtr 자체가 object share가 가능하다.
		CMessage m_message;
	};

	/* ACR 재접속시 수신 미보장 데이터를 다시 보내는 역할, 
	그리고 이미 받은 메시지를 재접속 후 재수신 과정에서 그냥 버리게 하는 여갈을 한다. */
	class CAcrMessageRecovery
	{
	public:
		// main lock 혹은 socket의 send queue lock에 의해 보호된다. 이 변수는 둘 중 하나를 가리킨다.
		// 자세한 것은 ACR 명세서에 있음.
		CriticalSection* m_auxCritSec;

		/* 수신 미보장 부준, 즉 상대에게 송신했지만 아직 그것에 대한 ack가 오지 않은 것들이다.
		ACK가 도착하면 제거된다.
		ACR로 재연결 성공시 여기 있던 내용들은 재전송된다. 
		** 주의: 이걸 억세스할때 m_critSec로 먼저 보호할 것! ** */
		CFastList2<CUnguarantee, int> m_unguarantees_NOLOCK;

		// 마지막으로 MessageID 받은 것들 중 최종에 대한 ack를 보낸 시간
		int64_t m_lastAckSentTime;

		/* 마지막으로 수신한 메시지의 messageID에 1을 더한 값
		받는 측에서는 기 처리한 메시지가 또 오면 버려야 하는데,
		이렇게 하기 위해 이 클래스는 '상대방으로부터 마지막으로 받아 처리한 MessageID' 상태를 가져야 한다.
		그것을 여기서 한다.
		*/
		int m_nextMessageIDToReceive;

		// 로컬에서 상대에게 메시지를 보내게 되면 이 값을 보낸다. 그리고 +1 됨.
		int m_nextMessageIDToSend;

		CAcrMessageRecovery(int nextMessageIDToSend, int nextMessageIDToReceive);

		int PopNextMessageIDToSend();
		int PeekNextMessageIDToSend();
		int PeekNextMessageIDToReceive();

		bool ProcessReceivedMessageID(int messageID);

		void Unguarantee_Add(int messageID, const CSendFragRefs &messageWithSplitter);
		void Unguarantees_RemoveUntil(int ackMessageID);
	};
}
