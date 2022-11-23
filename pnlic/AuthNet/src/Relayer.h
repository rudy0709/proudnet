/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once

#include "ReliableUDPFrame.h"
#include "../include/FakeClr.h"
#include "../include/Enums.h"

namespace Proud
{
	// 릴레이 되는 메시지에 들어가는, '릴레이 최종 수신자 리스트'의 각 항목
	struct RelayDest
	{
		HostID m_sendTo;
		// p2p reliable message인 경우 reliable UDP layer에서 사용되는 frame number.
		// 릴레이는 TCP로 이루어지므로 수신측에 reliable하게 가짐. 따라서 수신측은 ack를 보내지 말고, 송신측도 frame을 보낸 후 바로 ack를 받았다는 가정하의 처리를 해야 한다.
		int m_frameNumber;
	};

	class RelayDestList : public CFastArray < RelayDest, true, false, int >
	{
	};
}
