#include "stdafx.h"
#include "NetServer.h"
#include "TestUdpConnReset.h"

namespace Proud 
{
	CTestUdpConnReset::CTestUdpConnReset(void)
	{
		memset(m_payload,0,sizeof(m_payload));
	}

	CTestUdpConnReset::~CTestUdpConnReset(void)
	{
	}

	void CTestUdpConnReset::DoTest()
	{

#ifdef WSAECONNRESET_TEST_POSTPONED
		// 소켓 생성
		m_socket = FastSocketPtr(new CFastSocket(SocketType_Udp, this));
		m_socket->Bind();
		
		// 존재 안하는 수신자에게 패킷 쏘기. ICMPCONNRESET을 유도하기 위함
		SocketErrorCode e = BlockedSendTo(m_payload,sizeof(m_payload), AddrPort::FromIPPort(_PNT("22.33.44.55"),5555), 1000);
		if(e == SocketErrorCode_ConnectResetByRemote || e == SocketErrorCode_Ok)
		{
			// 의도된 상황이다.
		}
		else
		{
			String txt;
			txt.Format(_PNT("An Odd Error by sending UDP Packet to Non-Exist Receivers! %d"), e);
			m_owner->EnqueueUnitTestFailEvent(txt);
			return;
		}

		// 미존재 수신자에게 패킷을 쐈으므로 WSAECONNRESET이 도착하거나 말아야 함.
		e = WaitForIcmpConnReset();
		if(e != SocketErrorCode_Ok) // WSAECONNRESET은 BlockedRecvFrom 내부에서 소화되므로
		{
			String txt;
			txt.Format(_PNT("A Different Error Occurred while Waiting WSAECONNRESET! %d"), e);
			m_owner->EnqueueUnitTestFailEvent(txt);
			return;
		}

		// 존재 수신자에게 패킷을 쏜다.
		// 문제 발생: 존재 수신자가 뭐가 있지? UDP 포트를 받을 수 있는 곳이어야 하며 NO loopback이어야 하는데!

		// 존재 수신자에게 패킷을 쏴도 WSAECONNRESET이 연발하지 않아야 한다. 만약 연발하면 서버도 per-client UDP socket을 둬야 함을 의미한다.
		

		/*
		
		Sendto 리턴값이 WSAECONNRESET이면 The virtual circuit was reset by the remote side executing a hard or abortive close. For UPD sockets, the remote host was unable to deliver a previously sent UDP datagram and responded with a "Port Unreachable" ICMP packet. The application should close the socket as it is no longer usable.라고 하는데? 
		즉, 소켓을 재생성해야 한다고라!
		그러나 반례도 있음.
		I found the reason and the fix to the problem.

		If you have a non-blocking recvfrom() loop like this to read from your socket, you are doing the wrong thing:


		do
		{
		ret = recvfrom(...);
		if ( ret != SOCKET_ERROR )
		dispatch_data ();
		}while ( ret != SOCKET_ERROR )


		Because WSAECONNRESET obviously causes a SOCKET_ERROR but actually there is no error and you should keep reading the socket when WSAECONNRESET occurs. So it should read something like this;


		do
		{
		ret = recvfrom(...);
		if ( ret == SOCKET_ERROR )
		err = WSAGetLastError ( );
		else
		dispatch_data ();
		}while ( ret != SOCKET_ERROR || ( ret == SOCKET_ERROR && err == WSAECONNRESET ) )


		It turns out that Q3 is actually handling this issue like this. It also includes the WSAWOULDBLOCK as an OK to read error.

		*/
		

#endif // WSAECONNRESET_TEST_POSTPONED

	}
}