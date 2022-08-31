#include "stdafx.h"
#include <stack>
#include "NetServer.h"
#include "ReportError.h"
#include "LoopbackHost.h"

#include "quicksort.h"
#include "../include/ClassBehavior.h"

#ifdef SUPPORTS_WEBGL

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "SendOpt.h"
#include "Base64.h"

#ifdef _MSC_VER
#pragma warning (disable:4996)
#endif

namespace Proud
{
	void CNetServerImpl::Start_WebSocketServer(CStartServerParameter &param)
	{
		try
		{
			m_webSocketParam = param.m_webSocketParam;

			// 웹소켓 생성
			if (m_webSocketParam.webSocketType == WebSocket_Ws)
			{
				m_webSocketServer = make_shared<WsServer>();
				m_webSocketServer->config.port = m_webSocketParam.listenPort;
				m_webSocketServer->config.thread_pool_size = m_webSocketParam.threadCount;
				m_webSocketServer->config.timeout_request = m_webSocketParam.timeoutRequest;
				m_webSocketServer->config.timeout_idle = m_webSocketParam.timeoutIdle;


				string endpoint = StringT2A(m_webSocketParam.endpoint).GetString();

				m_webSocketServer->config.address = StringT2A(param.m_localNicAddr).GetString();
				m_webSocketServer->config.reuse_address = false;

				// 콜백 정의
				DefineWebSocketCallbacks<WsServer::Connection, WsServer::Message, WsServer::Endpoint>(m_webSocketServer->endpoint[endpoint], m_WebSocketConnToClientMap);
			}
			else if (m_webSocketParam.webSocketType == WebSocket_Wss)
			{
				string certFile = StringT2A(m_webSocketParam.certFile).GetString();
				string privateKeyFile = StringT2A(m_webSocketParam.privateKeyFile).GetString();

				// 인증서 파일이 존재하는지, 읽기 권한이 있는지 확인
				if (access(certFile.c_str(), 4 | 0) != 0 || access(privateKeyFile.c_str(), 4 | 0) != 0)
					throw Exception("Specified path for Certification file or Private key file doesn't exist! Or No permission to those files.");

				m_webSocketServerSecured = make_shared<WssServer>(certFile, privateKeyFile);

				m_webSocketServerSecured->config.port = m_webSocketParam.listenPort;
				m_webSocketServerSecured->config.thread_pool_size = m_webSocketParam.threadCount;
				m_webSocketServerSecured->config.timeout_request = m_webSocketParam.timeoutRequest;
				m_webSocketServerSecured->config.timeout_idle = m_webSocketParam.timeoutIdle;

				string endpoint = StringT2A(m_webSocketParam.endpoint).GetString();

				m_webSocketServerSecured->config.address = StringT2A(param.m_localNicAddr).GetString();
				m_webSocketServerSecured->config.reuse_address = false;

				// 콜백 정의
				DefineWebSocketCallbacks<WssServer::Connection, WssServer::Message, WssServer::Endpoint>(m_webSocketServerSecured->endpoint[endpoint], m_WebSocketSecuredConnToClientMap);
			}

			// 웹서버 시작
			m_webSocketServerThread = thread([this]() {
				if (m_webSocketParam.webSocketType == WebSocket_Ws)
					m_webSocketServer->start();
				else if (m_webSocketParam.webSocketType == WebSocket_Wss)
					m_webSocketServerSecured->start();
			});
		}
		catch (Exception& e)
		{
			// 포트 바인드가 실패하거나, wss의 경우 인증서관련 문제가 있으면 들어올것이다.
			if (m_webSocketParam.webSocketType == WebSocket_Ws)
				m_webSocketServer.reset();
			else if (m_webSocketParam.webSocketType == WebSocket_Wss)
				m_webSocketServerSecured.reset();

			throw e;
		}
	}

	void CNetServerImpl::NotifyStartupEnvironment(shared_ptr<CRemoteClient_S> rc)
	{
		SocketToHostsMap_SetForAnyAddr(rc->m_tcpLayer, rc);

		// 서버의 환경설정을 보낸다.
		CMessage sendMsg;
		sendMsg.UseInternalBuffer();
		Message_Write(sendMsg, MessageType_NotifyStartupEnvironment);

		if (m_simplePacketMode == false)
		{
			sendMsg.Write(m_logWriter ? true : false);
			Message_Write(sendMsg, m_settings);
			// 공개키는 사용할 일이 없으므로 보내지 않는다.

			sendMsg.Write((int)GetPreciseCurrentTimeMs());
		}

		rc->m_tcpLayer->AddToSendQueueWithoutSplitterAndSignal_Copy(
			rc->m_tcpLayer,
			CSendFragRefs(sendMsg));

		m_candidateHosts.Add(rc.get(), rc);
	}
}

#endif
