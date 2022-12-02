#pragma once 

#include "LeanType.h"
#include "../include/AddrPort.h"
#include "FastList2.h"

namespace Proud
{
	using namespace std;
	
	// 너무 키우지 말자. 너무 크면 스택 오버플로 위험. 
	// 로컬변수로 fixed length array의 크기로 쓰이기 때문임. 
	// 크면 클수록 좋지만 512 * 20 정도면 10KB니까, 조금은 부담스럽네. 그리고 한 context switch에서 512개의 소켓을 처리하는 상황이면 엄청난 동접이니까 enough.
	// 2048로 했을 경우 PS4에서 문제가 발생하니까 함부로 건드리지 말 것...
	// Win32에서는 이 한계가 설령 없지만, CIoEventStatusList가 지나치게 커지면서 resize cost가 막대해지면 안되므로 이 한계를 그대로 쓰도록 하자.
	const int MaxPollCount = 1024;

	class CSuperSocket;
	class CIoEventStatus;

	/* iocp,epoll,kqueue에 associate될 수 있는, 즉 socket or file descriptor를 가리킨다.
	
	iocp에서 i/o completion 혹은 epoll에서 i/o available에 대한 event 및 socket의 i/o state를 가지고 있어야 한다.
	CSuperSocket 등 뭔가 큰 것을 가지는 것이 이 인터페이스를 구현해야 한다.
	*/
	class IIoFileDescriptor
	{
	public:
		virtual ~IIoFileDescriptor() {}

		/* param_shared_from_this에 대해:
		code profile 결과 shared_from_this for CSuperSocket이 느린 것으로 파악됨.
		따라서 S->Func() 대신 S->Func(S)가 더 나음.
		그러나 IIoFileDescriptor는 S의 실제 타입을 모름. 
		따라서 shared_ptr 객체 자체의 주소를 void*로 강제 캐스팅 후, 실 구현 측에서 역 캐스팅해서 사용하자.
		*/
#if defined(_WIN32)
		// socket io completion 이벤트를 처리한다.
		// "overlapped send or recv를 걸어놓은 것이 완료됐다"를 의미
		// iocp에 한함.
		virtual void OnSocketIoCompletion(
			void* param_shared_from_this_ptr,
			CIoEventStatus &comp ) = 0;
#else
		// socket avail io 이벤트를 처리한다.
		// "non block으로 send or recv를 성공적으로 할 수 있는 상태이다"를 의미
		// epoll, poll, kqueue의 경우에 한함.
		virtual void OnSocketIoAvail(
			void* param_shared_from_this_ptr, 
			CIoEventStatus &comp ) = 0;
#endif
	};

	enum IoEventType
	{
		// 한 SuperSocket에 대해 send or connect가 쌓였다.
		IoEventType_Send,
		// 한 SuperSocket에 대해 receive or accept
		IoEventType_Receive,
#ifndef _WIN32
		IoEventType_ReferFlags, // io event type의 bitwise 조합을 참고하라는 뜻
#endif
		IoEventType_LAST,
	};

	// iocp, epoll, kqueue, simple에서 얻은 이벤트
	// iocp의 경우: completion 값이다. recvFrom 등도 채워진다.
	// 기타의 경우: io avail event의 값이다. non-block syscall 후 나머지를 얻어올 수 있다.
	class CIoEventStatus
	{
	public:
		// 완료 타입. ReferCustomValue인 경우 이 클래스의 나머지 값은 모두 무시된다.
		// 대신 CThreadPoolImpl의 custom value queue를 접근하라.
		IoEventType m_type;

		// 완료된 결과와 관련된 객체. 이 객체 값은 CompletionPort.AssociateSocket에서 주어진 object이다.
		shared_ptr<CSuperSocket> m_superSocket;

		// 완료가 실패시 여기 에러 코드가 온다.
		SocketErrorCode m_errorCode;

		// 송수신이 완료된 크기
		int m_completedDataLength;
#ifdef _WIN32
		// recv, recvfrom에서 수신된 flags값. overlapped I/O에도 업뎃되므로 유지해야 하기에 멤버로 선언했다.
		uint32_t m_recvFlags;
#else
		uint32_t m_eventFlags;
#endif // _WIN32
		// async UDP recvfrom에 의한 결과에서나 유효하다.
		AddrPort m_receivedFrom;

		inline CIoEventStatus()
		{
			m_type = IoEventType_LAST;
			m_errorCode = SocketErrorCode_Ok;

			m_completedDataLength = 0;
#ifdef _WIN32
			m_recvFlags = 0;
#else
			m_eventFlags = 0;
#endif // _WIN32
			m_receivedFrom = AddrPort::Unassigned;
		}
	};

	// TODO: mad ping case에서 순간 엄청난 양의 수신을 하는 경우 resize cost가 너무 크다.
	// 따라서, GQCS 부분에서, yield if no more 방식으로 바꾸고, 반복해서 ProcessAllEvents를 하도록 고치자.
	class CIoEventStatusList :public CFastArray<CIoEventStatus, true, false, int> {};
}