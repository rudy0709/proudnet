/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>

#include "../include/Exception.h"
#include "../include/Singleton.h"
#include "../include/Tracer.h"
#include "../include/Event.h"
#include "../include/ErrorInfo.h"
#include "../include/sysutil.h"
#include "../include/atomic.h"
#include "FastSocket.h"
#include "SocketUtil.h"
#include "IntraTracer.h"
#include "FakeWinsock.h"
#include "ReportError.h"
#include "../include/stacktrace.h"
//#include "../UtilSrc/DebugUtil/NetworkAnalyze.h"

//#define HasAsyncIoIssued(lpOverlapped) (((DWORD)(lpOverlapped)->Internal) != 0)


namespace Proud
{
	volatile int32_t g_msgSizeErrorCount = 0;		// WSAEMSGSIZE 에러난 횟수
	volatile int32_t g_intrErrorCount = 0;			// WSAEINTR 에러난 횟수
	volatile int32_t g_netResetErrorCount = 0;		// WSAENETRESET(10052) 에러난 횟수
	volatile int32_t g_connResetErrorCount = 0;	// WSAECONNRESET(10054) 에러난 횟수

#ifdef _WIN32
	const int AddrLengthInTdi = sizeof(ExtendSockAddr) + 16;
#endif

	volatile int32_t g_TotalSocketCount = 0;

	int send_segmented_data(SOCKET socket, CFragmentedBuffer& sendBuffer, unsigned int flags);
	int sendto_segmented_data(SOCKET socket, CFragmentedBuffer& sendBuffer, unsigned int flags, const sockaddr* sendTo, int sendToLen);


#if defined(_WIN32)
	volatile LPFN_ACCEPTEX CFastSocket::lpfnAcceptEx = NULL;
	volatile LPFN_CONNECTEX CFastSocket::lpfnConnectEx = NULL;

	/* 소켓을 초기화한다. 앱 시작시 콜 미리 해둬야.
	리턴: 성공하면 true */
	bool Socket_StartUp()
	{
		uint16_t wVersionRequested;
		WSADATA wsaData;
		int err;

		wVersionRequested = MAKEWORD(2, 2);
		err = WSAStartup(wVersionRequested, &wsaData);
		if (err != 0)
		{
			// Tell the user that we couldn't find a usable
			// WinSock DLL.
			return 0;
		}
		// Confirm that the WinSock DLL supports 2.2.
		// Note that if the DLL supports versions greater
		// than 2.2 in addition to 2.2, it will still return
		// 2.2 in wVersion since that is the version we
		// requested.
		if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
		{
			// Tell the user that we couldn't find a usable
			// WinSock DLL.
			WSACleanup();
			return 0;
		}
		return 1;
	}

	void Socket_CleanUp()
	{
		WSACleanup();
	}
#endif

	// 테스트로 추가한 변수이다. 나중에 삭제하자.
	String g_TEST_CloseSocketOnly_CallStack;

	void TEST_CaptureCallStack()
	{
#ifdef TEST_CloseSocketOnly_CallStack
		printf("*** Start Callstack ***\n");
		if (g_TEST_CloseSocketOnly_CallStack.IsEmpty())
		{
			CFastArray<String> temp;
			GetStackTrace(temp);
			String temp2;
			int cnt = 0;
			for (auto i : temp)
			{
				temp2 += i;
				std::wcout << _PNT("[") << cnt++ << _PNT(" frame] ") << i.c_str() << std::endl;
				temp2 += _PNT("\n");
			}
			g_TEST_CloseSocketOnly_CallStack = temp2;
		}
		printf("*** End Callstack ***\n");
#endif
	}


	inline bool IsIntrError(CFastSocket* fastsocket, SocketErrorCode e)
	{
		assert(fastsocket != NULL);
		bool ret = (e == SocketErrorCode_Intr && fastsocket->StopIoRequested());
		if (ret)
			AtomicIncrement32(&g_intrErrorCount);
		return ret;
	}

	SocketErrorCode EnableDualStack(SOCKET s)
	{
		// PS4에서는 IPv4 socket만 사용 가능하므로
#ifndef __ORBIS__
		int val = 0;
		socklen_t len = sizeof(val);
		if (::getsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&val, &len) == 0)
		{
			if (val != 0)
			{
				val = 0;
				if (::setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&val, sizeof(val)) != 0)
				{
					return (SocketErrorCode)WSAGetLastError();
				}
			}

			return SocketErrorCode_Ok;
		}

		return (SocketErrorCode)WSAGetLastError();
#else
		// PS4 에서는 IPv4 Socket 만 사용 가능 하므로
		return SocketErrorCode_Error;
#endif
	}

	// IPv6 dual socket을 만들거나, IPv4 socket을 만든다.
	// 생성한 소켓을 리턴하고 outAddrFamily에 생성된 소켓의 IP version을 채운다.
	SOCKET CreateDualStackSocketOrIPv4Socket(SocketType socketType, int& outAddrFamily)
	{
		SOCKET socket;

		switch (socketType)
		{
		case SocketType_Tcp:
		{
#if defined (_WIN32)
			socket = WSASocket(AF_INET6, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
#else
			socket = ::socket(AF_INET6, SOCK_STREAM, 0);
#endif
			outAddrFamily = AF_INET6;

		} break;
		case SocketType_Udp:
#if defined (_WIN32)
			socket = WSASocket(AF_INET6, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
#else
			socket = ::socket(AF_INET6, SOCK_DGRAM, 0);
#endif
			outAddrFamily = AF_INET6;
			break;
		case SocketType_Raw:
#if defined (_WIN32)
			socket = WSASocket(AF_INET6, SOCK_RAW, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
#else
			socket = ::socket(AF_INET6, SOCK_RAW, 0);
#endif
			outAddrFamily = AF_INET6;
			break;
		default:
			ShowUserMisuseError(_PNT("CreateDualStackSocketOrIPv4Socket failed. Invalid Parameter!"));
			socket = 0;
			break;
		}

		SocketErrorCode ret = EnableDualStack(socket);
		if (ret != SocketErrorCode_Ok)
		{
#if !defined(_WIN32)
			::close(socket);
#else
			closesocket(socket);
#endif

			// dual stack 설정 실패 시에는 강제로 ipv4 소켓으로 다시 바꾼다.
			switch (socketType)
			{
			case SocketType_Tcp:
#if defined (_WIN32)
				socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
#else
				socket = ::socket(AF_INET, SOCK_STREAM, 0);
#endif
				outAddrFamily = AF_INET;
				break;
			case SocketType_Udp:
#if defined (_WIN32)
				socket = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
#else
				socket = ::socket(AF_INET, SOCK_DGRAM, 0);
#endif
				outAddrFamily = AF_INET;
				break;
			case SocketType_Raw:
#if defined (_WIN32)
				socket = WSASocket(AF_INET, SOCK_RAW, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
#else
				socket = ::socket(AF_INET, SOCK_RAW, 0);
#endif
				outAddrFamily = AF_INET;
				break;
			default:
				ShowUserMisuseError(_PNT("CreateDualStackSocketOrIPv4Socket failed. Invalid Parameter!"));
				socket = 0;
				break;
			}
		}

		return socket;
	}

	// 기존 소켓 핸들로 CFastSocket 객체를 만든다.
	// 단, IPv6 dual stack만을 허락한다.
	SocketCreateResult CFastSocket::CreateFromIPv6Socket(int addressFamily, SOCKET existingSocket)
	{
		SocketCreateResult ret;
		ret.socket = shared_ptr<CFastSocket>(new CFastSocket);
		ret.socket->m_socket = existingSocket;
		ret.socket->m_addrFamily = addressFamily;

		SocketErrorCode e = EnableDualStack(existingSocket);
		if (e != SocketErrorCode_Ok)
		{
			stringstream part;
			part << "Failed to set socket option IPv6Only=false. error code==" << e;
			// 롤백 처리
			ret.socket->m_socket = InvalidSocket; // 롤백
			ret.socket = shared_ptr<CFastSocket>(); // 그리고 나서 버려야.
			return ret;
		}

		// 성공
		return ret;
	}

	// 새 소켓을 만든다.
	SocketCreateResult CFastSocket::Create(SocketType type)
	{
		SocketCreateResult ret;
		ret.socket = shared_ptr<CFastSocket>(new CFastSocket);

		// 소켓 생성
		ret.socket->m_socket = CreateDualStackSocketOrIPv4Socket(type, /*out*/ ret.socket->m_addrFamily);

		if (!IsValidSocket(ret.socket->m_socket))
		{
			auto getLastErrorResult = WSAGetLastError();

			Tstringstream part;
			part << "Failed to Create the Socket. ";

#if defined(_WIN32)
			part << "WSAGetLastError : " << getLastErrorResult;
			if (WSAEMFILE == getLastErrorResult)
			{
				part << ". Consider setting MaxUserPort value in Windows Registry.";
			}
#else
			part << "errno : " << getLastErrorResult;
			if (EMFILE == getLastErrorResult || ENFILE == getLastErrorResult)
			{
				part << ". Consider using ulimit command with root login for maximum count of file descriptors.";
			}
#endif

			ret.errorText = part.str().c_str();

			// 롤백 처리
			ret.socket = shared_ptr<CFastSocket>();  // 버린다.
			return ret;
		}

		if (CNetConfig::EnableAbortiveSocketClose == true)
		{
			// 스트레스 테스트 등을 진행할 때 클라이언트측이 빠르게 접속, 종료를 반복하게 되면 프넷은 클라이언트에서 close르 먼저 하기 때문에
			// Active Close가 클라이언트에서 발생합니다. Active Close로 인해서 TIME_WAIT가 걸리고 짧은 시간에 클라이언트측에서 접속, 종료를 반복할 경우 TIME_WAIT 상태의 포트가
			// 많아지게 될 것이고, TIME_WAIT 상태의 포트는 사용할 수 없기 때문에 사용할 수 있는 클라이언트측의 port가 소진되는 것으로 인해서 서버에 연결을 못하는 상태가 발생할 수 있습니다.
			ret.socket->SetLingerOption(1, 0);
		}

		// 성공
		return ret;
	}


	SocketErrorCode CFastSocket::Bind()
	{
		return Bind(AddrPort::FromIPPort(AF_INET6, _PNT(""), 0));
	}

	SocketErrorCode CFastSocket::Bind(int port)
	{
		return Bind(AddrPort::FromIPPort(AF_INET6, _PNT(""), (uint16_t)port));
	}

	SocketErrorCode CFastSocket::Bind(const PNTCHAR* hostName, int port)
	{
		AddrPort addr;

		NamedAddrPort namedAddr;

		// 사용자가 hostName을 안 넣는 경우도 고려
		if (hostName && Tstrlen(hostName) > 0)
		{
			namedAddr.m_addr = hostName;
			addr = AddrPort::From(namedAddr);
		}

		// 포트값은 꼭 들어가야.
		namedAddr.m_port = static_cast<uint16_t>(port);

		return Bind(addr);
	}

	SocketErrorCode CFastSocket::Bind(const AddrPort &localAddr)
	{
		m_bindAddress = localAddr; // bind가 성공하건 안하건 일단 세팅. 나중에 재시도할 때 사용되므로.

		int e = BindSocket(m_addrFamily, m_socket, localAddr);
		if (e != 0)
		{
#if defined(_WIN32)
			String text;
			text.Format(_PNT("Failed to bind UDP local address to %s."), localAddr.ToString().GetString());

			CErrorReporter_Indeed::Report(text);
#endif

			PostSocketWarning(e, _PNT("BD"));
			return (SocketErrorCode)e;
		}

		return SocketErrorCode_Ok;
	}

	AddrPort CFastSocket::GetBindAddress()
	{
		return m_bindAddress;
	}

#if defined(_WIN32)
	void CFastSocket::WSAEventSelect(Event* event, int networkEvents)
	{
		if (::WSAEventSelect(m_socket, event->m_event, networkEvents) != 0)
		{
			PostSocketWarning(WSAGetLastError(), _PNT("ES"));
		}
	}
#endif

	void CFastSocket::PostSocketWarning(uint32_t err, const PNTCHAR* where)
	{
#if !defined(_WIN32)
		if (!CFastSocket::IsWouldBlockError((SocketErrorCode)err))
#else
		if (!CFastSocket::IsPendingErrorCode((SocketErrorCode)err))
#endif
		{
			if (m_verbose == true)
			{
				String msg;
				msg.Format(_PNT("Fail from %s: %d"), where, err);
				// 주석으로 막지 말고 debug build에 한해 warning이 debug output에 표시되게 합시다.
				// NTTNTRACE 쓰라는 뜻.
				// 여기 말고도 모든 OnSocketWarning 쓰던 곳에 수정하세요.
				NTTNTRACE(StringT2A(msg).GetString());
				//				m_dg->OnSocketWarning(this, msg);
			}
		}
	}

	SocketCreateResult CFastSocket::Accept(SocketErrorCode& errCode)
	{
		SOCKET newSocket;

		ExtendSockAddr sockAddr;
#ifndef __ORBIS__
		int sockAddrLen = sizeof(sockAddr);
#else
		int sockAddrLen = sizeof(sockAddr.u.v4);
#endif

	_Retry:
		newSocket = ::accept(m_socket, (sockaddr*)&sockAddr, (socklen_t*)&sockAddrLen);

		if (!IsValidSocket(newSocket))
		{
			errCode = (SocketErrorCode)WSAGetLastError();
			if (IsIntrError(this, errCode))
				goto _Retry;

			// 에러 리턴
			SocketCreateResult r;
			Tstringstream ss;
			ss << "Socket accept fail. socket error=" << errCode;
			r.errorText = ss.str().c_str();
			return r;
		}
		else
		{
			return CFastSocket::CreateFromIPv6Socket(sockAddr.u.s.sa_family, newSocket /*, m_dg*/);
		}
	}

	// 접속을 건다.
	SocketErrorCode CFastSocket::Connect(const AddrPort& hostAddrPort)
	{
		while (true)
		{
			SocketErrorCode err = Socket_Connect(m_addrFamily, m_socket, hostAddrPort);
			if (err == SocketErrorCode_Ok)
				return SocketErrorCode_Ok;

			// EINTR등이면 재시도한다. 단, socket is not closed이어야.
			if (!IsIntrError(this, err))
			{
				// NOTE: EWOULDLOCK 상황이면 에러값 그대로 리턴한다.
#ifdef _WIN32
				if (!IsPendingErrorCode(err))
#else
				if (!IsWouldBlockError(err))
#endif
				{
					PostSocketWarning(err, _PNT("FS.C"));
				}
				return err;
			}
		}
	}


#if !defined(_WIN32)
	// 실제 수신한 양은 resize딘 m_recvBuffer로 확인하면 됨.
	// m_recvBuffer.GetCount()가 0인 경우 오류가 있거나 받은 datagram의 payload가 0을 의미. 하지만 PN 에서는 0바이트 datagram을 절대 안 보낸다.
	// 따라서 오류로 간주해 버리자. 오류값은 return value 에 있다.
	// 어디로부터 수신했는지는 m_recvedFrom으로 확인하라.
	SocketErrorCode CFastSocket::RecvFrom(int length)
	{
		if (length <= 0)
			return SocketErrorCode_InvalidArgument;// 0 이하의 수신 자체는 무의미한데다 socket 오동작을 유발하므로

		m_recvBuffer.SetCount(length);

#ifndef __ORBIS__
		m_recvedFromLen = sizeof(m_recvedFrom);
#else
		m_recvedFromLen = sizeof(m_recvedFrom.u.v4);
#endif

	_Retry:
		int ret = recvfrom(m_socket,
			m_recvBuffer.GetData(),
			length,
			0,
			(struct sockaddr *)&m_recvedFrom,
			(socklen_t *)&m_recvedFromLen);

		if (ret >= 0)
		{
			// 버퍼 사이즈를 다시 잡는다. 읽은 양만큼.
			m_recvBuffer.SetCount(ret);

			return SocketErrorCode_Ok;
		}

		int err = errno;
		switch (err)
		{
		case EMSGSIZE:
			AtomicIncrement32(&g_msgSizeErrorCount);
			break;
		case SocketErrorCode_Intr:
			AtomicIncrement32(&g_intrErrorCount);
			if (!StopIoRequested())
				goto _Retry;
			break;
		case ENETRESET:
			AtomicIncrement32(&g_netResetErrorCount);
			break;
		case ECONNRESET:
			AtomicIncrement32(&g_connResetErrorCount);
			break;
		}

		// 에러가 난 케이스이다.
		m_recvBuffer.SetCount(0);

		if (IsWouldBlockError((SocketErrorCode)err) == false)
			PostSocketWarning(err, _PNT("FS.RF"));

		return (SocketErrorCode)err;
	}

	SocketErrorCode CFastSocket::SendTo_TempTtl(CFragmentedBuffer& sendBuffer, AddrPort sendTo, int ttl, int* doneLength)
	{
		*doneLength = 0;

		int totalLength = sendBuffer.GetLength();
		if (totalLength <= 0)
			return SocketErrorCode_InvalidArgument;// 0 이하의 수신 자체는 무의미한데다 socket 오동작을 유발하므로

		//XXX_TEST13();

		ExtendSockAddr sendToA;
		ErrorInfo errInfo;

#ifndef __ORBIS__
		socklen_t sendToALen = sizeof(sendToA);
		sendTo.ToNativeV6(sendToA);
#else
		socklen_t sendToALen = sizeof(sendToA.u.v4);
		if (!sendTo.ToNativeV4(sendToA, errInfo))
			return SocketErrorCode_AddressNotAvailable;
#endif

		int origTtl;
		bool ttlModified = false;

		if (!AssureUnicast(sendTo))
			return SocketErrorCode_AccessError;

		if (ttl >= 0)
		{
			// 임시로 ttl을 변경한다. 보내기 후 바로 복원한다. reactor pattern에서는 그렇게 해야 한다.
			if (GetTtl(origTtl) == SocketErrorCode_Ok && SetTtl(ttl) == SocketErrorCode_Ok)
			{
				ttlModified = true;
			}
			else
			{
				assert(0); // UNEXPECTED!
			}
		}

	_Retry:
		int ret = sendto_segmented_data(m_socket,
			sendBuffer,
			0,
			(const struct sockaddr*)&sendToA,
			(socklen_t)sendToALen);

		if (ret < 0)
		{
			int err = errno;

			if (!CFastSocket::IsWouldBlockError((SocketErrorCode)err))
				PostSocketWarning(err, _PNT("FS.ST"));

			// 오류 처리를 내도 복원은 선결해야.
			if (ttlModified)
				SetTtl(origTtl);

			if (Proud::IsIntrError(this, (SocketErrorCode)err))
				goto _Retry;

			return (SocketErrorCode)err;
		}

		// 자 이제 복원하자.
		if (ttlModified)
			SetTtl(origTtl);

		*doneLength = ret;

		return SocketErrorCode_Ok;
	}

	// 실제 받은 양은 resize된 m_recvBuffer를 참고하라
	// 받은 양이 0인 경우 graceful shutdown or error임을 의미한다.
	SocketErrorCode CFastSocket::Recv(int length)
	{
		if (length <= 0)
			return SocketErrorCode_InvalidArgument; // 0 이하의 수신 자체는 무의미한데다 socket 오동작을 유발하므로

		//XXX_TEST13();

		m_recvBuffer.SetCount(length);

	_Retry:
		int ret = recv(m_socket,
			m_recvBuffer.GetData(),
			length,
			0);

		if (ret < 0)
		{
			m_recvBuffer.SetCount(0);

			int err = errno;
			if (IsWouldBlockError((SocketErrorCode)err) == false)
				PostSocketWarning(err, _PNT("FS.R"));

			if (Proud::IsIntrError(this, (SocketErrorCode)err))
				goto _Retry;

			return (SocketErrorCode)err;
		}

		// 실제 읽은 양만큼 다시 버퍼를 셋팅한다.
		m_recvBuffer.SetCount(ret);


		//cout << "TCP recv received " << ret << " bytes.\n";


		/*		{
		// 3003!!!!
		// TCP에서 보낸적도 없는건 확실하다. 그렇다면 3003을 TCP socket에서는 바로 받는 순간 존재하는가?
		int i = Array_GetMatchIndex(m_recvBuffer.GetData(), m_recvBuffer.GetCount(), Rmi3003Patterns::Instance);
		assert(i<0);
		}*/




		return SocketErrorCode_Ok;
	}

	// socket을 non-block으로 설정하지 않은 이상 블러킹한다.
	Proud::SocketErrorCode CFastSocket::Send(CFragmentedBuffer& sendBuffer, int* doneLength)
	{

		/*		{ // 3003!!!
		CMessage tempMsg;
		tempMsg.UseInternalBuffer();


		// 혹시 이건 버그의 원인일라나? 펄백 해보자 3003!!!
		for(int i=0;i<sendBuffer.GetSegmentCount();i++)
		{
		WSABUF x = sendBuffer.m_buffer[i];
		tempMsg.Write((uint8_t*)x.buf, x.len);
		}

		if(Array_GetMatchIndex(tempMsg.GetData(), tempMsg.GetLength(), Rmi3003Patterns::Instance)>=0)
		assert(0);

		}*/


		*doneLength = 0;

		if (sendBuffer.GetLength() <= 0)
			return SocketErrorCode_InvalidArgument;// 0 이하의 수신 자체는 무의미한데다 socket 오동작을 유발하므로

	_Retry:
		int ret = send_segmented_data(m_socket,
			sendBuffer,
			0);

		if (ret < 0)
		{
			int err = errno;
			if (Proud::IsIntrError(this, (SocketErrorCode)err))
				goto _Retry;

			if (IsWouldBlockError((SocketErrorCode)err) == false)
				PostSocketWarning(err, _PNT("FS.S"));

			return (SocketErrorCode)err;
		}

		*doneLength = ret;

//		cout << "TCP send done: " << *doneLength << " bytes.\n";

		return SocketErrorCode_Ok;
	}

#else
	Proud::SocketErrorCode CFastSocket::IssueRecv(int length)
	{
		if (length <= 0)
			return SocketErrorCode_InvalidArgument; // 0 이하의 수신 자체는 무의미한데다 socket 오동작을 유발하므로

		//XXX_TEST13();

		m_recvBuffer.SetCount(length);

		WSABUF buf;
		buf.buf = (char*)m_recvBuffer.GetData();
		buf.len = length;

		m_recvFlags = 0;

		while (1)
		{
			int ret = ::WSARecv(m_socket,
				&buf, 1,
				NULL,
				&m_recvFlags,
				&m_recvOverlapped, NULL);

			// NOTE: WSARecv, WSARecvFrom은 recv, recvfrom의 리턴값과 의미가 다르다. MSDN 참고.
			if (ret == 0)
			{
				return SocketErrorCode_Ok;
			}

			uint32_t lastError = WSAGetLastError();
			switch (lastError)
			{
			case WSA_IO_PENDING:
				return SocketErrorCode_Ok;
			case SocketErrorCode_Intr:
				AtomicIncrement32(&g_intrErrorCount);
				if (StopIoRequested())
					return SocketErrorCode_Intr;
				break;
			default:
				// Linux의 EAGAIN은 Win32에서 WOULD_BLOCK으로 포함 되었으므로,
				// WOULD_BLOCK과 동일 하게 처리 해 주면 된다(따로 처리 해 줄 필요 없다).
				PostSocketWarning(lastError, _PNT("FS.IR"));
				return (SocketErrorCode)lastError;
			}
		}
	}

	SocketErrorCode CFastSocket::IssueRecvFrom(int length)
	{
		if (length <= 0)
			return SocketErrorCode_InvalidArgument;// 0 이하의 수신 자체는 무의미한데다 socket 오동작을 유발하므로

		//XXX_TEST13();

		m_recvBuffer.SetCount(length);

		WSABUF buf;
		buf.buf = (char*)m_recvBuffer.GetData();
		buf.len = length;

		m_recvFlags = 0;

		//int retryCount = 0;
		while (1)
		{
#ifndef __ORBIS__
			m_recvedFromLen = sizeof(m_recvedFrom);
#else
			m_recvedFromLen = sizeof(m_recvedFrom.u.v4);
#endif

			// NOTE: WSARecv, WSARecvFrom은 recv, recvfrom의 리턴값과 의미가 다르다. MSDN 참고.
			int ret = ::WSARecvFrom(m_socket,
				&buf, 1,
				NULL,
				&m_recvFlags,
				(SOCKADDR*)& m_recvedFrom, &m_recvedFromLen,
				&m_recvOverlapped, NULL);

			if (ret == 0)
				return SocketErrorCode_Ok; // 성공한거다.

			uint32_t err = WSAGetLastError();
			switch (err)
			{
			case WSA_IO_PENDING:
				return SocketErrorCode_Ok; // 성공한거다.
			case WSAEMSGSIZE:
				/* 수신은 되었지만 나머지 패킷들은 모두 무시된다.
				뒷 부분이 잘린 패킷을 받았으므로 사용하는 것도 거시기하다. 그냥 버려야 한다.
				어차피 PN 자체가 UDP coalesce를 MTU 크기 기반으로 하므로 정상적 상황에서는 이런 오류가 올 일이 없어야 한다.
				MSDN에서는 WSA_IO_PENDING 아니면 no completion signal이므로
				재시도로 이어져야 한다. */
				AtomicIncrement32(&g_msgSizeErrorCount);
			case SocketErrorCode_Intr: // linux에서는 EINTR
				/* WSAEINTR or EINTR 다루기.pptx */
				AtomicIncrement32(&g_intrErrorCount);
				if (StopIoRequested())
					return SocketErrorCode_Intr;
				break;
			case WSAENETRESET:
				// NetReset의 경우도 connreset과 동일하게 처리. WSARecvFrom 도움말에 의하면 TTL expired 패킷이 와도 이게 온다고라.
				// 근거: http://msdn.microsoft.com/en-us/library/ms741686(v=vs.85).aspx
				AtomicIncrement32(&g_netResetErrorCount);
				break;
			case WSAECONNRESET:
				/*				실험 결과,
				이미 닫힌 소켓은 10054가 절대 안온다.
				ICMP host unreachable이 도착한 횟수만큼만 WSARecvFrom가 온다. 즉 무한루프의 위험이 없다. (그럼 VTG CASE는 뭐지?)
				GQCS는 안온다.

				따라서 횟수 제한 없이 재시도를 해도 된다. 즉 이 로직이 맞다능.

				if(m_stopIoRequested)
				ShowUserMisuseError(_PNT("임시~~~WSAECONNRESET!"));
				else
				{
				String text;
				text.Format(_PNT("건재한 UDP 소켓에서 WSAECONNRESET이 옴. 재시도 횟수=%d\n"),retryCount);
				OutputDebugStringW(text);
				retryCount++;
				}*/
				AtomicIncrement32(&g_connResetErrorCount);
				break;
			default:
				// 에러가 난 케이스다.

				// Linux의 EAGAIN은 Win32에서 WOULD_BLOCK으로 포함 되었으므로,
				// WOULD_BLOCK과 동일 하게 처리 해 주면 된다(따로 처리 해 줄 필요 없다).
				PostSocketWarning(err, _PNT("FS.ISF"));
				return (SocketErrorCode)err;
			}
		}
		return SocketErrorCode_Ok;
	}

	SocketErrorCode CFastSocket::IssueSend(uint8_t* data, int count)
	{
		if (count <= 0)
			return SocketErrorCode_InvalidArgument;// 0 이하의 수신 자체는 무의미한데다 socket 오동작을 유발하므로

		//XXX_TEST13();
		m_sendBuffer.SetCount(count);

		// ikpil.choi 2016-11-07 : memcpy_s 로 변경, destSize(2번째 인자) 값이 항상 올바른 값이여야 합니다.
		// GetCount() 가 1 바이트인것을 어떻게 보장 할 것인가?
		//UnsafeFastMemcpy(m_sendBuffer.GetData(), data, count);
		memcpy_s(m_sendBuffer.GetData(), m_sendBuffer.GetCount(), data, count);

		WSABUF buf;
		buf.buf = (char*)m_sendBuffer.GetData();
		buf.len = count;

		DWORD flags = 0;

		while (1)
		{
			int ret = ::WSASend(m_socket,
				&buf, 1,
				NULL,
				flags,
				&m_sendOverlapped, NULL);

			// NOTE: WSASend, WSASendTo은 send, sendto의 리턴값과 의미가 다르다. MSDN 참고.
			if (ret == 0)
			{
				m_ioPendingCount++;
				return SocketErrorCode_Ok;
			}

			uint32_t lastError = WSAGetLastError();
			switch (lastError)
			{
			case WSA_IO_PENDING: // pending이 있으나 성공
				m_ioPendingCount++;
				return SocketErrorCode_Ok;
			case SocketErrorCode_Intr:
				/* WSAEINTR or EINTR 다루기.pptx */
				AtomicIncrement32(&g_intrErrorCount);
				if (StopIoRequested())
					return SocketErrorCode_Intr;
				break; // 재시도
			default:	// 실패
				// Linux의 EAGAIN은 Win32에서 WOULD_BLOCK으로 포함 되었으므로,
				// WOULD_BLOCK과 동일 하게 처리 해 주면 된다(따로 처리 해 줄 필요 없다).
				PostSocketWarning(lastError, _PNT("FS.PSW"));
				return (SocketErrorCode)lastError;
			}
		}
	}

	SocketErrorCode CFastSocket::IssueSend_NoCopy(CFragmentedBuffer& sendBuffer)
	{
		if (sendBuffer.GetLength() <= 0)
			return SocketErrorCode_InvalidArgument;// 0 이하의 수신 자체는 무의미한데다 socket 오동작을 유발하므로

		//		m_sendBuffer.SetCount(count);
		//		memcpy(m_sendBuffer.GetData(), data, count);

		DWORD flags = 0;

		BoundCheckInt32(sendBuffer.GetSegmentCount(), __FUNCTION__);

		while (1)
		{
			int ret = ::WSASend(m_socket,
				sendBuffer.GetSegmentPtr(),
				(int)sendBuffer.GetSegmentCount(),
				NULL,
				flags,
				&m_sendOverlapped, NULL);

			// NOTE: WSASend, WSASendTo은 send, sendto의 리턴값과 의미가 다르다. MSDN 참고.
			if (ret == 0)
			{
				m_ioPendingCount++;
				return SocketErrorCode_Ok;
			}

			uint32_t lastError = WSAGetLastError();
			switch (lastError)
			{
			case WSA_IO_PENDING:
				m_ioPendingCount++;
				return SocketErrorCode_Ok;

			case SocketErrorCode_Intr:	//WSAEINTR or EINTR 다루기.pptx
				AtomicIncrement32(&g_intrErrorCount);
				if (StopIoRequested())
					return SocketErrorCode_Intr;
				break; // retry

			default:
				// Linux의 EAGAIN은 Win32에서 WOULD_BLOCK으로 포함 되었으므로,
				// WOULD_BLOCK과 동일 하게 처리 해 주면 된다(따로 처리 해 줄 필요 없다).
				PostSocketWarning(lastError, _PNT("FS.IS"));
				return (SocketErrorCode)lastError;
			}
		}
	}

	// ttl<0이면 ttl 미변경. 그 외의 경우 ttl을 보낼때 임시로 ttl을 변경한다.
	// 주의: 이 함수는 AddressNotAvailable를 리턴할 수도 있다. socket이 ipv4인데 sendTo가 ipv6 아닌지 체크할 것.
	SocketErrorCode CFastSocket::IssueSendTo_NoCopy_TempTtl(CFragmentedBuffer& sendBuffer, const AddrPort& sendTo, int ttl)
	{
		int totalLength = sendBuffer.GetLength();
		if (totalLength <= 0)
			return SocketErrorCode_InvalidArgument;// 0 이하의 수신 자체는 무의미한데다 socket 오동작을 유발하므로

		//XXX_TEST13();

		ExtendSockAddr sendToA;
#ifndef __ORBIS__
		int sendToALen = sizeof(sendToA);
#else
		int sendToALen = sizeof(sendToA.u.v4);
#endif
		memset(&sendToA, 0, sizeof(sendToA));
		ErrorInfo err;
		switch (m_addrFamily)
		{
		case AF_INET:
			if(!sendTo.ToNativeV4(sendToA, err))
			{
				// 얼레? socket은 ipv4인데 sendto가 ipv6이네?
				return SocketErrorCode_AddressNotAvailable;
			}
			break;
		case AF_INET6:
			sendTo.ToNativeV6(sendToA);
			break;
		}

		if (!AssureUnicast(sendTo))
			return SocketErrorCode_AccessError;


		DWORD flags = 0;

		if (ttl >= 0)
		{
			// 임시로 ttl을 변경한다. 복원은 send completion에서 수행하도록 하자.
			int o﻿rigTtl;
			if (GetTtl(o﻿rigTtl) == SocketErrorCode_Ok && SetTtl(ttl) == SocketErrorCode_Ok)
			{
				m_ttlToRestoreOnSendCompletion = o﻿rigTtl;
				m_ttlMustBeRestoreOnSendCompletion = true;
			}
			else
			{
				assert(0); // UNEXPECTED!
			}
		}


		BoundCheckInt32(sendBuffer.GetSegmentCount(), __FUNCTION__);
		while (1)
		{
			int ret = ::WSASendTo(m_socket,
				sendBuffer.GetSegmentPtr(), (int)sendBuffer.GetSegmentCount(),
				NULL,
				flags,
				(SOCKADDR*)& sendToA, sendToALen,
				&m_sendOverlapped, NULL);

			// NOTE: WSASend, WSASendTo은 send, sendto의 리턴값과 의미가 다르다. MSDN 참고.
			if (ret == 0)
			{
				return SocketErrorCode_Ok;
			}

			uint32_t lastError = WSAGetLastError();

			switch (lastError)
			{
			case WSA_IO_PENDING:
				return SocketErrorCode_Ok;

			case SocketErrorCode_Intr:
				AtomicIncrement32(&g_intrErrorCount);
				if (StopIoRequested())
					return SocketErrorCode_Intr;
				break;

			default:
				// Linux의 EAGAIN은 Win32에서 WOULD_BLOCK으로 포함 되었으므로,
				// WOULD_BLOCK과 동일 하게 처리 해 주면 된다(따로 처리 해 줄 필요 없다).
				RestoreTtlOnCompletion();
				PostSocketWarning(lastError, _PNT("FS.IS"));
				// NOTE: 10054, 10052 or WSAEMSGSIZE는 상대방에게 성공적으로 전달했으나 no completion signal이므로 그냥 오류 처리해도 무방.
				return (SocketErrorCode)lastError;
			}
		}
	}

	SocketErrorCode CFastSocket::IssueSendTo(uint8_t* data, int count, AddrPort sendTo)
	{
		if (count <= 0)
			return SocketErrorCode_InvalidArgument;// 0 이하의 수신 자체는 무의미한데다 socket 오동작을 유발하므로

		//XXX_TEST13();

		m_sendBuffer.SetCount(count);
		// ikpil.choi 2016-11-07 : memcpy_s 로 변경, destSize(2번째 인자) 값이 항상 올바른 값이여야 합니다.
		// GetCount() 가 1 byte 용인지 어떻게 보장 할 것인가?
		//UnsafeFastMemcpy(m_sendBuffer.GetData(), data, count);
		memcpy_s(m_sendBuffer.GetData(), m_sendBuffer.GetCount(), data, count);

		WSABUF buf;
		buf.buf = (char*)m_sendBuffer.GetData();
		buf.len = count;

		ExtendSockAddr sendToA;
		ErrorInfo err;
#ifndef __ORBIS__
		int sendToALen = sizeof(sendToA);
		sendTo.ToNativeV6(sendToA);
#else
		int sendToALen = sizeof(sendToA.u.v4);
		if (!sendTo.ToNativeV4(sendToA, err))
		{
			return SocketErrorCode_AddressNotAvailable;
		}
#endif

		DWORD flags = 0;

		if (!AssureUnicast(sendTo))
			return SocketErrorCode_AccessError;


		while (1)
		{
			int ret = ::WSASendTo(m_socket,
				&buf, 1,
				NULL,
				flags,
				(SOCKADDR*)& sendToA, sendToALen,
				&m_sendOverlapped, NULL);

			// NOTE: WSASend, WSASendTo은 send, sendto의 리턴값과 의미가 다르다. MSDN 참고.
			if (ret == 0)
			{
				return SocketErrorCode_Ok;
			}

			uint32_t lastError = WSAGetLastError();
			switch (lastError)
			{
			case WSA_IO_PENDING:
				return SocketErrorCode_Ok;

			case SocketErrorCode_Intr:
				AtomicIncrement32(&g_intrErrorCount);
				if (StopIoRequested())
					return SocketErrorCode_Intr;
				break;

			default:
				// Linux의 EAGAIN은 Win32에서 WOULD_BLOCK으로 포함 되었으므로,
				// WOULD_BLOCK과 동일 하게 처리 해 주면 된다(따로 처리 해 줄 필요 없다).
				PostSocketWarning(lastError, _PNT("FS.PSW"));
				// NOTE: 10054, 10052 or WSAEMSGSIZE는 상대방에게 성공적으로 전달했으나 no completion signal이므로 그냥 오류 처리해도 무방.
				return (SocketErrorCode)lastError;
			}
		}
	}
#endif

	// 사용하는 함수들임. 지우지 말것
	// 테스트에서 씀
	SocketErrorCode CFastSocket::SendTo(uint8_t* data, int length, AddrPort sendTo, int* doneLength)
	{
		*doneLength = 0;

		if (!AssureUnicast(sendTo))
			return SocketErrorCode_AccessError;

		ExtendSockAddr sendToA;

		ErrorInfo errInfo;

#ifndef __ORBIS__
		int sendToALen = sizeof(sendToA);
		sendTo.ToNativeV6(sendToA);
#else
		int sendToALen = sizeof(sendToA.u.v4);
		if (!sendTo.ToNativeV4(sendToA, errInfo))
		{
			return SocketErrorCode_AddressNotAvailable;
		}
#endif

		int r;
	_Retry:
		r = ::sendto(m_socket,
			(const char*)data,
			length,
			0,
			(const struct sockaddr*)&sendToA,
			sendToALen);
		if (r < 0)
		{
			SocketErrorCode err = (SocketErrorCode)WSAGetLastError();
			if (IsIntrError(this, err))
				goto _Retry;
			return (SocketErrorCode)err;
		}

		*doneLength = r;
		return SocketErrorCode_Ok;
	}

	CFastSocket::CFastSocket()
	{
		SocketInitializer::GetSharedPtr(); // WSAStartup 유도

		m_socket = InvalidSocket;

		AtomicIncrement32(&g_TotalSocketCount);

		m_enableBroadcastOption = false;

		//m_ignoreNotSocketError = false;
		//m_restoredCount = 0;
		{
			CriticalSectionLock lock(m_socketClosed_CS, true);
			m_socketClosedOrClosing_CS_PROTECTED = 0;
		}

		m_verbose = false;
		m_stopIoRequested_USE_FUNCTION = IoState_False;
		m_ioPendingCount = 0;

		//m_sendCompletionEvent = NULL;
		//m_recvCompletionEvent = NULL;

		//m_iocpAssocCount_TTTEST = 0;

		// 대량의 클라를 일시에 파괴시 lookaside allocator free에서 에러가 나길래 fast heap 미사용!
		//		m_sendBuffer.UseFastHeap(false);
		//		m_recvBuffer.UseFastHeap(false);

		//		memset(&m_recvOverlapped, 0, sizeof(m_recvOverlapped));
		//		memset(&m_sendOverlapped, 0, sizeof(m_sendOverlapped));
		//		memset(&m_AcceptExOverlapped, 0, sizeof(m_AcceptExOverlapped));

		// 이 값은 overlapped recvfrom이 완료될 때 채워지므로 유효하게 유지해야 한다.
		memset(&m_recvedFrom, 0, sizeof(m_recvedFrom));
		m_recvedFromLen = 0;
#ifdef _WIN32
		m_recvFlags = 0;
#endif

		m_ttlToRestoreOnSendCompletion = -1;
		m_ttlMustBeRestoreOnSendCompletion = false;

		// 이것을 해 주어야 send or recv 실행때마다 realloc을 하지 않는다.
		// 잦은 realloc은 memory heap external frag를 일으켜서 결국 메모리 고갈로 이어진다.
		m_sendBuffer.SuspendShrink();
		m_recvBuffer.SuspendShrink();

		/*	//3003!!!혹시 resize가 문제?
		m_recvBuffer.SetCount(100 * 1024);
		m_recvBuffer.SetCount(0); */
	}

#if defined(_WIN32)
	/** async issue의 결과를 기다린다.
	아직 아무것도 완료되지 않았으면 null을 리턴한다.
	만약 완료 성공 또는 소켓 에러 등의 실패가 생기면 객체를 리턴하되 m_errorCode가 채워져 있다.
	수신 완료시 데이터를 가져오는 방법: GetRecvBuffer()를 호출하되 파라메터로 가져올 데이터 크기를 넣어줌 된다.*/
	bool CFastSocket::GetRecvOverlappedResult(bool waitUntilComplete, OverlappedResult& ret)
	{
		DWORD done;
		DWORD flags;

		// WSAGetOverlappedResult를 호출 전에 이걸 검증하는 것이 FASTER!
		if (!HasOverlappedIoCompleted(&m_recvOverlapped))
			return false;

		BOOL b = WSAGetOverlappedResult(m_socket, &m_recvOverlapped, &done, waitUntilComplete, &flags);

		if (!b && WSAGetLastError() == WSA_IO_INCOMPLETE)
			return false;

		//assert(!(b && WSAGetLastError()==WSA_IO_INCOMPLETE)); // 이런 경우가 없어야 하는데 테스트 로비 클라에서 이런 경우가 있는듯


		ret.m_retrievedFlags = flags;
		ret.m_completedDataLength = done;
		if (!b)
			ret.m_errorCode = (SocketErrorCode)WSAGetLastError();

		// 이제 처리했으니 청소해버린다.
		ret.m_receivedFrom.FromNative(m_recvedFrom);

		return true;
	}

	bool CFastSocket::GetSendOverlappedResult(bool waitUntilComplete, OverlappedResult& ret)
	{
		DWORD done;
		DWORD flags;

		// WSAGetOverlappedResult를 호출 전에 이걸 검증하는 것이 FASTER!
		if (!HasOverlappedIoCompleted(&m_sendOverlapped))
			return false;

		BOOL b = WSAGetOverlappedResult(m_socket, &m_sendOverlapped, &done, waitUntilComplete, &flags);

		if (!b)
		{
			uint32_t e = WSAGetLastError();
			if (e == WSA_IO_INCOMPLETE)
				return false;
		}
		if (b && !done && !flags) // 실험해본 결과 IssueSend 자체를 안한 상태에서 이 조건이 온다. 이때는 아직 안보내졌다고 뻥쳐야 한다.
		{
			return false;
		}
		ret.m_retrievedFlags = flags;
		ret.m_completedDataLength = done;
		if (!b)
			ret.m_errorCode = (SocketErrorCode)WSAGetLastError();

		return true;
	}
#endif

	void CFastSocket::Listen()
	{
		SocketErrorCode err = Socket_Listen(m_socket);
		if (err != SocketErrorCode_Ok)
		{
			// 리턴값 없는 함수이면, 이 함수는 사용자가 잘못 사용할 경우 throw를 해야 타당함.
			throw Exception(StringA::NewFormat("Listen failed! error=%d", err).GetString());
		}
	}


	Proud::AddrPort CFastSocket::GetSockName()
	{
		return Socket_GetSockName(m_socket);
	}

	Proud::AddrPort CFastSocket::GetPeerName()
	{
		ExtendSockAddr o;
#ifndef __ORBIS__
		int olen = sizeof(o);
#else
		int olen = sizeof(o.u.v4);
#endif
		if (::getpeername(m_socket, (sockaddr*)&o, (socklen_t *)&olen) != 0)
			return AddrPort::Unassigned;
		else
		{
			AddrPort ret;

			if (o.u.s.sa_family == AF_INET)
			{
				ret.FromNativeV4(o.u.v4);
			}
			else if (o.u.s.sa_family == AF_INET6)
			{
				ret.FromNativeV6(o.u.v6);
			}

			return ret;
		}
	}

	//	void Socket::SetSendCompletionEvent( Event* event )
	//	{
	//		m_sendCompletionEvent = event;
	//	}
	//
	//	void Socket::SetRecvCompletionEvent( Event* event )
	//	{
	//		m_recvCompletionEvent = event;
	//	}

	int CFastSocket::SetSocketReuseAddress(bool enable)
	{
		int reuse = enable ? 1 : 0;
		int r = ::setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));
		if (r != 0)
#if !defined(_WIN32)
			PostSocketWarning(errno, _PNT("FS.SSRA"));
#else
			PostSocketWarning(WSAGetLastError(), _PNT("FS.SSRA"));
#endif
		return r;
	}

	int CFastSocket::SetLingerOption(uint16_t const onoff, uint16_t const linger)
	{
		//--------------------------------------------------
		// l_onoff : linger 옵션을 끌것인지 킬 것인지 결정
		// l_linger : 기다리는 시간의 결정
		// l_onoff == 0 : 이 경우 l_linger의 영향을 받지 않는다. 소켓의 기본설정으로 소켓버퍼에 남아 있는 모든 데이터를 보낸다.
		// l_onoff > 0 이고 l_linger == 0 : close()는 바로 리턴을 하며, 소켓버퍼에 아직 남아있는 데이터는 버려 버린다. hard 혹은 abortive 종료라고 부른다.
		// l_onoff > 0 이고 l_linger > 0 : 버퍼에 남아있는 데이터를 모두 보내는 우아한 연결 종료를 행한다. 이때 close()에서는 l_linger에 지정된 시간만큼 블럭상태에서 대기한다.

		struct linger ling;
		ling.l_onoff = onoff;
		ling.l_linger = linger;

		int r = ::setsockopt(m_socket, SOL_SOCKET, SO_LINGER, (const char*)&ling, sizeof(ling));

		if (r != 0)
		{
#if !defined(_WIN32)
			PostSocketWarning(errno, _PNT("FS.SLO"));
#else
			PostSocketWarning(WSAGetLastError(), _PNT("FS.SLO"));
#endif
		}
		return r;
	}


	int CFastSocket::SetSendBufferSize(int size)
	{
		int v = size;
		int r = ::setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (const char*)& v, sizeof(v));
		if (r != 0)
#if !defined(_WIN32)
			PostSocketWarning(errno, _PNT("FS.SSBS"));
#else
			PostSocketWarning(WSAGetLastError(), _PNT("FS.SSBS"));
#endif
		return r;
	}

	bool CFastSocket::IsSocketEmpty()
	{
		return IsValidSocket(m_socket);
	}

	int CFastSocket::SetRecvBufferSize(int size)
	{
		int v = size;
		int ret = ::setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (const char*)& v, sizeof(v));
		if (ret != 0)
			PostSocketWarning(WSAGetLastError(), _PNT("FS.SRBS"));
		return ret;
	}

	void CFastSocket::CloseSocketOnly()
	{
//		TEST_CaptureCallStack();

#ifdef TEST_CloseSocketOnly_CallStack
		printf("[ProudNetClient] active close at : %s\n", __FUNCTION__);
#endif

#if defined(_WIN32)
		XXXHeapChk();
#endif

		CriticalSectionLock lock(m_socketClosed_CS, false);
		lock.UnsafeLock(); // 드물지만 파괴자에서 잘못된 CS를 접근하는 경우 ShowUserMisuseError는 쐣

		if (AtomicCompareAndSwap32(0, 1, &m_socketClosedOrClosing_CS_PROTECTED) == 0)
		{
			AssertCloseSocketWillReturnImmediately(m_socket);
#if !defined(_WIN32)
			::close(m_socket);
#else
			closesocket(m_socket); // socket handle 값은 그대로 둔다.
			PNTRACE(LogCategory_System, "%s(%d)\n", __FUNCTION__, m_socket);
#endif
		}
	}

	bool CFastSocket::GetVerboseFlag()
	{
		return m_verbose;
	}

	void CFastSocket::SetVerboseFlag(bool val)
	{
		m_verbose = val;
	}

	CFastSocket::~CFastSocket()
	{
		CloseSocketOnly();
		AtomicDecrement32(&g_TotalSocketCount);
	}

	SocketErrorCode CFastSocket::Shutdown(ShutdownFlag how)
	{
//		TEST_CaptureCallStack();

#ifdef TEST_CloseSocketOnly_CallStack
		printf("[ProudNetClient] active close at : %s\n", __FUNCTION__);
#endif


		if (::shutdown(m_socket, (int)how) == 0) // TCP,UDP 모두 유효
			return SocketErrorCode_Ok;
		else
		{
			return (SocketErrorCode)WSAGetLastError();
		}
	}

#if defined(_WIN32)
	void CFastSocket::DirectBindAcceptEx()
	{
		UUID GuidAcceptEx = WSAID_ACCEPTEX;
		UUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
		DWORD dwBytes;

		//----------------------------------------
		// Load the AcceptEx function into memory using WSAIoctl.
		// The WSAIoctl function is an extension of the ioctlsocket()
		// function that can use overlapped I/O. The function's 3rd
		// through 6th parameters are input and output buffers where
		// we pass the pointer to our AcceptEx function. This is used
		// so that we can call the AcceptEx function directly, rather
		// than refer to the Mswsock.lib library.
		WSAIoctl(m_socket,
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&GuidAcceptEx,
			sizeof(GuidAcceptEx),
			(void*)&lpfnAcceptEx,
			sizeof(lpfnAcceptEx),
			&dwBytes,
			NULL,
			NULL);

		////----------------------------------------
		//// Load the AcceptEx function into memory using WSAIoctl.
		//// The WSAIoctl function is an extension of the ioctlsocket()
		//// function that can use overlapped I/O. The function's 3rd
		//// through 6th parameters are input and output buffers where
		//// we pass the pointer to our AcceptEx function. This is used
		//// so that we can call the AcceptEx function directly, rather
		//// than refer to the Mswsock.lib library.
		//WSAIoctl(m_socket,
		//	SIO_GET_EXTENSION_FUNCTION_POINTER,
		//	&GuidGetAcceptExSockAddrs,
		//	sizeof(GuidGetAcceptExSockAddrs),
		//	&lpfnGetAcceptExSockAddrs,
		//	sizeof(lpfnGetAcceptExSockAddrs),
		//	&dwBytes,
		//	NULL,
		//	NULL);
	}

	// overlapped accept를 issue한다.
	SocketErrorCode CFastSocket::AcceptEx(const shared_ptr<CFastSocket>& openedSocket)
	{
		if (lpfnAcceptEx == NULL)
			DirectBindAcceptEx();

		//NTTNTRACE("*** AcceptEx ***\n");
		m_recvBuffer.SetCount(AddrLengthInTdi * 2);
		DWORD bytesReceived = 0;
	retry:
		BOOL ret = lpfnAcceptEx(
			m_socket,
			openedSocket->m_socket,
			m_recvBuffer.GetData(),
			0,
			AddrLengthInTdi,
			AddrLengthInTdi,
			&bytesReceived,
			&m_recvOverlapped);

		if (ret == false)
		{
			DWORD err = WSAGetLastError();

			if (CFastSocket::IsPendingErrorCode((SocketErrorCode)err))
				return SocketErrorCode_Ok;

			if (IsIntrError(this, (SocketErrorCode)err))
				goto retry;

			PostSocketWarning(err, _PNT("FS.AE"));
			return (SocketErrorCode)err;
		}

		return SocketErrorCode_Ok;
	}

	void CFastSocket::DirectBindConnectEx()
	{
		UUID GuidConnectEx = WSAID_CONNECTEX;
		DWORD dwBytes = 0;

		WSAIoctl(m_socket,
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&GuidConnectEx,
			sizeof(GuidConnectEx),
			(void*)&lpfnConnectEx,
			sizeof(lpfnConnectEx),
			&dwBytes,
			NULL,
			NULL);
	}

	// 주의: 이 함수는 SocketErrorCode_AddressNotAvailable를 리턴하는 경우도 있다. 자세한건 아래 주석을 참고할 것.
	Proud::SocketErrorCode CFastSocket::ConnectEx(AddrPort addrPort)
	{

		if (lpfnConnectEx == NULL)
			DirectBindConnectEx();

		//이미 바인드 되어있는 상태여야 한다.
		if (!BindAppliedSocket())
		{
			NTTNTRACE("WARNING: ConnectEX Socket Not Bind! Auto Bind!");
			//			m_dg->OnSocketWarning(this,_PNT("WARNING: ConnectEX Socket Not Bind! Auto Bind!"));
			Bind();
		}

		ExtendSockAddr sockAddr;
		memset(&sockAddr, 0, sizeof(sockAddr));
		ErrorInfo errorInfo;
		switch (m_addrFamily)
		{
		case AF_INET:
			if(!addrPort.ToNativeV4(sockAddr, errorInfo))
			{
				// socket은 ipv4인데 파라메터 주소는 ipv6라면 안되지!
				return SocketErrorCode_AddressNotAvailable;
			}
			break;

		case AF_INET6:
			addrPort.ToNativeV6(sockAddr);
			break;
		}

	retry:
#ifndef __ORBIS__
		int sockAddrLen = sizeof(ExtendSockAddr);
#else
		int sockAddrLen = sizeof(sockaddr_in);
#endif

		//associate를 먼저해야 하겠다.
		BOOL ret = lpfnConnectEx(
			m_socket,
			(struct sockaddr*)&sockAddr,
			sockAddrLen,
			NULL,
			0,
			NULL,
			&m_sendOverlapped);

		if (!ret)
		{
			SocketErrorCode code = (SocketErrorCode)WSAGetLastError();

			if (code != WSA_IO_PENDING)
			{
				return code;
			}

			// NOTE: windows에서는 WSAEINTR 에러가 뜨는 경우가 드물게 있는데(가령 드라이버 버그)
			// 이러한 경우를 위한 방탄 처리입니다.
			if (IsIntrError(this, code))
				goto retry;
		}

		return SocketErrorCode_Ok;
	}
#endif

	// overlapped or non-block connect에 대한 이벤트가 왔을때 마무리 처리를 한다.
	// 마무리한 후 결과 값을 리턴한다: 연결 성공했음? 실패했음?
	SocketErrorCode CFastSocket::ConnectExComplete()
	{
#if defined (_WIN32)
		// Win32 ConnectEx 호출에 대한 마무리
		if (setsockopt(m_socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0) < 0)
			return (SocketErrorCode)WSAGetLastError();

		return SocketErrorCode_Ok;
#else
		// select(),poll()은 플랫폼마다 사용법과 behavior가 서로 다르고, 일부 플랫폼은 그나마 제대로 얻지도 못한다.
		// 따라서 zero-send를 통해 연결이 진행중인지 연결이 성사됐는지 체크함으로 일관화하자.
		return Tcp_Send0ByteForConnectivityTest();
#endif
	}

	// bind가 호출된 바 있으면, 즉 포트값이 할당되어 있으면, true.
	bool CFastSocket::BindAppliedSocket()
	{
		AddrPort port = GetSockName();

		return (port.m_port >= 1 && port.m_port <= 65534);
	}

	// TcpListenSocket으로부터 accepted socket을 가져온다.
	// NOTE: AcceptEx는 이미 만들어진 socket을 accepted socket으로 변신시키며,
	void CFastSocket::FinalizeAcceptEx(const shared_ptr<CFastSocket>& TcpListenSocket, AddrPort& localAddr, AddrPort &remoteAddr)
	{
#if defined (_WIN32)
		ExtendSockAddr la, ra;

#ifndef __ORBIS__
		int llen = sizeof(la), rlen = sizeof(ra);
#else
		int llen = sizeof(la.u.v4), rlen = sizeof(ra.u.v4);
#endif
		assert(TcpListenSocket->m_recvBuffer.GetCount() > 0);

		GetAcceptExSockaddrs(TcpListenSocket->m_recvBuffer.GetData(),
			0,
			AddrLengthInTdi,
			AddrLengthInTdi,
			(sockaddr**)&la,
			&llen,
			(sockaddr**)&ra,
			&rlen);

		localAddr.FromNative(la);
		remoteAddr.FromNative(ra);

		if (setsockopt(m_socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
			(char *)&TcpListenSocket->m_socket, sizeof(TcpListenSocket->m_socket)) == SOCKET_ERROR)
		{
			uint32_t e = WSAGetLastError();
			e = 0;
		}
#endif
		// 소켓 주소를 가져온다.
		// (윈도의 경우: GetAcceptExSockaddrs가 잘 작동 안하지만, 속도 희생해서라도, 어쩔 수 없다.)
		// 만약 위 함수가 실패한다면 아래 값들은 none이 세팅됨.
		remoteAddr = GetPeerName();
		localAddr = GetSockName();
	}

#ifdef USE_DisableUdpConnResetReport
	/* 본 기능은 더 이상 쓰지 않는다.
	수신자 사이의 라우터가 ICMP port unreachable 발생시
	*/
	/** UDP socket으로 sendto를 했는데 상대 IP는 유효하나 port가 없으면 ICMP port unreachable이 도착하며
	localhost에서 WSAECONNRESET이 도착한다. 여기까지는 그렇다 치자. WSAECONNRESET 도착 후 송수신이 되지
	못하는 문제가 발견됐다! 이것 때문에 UDP SOCKET이 ICMP 거시기를 받으면 멀쩡한 타 PEER와의 통신까지
	먹통이 되는 문제가 있다. 따라서 아예 ICMP 오류 리포팅 자체를 막아버리자. */
	void CFastSocket::DisableUdpConnResetReport()
	{
		uint32_t dwBytesReturned = 0;
		BOOL bNewBehavior = FALSE;
		uint32_t status;

		// disable  new behavior using
		// IOCTL: SIO_UDP_CONNRESET
		status = WSAIoctl(m_socket, SIO_UDP_CONNRESET,
			&bNewBehavior, sizeof(bNewBehavior),
			NULL, 0, &dwBytesReturned,
			NULL, NULL);

		if (SOCKET_ERROR == status)
		{
			uint32_t dwErr = WSAGetLastError();
			if (IsWouldBlockError(dwErr))
			{
				//// nothing to do
				//return(FALSE);
			}
			else
			{
				//printf("WSAIoctl(SIO_UDP_CONNRESET) Error: %d\n", dwErr);
				//return(FALSE);
			}
		}
	}
#endif // USE_DisableUdpConnResetReport

	void CFastSocket::SetBlockingMode(bool isBlockingMode)
	{
		SocketErrorCode error = Socket_SetBlocking(m_socket, isBlockingMode);
		if (error != SocketErrorCode_Ok)
		{
			assert(0); // 있어서는 안되는 상황
			PostSocketWarning(error, _PNT("set blocking mode"));
		}
	}

#ifndef _WIN32
	bool CFastSocket::GetBlockingMode()
	{
		bool ret;
		SocketErrorCode error = Socket_GetBlocking(m_socket, &ret);
		if (error != SocketErrorCode_Ok)
		{
			assert(0); // 있어서는 안되는 상황
			PostSocketWarning(error, _PNT("get blocking mode"));
		}
		return ret;
	}
#endif

	uint8_t* CFastSocket::GetRecvBufferPtr()
	{
		return m_recvBuffer.GetData();
	}

	//	BYTE* Socket::GetSendBufferPtr()
	//	{
	//		return m_sendBuffer.GetData();
	//	}

	SocketErrorCode CFastSocket::EnableBroadcastOption(bool enable)
	{
		bool v = enable ? 1 : 0;
		int r = ::setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, (const char*)& v, sizeof(v));
		if (r != 0)
		{
			uint32_t err = WSAGetLastError();
			PostSocketWarning(err, _PNT("FS.EBO"));
			return (SocketErrorCode)err;
		}
		m_enableBroadcastOption = enable;
		return SocketErrorCode_Ok;
	}


	SocketErrorCode CFastSocket::SetTtl(int ttl)
	{
		int val; int vallen = sizeof(val);
		int ret;

		if (GetTtl(val) != SocketErrorCode_Ok)
		{
			NTTNTRACE("This socket doesn't support TTL change!");
			//			m_dg->OnSocketWarning(this, _PNT("This socket doesn't support TTL change!"));
			return (SocketErrorCode)WSAGetLastError();
		}
		else
		{
			val = ttl;
			if ((ret = ::setsockopt(m_socket, IPPROTO_IPV6, IPV6_HOPLIMIT, (const char*)&val, vallen)) != 0)
			{
				if ((ret = ::setsockopt(m_socket, IPPROTO_IP, IP_TTL, (const char*)&val, vallen)) != 0)
				{
					NTTNTRACE("Cannot change TTL!");
					//				m_dg->OnSocketWarning(this, _PNT("Cannot change TTL!"));
					return (SocketErrorCode)WSAGetLastError();
				}
			}
		}
		return SocketErrorCode_Ok;
	}

	bool CFastSocket::IsUdpStopErrorCode(SocketErrorCode code)
	{
#if !defined(_WIN32)
		return code == SocketErrorCode_NotSocket;
#else
		// 주의: SocketErrorCode_ConnectResetByRemote 는 ICMP connreset에서 오는건데 이건 계속 UDP 통신을 할 수 있는 조건이므로 false이어야 함!
		return code == SocketErrorCode_NotSocket || code == SocketErrorCode_OperationAborted;
#endif
	}

	bool CFastSocket::IsTcpStopErrorCode(SocketErrorCode code)
	{
		return code != 0;
	}


	// http://linux.die.net/man/3/errno 에 의하면, EAGAIN과 EWOULDBLOCK은 같은 의미지만,
	// 일부 유닉스는 이 값이 서로 다르다.
	// 따라서 모두 동일하게 처리를 해주어야 한다. 이 함수는 그것을 위한 helper임.
	bool CFastSocket::IsWouldBlockError(SocketErrorCode code)
	{
#ifdef _WIN32
		// EAGAIN = EWOULDBLOCK이지만 일부 OS에서는 의미는 같지만 값이 다르다. 그래서 꼭 체크해 주어야 한다.
		return code == SocketErrorCode_WouldBlock || code == SocketErrorCode_InProgress;
#else
		// *주의* ENOTCONN은 여기서 핸들링하지 말자.
		// EAGAIN = EWOULDBLOCK이지만 일부 OS에서는 의미는 같지만 값이 다르다. 그래서 꼭 체크해 주어야 한다. (POSIX 1-2001)
		return code == SocketErrorCode_WouldBlock || code == SocketErrorCode_Again || code == SocketErrorCode_InProgress;
#endif
	}

#ifdef _WIN32
	bool CFastSocket::IsPendingErrorCode(SocketErrorCode code)
	{
		return code == SocketErrorCode_WouldBlock || code == SocketErrorCode_IoPending || code == SocketErrorCode_InProgress;
	}
#endif

#if defined(_WIN32)
	Proud::SocketErrorCode CFastSocket::AllowPacketFragmentation(bool allow)
	{
		uint32_t val = allow ? 0 : 1;

		if (::setsockopt(m_socket, IPPROTO_IP, IP_DONTFRAGMENT, (char*)&val, sizeof(val)) != 0)
			return (SocketErrorCode)WSAGetLastError();
		else
			return SocketErrorCode_Ok;
	}
#endif

	bool CFastSocket::IsClosed()
	{
		// 드문 케이스 땜시 부득이
		if (!m_socketClosed_CS.IsValid())
			return true;

		CriticalSectionLock lock(m_socketClosed_CS, true);
		return m_socketClosedOrClosing_CS_PROTECTED != 0;
	}

	void CFastSocket::EnableNagleAlgorithm(bool enable)
	{
		int tcpNoDelay = enable ? 0 : 1;

		int ret = ::setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&tcpNoDelay, sizeof(tcpNoDelay));
		int error = WSAGetLastError();
		if (ret < 0)
		{
			// 소켓 수준에서 유효하지 않은 소켓이 종료 되었습니다.
			if (error == EINVAL)
			{
				return;
			}
			assert(0);
		}
	}

	bool CFastSocket::AssureUnicast(AddrPort sendTo)
	{
		if (sendTo.IsUnicastEndpoint() == false)
		{
			NTTNTRACE("Sending to 255.255.255.255 or 0.0.0.0 is not permitted!");
		}
		else if (/*sendTo.m_port == 0xffff || */sendTo.m_port == 0) // OS에 의해 차단되지 않으려면 필수
		{
			NTTNTRACE("Sending to prohibited port is not permitted!");
			//			m_dg->OnSocketWarning(this,_PNT("Sending to prohibited port is not permitted!"));
			return false;
		}

		return true;
	}

	int CFastSocket::GetSendBufferSize(int *outsize)
	{
		int vlen = sizeof(int);
		int r = ::getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char*)outsize, (socklen_t *)&vlen);
#if !defined(_WIN32)
		if (r < 0)
			PostSocketWarning(errno, _PNT("FS.GSBS"));
#else
		if (r != 0)
			PostSocketWarning(WSAGetLastError(), _PNT("FS.GSBS"));
#endif
		return r;
	}

	int CFastSocket::GetRecvBufferSize(int *outsize)
	{
		int vlen = sizeof(int);
		int r = ::getsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char*)outsize, (socklen_t *)&vlen);

		if (r < 0)
			PostSocketWarning(WSAGetLastError(), _PNT("FS.GRBS"));
		return r;
	}

#if defined(SO_SNDLOWAT) && !defined(_WIN32)
	void CFastSocket::SetSendLowWatermark(int size)
	{
		int v = size;
		// NOTE: SO_SNDLOWAT는 win32에 define되어 있으나, 사용할 수 없는 파라메터이다.
		// https://msdn.microsoft.com/en-us/library/windows/desktop/ms740532(v=vs.85).aspx
		int r = setsockopt(m_socket, SOL_SOCKET, SO_SNDLOWAT, (const void*)&v, (socklen_t)sizeof(v));
		if (r < 0)
			PostSocketWarning(errno, _PNT("FS.SSLWM"));
	}
#endif

#ifdef SO_NOSIGPIPE
	// 소켓 Close시 SIG_PIPE 에러가 안나도록 해준다.
	// https://developer.apple.com/library/ios/documentation/NetworkingInternetWeb/Conceptual/NetworkingOverview/CommonPitfalls/CommonPitfalls.html
	// iOS 뿐만 아니라 SO_NOSIGPIPE를 쓸 수 있는 모든 OS에 대해. PS4 등.
	void CFastSocket::SetNoSigPipe(bool enable)
	{
		int v = enable ? 1 : 0;
		int r = setsockopt(m_socket, SOL_SOCKET, SO_NOSIGPIPE, (void *)&v, sizeof(int));
		if (r < 0)
			PostSocketWarning(errno, _PNT("FS.SNSP"));
	}
#endif

	SocketErrorCode CFastSocket::GetTtl(int& outTtl)
	{
		int vallen = sizeof(int);
		int ret;

		// 분명 V6 소켓인데 아래와 같은 코드가 작동하질 않고 IPv4 TTL 을 정상적으로 가져온다 ㄱ-
		// 따라서 HopLimit 을 가져 오도록 시도 후에 TTL 로 변경 하고 그마저도 실패하면 완전 실패 처리 한다.
		if ((ret = ::getsockopt(m_socket, IPPROTO_IPV6, IPV6_HOPLIMIT, (char*)&outTtl, (socklen_t *)&vallen)) != 0)
		{
			int vallen_ = sizeof(int);
			if ((ret = ::getsockopt(m_socket, IPPROTO_IP, IP_TTL, (char*)&outTtl, (socklen_t *)&vallen_)) != 0)
			{
				NTTNTRACE("This socket doesn't support TTL change!");
				//			m_dg->OnSocketWarning(this, _PNT("This socket doesn't support TTL change!"));
				return (SocketErrorCode)WSAGetLastError();
			}
		}

		return SocketErrorCode_Ok;
	}

	void CFastSocket::RestoreTtlOnCompletion()
	{
		if (m_ttlMustBeRestoreOnSendCompletion)
		{
			m_ttlMustBeRestoreOnSendCompletion = false;
			SetTtl(m_ttlToRestoreOnSendCompletion);
		}
	}

	/* 0 byte send를 시도한다. 0 byte send이므로 항상 즉시 리턴한다.
	TCP socket의 경우 TCP 써킷이 살아있는지 검사하기 위해 이것을 사용할 수 있다.

	Q: getsockopt(SO_ERROR)로 얻을 수 있지 않나요?
	A: 과거 마지막 호출했던 socket function의 리턴값을 줍니다. 즉 현재 시점이 아닌 과거 시점의 상태입니다.
	0 byte send는 현재의 TCP 써킷 생존 여부를 알 수 있습니다.

	리턴값:
	Ok: 이미 연결되어 있음
	ENOTCONN: 아직 연결중
	기타: 과거 연결되어 있었으나 지금은 연결 해제됨
	*/
	SocketErrorCode CFastSocket::Tcp_Send0ByteForConnectivityTest()
	{
		char nil[1];
		int flags = 0;

#if !defined(_WIN32)
		// Requests not to send SIGPIPE on errors on stream oriented sockets when the other end breaks the connection. The EPIPE error is still returned.
		// http://linux.die.net/man/2/sendmsg
		flags |= MSG_NOSIGNAL;
#endif

		int x;
	L1:
		x = ::send(m_socket, nil, 0, flags); // non-block이므로 그냥 넘어갈 것임
		if (x == 0)
			return SocketErrorCode_Ok;

#ifndef _WIN32
		// unix에서는 would block이 와도 성공 케이스다.
		if (CFastSocket::IsWouldBlockError((SocketErrorCode)errno))
		{
			return SocketErrorCode_Ok;
		}
#endif

		else
		{
			SocketErrorCode e = (SocketErrorCode)WSAGetLastError();
			// EINTR은 driver에 의해 발생할 수도 있으므로 이러한 경우 재시도해야.
			if (IsIntrError(this, e))
				goto L1;

			return e;
		}
	}

	void CFastSocket::RequestStopIo_CloseOnWin32()
	{
		m_stopIoRequested_USE_FUNCTION = IoState_True;

		/* overlapped io를 건 것들을 모두 중단시켜서, completion with failure가 뜨게 만들어야 한다.
		안타깝게도 CancelIoEx는 이를 하지 못한다.
		다행히 win32에서는 close socket handle value는 한동안 재사용되지 않음이 실험 결과 확인됨.
		(msdn에서는 재사용의 위험이 있으니 마지막 스레드에서 close를 하라고 하지만)
		따라서 close를 해버리자.

		Linux에서는 핸들값이 즉시 재사용된다. RMI 3003 문제가 그거때문이었고.
		따라서 최후의 스레드에서 소켓을 닫아야 하므로, 즉 정말 아무도 안 쓰는게 확인되면 그제서야
		close를 해야 하므로, 여기서는 위 변수만 변경하고 다른건 하지 말아야 한다.

		안타깝게도 shutdown은 UDP의 async i/o를 중단시키지 못한다. close를 어쩔 수 없지만 해야 한다.
		linux에서는 그런거 체크할 필요 없다. 커널이 메모리 액세스를 걸고 있는 overlapped 방식이 아니니까.

		추후에, cancel io를 하는 방법을 찾게 되면 여기를 고치도록 하자.
		*/

		// Linux 서버에 Windows 클라이언트가 접속 시 OnClientLeave 콜백이 호출되지 않는 현상이 있어서 closesocket 호출에서 shutdown 호출로 수정
		Shutdown(ShutdownFlag_Both);

#ifdef _WIN32
		// Windows UDP의 경우 shutdown만 하면 클라이언트 disconnect가 정상적으로 되지 않고 무한 루프를 도는 현상이 있어서 추가
		// Windows TCP의 경우도 shutdown 후 closesocket을 하여도 상관없다.
		CloseSocketOnly();
#endif

//		// 이 함수는 최초 호출할 때 빼고는 immutable이므로, if구문 안에서 한다.
//		// 일정 시간마다 DoGarbageCollect에서 이 함수를 지속 호출하므로 최초 1회만 해도 괜찮다.
//		CancelEveryIo();
//		// NOTE: linux에서는 아래 associatedIoQueue.RemoveSocket을 수행.
	}

#ifdef _WIN32
	FastSocketSelectContext::FastSocketSelectContext()
	{
		FD_ZERO(&m_readSocketList);
		FD_ZERO(&m_writeSocketList);
		FD_ZERO(&m_exceptionSocketList);
	}

	void FastSocketSelectContext::AddWriteWaiter(CFastSocket& socket)
	{
		FD_SET(socket.m_socket, &m_writeSocketList);
	}

	void FastSocketSelectContext::AddExceptionWaiter(CFastSocket& socket)
	{
		FD_SET(socket.m_socket, &m_exceptionSocketList);
	}

	void FastSocketSelectContext::Wait(uint32_t miliSec)
	{
		timeval* pTimeout = 0;
		timeval timeOut;
		if (miliSec != INFINITE)
		{
			timeOut.tv_sec = miliSec / 1000;
			timeOut.tv_usec = (miliSec % 1000) * 1000;
			pTimeout = &timeOut;
		}
		::select(0, &m_readSocketList, &m_writeSocketList, &m_exceptionSocketList, pTimeout);
	}


	// 소켓 연결 결과를 얻는다.
	bool FastSocketSelectContext::GetConnectResult(CFastSocket& socket, SocketErrorCode& outCode)
	{
		if (FD_ISSET(socket.m_socket, &m_writeSocketList) != 0)
		{
			outCode = SocketErrorCode_Ok;
			return true;
		}
		if (FD_ISSET(socket.m_socket, &m_exceptionSocketList) != 0)
		{
			int outCode0 = 0;
			int outCode0Len = sizeof(outCode0);
			int ret = getsockopt(socket.m_socket, SOL_SOCKET, SO_ERROR, (char*)&outCode0, (socklen_t*)&outCode0Len);
			// getsockopt 가 실패하면 연결실패임.
			if (ret >= 0 && outCode == 0)
			{
				outCode = (SocketErrorCode)outCode0;
				return true;
			}
		}
		return false;

	}
#endif

	SocketInitializer::SocketInitializer()
	{
#if defined(_WIN32)
		Socket_StartUp();
#endif
	}

	SocketInitializer::~SocketInitializer()
	{
#if defined(_WIN32)
		Socket_CleanUp();
#endif
	}
}
