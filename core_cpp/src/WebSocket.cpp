#include "stdafx.h"


#ifdef SUPPORTS_WEBGL

#include "WebSocket.h"
#include "NetServer.h"
#include "Base64.h"

namespace Proud
{

	// 이미 socket fd가 만들어진 상태이고, 이 생성자 함수에서는 fail case가 전혀 없다.
	// 따라서 public 생성자 함수로 고고고. CSuperSocket.New와 달리.
	CWebSocket::CWebSocket(CNetCoreImpl *owner, shared_ptr<WsServer::Connection> connection)
		:CSuperSocket(owner, SocketType_WebSocket)
	{
		m_webSocket = connection;
	}

	// 이미 socket fd가 만들어진 상태이고, 이 생성자 함수에서는 fail case가 전혀 없다.
	// 따라서 public 생성자 함수로 고고고. CSuperSocket.New와 달리.
	CWebSocket::CWebSocket(CNetCoreImpl *owner, shared_ptr<WssServer::Connection> connection)
		:CSuperSocket(owner, SocketType_WebSocket)
	{
		m_webSocketSecured = connection;
	}

	// AddToSendQueue 계열 함수와 동일. 다만, WebSocket 전용이다.
	void CWebSocket::AddToSendQueueWithoutSplitterAndSignal_Copy_WebSocket(
		const CSendFragRefs &sendData)
	{
		CNetServerImpl* svr = dynamic_cast<CNetServerImpl*>(m_owner);
		ByteArrayPtr arr;
		arr.UseInternalBuffer(); // ByteArray를 쓰면 memory pooling을 못해서 성능 방해다.
		sendData.ToAssembledByteArray(arr);

		// 타입만 보낼때는 Null termination을 보장해준다.
		if (arr.GetCount() == 1)
			arr.Add(0x00);

		// Base64 Encode
		StringA strEncoded;
		Base64::Encode(arr, strEncoded);

		if (svr->m_webSocketParam.webSocketType == WebSocket_Ws)
		{
			if (m_webSocket.get() != nullptr)
			{
				// NDN 이슈중에, wsConnection를 람다캡처 안 해놓았더니 wsConnection가 invalid하는 경우가 있었다.
				// 따라서 shared_ptr인 this.m_webSocket도 람다캡처 해놓는다.
				shared_ptr<WsServer::Connection> wsConnection = m_webSocket;

				shared_ptr<WsServer::SendStream> sendStream = make_shared<WsServer::SendStream>();
				sendStream->write(strEncoded.GetString(), strEncoded.GetLength());
			
				int doneBytes = sendStream->size();
				shared_ptr<CSuperSocket> this_ptr = shared_from_this();
				
				svr->m_webSocketServer->send(wsConnection, sendStream, [this_ptr, doneBytes, svr, wsConnection](const error_code& ec) {
				if (ec)	// 보내기가 실패할 경우 해당 연결을 제거한다.
					svr->m_webSocketServer->close_connection(wsConnection, svr->m_webSocketParam.endpoint, ec);
				else
					this_ptr->m_owner->OnMessageSent(doneBytes, SocketType_WebSocket);
			});
		}
		}
		else if (svr->m_webSocketParam.webSocketType == WebSocket_Wss)
		{
			if (m_webSocketSecured.get() != nullptr)
			{
				// NDN 이슈중에, wsConnection를 람다캡처 안 해놓았더니 wsConnection가 invalid하는 경우가 있었다.
				// 따라서 shared_ptr인 this.m_webSocket도 람다캡처 해놓는다.
				shared_ptr<WssServer::Connection> wssConnection = m_webSocketSecured;

				shared_ptr<WssServer::SendStream> sendStream = make_shared<WssServer::SendStream>();
				sendStream->write(strEncoded.GetString(), strEncoded.GetLength());

				int doneBytes = sendStream->size();
				shared_ptr<CSuperSocket> this_ptr = shared_from_this();

				svr->m_webSocketServerSecured->send(wssConnection, sendStream, [this_ptr, doneBytes, svr, wssConnection](const error_code& ec) {
				if (ec)	// 보내기가 실패할 경우 해당 연결을 제거한다.
					svr->m_webSocketServerSecured->close_connection(wssConnection, svr->m_webSocketParam.endpoint, ec);
				else
					this_ptr->m_owner->OnMessageSent(doneBytes, SocketType_WebSocket);
			});
		}
	}
}
}

#endif
