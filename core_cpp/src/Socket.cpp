/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
 #include <new>
 #include "Socket.h"
 #if !defined(_WIN32)
     #include <typeinfo>
 #endif
 
 namespace Proud
 {
 #ifdef _WIN32
 	SocketSelectContext* SocketSelectContext::New()
 	{
 		return new SocketSelectContextImpl();
 	}

 	void SocketSelectContextImpl::AddWriteWaiter( CSocket& socket )
 	{
 		CSocketImpl* ptr = nullptr;
 		try
		{
 			ptr = dynamic_cast<CSocketImpl*>(&socket);
 		}
 		catch(std::bad_cast& e)
 		{
 			CFakeWin32::OutputDebugStringA(e.what());
 			CauseAccessViolation();
 		}

 		if(nullptr != ptr)
 			m_socketctx.AddWriteWaiter(*(ptr->m_socketCreateResult.socket.get()));
 	}
 
 	void SocketSelectContextImpl::AddExceptionWaiter( CSocket& socket )
 	{
 		CSocketImpl* ptr = nullptr;
 		try
		{
 			ptr = dynamic_cast<CSocketImpl*>(&socket);
 		}
 		catch(std::bad_cast& e)
 		{
 			CFakeWin32::OutputDebugStringA(e.what());
 			CauseAccessViolation();
 		}
 
 		if(nullptr != ptr)
 			m_socketctx.AddExceptionWaiter(*(ptr->m_socketCreateResult.socket.get()));
 	}
     
 	void SocketSelectContextImpl::Wait( uint32_t miliSec )
 	{
 		m_socketctx.Wait(miliSec);
 	}
 
 	bool SocketSelectContextImpl::GetConnectResult( CSocket& socket, SocketErrorCode& outCode )
 	{
 		CSocketImpl* ptr = nullptr;
 
 		try
		{
 			ptr = dynamic_cast<CSocketImpl*>(&socket);
 		}
 		catch(std::bad_cast& e)
 		{
 			CFakeWin32::OutputDebugStringA(e.what());
 			CauseAccessViolation();
 		}
 
 		if(nullptr != ptr)
 			return m_socketctx.GetConnectResult(*(ptr->m_socketCreateResult.socket.get()), outCode);

 		return false;
 	}
 #endif // _WIN32
 
 	void CSocketImpl::OnSocketWarning(CFastSocket* /*socket*/, String msg)
 	{
 		m_internalDg->OnSocketWarning(this, msg);
 	}
 
 	CSocketImpl::CSocketImpl( SocketType type, ISocketDelegate* dg ) : m_internalDg(dg)
 	{
		m_socketCreateResult = CFastSocket::Create(type);
		m_internalDg = dg;
 	}
 
 	bool CSocketImpl::Bind()
 	{
 		return SocketErrorCode_Ok == m_socketCreateResult.socket->Bind();
 	}
 
 	bool CSocketImpl::Bind( int port )
 	{
 		return SocketErrorCode_Ok == m_socketCreateResult.socket->Bind(port);
 	}
 
 	bool CSocketImpl::Bind( const PNTCHAR* addr, int port )
 	{
 		return SocketErrorCode_Ok == m_socketCreateResult.socket->Bind(addr, port);
 	}
 
 	SocketErrorCode CSocketImpl::Connect( String hostAddr, int hostPort )
 	{
		AddrPort address;
		SocketErrorCode errorCode;

		if (AddrPort::FromHostNamePort(&address, errorCode, hostAddr, (uint16_t)hostPort) == false)
		{
			return errorCode;
		}

 		return m_socketCreateResult.socket->Connect(address);
 	}
     
 #if defined(_WIN32)
  	SocketErrorCode CSocketImpl::IssueRecvFrom( int length )
 	{
 		return m_socketCreateResult.socket->IssueRecvFrom(length);
 	}
 
 	SocketErrorCode CSocketImpl::IssueSendTo(uint8_t* data, int count, AddrPort sendTo)
 	{
 		return m_socketCreateResult.socket->IssueSendTo(data, count, sendTo);
 	}
 
 
 	SocketErrorCode CSocketImpl::IssueRecv( int length )
 	{
 		return m_socketCreateResult.socket->IssueRecv(length);
 	}
 
 	SocketErrorCode CSocketImpl::IssueSend(uint8_t* data, int count)
 	{
 		return m_socketCreateResult.socket->IssueSend(data, count);
 	}
 
 	bool CSocketImpl::GetRecvOverlappedResult( bool waitUntilComplete, OverlappedResult &ret )
 	{
 		return m_socketCreateResult.socket->GetRecvOverlappedResult(waitUntilComplete, ret);
 	}
 
 	bool CSocketImpl::GetSendOverlappedResult( bool waitUntilComplete, OverlappedResult &ret )
 	{
 		return m_socketCreateResult.socket->GetSendOverlappedResult(waitUntilComplete, ret);
 	}
 #endif
 
 	AddrPort CSocketImpl::GetSockName()
 	{
 		return m_socketCreateResult.socket->GetSockName();
 	}
 
 	AddrPort CSocketImpl::GetPeerName()
 	{
 		return m_socketCreateResult.socket->GetPeerName();
 	}
 
 	void CSocketImpl::SetBlockingMode( bool isBlockingMode )
 	{
 		m_socketCreateResult.socket->SetBlockingMode(isBlockingMode);
 	}
 
 	uint8_t* CSocketImpl::GetRecvBufferPtr()
 	{
 		return m_socketCreateResult.socket->GetRecvBufferPtr();
 	}
 
 	CSocket *CSocket::New(SocketType type,ISocketDelegate *dg)
 	{
 		return new CSocketImpl(type,dg);
 	}
 }
