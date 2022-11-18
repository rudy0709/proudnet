/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#if defined(_WIN32)

#include "stdafx.h"

#include "../include/coinit.h"
#include "stlutil.h"
#include "DumpServer.h"
#include "../include/ThreadUtil.h"
#include "DumpC2S_stub.cpp"
#include "Dumps2c_proxy.cpp"
#include <conio.h>
#include "RmiContextImpl.h"
#include <comdef.h>

using namespace std;

namespace Proud
{

CDumpServerImpl::CDumpServerImpl(IDumpServerDelegate* dg): m_dg(dg)
{

	m_server.Attach(CNetServer::Create());
	m_server->AttachProxy(&m_s2cProxy);
	m_server->AttachStub(this);
	m_server->SetEventSink(this);
}

CDumpServerImpl::~CDumpServerImpl(void)
{
}

void CDumpServerImpl::RunMainLoop()
{
	CCoInitializer coi;

	// 시작
	CStartServerParameter sp;
	m_dg->OnStartServer(sp);

	sp.m_protocolVersion = Guid(DumpProtocolVersion);
	// 스레드 풀 갯수를 충분히 둔다. 덤프 파일을 기록하는 루틴이 오래 걸리기 때문이다.
	sp.m_threadCount = GetNoofProcessors() * 3;

	// 2009.12.02 add by ulelio : dump server에서는 udp를 사용하지 않는다.
	m_server->SetDefaultFallbackMethod(FallbackMethod_ServerUdpToTcp);

	try
	{
		m_server->Start(sp);
	}
	catch (Proud::Exception &e)
	{
		m_dg->OnServerStartComplete(e.m_errorInfoSource);
	}

	m_dg->OnServerStartComplete(ErrorInfoPtr());
	
	while (m_dg->MustStopNow() == false)
	{
		try
		{
			//m_elapsedTime = (float)m_timer->GetElapsedTimeMs();
			FrameMove();
		}
		catch (_com_error &e)
		{
			e;
			//WARN_COMEXCEPTION(e);
		}
		Sleep(1);
	}
}

//void CDumpServerImpl::ProcessDisconnectedClient(const CNetPeerInfo& peer)
//{
//	//CriticalSectionLock lock(*m_dg->GetCriticalSection(),TRUE);

//	//CDumpCliPtr_S c=GetClientByHostID(clientInfo->m_HostID);
//	//if(c)
//	//{
//	//}
//}

void CDumpServerImpl::FrameMove()
{
	CriticalSectionLock lock(*m_dg->GetCriticalSection(), true);
	m_dg->OnFrameMove();
}

void CDumpServerImpl::OnClientJoin(CNetClientInfo *clientInfo)
{
	CriticalSectionLock lock(*m_dg->GetCriticalSection(), true);

	CDumpCliPtr_S newCl(new CDumpCli_S(this, clientInfo->m_HostID));
	m_clients.insert(CDumpClients::value_type(clientInfo->m_HostID, newCl));
}

void CDumpServerImpl::OnClientLeave(CNetClientInfo *clientInfo, ErrorInfo *errorinfo, const ByteArray& comment)
{
	CriticalSectionLock lock(*m_dg->GetCriticalSection(), true);
	//ProcessDisconnectedClient(peer);

	m_clients.erase(clientInfo->m_HostID);
}

CDumpCliPtr_S CDumpServerImpl::GetClientByHostID(HostID client)
{
	CriticalSectionLock lock(*m_dg->GetCriticalSection(), 1);

	return smartptrmap_find(m_clients, client);
}

// float CDumpServerImpl::GetElapsedTime()
// {
// 	return m_elapsedTime;
// }

int CDumpServerImpl::GetUserNum()
{
	CriticalSectionLock lock(*m_dg->GetCriticalSection(), 1);
	return (int)m_clients.size();
}

CDumpCli_S::CDumpCli_S(CDumpServerImpl* server, HostID hostID)
	: m_server(server)
	, m_HostID(hostID)
	, m_fileHandle(INVALID_HANDLE_VALUE)
{
}

CDumpCli_S::~CDumpCli_S(void)
{
	if (m_fileHandle != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(m_fileHandle);
		m_fileHandle = INVALID_HANDLE_VALUE;
	}
}

bool CDumpServerImpl::Dump_Start(Proud::HostID remote, Proud::RmiContext &rmiContext)
{
	CriticalSectionLock lock(*m_dg->GetCriticalSection(), 1);
	// 아직 파일 생성중이 아니면 파일을 생성한다.
	CDumpCliPtr_S cli = GetClientByHostID(remote);
	if (cli && (cli->m_fileHandle == INVALID_HANDLE_VALUE))
	{
		CNetClientInfo ClientInfo;
		if (m_server->GetClientInfo(remote, ClientInfo))
		{
			// lock은 피한다. 오래 걸리니까.
			lock.Unlock();

			String filePath = m_dg->GetDumpFilePath(remote, ClientInfo.m_TcpAddrFromServer, CPnTime::GetTickCount());

			// non MFC class를 이용해서 파일에 기록한다.

			const HANDLE fileHandle(::CreateFile(filePath, FILE_WRITE_DATA, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
			if (fileHandle == INVALID_HANDLE_VALUE)
			{
				// 특별히 에러를 발생하지는 않고, 그냥 리턴한다. 이후의 메시지는 모두 버린다.
				return true;
			}

			lock.Lock();
			// 생성한 파일을 연결시킨다.
			cli = GetClientByHostID(remote);
			if (cli)
			{
				cli->m_fileHandle = fileHandle;
			}
		}
	}
	return true;
}

bool CDumpServerImpl::Dump_Chunk(HostID remote, Proud::RmiContext &rmiContext, const ByteArray &chunk)
{
	CriticalSectionLock lock(*m_dg->GetCriticalSection(), 1);

	// 이미 파일을 생성한 상태이어야 한다.
	CDumpCliPtr_S cli = GetClientByHostID(remote);
	if (cli && (cli->m_fileHandle != INVALID_HANDLE_VALUE))
	{
		CNetClientInfo ClientInfo;
		if (m_server->GetClientInfo(remote, ClientInfo))
		{
			// ack보내기
			m_s2cProxy.Dump_ChunkAck(remote, g_ReliableSendForPN);
			// lock은 피한다. 오래 걸리니까.
			lock.Unlock();

			DWORD fileWritten;
			::WriteFile(cli->m_fileHandle, chunk.GetData(), chunk.GetCount(), &fileWritten, NULL);
		}
	}
	return true;
}

bool CDumpServerImpl::Dump_End(HostID remote, Proud::RmiContext &rmiContext)
{
	CriticalSectionLock lock(*m_dg->GetCriticalSection(), 1);

	// 파일을 닫는다. 더 이상 처리할 것이 없다.
	CDumpCliPtr_S cli = GetClientByHostID(remote);
	if (cli && (cli->m_fileHandle != INVALID_HANDLE_VALUE))
	{
		CNetClientInfo ClientInfo;
		if (m_server->GetClientInfo(remote, ClientInfo))
		{
			::CloseHandle(cli->m_fileHandle); // 파일을 닫는다.
		}
	}
	return true;
}

CDumpServer* CDumpServer::Create(IDumpServerDelegate* dg)
{
	return new CDumpServerImpl(dg);
}

}

#endif // _WIN32
