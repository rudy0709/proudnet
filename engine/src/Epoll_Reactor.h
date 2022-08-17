/* ProudNet
 
 이 프로그램의 저작권은 넷텐션에게 있습니다.
 이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
 계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
 무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.
 
 ** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
 */

#pragma once

#if defined(__linux__)
	#include <sys/epoll.h>
	#include <sys/types.h>  
	#include <sys/time.h>
	#include <poll.h>
#endif

#if defined(__MACH__) || defined(__FreeBSD__)
	#include <sys/event.h> // kqueue
	#include <sys/types.h>
	#include <sys/time.h>
#endif

#include "../include/AddrPort.h"
#include "../include/BasicTypes.h"
#include "../include/CriticalSect.h"
#include "../include/FastList.h"
#include "../include/SocketEnum.h"
#include "FastMap2.h"
#include "FastList2.h"
#include "LeanType.h"
#include "ioevent.h"
#include "TimeAlarm.h"
#include "SocketPtrAndSerial.h"

namespace Proud
{

#if !defined (_WIN32)

	// kqueue,epoll의 bitwise event flags
	enum IoFlags
	{
		IoFlags_In = 1,
		IoFlags_Out = 2,
		//EPOLLPRI = 4,
		IoFlags_Error = 8,
		IoFlags_Hangup = 0x10,
		//EPOLLET = 0x20,
		//EPOLLONESHOT = 0x40,
	};

	// simple poll, kqueue, epoll에서 발생한 i/o event notification 객체
	// 어느 소켓인지, 그 소켓과 관련된 사용자 key 값이 뭔지, i/o type이 뭔지 등을 리턴한다.
	// reactor 전용.
	struct IoEventParam 
	{
#if defined(__linux__)
		int		m_nOperation;
		int		m_nOption;
#elif defined(__MACH__) || defined(__FreeBSD__)
		int		m_nFilter;
		int		m_nFlags;
#endif
		// socket handle
		int		m_nFD;
		// user defined ptr
		void   *m_lpUdata;
	};
	
	/* io avail event가 발생하면 그것을 노티해주는 객체의 총칭.
	즉, epoll,kqueue wrapper.
	현재 운영체제는 크게 reactor or proactor로 갈라진다. 따라서 이러한 형태로 만들어진 클래스가 더 유지보수가 쉽다.
	iocp와 달리, CThreadPoolImpl의 각 스레드마다 이 객체를 가진다.
	NOTE: 한때는 custom event를 즉시 처리하기 위해 eventfd for Linux, socketpair for FreeBSD, PQCS for Win32를 
	썼으나[1], 초당 수십만회의 custom event를 처리해야 하는 상황에서는 잦은 thread context switch와 syscall이 문제가 되었다.
	따라서 이들을 모두 없애고, 어차피 200us마다 worker thread가 awake 후 socket event들을 처리할 때 이들을 가치 처리하게
	했다. 이러면 초당 수십만회가 발생하더라도 사람이 인지 못하는 200us의 딜레이를 댓가로 서버의 CPU 사용량을 크게 절약한다. 
	따라서 [1]을 제거해 버렸다. */
	class CIoReactorEventNotifier 
	{
		friend class CFastSocket;

	public:
		CIoReactorEventNotifier();
		virtual ~CIoReactorEventNotifier();

		// event fd 가 리눅스와 맥이 다르다
		// 문제는 기존에 public 으로 선언 되어 있었는데
		// 이를 외부에서 맘껏 참조 하고 있다.
		// 코드의 통일성을 위하여 private 으로 바꾼후 getter 메소드를 만듬
	private:
		// epoll or kqueue file descriptor
		int m_epfd;		

	public:
		// m_associatedSockets를 보호한다.
		CriticalSection  m_cs;			// epoll or kqueue를 보호할 critsec

	private:

		CTimeAlarm m_epollWorkCount_Timer;
		
	public:
		/* 2014.07.09 : seungchul.lee 
		* 초당 epoll_wait loop 횟수
		* GetWorkerThread에서 사용 */
		volatile int m_epollWorkCountPerSec;

	public:
#if defined (__linux__) || defined (__MACH__) || defined(__FreeBSD__) // epoll or kqueue가 있는 OS에 대해
		virtual void SetEvent(IoEventParam *lpParam);		// 이벤트를 이벤트 리스트에 추가한다.
#endif
		virtual void Poll(CIoEventStatusList &ret, uint32_t maxWaitTimeMs);
		virtual void AssociateSocket(const shared_ptr<CSuperSocket>& socket, bool isEdgeTrigger );
#if defined (__linux__) || defined (__MACH__) || defined(__FreeBSD__) // epoll or kqueue가 있는 OS에 대해
		virtual void AssociatedSocket_ChangeToEdgeTrigger(const shared_ptr<CSuperSocket>& socket);
#endif

		virtual bool RemoveSocket(const SocketPtrAndSerial& key);

	private:
#if defined(__MACH__) || defined(__FreeBSD__) || defined(__linux__)
		void Poll_epollOrKqueue(CIoEventStatusList &ret, uint32_t maxWaitTime);
#else
		void Poll_simplePoll(CIoEventStatusList &ret, uint32_t maxWaitTime);	
#endif
	public:
		// 이 ioqueue에서 event noti를 받는 socket들.
		// event noti란, iocp의 경우 completion을, 여타의 경우 buffer available을 의미.
		// 이게 굳이 필요한 이유는 grand kernel noti 기능밖에 없는 플랫폼(freebsd 4.1 미만)을 위함.
		AssociatedSockets m_associatedSockets;  

#if !(defined(__MACH__) || defined(__FreeBSD__) ||  defined(__linux__))
	private:
		// epoll,kqueue를 지원 안하는 플랫폼에서 빌드할 경우 simple poll을 하는데
		// 그때 사용되는 변수들이다. 
		// 로컬 변수로 둬도 되지만 가변 크기이므로 빠른 액세스를 위해.
		CFastArray<pollfd> m_tempPollFdList;
		CFastArray<CFastSocket*> m_tempPollSocketList;
#endif 
	};


#endif //_WIN32
}
