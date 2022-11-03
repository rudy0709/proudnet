/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once

#include "../include/FakeClr.h"
#include "../include/FastArray.h"
#include "../include/Message.h"
#include "enumimpl.h"
#include "ReliableUDPConfig.h"

class CMessage;

namespace Proud
{
	class ReliableUdpFrame
	{
	public:
		// frame type
		ReliableUdpFrameType m_type;

		// data frame인 경우 frame number. ack frame인 경우 안 쓰이는 값이다.
		int m_frameNumber;

		// ack를 갖고 있는지 여부와, 수신받고자 하는 next expected frame number. 
		// data frame인 경우 piggybag된 ack이며, 순수 ack frame인 경우 그냥 ack 정보만 갖고 있는다.
		bool m_hasAck;
		int m_ackFrameNumber;
		bool m_maySpuriousRto;	// true인 경우, 수신측에서 spurious rto,즉 송신자가 지나치게 rto를 짧게 잡아서 무의미한 재송신이 벌어지는 것일 수 있다.

		// for data frame
		ByteArrayPtr m_data;
		void CloneTo(ReliableUdpFrame &dest);

		inline ReliableUdpFrame()
		{
			m_type = ReliableUdpFrameType_None;
			m_frameNumber = 0;
			m_hasAck = false;
			m_ackFrameNumber = 0;
			m_maySpuriousRto = false;
		}
	};

	// sender window에서는 보냈던 시간 등을 추가로 간직해야 하므로, 상속 클래스 형태다.
	// (가상 함수도 없음. 상속은 느리다고 어쩌고 딴지걸면 때찌할거임)
	class SenderFrame : public ReliableUdpFrame
	{
	public:
		int64_t m_lastSentTimeMs;	 // 마지막으로 보낸시간
		int64_t m_resendCoolTimeMs; // RT0(Retransmission Timeout)
		int64_t m_firstSentTimeMs;  // 처음 보내진 시간. 최악의 상황을 감지하기 위함.
		int m_sentCount;	// 이미 송신한 횟수. 0이면 아직 보내진 적이 없음.
		bool m_needFastRetransmit;	// TCP fast retransmit과 같음

		inline SenderFrame()
		{
			m_firstSentTimeMs = 0;
			m_lastSentTimeMs = 0;
			m_resendCoolTimeMs = 0;
			m_sentCount = 0;
			m_needFastRetransmit = false;
		}
	};

}
