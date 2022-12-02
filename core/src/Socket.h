/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once
 
 #include "FastSocket.h"
 #include "../include/SocketInterface.h"
 #include "NumUtil.h"
 
 //#pragma pack(push,8)
 
 namespace Proud
 {
 #ifdef _WIN32
 	class SocketSelectContextImpl : public SocketSelectContext
 	{
 		FastSocketSelectContext m_socketctx;
 
 	public:
 		SocketSelectContextImpl() {}
 
 		void AddWriteWaiter(CSocket& socket);
 		void AddExceptionWaiter( CSocket& socket );
 		void Wait(uint32_t miliSec);
 		bool GetConnectResult(CSocket& socket, SocketErrorCode& outCode);
 	};
 #endif // _WIN32
 
 	class CSocketImpl :public CSocket
 		//public IFastSocketDelegate
 	{
 		friend class SocketSelectContextImpl;
  	private:
 		// Proud FastSocket Instance
		SocketCreateResult m_socketCreateResult;
 		// Socket Delegate
 		ISocketDelegate* m_internalDg;
 
 		virtual void OnSocketWarning(CFastSocket* socket, String msg);
 
 	public:
 		CSocketImpl(SocketType type, ISocketDelegate* dg);
 
 	public:
 		bool Bind();
 		bool Bind(int port);
 		bool Bind( const PNTCHAR* addr, int port );
 
 		SocketErrorCode Connect(String hostAddr, int hostPort);

#if defined(_WIN32)
 		SocketErrorCode IssueRecvFrom(int length);
 		SocketErrorCode IssueSendTo(uint8_t* data, int count, AddrPort sendTo);
 		SocketErrorCode IssueRecv(int length);
 		SocketErrorCode IssueSend(uint8_t* data, int count);
 		bool GetRecvOverlappedResult(bool waitUntilComplete, OverlappedResult &ret);
 		bool GetSendOverlappedResult(bool waitUntilComplete, OverlappedResult &ret);
#endif		

		AddrPort GetSockName();
 		AddrPort GetPeerName();
 
 		void SetBlockingMode(bool isBlockingMode);
 		uint8_t* GetRecvBufferPtr();
 
 #ifdef _WIN32
 #pragma push_macro("new")
 #undef new
 		// 이 클래스는 ProudNet DLL 경우를 위해 커스텀 할당자를 쓰되 fast heap을 쓰지 않는다.
 		DECLARE_NEW_AND_DELETE
 #pragma pop_macro("new")
 #endif
 	};
 }
 
 //#pragma pack(pop)