/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once

#include "../include/sysutil.h"
#include "../include/MilisecTimer.h"
#include "LeanType.h"
#include "../include/AddrPort.h"
#include "FastList2.h"
#include "ioevent.h"
#include "FastMap2.h"
#include "SocketPtrAndSerial.h"

namespace Proud
{

	class CIoEventStatus;

#if defined(_WIN32)
	
	class CompletionPort;

	// NOTE: CompletionStatus 객체는 CIoEventStatus로 개명됐음.	

	// IOCP wrapper.
	// 사용법: Socket.SetCompletionContext=>Completion.AssociateSocket=>Socket.Issue* or Socket.AcceptEx
	class CompletionPort
	{
		friend CSuperSocket;

		HANDLE m_IOCP;

	public:
		CriticalSection m_cs; // 아래 목록 보호

		// 이 ioqueue에서 event noti를 받는 socket들.
		// event noti란, iocp의 경우 completion을, 여타의 경우 buffer available을 의미.
		// 이게 굳이 필요한 이유는 grand kernel noti 기능밖에 없는 플랫폼(freebsd 4.1 미만)을 위함.
		AssociatedSockets m_associatedSockets;

	public:
		void PostCompletionStatus();
		void AssociateSocket(const shared_ptr<CSuperSocket>& socket);
		bool RemoveSocket(const SocketPtrAndSerial& key);
		void GetQueuedCompletionStatusEx(CIoEventStatusList &ret, uint32_t maxWaitTime);

		CompletionPort();
		~CompletionPort();
	};

#endif
}

