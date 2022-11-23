#include "stdafx.h"
#include "NetServer.h"
#include "SendFragRefs.h"

namespace Proud
{
	void CNetServerImpl::OnConnectSuccess(CSuperSocket* socket)
	{
		// NS가 이걸 받을 일이 있나? 이 함수는 빈 상태로 둔다.
	}

	void CNetServerImpl::OnConnectFail(CSuperSocket* socket, SocketErrorCode code)
	{
		// NS가 이걸 받을 일이 있나? 이 함수는 빈 상태로 둔다.
	}

	void CNetServerImpl::OnAccepted(CSuperSocket* newSocket, AddrPort tcpLocalAddr, AddrPort tcpRemoteAddr)
	{
		// 새 연결이 성공적으로 들어왔다.
		CriticalSectionLock clk(GetCriticalSection(), true);
		int64_t currTime = GetPreciseCurrentTimeMs();

		// create unmature client. 새 remote client가 되기도 하지만
		// 연결 유지 기능의 재접속 연결일 수도 있다. 
		// 재접속 연결인 경우 재접속 인증이 완료되면 자신이 가진 socket을 
		// 원래 연결에게 건네주고 자신은 죽는다.
		//CRemoteClient_S *rc = new CRemoteClient_S(m_owner, newSocket, assignedUdpSocket, tcpRemoteAddr);
		CRemoteClient_S* rc = new CRemoteClient_S(this, newSocket, tcpRemoteAddr);

		// 주의: 여기까지 실행됐으면 이제 newSocket의 소유권은 이양.
		m_validSendReadyCandidates.Add(newSocket, rc->m_tcpLayer);

		// completion context를 세팅해야 CompletionPort 객체가 지대루 작동한다.
		//rc->m_tcpLayer->m_socket->SetCompletionContext(rc);
		//SocketToHostsMap_SetForAnyAddr(newSocket, rc);

		// 이 함수는 listening socket의 io event 처리 스레드에서 호출된다. 따라서,
		// thread pool에 assoc.
		m_netThreadPool->AssociateSocket(rc->m_tcpLayer->GetSocket());

#ifdef _DEBUG
		/* 만약 zero copy send로 설정되어 있으면 아래 구문이 블러킹이 일어날거다.
		굳이 zero copy send로 수정하고자 한다면 아래 보내기 기능도 non blocked send로 로직을 바꾸어야 하겠다. */
		{
			int sendBufSize = 0;
			if (rc->m_tcpLayer->GetSocket()->GetSendBufferSize(&sendBufSize) == 0)
			{
				assert(sendBufSize > policyFileText.GetLength() + 1);
			}
		}
#endif
		// unity나 flash의 경우를 위해, native라도 처음에는 policy file text를 맨 처음 보내는 메시지로 쓴다.
		// policy text를 non block or async send를 수행.
		// 한큐에 끝난다. 위에서 버퍼 크기가 policy test 이하임이 확인되므로.
		{
			CMessage sendMsg;
			sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
			sendMsg.Write((uint8_t*)(const char*)policyFileText, policyFileText.GetLength() + 1);

			rc->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(CSendFragRefs(sendMsg), SendOpt());
			//rc->m_tcpLayer->IssueSendOnNeed(false); // 내부 AddToSendQueueWithSplitterAndSignal_Copy 함수 내부에서 사용
		}

		{
			CMessage sendMsg;
			sendMsg.UseInternalBuffer(); // UseInternalBuffer가 obj-pool을 쓰므로 서버 실행 시간이 길어질수록 realloc 낭비가 줄어들게 됨.
			sendMsg.Write((char)MessageType_NotifyStartupEnvironment);

			if (m_simplePacketMode == false)
			{
				sendMsg.Write(m_logWriter ? true : false);
				sendMsg.Write(m_settings);
				sendMsg.Write(m_publicKeyBlob);
			}

			rc->m_tcpLayer->AddToSendQueueWithSplitterAndSignal_Copy(CSendFragRefs(sendMsg), SendOpt(), m_simplePacketMode);
		}

#if defined (_WIN32)
		// 여기서 바로 issue first recv.
		rc->m_tcpLayer->IssueRecv(true);
#endif // _WIN32

		// add it to list
		m_candidateHosts.Add(rc, rc);

		if (m_logWriter)
		{
			String txt;
			txt.Format(_PNT("new TCP socket is accepted. RemoteClient_S=%p,TCP socket=%s"),
				rc, tcpRemoteAddr.ToString().GetString());
			m_logWriter->WriteLine(0, LogCategory_System, HostID_Server, txt);
		}
	}
}
