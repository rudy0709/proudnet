#pragma once

#include "../include/BasicTypes.h"

// websocket server lib는 서버 모듈에서만 참조하도록 하자.
// C++ 클라이언트 모듈에서 참조하게 되면 라이브러리가 쓸데없이 커진다. openssl,asio 등.
// 그래서 CSuperSocket을 상속받되 이를 서버에서만 사용하도록 하자.

#ifdef SUPPORTS_WEBGL

#ifdef _MSC_VER
#pragma warning (disable:4996)
#endif

#include "server_ws.hpp"
#include "server_wss.hpp"

#include "NetCore.h"

namespace Proud
{
	// CSuperSocket에서 WssServer::Connection 객체를 가지면, websocket server 모듈과 openssl이 덩달아 들어가버리게 된다.
	// 즉, 라이브러리가 쓸데없이 커진다.
	// 그래서 CSuperSocket을 상속받되 상속된 클래스는 서버에서만 사용하도록 하자.
	class CWebSocket :public CSuperSocket
	{
	public:
		CWebSocket(CNetCoreImpl *owner, shared_ptr<WsServer::Connection> connection);
		CWebSocket(CNetCoreImpl *owner, shared_ptr<WssServer::Connection> connection);

		// 둘 중에 하나만 사용된다.
		shared_ptr<WsServer::Connection> m_webSocket;
		shared_ptr<WssServer::Connection> m_webSocketSecured;

		void AddToSendQueueWithoutSplitterAndSignal_Copy_WebSocket(
			const CSendFragRefs &sendData) override;
	};
}

#endif
