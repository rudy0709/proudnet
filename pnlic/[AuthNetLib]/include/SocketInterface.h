/*
ProudNet v1


이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의 : 저작물에 관한 위의 명시를 제거하지 마십시오.


This program is soley copyrighted by Nettention.
Any use, correction, and distribution of this program are subject to the terms and conditions of the License Agreement.
Any violated use of this program is prohibited and will be cause of immediate termination according to the License Agreement.

** WARNING : PLEASE DO NOT REMOVE THE LEGAL NOTICE ABOVE.


此程序的版权归Nettention公司所有。
与此程序的修改、使用、发布相关的事项要遵守此程序的所有权者的协议。
不遵守协议时要原则性的禁止擅自使用。
擅自使用的责任明示在与此程序所有权者的合同书里。

** 注意：不要移除关于制作物的上述明示。


このプログラムの著作権はNettentionにあります。
このプログラムの修正、使用、配布に関する事項は本プログラムの所有権者との契約に従い、
契約を遵守しない場合、原則的に無断使用を禁じます。
無断使用による責任は本プログラムの所有権者との契約書に明示されています。

** 注意：著作物に関する上記の明示を除去しないでください。

*/

#pragma once

//#pragma pack(push,8)

namespace Proud
{
	class CSocket;

#ifdef _WIN32

	/** \addtogroup net_group
	*  @{
	*/

	/**
	\~korean
	SocketSelectContext class
	- 사용법 예제는 Sample중 SimpleHttpConnect를 참고하시면 됩니다.
	- socket의 select() non-block model 을 위한 용도
	- 주의: Wait 호출 후에는 FD_SET의 내용이 바뀐다. 따라서 이 객체는 1회성으로 쓰여야 한다.
	- Win32에서만 지원하는 기능입니다. iOS, linux에서는 이것 대신 ::poll()을 사용하십시오.

	\~english TODO:translate needed.
	SocketSelectContext class
-
	- This it for select() non-block model of socket
	- Note: After Wait calling, informationf of FD_SET will be changed. So this object has to use for once.
	- This class can work only in Win32 platform. Use ::poll() in iOS or Linux.

	\~chinese
	SocketSelectContext class
	- 使用方法例子参考sample中SimpleHttpConnect即可。
	- 为了socket的select() non-block model的用途。
	- 注意：Wait呼叫以后会改变FD_SET的内容。所以此对象要用于一次性。
	- 只在Win32可以支持。在iOS, linux使用::poll()来代替它吧。

	\~japanese
	\~
	*/
	class SocketSelectContext
	{
	public:
		SocketSelectContext() {}

		virtual void AddWriteWaiter(CSocket& socket) = 0;
		virtual void AddExceptionWaiter( CSocket& socket ) = 0;
		virtual void Wait(uint32_t miliSec) = 0;
		virtual bool GetConnectResult(CSocket& socket, SocketErrorCode& outCode) = 0;

		PROUD_API static SocketSelectContext *New();
	};
#endif // _WIN32

	/**
	\~korean
	Socket Delegate Interface
	- Socket에 관련된 에러를 OnSocketWarning 함수에서 받을수 있다.

	\~english
	Socket Delegate Interface
	- You can receive Socket related error from OnSocketWarning function.

	\~chinese
	Socket Delegate Interface
	- Socket相关的错误可以在OnSocketWarning函数里接收。

	\~japanese
	\~
	*/
	class ISocketDelegate
	{
	public:
		virtual ~ISocketDelegate() {}
		virtual void OnSocketWarning(CSocket* soket, String msg) = 0;
	};

	/**
	\~korean
	CSocket class
	- Proud의 NetClient 를 쓰지 않고 외부의 Server나 http에 접근할때 쓰면 유용하다.
	- 내부적으로 Proud::FastSocket 을 사용한다.

	\~english
	CSocket class
	- It is useful when you access external Server or http without NetClient of ProudNet.
	- Use Proud::FastSocket internally

	\~chinese
	CSocket class
	- 不使用Proud的NetClient，访问外部的Server或者http的时候使用的话有用。
	- 使用内部 Proud::FastSocket%。

	\~japanese
	\~
	*/
	class CSocket
	{
	public:
		virtual ~CSocket(){}

	public:
		virtual bool Bind() = 0;
		virtual bool Bind(int port) = 0;
		virtual bool Bind( const PNTCHAR* addr, int port ) = 0;

		/**
		\~kore﻿an
		Connect 한다.
		\param hostAddr host 의 주소
		\param hostPort host 의 port

		\~english TODO:translate needed.

		\~chinese
		进行connect。
		\param hostAddr host的地址。
		\param hostPort host的port。

		\~japanese
		\~
		*/
		virtual SocketErrorCode Connect(String hostAddr, int hostPort) = 0;

#if !defined(_WIN32)
// 		virtual SocketErrorCode NonBlockedRecvFrom(int length) = 0;
// 		virtual SocketErrorCode NonBlockedSendTo( BYTE* data, int count, AddrPort sendTo ) = 0;
// 		virtual SocketErrorCode NonBlockedRecv(int length) = 0;
// 		virtual SocketErrorCode NonBlockSend( BYTE* data, int count) = 0;
#else
		/**
		\~korean
		UDP socket
		- Recv를 Issue한다.
		\param length 버퍼의 크기
		\return 소켓 Error를 리턴해줍니다. SocketErrorCode_Ok 이면 정상

		\~english TODO:translate needed.

		\~chinese
		UDP socket
		- 把Recv进行issue。
		\param length buffer的大小。
		\return 返回socket的Error。SocketErrorCode_Ok 的话正常。

		\~japanese
		\~
		*/
		virtual SocketErrorCode IssueRecvFrom(int length) = 0;

		/**
		\~korean
		UDP socket
		- Send를 Issue한다.
		\param data 보낼 data의 배열
		\param count 배열의 크기
		\param sendTo 보낼 주소
		\return 소켓 Error를 리턴해줍니다. SocketErrorCode_Ok 이면 정상

		\~english TODO:translate needed.

		\~chinese
		UDP socket
		- 把Send进行Issue。
		\param data 要发送的data的数组。
		\param count 数组的大小。
		\param sendTo 要发送的地址。
		\return 返回socket的Error。SocketErrorCode_Ok 的话正常。

		\~japanese
		\~
		*/
		virtual SocketErrorCode IssueSendTo( uint8_t* data, int count, AddrPort sendTo ) = 0;

		/**
		\~korean
		TCP socket
		- Recv를 Issue한다.
		\param length 버퍼의 크기
		\return 소켓 Error를 리턴해줍니다. SocketErrorCode_Ok 이면 정상

		\~english TODO:translate needed.

		\~chinese
		UDP socket
		- 把Recv进行issue。
		\param length buffer的大小。
		\return 返回socket的Error。SocketErrorCode_Ok 的话正常。

		\~japanese
		\~
		*/
		virtual SocketErrorCode IssueRecv(int length) = 0;

		/**
		\~korean
		TCP socket
		- Send 를 Issue 한다.
		\param data 보낼 data 의 배열
		\param count 배열의 크기
		\return 소켓 Error 를 리턴해줍니다. SocketErrorCode_Ok이면 정상

		\~english TODO:translate needed.

		\~chinese
		UDP socket
		- 把Send进行Issue。
		\param data 要发送的data的数组。
		\param count 数组的大小。
		\return 返回socket的Error。SocketErrorCode_Ok 的话正常。

		\~japanese
		\~
		*/
		virtual SocketErrorCode IssueSend( uint8_t* data, int count) = 0;

		/**
		\~korean
		async issue의 결과를 기다린다.
		- 아직 아무것도 완료되지 않았으면 null을 리턴한다.
		- 만약 완료 성공 또는 소켓 에러 등의 실패가 생기면 객체를 리턴하되 m_errorCode가 채워져 있다.
		\param waitUntilComplete 완료될때까지 기다릴 것인지 결정한다.
		\param ret 결과 \ref OverlappedResult 참조
		\return 성공이면 true, 실패이면 false리턴

		\~english TODO:translate needed.

		\~chinese
		等待async issue的结果。
		- 什么都没有结束的话返回null。
		- 如果成功结束或者发生socket错误的失败的的话，返回对象但是会填充m_errorCode。
		\param waitUntilComplete 决定是否要等到结束。
		\param ret 参照结果\ref OverlappedResult%。
		\return 成功的话true，失败的话返回false。

		\~japanese
		\~
		*/
		virtual bool GetRecvOverlappedResult(bool waitUntilComplete, OverlappedResult &ret) = 0;

		/**
		\~korean
		async issue의 결과를 기다린다.
		- 아직 아무것도 완료되지 않았으면 null을 리턴한다.
		- 만약 완료 성공 또는 소켓 에러 등의 실패가 생기면 객체를 리턴하되 m_errorCode가 채워져 있다.
		\param waitUntilComplete 완료될때까지 기다릴 것인지 결정한다.
		\param ret 결과 \ref OverlappedResult 참조
		\return 성공이면 true, 실패이면 false리턴

		\~english TODO:translate needed.

		\~chinese
		等async issue的结果。
		- 什么都没有结束的话返回null。
		- 如果成功结束或者发生socket错误的失败的的话，返回对象但是会填充m_errorCode。
		\param waitUntilComplete 决定是否要等到结束。
		\param ret 参照结果\ref OverlappedResult%。
		\return 成功的话true，失败的话返回false。

		\~japanese
		\~
		*/
		virtual bool GetSendOverlappedResult(bool waitUntilComplete, OverlappedResult &ret) = 0;


#endif

		/**
		\~korean
		소켓의 주소를 가져온다.

		\~english TODO:translate needed.

		\~chinese
		拿来socket的地址。

		\~japanese
		\~
		*/
		virtual AddrPort GetSockName() = 0;

		/**
		\~korean
		peer 에 대한 소켓의 주소를 가져온다.

		\~english TODO:translate needed.

		\~chinese
		拿来对peer的socket地址。

		\~japanese
		\~
		*/
		virtual AddrPort GetPeerName() = 0;

		/**
		\~korean
		통신에 대하여 블럭킹 모드를 사용할 것인지 선택한다.

		\~english TODO:translate needed.

		\~chinese
		选择对通信是否使用blocking模式。

		\~japanese
		\~
		*/
		virtual void SetBlockingMode(bool isBlockingMode) = 0;

		/**
		\~korean
		recv 버퍼의 포인터를 얻어온다.

		\~english TODO:translate needed.

		\~chinese
		获取recv buffer的指针。

		\~japanese
		\~
		*/
		virtual uint8_t* GetRecvBufferPtr() = 0;

		/**
		\~korean
		recv host의 ip Address를 얻어온다.

		\~english TODO:translate needed.

		\~chinese
		获取recv host的ip Address。

		\~japanese
		\~
		*/
		PROUD_API static String GetIPAddress(String hostName);

		/**
		\~korean
		CSocket 객체를 생성한다.
		\param auxSocket socket객체
		\param dg 소켓의 이벤트를 받을 객체. \ref ISocketDelegate 를 참조

		\~english TODO:translate needed.

		\~chinese
		生成 CSocket%对象。
		\param auxSocket socket对象。
		\param dg 接收socket的event的对象。参照\ref ISocketDelegate%。

		\~japanese
		\~
		*/
		PROUD_API static CSocket *New(SOCKET auxSocket,ISocketDelegate* dg);

		/**
		\~korean
		CSocket 객체를 생성한다.
		\param type 소켓의 Type을 정한다.
		\param dg 소켓의 이벤트를 받을 객체. \ref ISocketDelegate 를 참조

		\~english TODO:translate needed.

		\~chinese
		生成 CSocket%对象。
		\param type 指定socket的Type。
		\param dg 接收socket的event的对象。参照\ref ISocketDelegate%。

		\~japanese
		\~
		*/
		PROUD_API static CSocket *New(SocketType type,ISocketDelegate *dg);
	};

	/**  @} */
}

//#pragma pack(pop)
