#pragma once

#include <memory>

#include "../include/FastArray.h"
#include "../include/Enums.h"
#include "../include/AddrPort.h"

namespace Proud
{
	using namespace std;

	class CRemoteClient_S;
	class CSendFragRefs;
	class CSuperSocket;

	// RUDP 릴레이 정보
	struct ReliableDestInfo
	{
		int frameNumber; // 릴레이할 RUDP data frame 넘버

		// 수신자 remote 객체.
		shared_ptr<CRemoteClient_S> sendToRC;
		CSendFragRefs* payloadToSend;

		// Remote가 뿐만 아니라 Socket도 직접 멤버로 가진다.
		// #NOTE_AFTER_MAIN_UNLOCK 참고.
		shared_ptr<CSuperSocket> tcpSocket;

		HostID sendTo; // 릴레이 수신자
	};

	class ReliableDestInfoArray :public CFastArray<ReliableDestInfo> {};

	struct UnreliableDestInfo
	{
		// NOTE: 이런 클래스를 만들 때는, shared_ptr<CSuperSocket>을 직접 멤버로 가져야 한다. 아래 변수처럼. #NOTE_AFTER_MAIN_UNLOCK
		// main unlock 후부터는 remote가 가지고 있는 shared_ptr<CSuperSocket> 멤버가 돌연 null이 될 수 있기 때문이다.

		// 송신에 사용할 socket
		shared_ptr<CSuperSocket> m_socket;
		// 송신할 payload 데이터
		// 송신 처리 함수가 리턴하기 전에 이 구조체는 파괴된다. 따라서 이 raw ptr이 가리키는 객체가 댕글링하지는 않는다.
		CSendFragRefs* payloadToBeSent;
		bool useUdpSocket;
		HostID sendTo;
		AddrPort destAddr;
	};

	class UnreliableDestInfoArray : public CFastArray<UnreliableDestInfo, true, false, int> {};
}