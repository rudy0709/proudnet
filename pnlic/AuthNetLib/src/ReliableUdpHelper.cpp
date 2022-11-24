/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "ReliableUdpHelper.h"
#include "SendFragRefs.h"

namespace Proud 
{
	// P2P reliable 메시징에서 사용될 data frame 패킷들의 최초 값을 생성한다.
	// simple protocol mode에서는, 패킷 캡처-복제 더미 테스트가 가능해야 하므로, 항상 동일한 값을 리턴한다.
	int CRemotePeerReliableUdpHelper::GetRandomFrameNumber( Random &random, bool simpleProtocolMode )
	{
		if(simpleProtocolMode)
		{
			return 100;
		}
		else
     	{
    		// 디버깅하기 편하려면 처음 시작하는 난수 값은 끝 두자리가 십진수로 0이어야 한다.
     		// 음수이면 디버깅할때 버그로 오인하므로 양수로 하자. 어차피 그래도 해킹당하기 어렵다.
			int a = random.Next(INT_MAX);
			if(a<0) 
				a = -a;
			a/=100;
			a*=100;

			// 절대 0이 나오면 안된다. P2PMemberJoin RMI에서 0이면 reset하지 말라는 의미로 사용하니까.
			if (a == 0)
				a = 1;

			return a;
		}
	}

	// header는 heap access를 줄이기 위해 이 함수를 호출하는데서 stack alloc을 해둔 초기화된 객체이어야 한다.
	// 함수 내부에서 임시로 쓰는 변수이므로 사용 후 폐기 가능.
	void CRemotePeerReliableUdpHelper::BuildSendDataFromFrame( ReliableUdpFrame &frame, CSendFragRefs &ret, CMessage& header )
	{
		ret.Clear();

		header.Write((char)MessageType_ReliableUdp_Frame);
		header.Write(frame.m_type);
		switch (frame.m_type)
		{
		case ReliableUdpFrameType_Ack:
			header.Write(frame.m_ackFrameNumber);
			header.Write(frame.m_maySpuriousRto);
			ret.Add(header);
//			puts("SENDACKSENDACKSENDACKSENDACK");
			break;
			//case ReliableUdpFrame::Type_Disconnect:
			//    msg.Write(frame.m_frameID);
			//    msg.Write(frame.m_data);
			//    break;
		case ReliableUdpFrameType_None:
			assert(false);
			break;
		case ReliableUdpFrameType_Data:
			header.Write(frame.m_frameNumber);
			header.Write(frame.m_hasAck);
			if(frame.m_hasAck)
			{
				header.Write(frame.m_ackFrameNumber);	// piggybag되는 ack frame number
				header.Write(frame.m_maySpuriousRto);
			}
			header.WriteScalar((int)frame.m_data.GetCount());		// msg buffer의 크기
			ret.Add(header);
			ret.Add(frame.m_data.GetData(), (int)frame.m_data.GetCount());
			break;
		}
	}

	void CRemotePeerReliableUdpHelper::BuildRelayed2LongDataFrame( int frameNumber, const CSendFragRefs &content, ReliableUdpFrame &ret )
	{
		// build long frame, compatible to reliable UDP data frame
		ret.m_type = ReliableUdpFrameType_Data;
		ret.m_frameNumber = frameNumber;
		ret.m_data.UninitBuffer();
		ret.m_data.UseInternalBuffer();
		content.CopyTo(ret.m_data); // copied!
	}

}