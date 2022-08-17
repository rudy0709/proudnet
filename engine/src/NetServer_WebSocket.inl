#pragma once 

#ifdef SUPPORTS_WEBGL

#include "WebSocket.h"

namespace Proud
{
	// C++ websocket server lib에서의 콜백 함수들이다. 수신,소켓연결 받음,해제 등을 처리한다.
	template<typename ConnectionT, typename MessageT, typename EndPointT>
	void CNetServerImpl::DefineWebSocketCallbacks(EndPointT& endpoint, std::map<shared_ptr<ConnectionT>, shared_ptr<CRemoteClient_S>>& connectionMap)
	{
		endpoint.on_open = [this, &connectionMap](shared_ptr<ConnectionT> connection) {

			// websocket accept 을 처리한다.

			CriticalSectionLock clk(GetCriticalSection(), true);

			// 접속한 클라이언트의 IP
			AddrPort endPoint_AddrPort;
			String remoteAddress = StringA2T(connection->remote_endpoint_address);
			if (remoteAddress.Find('.') > -1)
				endPoint_AddrPort = AddrPort::FromIPPortV4(remoteAddress, connection->remote_endpoint_port);	// IPv4
			else
				endPoint_AddrPort = AddrPort::FromIPPortV6(remoteAddress, connection->remote_endpoint_port);	// IPv6

			shared_ptr<CSuperSocket> superSocket = make_shared<CWebSocket>(this, connection);
			shared_ptr<CRemoteClient_S> rc = make_shared<CRemoteClient_S>(
				this,
				superSocket,
				endPoint_AddrPort,
				m_settings.m_defaultTimeoutTimeMs,
				m_settings.m_autoConnectionRecoveryTimeoutTimeMs);

			NotifyStartupEnvironment(rc);


			if (m_logWriter)
			{
				Tstringstream ss;
				ss << _PNT("new Websocket is accpted. RemoteClient_S=[") << rc;
				ss << _PNT("], WebSocket=[") << connection->remote_endpoint_address.c_str() << _PNT(":") << connection->remote_endpoint_port << _PNT("]");
				m_logWriter->WriteLine(0, LogCategory_System, HostID_Server, ss.str().c_str());
			}

			{
				CriticalSectionLock lock2(m_webSocketConnToClientMapCritSec, true);
				// connecion으로 supersocket을 찾을수 있도록 한다.
				connectionMap.emplace(make_pair(connection, rc/*superSocket*/));
			}
		};

		endpoint.on_message = [this, &connectionMap](shared_ptr<ConnectionT> connection, shared_ptr<MessageT> message) {

			// websocket의 메시지 수신 처리.
			// MessageType_XXX부터 시작하는 내용이다.

			string msg = message->string();

			// m_webSocketConnToClientMapCritSec lock 후에
			CriticalSectionLock lock2(m_webSocketConnToClientMapCritSec, true);

			// websocket to SuperSocket map에서 lookup하자. 
			auto iter = connectionMap.find(connection);
			if (iter != connectionMap.end())
			{
				// lookup 후 결과물은 shared_ptr이므로 로컬 변수로 갖고 있는다. 따라서 
				// m_webSocketConnToClientMapCritSec는 unlock 해놔도 된다.
				lock2.Unlock();

				// 이 상태에서 나머지 처리를 하자. main lock은 하지 말자. main lock이나 supersocket lock은 필요하면 OnMessageReceived 안에서 알아서 할거다.

				CReceivedMessageList msgList;

				msgList.AddTail();
				CReceivedMessage& ri = msgList.GetTail();
				ri.m_unsafeMessage.UseInternalBuffer();
				ri.m_unsafeMessage.Write((const uint8_t*)msg.c_str(), msg.size());
				
				OnMessageReceived(msg.length(), msgList, iter->second->m_tcpLayer);
			}
		};

		endpoint.on_error = [this](shared_ptr<ConnectionT> connection, const error_code& ec) {

			// 에러를 핸들링

			if (m_logWriter)
			{
				Tstringstream ss;
				ss << _PNT("Websocket error. Websocket=[") << connection->remote_endpoint_address.c_str() << _PNT(":") << connection->remote_endpoint_port << _PNT("], ");
				ss << _PNT("Error=[") << ec.value() << _PNT(":") << ec.message().c_str() << _PNT("]");
				m_logWriter->WriteLine(0, LogCategory_System, HostID_Server, ss.str().c_str());
			}
		};

		endpoint.on_close = [this, &connectionMap](shared_ptr<ConnectionT> connection, int /*status*/, const string& /*reason*/) {

			// 연결 해제를 핸들링

			CriticalSectionLock clk(GetCriticalSection(), true);

			CriticalSectionLock lock2(m_webSocketConnToClientMapCritSec, true);
			auto iter = connectionMap.find(connection);
			if (iter != connectionMap.end())
			{
				// ACR이 없다. garbage한다.
				GarbageHost(iter->second,
					ErrorType_DisconnectFromRemote,
					ErrorType_TCPConnectFailure,
					iter->second->m_shutdownComment,
					_PNT("DWSC"),
					SocketErrorCode_Ok);
			}

			connectionMap.erase(connection);
		};
	}
}

#endif /* SUPPORTS_WEBGL */