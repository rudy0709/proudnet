/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once

#include "../include/FakeClr.h"
#include "ReliableUDPFrame.h"
#include "ReliableUDPHost.h"

namespace Proud
{
	class CRemotePeer_C;
	class CSendFragRefs;
	class CMessage;
	class CReceivedMessageList;
	class CReceivedMessage;
	class ReliableUdpHost;

	// P2P reliable 통신을 위한 객체
	// TODO: 굳이 분리할 필요는 없다. ReliableUdpHost와 합쳐버리자.
	class CRemotePeerReliableUdp
	{
	public:
		CHeldPtr<ReliableUdpHost> m_host;
		CRemotePeer_C *m_owner;

		// P2P reliable 통신이 실패하면 true로 세팅됨
		bool m_failed;

		CRemotePeerReliableUdp(CRemotePeer_C *owner);

		void ResetEngine(int frameNumber);
		void SendWithSplitter_Copy(const CSendFragRefs &sendData);
		int AllStreamToSenderWindowAndYieldFrameNumberForRelayedLongFrame();
		bool EnqueReceivedFrameAndGetFlushedMessages(ReliableUdpFrame &frame, CReceivedMessageList& ret, ErrorType &outError);
		bool EnqueReceivedFrameAndGetFlushedMessages( CMessage &msg, CReceivedMessageList &ret , ErrorType &outError);
		void Heartbeat();
		void SendOneFrame(ReliableUdpFrame &frame);
		HostID TEST_GetHostID();

		bool IsReliableChannel();
//		virtual int GetRecentUnreliablePingMs();
	};
}
