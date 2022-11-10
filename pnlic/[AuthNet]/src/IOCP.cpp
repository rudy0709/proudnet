/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "IOCP.h"
#include "../include/sysutil.h"
//#include "../include/NetConfig.h"
#include "enumimpl.h"
#include "FastSocket.h"


namespace Proud
{
	// QueueUserWorkItem이 쓰는 최대 스레드 수와 맞추어 주었다.
	// CThreadPoolImpl이 스레드 수를 유동으로 조절할 수 있기 때문이다. 따라서 최대값을 설정해 줄 필요가 있음.
	const int IOCP_NumberOfConcurrentThreads = 512;

#if defined(_WIN32)
	CompletionPort::CompletionPort()
	{
		m_IOCP = NULL;

		// IOCP 객체를 생성한다. (이제 ProudNet은 더 이상 Window9x를 지원하지 않는다.)
		m_IOCP = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, IOCP_NumberOfConcurrentThreads);
		if (m_IOCP == NULL)
		{
			throw Exception("Cannot create IOCP object!");
		}
	}

	// 주의: 이것이 실행되는 동안 다른 스레드에서 socket을 닫고 있는 중이 아님을 보장해야 한다.
	// 이건 사용자의 몫이다.
	void CompletionPort::AssociateSocket( CFastSocket* socket )
	{
		CriticalSectionLock lock(m_cs, true);

		if(socket->m_associatedIoQueue == this)
			return;

		if(socket->m_associatedIoQueue != NULL)
			throw Exception(_PNT("이미 다른 ioqueue에 assoc되어있음!"));

		if (socket->m_completionContext == NULL)
			throw Exception("Completion Context of Socket Needed to Be Set in Advance!");

		if (!::CreateIoCompletionPort((HANDLE)socket->m_socket,
			m_IOCP,
			(ULONG_PTR)socket,
			IOCP_NumberOfConcurrentThreads))
		{
			throw Exception("CreateIoCompletionPort failed! error code=%d", GetLastError());
		}

		socket->m_associatedIoQueue = this;
		socket->m_ioQueueIter = m_associatedSockets.AddTail(socket);
	}

	void CompletionPort::RemoveSocket(CFastSocket* socket)
	{
		CriticalSectionLock lock(m_cs, true);
		m_associatedSockets.RemoveAt(socket->m_ioQueueIter);
	}

	/* IOCP로부터 쌓인 이벤트를 가져온다. (그러나 소켓에 특화된 기능이다.)
	- 만약 아무것도 가져온게 없으면 null을 리턴한다. 이 때는 다룰 소켓이나 이벤트가 없으므로 재 폴링을 해야 한다.
	- completion 후의 수신된 데이터를 접근하려면 리턴된 CompletsionStatus에 대응하는 Socket object의
	내부 버퍼(송신 또는 수신)를 접근하면 된다. 이때 버퍼의 크기는 CompletionStatus의 멤버에 들어있다.
	어떤 버퍼를 접근하느냐 여부는 CompletionStatus.m_type에 따라 다르다.
	- fake iocp를 쓰는 경우 64개 이상의 socket에 대한 이벤트도 대기할 수 있다. 그러나 1ms단위로 대기했다가 모든 연계소켓을 체크하는 방식이므로 스레드는 좀 바쁘다.

	TODO: 나중에 https://trello.com/c/e2mloYoN/66-gqcsex-gqcs 에 따라 정리해 버리자.
	*/

	// CompletionPort::GetQueuedCompletionStatusEx와 같지만 1개만 뽑는다.
	// 하위 호환성 때문에 일단 넣어뒀지만 나중에는 GetQueuedCompletionStatusEx만 남기도록 하자.
	bool CompletionPort::GetQueuedCompletionStatus( CIoEventStatus *ret, uint32_t maxWaitTime)
	{
		CIoEventStatusArray lv;

		bool r = GetQueuedCompletionStatusEx( lv, 1, maxWaitTime);

		if(r)
		{
			*ret = lv[0];
		}

		return r;
	}

	// 1개 이상의 completion event를 pop한다. 
	// GQCSEX를 미지원하는 OS에서도 사용 가능한 함수다. 단, 1개만 뽑아낸다.	
	// 리턴값: 얻은 것이 있으면 true, 없으면 false. 
	bool CompletionPort::GetQueuedCompletionStatusEx(
		CIoEventStatusArray &ret, 
		int maxPollCount, 
		uint32_t maxWaitTime)
	{		
		bool isGqcsExSupported = IsGqcsExSupported();

		maxPollCount = min(maxPollCount, MaxPollCount);
		maxPollCount = max(0, maxPollCount);

		// GQCSEx가 지원안되는 os에서는 1개만의 poll을 한다. 지원하는 경우 다발 poll을 한다.
		if(!isGqcsExSupported)
			maxPollCount = 1;

		// GQCSEx의 리턴값은 믿지 않는다. 대신 overlapped 구조체가 건드려졌는지를 체크한다.
		PN_OVERLAPPED_ENTRY OverlappedEntry[MaxPollCount];

		// GQCSEx는 entriesRemoved가 >0인데도 배열에 아무것도 업데이트 안하는 경우가 있다.
		// 이러한 경우 초기화 안된 데이터는 쓰레기가 그대로 유지된다.
		// 따라서 아래처럼 리셋한다.
		// 이 때문에 code profiler결과 여기가 지목되겠지만, I/O가 많은 경우 오히려 syscall을 줄이므로
		// 굳이 없애지는 말자.
// #ifdef CLEAN_WITH_LOOP
// 		for(int i=0;i<maxPollCount;i++)
// 		{
// 			OverlappedEntry[i].lpCompletionKey = NULL;
// 			OverlappedEntry[i].lpOverlapped = NULL;
// 		}
// #else
// 		// 이게 더 빠르다.
// 		memset(&OverlappedEntry, 0, sizeof(OverlappedEntry[0])*maxPollCount);
// #endif
		// ::GQCSEX의 리턴값을 신뢰 안함. 대신 이 값이 안 채워졌는지로 검사함.
		// ::GQCSEX의 도움말과 달리, GQCSEX의 리턴값은 여러가지 상황 가령 sem_timeout 등으로 인해서 정상적으로 poll 했음에도 불구하고 false일 수 있으므로 비 신뢰.
		int entriesRemoved = 0; 

		if(isGqcsExSupported)
		{
			BOOL r = CKernel32Api::Instance().GetQueuedCompletionStatusEx(m_IOCP,
				OverlappedEntry,
				maxPollCount, (DWORD*)&entriesRemoved,
				maxWaitTime, FALSE);
			
			// 이 patchwork가 있어야. r=false인데 이게 1로 세팅되는 경우가 있다.
			if (!r)
				entriesRemoved = 0;
		}
		else
		{
			// GQCS의 리턴값은 무시한다. 64,121,1236 때문. 중요한 것은 overlapped 값과 completed bytes 값이다.
			OverlappedEntry[0].lpOverlapped = NULL;
			::GetQueuedCompletionStatus(m_IOCP,
				&OverlappedEntry[0].dwNumberOfBytesTransferred, 
				&OverlappedEntry[0].lpCompletionKey, 
				&OverlappedEntry[0].lpOverlapped, maxWaitTime);
			// GQCS()는 단 0 or 1개를 poll하므로
			if (OverlappedEntry[0].lpOverlapped != NULL)
				entriesRemoved = 1;
		}

		ret.SetCount(0);

		// 각 poll된 것에 대해 처리
		for (int i = 0; i < entriesRemoved; ++i)
		{
			CIoEventStatus status;
			if (OverlappedEntry[i].lpOverlapped == NULL)
			{
				// pOverlapped는 null이 절대 될 수 없다. 이러한 경우 의미없는 값으로 간주하고 poll 대상에 넣지 말자.
				continue;
			}

			// key에는 CFastSocket의 주소값이 들어가 있다.
			// 어차피 이 함수는 UDP socket의 경우 received from 값을 채워주어야 한다.
			// 그래서 key에 CFastSocket이 바로 들어가 있다.
			CFastSocket* spi = (CFastSocket*)OverlappedEntry[i].lpCompletionKey;
			if (spi != NULL)
			{
				status.m_completedDataLength = OverlappedEntry[i].dwNumberOfBytesTransferred; // 0이 아니라.
				// bytes가 음수이면 i/o error이다. 에러코드를 채워야.
				if (OverlappedEntry[i].dwNumberOfBytesTransferred < 0)
				{
					status.m_errorCode = SocketErrorCode_Error;
				}
				else
				{
					status.m_errorCode = SocketErrorCode_Ok;
				}

				status.m_fastSocket = spi;
				status.m_receivedFrom = AddrPort::Unassigned;
				status.m_recvFlags = 0;

				// 여기서부터 overlapped는 유효한 주소값이다.
				if (OverlappedEntry[i].lpOverlapped == &spi->m_recvOverlapped)
				{
					status.m_type = IoEventType_Receive;
					status.m_recvFlags = spi->m_recvFlags;
					status.m_receivedFrom.FromNative(spi->m_recvedFrom);
				}
				else if (OverlappedEntry[i].lpOverlapped == &spi->m_sendOverlapped)
				{
					status.m_type = IoEventType_Send;
				}
			}
			else
			{
				// key is null이면, CThreadPoolImpl이 가진 custom value queue에 뭔가가 들어갔다는 의미.
				// 거기서 꺼내 쓰자. 여기가 아니고.
				status.m_type = IoEventType_ReferCustomValue;
				status.m_fastSocket = NULL;
				status.m_errorCode = SocketErrorCode_Ok;
			}
			ret.Add(status); // 생성자 함수 외에서는 손대지 않은 나머지 인자들도 모두 복사하기 위해.
		}
	
		return ret.GetCount() > 0;
	}

	void CompletionPort::PostCompletionStatus()
	{
		// CThreadPoolImpl에 custom value queue가 따로 있으므로 그냥 이것만 한다.
		// GetQueuedCompletionStatusEx에서
		// pOverlapped는 NULL인 경우 did not poll로 간주하고 있기 때문에 -1을 넣는다.
		::PostQueuedCompletionStatus(m_IOCP, 0, NULL, (LPOVERLAPPED)-1);
	}

	CompletionPort::~CompletionPort()
	{
		CriticalSectionLock lock(m_cs, true); 
		if(m_associatedSockets.GetCount() > 0)
		{
			throw Exception("There are %d socket(s) associating to ioqueue!", m_associatedSockets.GetCount());
		}

		if(m_IOCP != NULL)
		{
			CloseHandle(m_IOCP);
			m_IOCP = NULL;
		}
	}
#endif // _WIN32
}
