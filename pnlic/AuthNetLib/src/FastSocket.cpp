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
#include "../include/sysutil.h"
#include "../include/atomic.h"
#include "FastSocket.h"
#include "SocketUtil.h"
#include "IntraTracer.h"
#include "FakeWinsock.h"
#include "ReportError.h"
//#include "../UtilSrc/DebugUtil/NetworkAnalyze.h"

#define _COUNTOF(array) (sizeof(array)/sizeof(array[0]))
//#define HasAsyncIoIssued(lpOverlapped) (((DWORD)(lpOverlapped)->Internal) != 0)


namespace Proud
{
	PROUDNET_VOLATILE_ALIGN int32_t g_msgSizeErrorCount = 0;		// WSAEMSGSIZE 에러난 횟수
	PROUDNET_VOLATILE_ALIGN int32_t g_intrErrorCount = 0;         // WSAEINTR 에러난 횟수
	PROUDNET_VOLATILE_ALIGN int32_t g_netResetErrorCount = 0;		// WSAENETRESET(10052) 에러난 횟수
	PROUDNET_VOLATILE_ALIGN int32_t g_connResetErrorCount = 0;	// WSAECONNRESET(10054) 에러난 횟수

#ifdef _WIN32
	const int AddrLengthInTdi = sizeof(sockaddr_in) + 16;
#endif

	int send_segmented_data(SOCKET socket, CFragmentedBuffer& sendBuffer, unsigned int flags);
	int sendto_segmented_data(SOCKET socket, CFragmentedBuffer& sendBuffer, unsigned int flags, const sockaddr* sendTo, int sendToLen);


#if defined(_WIN32)    
	PROUDNET_VOLATILE_ALIGN LPFN_ACCEPTEX CFastSocket::lpfnAcceptEx = NULL;
	PROUDNET_VOLATILE_ALIGN LPFN_CONNECTEX CFastSocket::lpfnConnectEx = NULL;

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

	inline bool IsIntrError(CFastSocket* fastsocket, SocketErrorCode e)
	{
		assert(fastsocket != NULL);
		bool ret = (e == SocketErrorCode_Intr && fastsocket->IsClosedOrClosing() == false);
		if (ret)
			AtomicIncrement32(&g_intrErrorCount);
		return ret;
	}

	void CFastSocket::Restore(bool isTcp)
	{
		CriticalSectionLock lock(m_socketClosed_CS, true);
		if (!m_socketClosedOrClosing_CS_PROTECTED)
			ThrowInvalidArgumentException();

		if (isTcp)
			m_socket = socket(AF_INET, SOCK_STREAM, 0);
		else
#if !defined(_WIN32)
			m_socket = socket(AF_INET, SOCK_DGRAM, 0);
#else
			m_socket = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
#endif

		if (!IsValidSocket(m_socket))
		{
			throw Exception("Failed to Create the Socket: %d", WSAGetLastError());
		}

		m_socketClosedOrClosing_CS_PROTECTED = 0; // 이게 아래로 와야!
		//m_restoredCount++;
	}

	CFastSocket::CFastSocket(SOCKET s)
	{
		m_enableBroadcastOption = false;

		m_socket = s;
		InitOthers();
	}

	CFastSocket::CFastSocket(SocketType type)
	{
		m_enableBroadcastOption = false;

		InitOthers();

		switch (type)
		{
		case SocketType_Tcp:
#if defined (_WIN32)
			m_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
#else
			m_socket = socket(AF_INET, SOCK_STREAM, 0);
#endif
			break;
		case SocketType_Udp:
#if defined (_WIN32)
			m_socket = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
#else
			m_socket = socket(AF_INET, SOCK_DGRAM, 0);
#endif
			break;
		case SocketType_Raw:
#if defined (_WIN32)
			m_socket = WSASocket(AF_INET, SOCK_RAW, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
#else
			m_socket = socket(AF_INET, SOCK_RAW, 0);
#endif
			break;
		default:
			ShowUserMisuseError(_PNT("Invalid Parameter in Socket ctor!"));
			break;
		}

		if (!IsValidSocket(m_socket))
		{
			throw Exception("Failed to Create the Socket: %d", WSAGetLastError());
		}

		// <=== 이 기능은 사용하지 말자. 무슨 부작용이 있을지 모르니까!!!
		// 어차피 MTU 커버리지가 낮은 라우터는 이 기능을 사용하더라도 문제 해결에 도움을 주지 못한다. 따라서 불필요!!

		// 		/* don't frag를 끈다. 즉 frag를 허용한다. 				
		// 		allow frag를 해두어야 ICMP까지도 다 막아버린 과잉진압 방화벽에 대해서 
		// 		path MTU discovery 과정에서 blackhole connection이 발생하지 않을 것이다.
		// 		확인된 바는 없고 추정일 뿐이지만 말이다. */
		// 		AllowPacketFragmentation(true);

	}

	SocketErrorCode CFastSocket::Bind()
	{
		AddrPort addr;
		addr.m_binaryAddress = 0; // any addr
		addr.m_port = 0; // any port
		return Bind(addr);
	}

	SocketErrorCode CFastSocket::Bind(int port)
	{
		AddrPort addr;
		addr.m_binaryAddress = 0; // any addr
		addr.m_port = port;
		return Bind(addr);
	}

	SocketErrorCode CFastSocket::Bind(const PNTCHAR* hostName, int port)
	{
		AddrPort addr;

		if (hostName && Tstrlen(hostName) > 0)
		{
			addr.m_binaryAddress = DnsForwardLookup(hostName);
		}
		addr.m_port = port;

		return Bind(addr);
	}

	SocketErrorCode CFastSocket::Bind(const AddrPort &localAddr)
	{
		m_bindAddress = localAddr; // bind가 성공하건 안하건 일단 세팅. 나중에 재시도할 때 사용되므로.

		int e = BindSocketIpv4(m_socket, localAddr.m_binaryAddress, localAddr.m_port);
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
				NTTNTRACE(StringT2A(msg));
				//				m_dg->OnSocketWarning(this, msg);
			}
		}
	}

	CFastSocket* CFastSocket::Accept(SocketErrorCode& errCode)
	{
		SOCKET newSocket;

	_Retry:
		newSocket = ::accept(m_socket, 0, 0);

		if (!IsValidSocket(newSocket))
		{
			errCode = (SocketErrorCode)WSAGetLastError();
			if (IsIntrError(this, errCode))
				goto _Retry;
			return NULL;
		}
		else
		{
			return new CFastSocket(newSocket/*, m_dg*/);
		}
	}

	SocketErrorCode CFastSocket::Connect(const String &hostAddressName, int hostPort)
	{
		while (true)
		{
			SocketErrorCode err = Socket_Connect(m_socket, hostAddressName, hostPort);
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

		m_recvedFromLen = sizeof(m_recvedFrom);

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
			if (IsClosedOrClosing() == false)
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

		sockaddr_in sendToA;

		socklen_t sendToALen = sizeof(sendToA);
		sendTo.ToNative(sendToA);

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

			if (err != EWOULDBLOCK)
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
			if (ret >= 0)
				return SocketErrorCode_Ok;

			uint32_t lastError = WSAGetLastError();
			switch (lastError)
			{
			case WSA_IO_PENDING:
				return SocketErrorCode_Ok;
			case SocketErrorCode_Intr:
				AtomicIncrement32(&g_intrErrorCount);
				if (IsClosedOrClosing())
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

		int retryCount = 0;
		while (1)
		{
			m_recvedFromLen = sizeof(m_recvedFrom);

			int ret = ::WSARecvFrom(m_socket,
				&buf, 1,
				NULL,
				&m_recvFlags,
				(SOCKADDR*)& m_recvedFrom, &m_recvedFromLen,
				&m_recvOverlapped, NULL);

			if (ret >= 0)
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
				if (IsClosedOrClosing())
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

				if(IsClosedOrClosing())
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
		UnsafeFastMemcpy(m_sendBuffer.GetData(), data, count);

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

			// 
			if (ret >= 0)
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
				if (IsClosedOrClosing())
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

		// 		m_sendBuffer.SetCount(count);
		// 		memcpy(m_sendBuffer.GetData(), data, count);

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

			if (ret >= 0)
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
				if (IsClosedOrClosing())
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
	SocketErrorCode CFastSocket::IssueSendTo_NoCopy_TempTtl(CFragmentedBuffer& sendBuffer, AddrPort sendTo, int ttl)
	{
		int totalLength = sendBuffer.GetLength();
		if (totalLength <= 0)
			return SocketErrorCode_InvalidArgument;// 0 이하의 수신 자체는 무의미한데다 socket 오동작을 유발하므로

		//XXX_TEST13();

		sockaddr_in sendToA;

		int sendToALen = sizeof(sendToA);
		sendTo.ToNative(sendToA);

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

			if (ret >= 0)
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
				if (IsClosedOrClosing())
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
		UnsafeFastMemcpy(m_sendBuffer.GetData(), data, count);

		WSABUF buf;
		buf.buf = (char*)m_sendBuffer.GetData();
		buf.len = count;

		sockaddr_in sendToA;
		int sendToALen = sizeof(sendToA);
		sendTo.ToNative(sendToA);

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

			if (ret >= 0)
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
				if (IsClosedOrClosing())
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

		sockaddr_in sendToA;

		int sendToALen = sizeof(sendToA);
		sendTo.ToNative(sendToA);

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

	void CFastSocket::InitOthers()
	{
		SocketInitializer::Instance();

		//m_ignoreNotSocketError = false;
		//m_restoredCount = 0;
		{
			CriticalSectionLock lock(m_socketClosed_CS, true);
			m_socketClosedOrClosing_CS_PROTECTED = 0;
		}

		m_associatedIoQueue = NULL;

		m_verbose = false;

		m_ioPendingCount = 0;

#ifndef _WIN32
		m_isLevelTrigger = false;
#endif
		//m_sendCompletionEvent = NULL;
		//m_recvCompletionEvent = NULL;

		//m_iocpAssocCount_TTTEST = 0;

		// 대량의 클라를 일시에 파괴시 lookaside allocator free에서 에러가 나길래 fast heap 미사용!
		//  		m_sendBuffer.UseFastHeap(false);
		//  		m_recvBuffer.UseFastHeap(false);

		// 		memset(&m_recvOverlapped, 0, sizeof(m_recvOverlapped));
		// 		memset(&m_sendOverlapped, 0, sizeof(m_sendOverlapped));
		// 		memset(&m_AcceptExOverlapped, 0, sizeof(m_AcceptExOverlapped));

		m_completionContext = NULL;

		// 이 값은 overlapped recvfrom이 완료될 때 채워지므로 유효하게 유지해야 한다.
		memset(&m_recvedFrom, 0, sizeof(m_recvedFrom));
		m_recvedFromLen = 0;
#ifdef _WIN32
		m_recvFlags = 0;
#endif 

		m_ttlToRestoreOnSendCompletion = -1;
		m_ttlMustBeRestoreOnSendCompletion = false;








		/*     //3003!!!혹시 resize가 문제?
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
			throw Exception(StringA::NewFormat("Listen failed! error=%d", err));
		}
	}


	Proud::AddrPort CFastSocket::GetSockName()
	{
		return Socket_GetSockName(m_socket);
	}

	Proud::AddrPort CFastSocket::GetPeerName()
	{
		sockaddr_in o;
		int olen = sizeof(o);
		if (::getpeername(m_socket, (sockaddr*)&o, (socklen_t *)&olen) != 0)
			return AddrPort::Unassigned;
		else
		{
			AddrPort ret;
			ret.m_binaryAddress = o.sin_addr.s_addr;
			ret.m_port = ntohs(o.sin_port);

			return ret;
		}
	}

	String CFastSocket::GetIPAddress(String hostName)
	{
		in_addr addr;
		addr.s_addr = Proud::DnsForwardLookup(hostName);
		return InetNtopV4(addr);
	}

	// 	void Socket::SetSendCompletionEvent( Event* event )
	// 	{
	// 		m_sendCompletionEvent = event;
	// 	}
	// 
	// 	void Socket::SetRecvCompletionEvent( Event* event )
	// 	{
	// 		m_recvCompletionEvent = event;
	// 	}

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

			lock.Unlock(); // 아래 RemoveSocket에서 거는 lock과 데드락이 생기지 않게 하기 위해

			// iocp or epoll과의 연결을 해제한다.
			if (m_associatedIoQueue != NULL)
			{
				//CriticalSectionLock lock(m_associatedIoQueue->m_cs, true);
				//m_associatedSockets.RemoveAt(m_ioQueueIter);

				// Linux에서 해당 Socket을 가지고 있는 IoNotifier가 SetDesiredThreadCount에 의해 다른 스레드로 Socket을 복사하는 중이라면
				// RemoveSocket이 정상적으로 처리되지 않을 것으로 보입니다.
				/* => RemoveSocket은 내부적으로 Proud.CThreadPoolImpl.m_cs를 lock합니다.
				그리고 SetDesiredThreadCount도 같은 lock을 하고 있고, 그 안에서 모자란 스레드를 생성하고 초과하는 스레드를 중단하라는 명령과 함께 소켓 이양 처리를 모두 완료한 후 리턴하는 것으로 압니다.
				그렇다면 아래 함수가 실행되는 동안에는 죽어야 할 스레드에서 찾는 헛짓거리를 할 가능성은 없다고 생각합니다.

				그리고 오늘 회의에서, SetDesiredThreadCount가 리턴하기 전에 죽을 스레드로부터 소켓들을 모두 이양하지 않는다고 했는데, 그러면 안됩니다.
				level trigger로 옮겨지게 한 이유는 이양한 후에 그것들의 이벤트가 언젠가는 반드시 발생하게 하려고입니다.
				이 때문에 SetDesiredThreadCount에서 리턴하게 전에 죽을 스레드로부터 소켓들을 모두 이양하게 바꾸세요. 그러면 위의 문제 해결이 성립됩니다.

				*/
				m_associatedIoQueue->RemoveSocket(this);

				m_associatedIoQueue = NULL;
			}
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
	}

	void CFastSocket::ForceSetCompletionContext(IIoEventContext* context)
	{
		//강제로 컨텍스트를 바꾼다...
		//함부로 쓰지 말것...
		m_completionContext = context;
	}

	void CFastSocket::SetCompletionContext(IIoEventContext* context)
	{
		if (m_completionContext != NULL && context != m_completionContext)
			throw Exception("Completion Context already set!");

		m_completionContext = context;
	}

	IIoEventContext* CFastSocket::GetIoEventContext()
	{
		return m_completionContext;
	}

	SocketErrorCode CFastSocket::Shutdown(ShutdownFlag how)
	{
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
	SocketErrorCode CFastSocket::AcceptEx(CFastSocket* openedSocket)
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

	SocketErrorCode CFastSocket::ConnectEx(String hostAddr, int hostPort)
	{
		AddrPort addrPort;
		addrPort.m_binaryAddress = Proud::DnsForwardLookup(hostAddr);
		if (addrPort.m_binaryAddress == INADDR_NONE)
		{
			uint32_t err = WSAGetLastError();
			PostSocketWarning(err, _PNT("FS.CE"));
			return (SocketErrorCode)err;
		}
		addrPort.m_port = hostPort;//htons((u_short)hostPort);

		return ConnectEx(addrPort);
	}

	Proud::SocketErrorCode CFastSocket::ConnectEx(AddrPort addrPort)
	{

		if (lpfnConnectEx == NULL)
			DirectBindConnectEx();

		//이미 바인드 되어있는 상태여야 한다.
		if (!IsBindSocket())
		{
			NTTNTRACE("WARNING: ConnectEX Socket Not Bind! Auto Bind!");
			//			m_dg->OnSocketWarning(this,_PNT("WARNING: ConnectEX Socket Not Bind! Auto Bind!"));
			Bind();
		}

		sockaddr_in sockAddr;
		memset(&sockAddr, 0, sizeof(sockAddr));
		sockAddr.sin_family = AF_INET;
		sockAddr.sin_addr.s_addr = addrPort.m_binaryAddress;
		sockAddr.sin_port = htons((u_short)addrPort.m_port);

	retry:
		//associate를 먼저해야 하겠다.
		BOOL ret = lpfnConnectEx(
			m_socket,
			(struct sockaddr*)&sockAddr,
			sizeof(struct sockaddr),
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

	bool CFastSocket::IsBindSocket()
	{
		AddrPort port = GetSockName();

		return (port.m_port >= 1 && port.m_port <= 65534);
	}

	// TcpListenSocket으로부터 accepted socket을 가져온다.
	// NOTE: AcceptEx는 이미 만들어진 socket을 accepted socket으로 변신시키며,
	void CFastSocket::FinalizeAcceptEx(CFastSocket* TcpListenSocket, AddrPort& localAddr, AddrPort &remoteAddr)
	{
#if defined (_WIN32)
		sockaddr_in la, ra;
		int llen = sizeof(la), rlen = sizeof(ra);
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
#if defined __ORBIS__
		int argp = isBlockingMode ? 0 : 1;
		sceNetSetsockopt(m_socket, SCE_NET_SOL_SOCKET, SCE_NET_SO_NBIO, &argp, sizeof(argp));
#elif defined (__linux__) || defined (__FreeBSD__) || defined(__MACH__) // PS4를 제외하고, FreeBSD를 쓰는 경우도.
		// fcntl()를 사용하면 GCC ARM에서 nonblock 임에도 불구하고 조금씩 block현상이 생김
		// 그리고 추가적으로 fcntl()을 사용시 connect가 안되는 현상이 발생 
		// fcntl() -> ioctl()로  수정함 modify by kdh
		//int flag = fcntl(m_socket, F_GETFL, 0);
		u_long argp = isBlockingMode ? 0 : 1;
		int flag = 1;
		flag = ::ioctl(m_socket, FIONBIO, &argp);
		if (flag == -1)
		{
			assert(0); // 있어서는 안되는 상황
			//PostSocketWarning(errno, _PNT("SetBlockingMode : fcntl get error"));
			PostSocketWarning(errno, _PNT("SetBlockingMode : ioctl get error"));
		}

		//  		int newFlag;
		//  		if(isBlockingMode)
		//  			newFlag = flag & ~O_NONBLOCK;
		//  		else
		//  			newFlag = flag | O_NONBLOCK;
		//  
		//          int ret = fcntl(m_socket, F_SETFL, newFlag );
		//  	
		//          if(ret == -1)
		//          {
		//  			assert(0); // 있어서는 안되는 상황
		//              //PostSocketWarning(errno, _PNT("SetBlockingMode : fcntl set error"));
		//  			PostSocketWarning(errno, _PNT("SetBlockingMode : ioctl get error"));
		//          }
#else
		u_long argp = isBlockingMode ? 0 : 1;
		::ioctlsocket(m_socket, FIONBIO, &argp);
#endif
		//// for marmalade ARM GCC test
		//assert(GetBlockingMode()==isBlockingMode);
	}

#ifndef _WIN32
	bool CFastSocket::GetBlockingMode()
	{
		int flag = fcntl(m_socket, F_GETFL, 0);
		if (flag == -1)
		{
			assert(0); // 있어서는 안되는 상황
			PostSocketWarning(errno, _PNT("GetBlockingMode : fcntl get error"));
		}

		if (flag & O_NONBLOCK)
			return false;
		else
			return true;
	}
#endif // _WIN32


	uint8_t* CFastSocket::GetRecvBufferPtr()
	{
		return m_recvBuffer.GetData();
	}

	// 	BYTE* Socket::GetSendBufferPtr()
	// 	{
	// 		return m_sendBuffer.GetData();
	// 	}

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
		if ((ret = ::getsockopt(m_socket, IPPROTO_IP, IP_TTL, (char*)&val, (socklen_t *)&vallen)) != 0)
		{
			NTTNTRACE("This socket doesn't support TTL change!");
			//			m_dg->OnSocketWarning(this, _PNT("This socket doesn't support TTL change!"));
			return (SocketErrorCode)WSAGetLastError();
		}
		else
		{
			val = ttl;
			if ((ret = ::setsockopt(m_socket, IPPROTO_IP, IP_TTL, (const char*)&val, vallen)) != 0)
			{
				NTTNTRACE("Cannot change TTL!");
				//				m_dg->OnSocketWarning(this, _PNT("Cannot change TTL!"));
				return (SocketErrorCode)WSAGetLastError();
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

#if !defined(_WIN32)
	// http://linux.die.net/man/3/errno 에 의하면, EAGAIN과 EWOULDBLOCK은 같은 의미지만,
	// 일부 유닉스는 이 값이 서로 다르다.
	// 따라서 모두 동일하게 처리를 해주어야 한다. 이 함수는 그것을 위한 helper임.
	bool CFastSocket::IsWouldBlockError(SocketErrorCode code)
	{
		// EAGAIN = EWOULDBLOCK이지만 일부 OS에서는 의미는 같지만 값이 다르다. 그래서 꼭 체크해 주어야 한다.
		return code == SocketErrorCode_WouldBlock || code == SocketErrorCode_Again || code == SocketErrorCode_InProgress;
	}
#else
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
		if (sendTo.m_binaryAddress == 0xffffffff) // OS에 의해 차단되지 않으려면 필수
		{
			NTTNTRACE("Sending to 255.255.255.255 is not permitted!");
			//			m_dg->OnSocketWarning(this,_PNT("Sending to 255.255.255.255 is not permitted!")); // NOTE: 255.255.255.255:65535로 쏘면 Win32는 막아버린다. 굳이 보내려면 239.255.255.250으로 브로드캐스트하덩가.

			return false;
		}
		else if (/*sendTo.m_port == 0xffff || */sendTo.m_port == 0) // OS에 의해 차단되지 않으려면 필수
		{
			NTTNTRACE("Sending to prohibited port is not permitted!");
			//			m_dg->OnSocketWarning(this,_PNT("Sending to prohibited port is not permitted!"));
			return false;
		}
		else if (sendTo.m_binaryAddress == 0) // OS에 의해 차단되지 않으려면 필수
		{
			NTTNTRACE("Sending to 0.0.0.0 is not permitted!");
			//			m_dg->OnSocketWarning(this,_PNT("Sending to 0.0.0.0 is not permitted!"));
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
		if ((ret = ::getsockopt(m_socket, IPPROTO_IP, IP_TTL, (char*)&outTtl, (socklen_t *)&vallen)) != 0)
		{
			NTTNTRACE("This socket doesn't support TTL change!");
			//			m_dg->OnSocketWarning(this, _PNT("This socket doesn't support TTL change!"));
		}

		return (SocketErrorCode)ret;
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
		else
		{
			SocketErrorCode e = (SocketErrorCode)WSAGetLastError();
			// EINTR은 driver에 의해 발생할 수도 있으므로 이러한 경우 재시도해야.
			if (IsIntrError(this, e))
				goto L1;

			return e;
		}
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
