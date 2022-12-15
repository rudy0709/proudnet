/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once

#include "../include/FakeClr.h"
#include "../include/FastColl.h"
#include "../include/AddrPort.h"
#include "LowFragMemArray.h"
#include "../include/ListNode.h"
#include "../include/SocketEnum.h"

#include "SocketUtil.h"

//#include "typedptr.h"
#include "BufferSegment.h"
#include "ReusableLocalVar.h"

// 주의: CFastSocket은 사용자 비노출이고, 이를 쓰는 Proud.Socket은 PIMPL로 CFastSocket을 쓰고 있다.
// 따라서 헤더파일에서 바로 platform specific socket header file을 include해도 된다...가 아니라 해야 한다.
// SetSendLowWatermark같은 것 때문이다.

#if !defined(_WIN32)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#else
#include <mswsock.h>
#pragma comment(lib,"Ws2_32.lib")
#endif
#include "../include/FastList.h"

#if defined __linux__ || __MACH__
#include <netdb.h>
#endif

#if !defined (_WIN32)
#include "Reactor.h"
#endif
//#include "ioevent.h"

namespace Proud
{
	using namespace std;
	
#if (_MSC_VER>=1400)
// 아래 주석처리된 pragma managed 전처리 구문은 C++/CLI 버전이 있었을 때에나 필요했던 것입니다.
// 현재는 필요없는 구문이고, 일부 환경에서 C3295 "#pragma managed는 전역 또는 네임스페이스 범위에서만 사용할 수 있습니다."라는 빌드에러를 일으킵니다.
//#pragma managed(push,off)
#endif


	extern volatile int32_t g_msgSizeErrorCount;
	extern volatile int32_t g_intrErrorCount;
	extern volatile int32_t g_netResetErrorCount;
	extern volatile int32_t g_connResetErrorCount;

	//////////////////////////////////////////////////////////////////////////
	// 전방 선언
#if !defined (_WIN32)
	class CIoReactorEventNotifier;
#endif

	class CFastSocket;
	typedef RefCount<CFastSocket> FastSocketPtr; 	// SSDP socket에서도 참조하며 Socket같은건 자주 만드는게 아니니까 이렇게 해도 OK. 자주 접근하므로 thread safe로 만들면 곤란. 웬만하면 쓰지 말자. RefCount이 assign op는 thread unsafe하니까.

	class CriticalSection;
	typedef RefCount<CriticalSection> CriticalSectionPtr;

	class CFastSocket;
	class CompletionPort;
#if defined (_WIN32)
	class IIoFileDescriptor;
	class CIoEventStatus;
#endif


	class CReceivedMessageList;
	class IHostObject;
	class CFinalUserWorkItem;

	//////////////////////////////////////////////////////////////////////////
	// 나머지

	enum IoState/*: int  -- ios에서 빌드 안됨 */
	{
		// Linux에서는 freed memory block이 0으로 채워진다.
		// 아래 값이 0인 경우, 행여나 버그가 있을 경우 댕글링 포인터가 발생하면,
		// NotWorking to Working으로의 CAS가 성공해 버릴 수 있다.
		// 설령 버그가 있더라도 그러한 위험으로부터 안전하기 위해서 아래와 같이 해놓았다.
		IoState_FIRST = 12345,
		IoState_NotWorking, // send or receive i/o event를 처리 안하고 있음
		IoState_Working, // 처리하고 있음
		IoState_NoMoreWorkGuaranteed, // 더 이상 i/o event가 없음을 보장함. delete is safe.
		IoState_True,
		IoState_False,
		IoState_LAST,
	};



	enum SocketEvent
	{
#if defined(_WIN32)
		SocketEvent_AcceptEvent = FD_ACCEPT,
#endif
	};

	class Event;

	// 자세한 것은 'About Socket Class.docx' 참조
	class CSocketBuffer :public CFastArray<uint8_t, false, true, int>
	{
	public:
		CSocketBuffer()
		{
			SetGrowPolicy(GrowPolicy_HighSpeed);
			SuspendShrink(); // 한번 늘어난 것은 줄이지 말자.
		}

	};

	// bsd socket과 winsock은 socket valid 여부를 검사하는 방법이 서로 다름. 그래서 이 함수를 써야.
	// (msdn: Socket Data Type)
	inline bool IsValidSocket(SOCKET fd)
	{
		// win32에서는 socket handle이 음수라도 valid할 수 있다. SOCKET 자체가 unsigned type이다!
		// 따라서 아래와 같이 검사해야 한다.
		// (msdn: Socket Data Type)
#if defined (_WIN32)
		return (fd != 0 && fd != InvalidSocket);
#else
		// bsd socket에서는 음수 여부만 검사하면 된다.
		return fd > 0;
#endif
	}

#if defined(_WIN32)
	struct OverlappedEx :public OVERLAPPED
	{
		//volatile bool m_issued; // true if issue function is already called 쓰지말자. 콜러에서 따로 가져야 한다.GQCS 등 외부 요인에 의해 제거되어야 하니까.

		inline OverlappedEx()
		{
			// ikpil.choi : 2016-11-03, memory safe function
			//ZeroMemory(this, sizeof(OVERLAPPED));
			Internal = 0;
			InternalHigh = 0;
			Offset = 0;
			OffsetHigh = 0;
			hEvent = 0;
		}
	};
#endif

	// 소켓 생성 함수의 리턴값.
	struct SocketCreateResult
	{
		shared_ptr<CFastSocket> socket; // 생성한 소켓
		String errorText; // 실패시 채워진다.
	};


	// overlapped I/O를 할 경우 validation 값을 여기서 가지므로 소멸 자체도 worker thread에서 하는 것이 필요하다.
	class CFastSocket : public CListNode<CFastSocket>
	{
		friend class CompletionPort;

		//		IFastSocketDelegate* m_dg;

// true이면, broadcast 가능 UDP socket이다.
		bool m_enableBroadcastOption;

		bool m_verbose; // true이면 경고 이벤트 핸들러가 호출됨

		// 여러 스레드에서 접근하므로 보호되어야 한다.
		CriticalSection m_socketClosed_CS;

		// 0 == false
		// other == true
		volatile int32_t m_socketClosedOrClosing_CS_PROTECTED; // volatile: IsClosedOrClosing 때문

		// Bind()를 호출 할 때 받은 주소 값이 여기 들어간다.
		AddrPort m_bindAddress;

#if defined(_WIN32)
		// issue-recv and issue-accept을 위해 사용됨
		OverlappedEx m_recvOverlapped;
		// issue-send and issue-connect를 위해 사용됨
		OverlappedEx m_sendOverlapped;

		// recv, recvfrom에서 수신된 flags값.
		// overlapped I/O에도 업뎃되므로 유지해야 하기에 멤버로 선언했다.
		DWORD m_recvFlags;
#endif

		// RecvFrom()에 의해 이 값이 채워진다. 따라서 unix에서도 쓰임.
		// 이 값은 overlapped recvfrom이 완료될 때 채워지므로 유효하게 유지해야 한다.
		ExtendSockAddr m_recvedFrom;
		int m_recvedFromLen;

		// WSARecv,WSARecvFrom,AcceptEx에서 사용되는 수신 버퍼
		// RecvFrom() or Recv()를 호출하면 여기가 채워진다.
		CSocketBuffer m_recvBuffer;

		// WSASend,WSASendTo용
		CSocketBuffer m_sendBuffer;

		// 다소 거시기한 디자인의 멤버
		int m_ttlToRestoreOnSendCompletion;
		bool m_ttlMustBeRestoreOnSendCompletion;

	public:
		int m_ioPendingCount; // IO PENDING이 일어난 횟수

	private:
		//SuperSocket의 동명 멤버와 같은 역할을 한다.
		// IoState_True or False이다. 헷갈리니, 웬만하면 이 변수 말고 아래 두 함수를 쓰도록 하자.
		volatile int32_t m_stopIoRequested_USE_FUNCTION;

	public:
		void RequestStopIo_CloseOnWin32();
		bool StopIoRequested() { return m_stopIoRequested_USE_FUNCTION == IoState_True; }

	public:

		inline const ExtendSockAddr& GetRecvedFrom() { return m_recvedFrom; }

		const int GetRecvBufferLength() { return m_recvBuffer.GetCount(); }

		//int m_restoredCount;
		//bool m_ignoreNotSocketError;

	private:
#if defined(_WIN32)
		volatile static LPFN_ACCEPTEX lpfnAcceptEx;
#endif

		bool AssureUnicast(AddrPort sendTo);

#if defined(_WIN32)
		//static LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockAddrs;
		void DirectBindAcceptEx();

		volatile static LPFN_CONNECTEX lpfnConnectEx;
		void DirectBindConnectEx();
	public:
		SocketErrorCode ConnectEx(AddrPort addrPort);

#endif
	public:
		//ConnectEx완료 처리를 한다.
		SocketErrorCode ConnectExComplete();

	public:
		int m_addrFamily;
		SOCKET m_socket;

	private:
		// 이거 말고 Create()를 쓰세요.
		CFastSocket();
	public:
		 ~CFastSocket();

		 static SocketCreateResult CreateFromIPv6Socket(int addressFamily, SOCKET existingSocket);
		 PROUD_API static SocketCreateResult Create(SocketType type);

		 bool IsSocketEmpty();
		 SocketErrorCode Tcp_Send0ByteForConnectivityTest();

		 PROUD_API int SetSocketReuseAddress(bool enable);
		 int SetLingerOption(uint16_t const onoff, uint16_t const linger);

		 int SetSendBufferSize(int size);
		 int SetRecvBufferSize(int size);

		 PROUD_API int GetSendBufferSize(int *outsize);
		 int GetRecvBufferSize(int *outsize);

#if !defined(SOL_SOCKET)
#	error You must include socket header file for correct compilation of the functions below.
#endif

#if defined(SO_SNDLOWAT) && !defined(_WIN32)
		void SetSendLowWatermark(int size);
#endif
#ifdef SO_NOSIGPIPE
		void SetNoSigPipe(bool enable);
#endif

		SocketErrorCode EnableBroadcastOption(bool enable);
		SocketErrorCode AllowPacketFragmentation(bool allow);

		PROUD_API SocketErrorCode Bind();
		PROUD_API SocketErrorCode Bind(int port);
		PROUD_API SocketErrorCode Bind(const PNTCHAR* hostName, int port);
		PROUD_API SocketErrorCode Bind(const AddrPort &localAddr);

		// Bind()에 들어 갔던 주소 인자를 리턴 한다.
		// NOTE : 0 means ANY.
		AddrPort GetBindAddress();

		SocketErrorCode SetTtl(int ttl);
		SocketErrorCode GetTtl(int& outTtl);
		void RestoreTtlOnCompletion();

		void WSAEventSelect(Event* event, int networkEvents);
		PROUD_API void Listen();
		void PostSocketWarning(uint32_t err, const PNTCHAR* where);
		SocketCreateResult Accept(SocketErrorCode& errCode);
		SocketErrorCode AcceptEx(const shared_ptr<CFastSocket>& openedSocket);
		void FinalizeAcceptEx(const shared_ptr<CFastSocket>& TcpListenSocket, AddrPort& localAddr, AddrPort &remoteAddr);
		SocketErrorCode Connect(const AddrPort& hostAddrPort);

		bool BindAppliedSocket();

#if !defined(_WIN32)
		SocketErrorCode RecvFrom(int length);
		SocketErrorCode SendTo_TempTtl(CFragmentedBuffer& sendBuffer, AddrPort sendTo, int ttl, int* doneLength);
		SocketErrorCode Recv(int length);
		//SocketErrorCode NonBlockSend( BYTE* data, int count);
		SocketErrorCode Send(CFragmentedBuffer& sendBuffer, int* doneLength);
#else
		SocketErrorCode IssueRecvFrom(int length);
		SocketErrorCode IssueSendTo(uint8_t* data, int count, AddrPort sendTo);
		// 		inline SocketErrorCode IssueSendTo_NoCopy( CFragmentedBuffer& sendBuffer, AddrPort sendTo )
		// 		{
		// 			return IssueSendTo_NoCopy_TempTtl(sendBuffer,sendTo,-1);
		// 		}
		SocketErrorCode IssueSendTo_NoCopy_TempTtl(CFragmentedBuffer& sendBuffer, const AddrPort& sendTo, int ttl);
		SocketErrorCode IssueRecv(int length);
		SocketErrorCode IssueSend(uint8_t* data, int count);
		SocketErrorCode IssueSend_NoCopy(CFragmentedBuffer& sendBuffer);
#endif

		// 테스트에서 씀
		SocketErrorCode SendTo(uint8_t* data, int length, AddrPort sendTo, int* doneLength);

#if defined(_WIN32)

		bool GetRecvOverlappedResult(bool waitUntilComplete, OverlappedResult &ret);
		bool GetSendOverlappedResult(bool waitUntilComplete, OverlappedResult &ret);
#endif
		PROUD_API AddrPort GetSockName();
		AddrPort GetPeerName();

		void CloseSocketOnly();
		bool GetVerboseFlag();
		void SetVerboseFlag(bool val);

		void SetBlockingMode(bool isBlockingMode);
#ifndef _WIN32
		bool GetBlockingMode();
#endif // _WIN32
		void EnableNagleAlgorithm(bool enable);

		uint8_t* GetRecvBufferPtr();
		//BYTE* GetSendBufferPtr();

		bool IsClosed();

		// i/o completion 에서 자주 호출하므로 무거운 CS lock을 피하기 위함.
		bool IsClosedOrClosing() const
		{
			// NO CS unlock!
			return m_socketClosedOrClosing_CS_PROTECTED != 0;
		}

		SocketErrorCode Shutdown(ShutdownFlag how);

#ifdef USE_DisableUdpConnResetReport
		void DisableUdpConnResetReport();
#endif // USE_DisableUdpConnResetReport

		static bool IsUdpStopErrorCode(SocketErrorCode code);
		static bool IsTcpStopErrorCode(SocketErrorCode code);

		static bool IsWouldBlockError(SocketErrorCode code);
#ifdef _WIN32
		static bool IsPendingErrorCode(SocketErrorCode code);
#endif

#ifdef _WIN32
#pragma push_macro("new")
#undef new
		// 이 클래스는 ProudNet DLL 경우를 위해 커스텀 할당자를 쓰되 fast heap을 쓰지 않는다.
		DECLARE_NEW_AND_DELETE
#pragma pop_macro("new")
#endif

	};

#ifdef _WIN32
	/* socket의 select() non-block model을 위한 용도
	주의: Wait 호출 후에는 FD_SET의 내용이 바뀐다. 따라서 이 객체는 1회성으로 쓰여야 한다.

	참고: ios,linux에서는 select()가 0부터 fd 값까지 모조리 루프를 도는 바보같은 얼개로 작동한다.
	그러므로, select() 대신 깔끔하게 poll()을 쓰자. 이거 쓰지 말고
	*/
	class FastSocketSelectContext
	{
		FD_SET m_readSocketList;
		FD_SET m_writeSocketList;
		FD_SET m_exceptionSocketList;
	public:
		FastSocketSelectContext();
		void AddWriteWaiter(CFastSocket& socket);
		void AddExceptionWaiter(CFastSocket& socket);
		void Wait(uint32_t miliSec);
		bool GetConnectResult(CFastSocket& socket, SocketErrorCode& outCode);
	};
#endif

	class SocketInitializer : public CSingleton<SocketInitializer>
	{
	public:
		SocketInitializer();
		~SocketInitializer();
	};

	extern String g_TEST_CloseSocketOnly_CallStack;
	void TEST_CaptureCallStack();

#if (_MSC_VER>=1400)
//#pragma managed(pop)
#endif
}
