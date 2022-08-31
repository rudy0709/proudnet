/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "DumpClient.h"

#if defined(_WIN32)
	#include "RmiContextImpl.h"
	#include "pidl/dumps2c_stub.cpp"
	#include "pidl/dumpc2s_proxy.cpp"
#endif

namespace Proud
{

#if defined(_WIN32)
	CDumpClientImpl::CDumpClientImpl(IDumpClientDelegate* dg)
		: m_dg(dg)
		, m_fileHandle(INVALID_HANDLE_VALUE)
	{
		m_sendProgress=0;
		m_sendTotal=1;

		m_state=Connecting;
		m_stopNow=false;

		m_client.Attach(CNetClient::Create());
		m_client->AttachProxy(&m_c2sProxy);
		m_client->AttachStub(this);
		m_client->SetEventSink(this);

	}

	CDumpClientImpl::~CDumpClientImpl(void)
	{
		if (m_fileHandle == INVALID_HANDLE_VALUE)
		{
			::CloseHandle(m_fileHandle);
			m_fileHandle = INVALID_HANDLE_VALUE;
		}
	}

	void CDumpClientImpl::Start( String serverAddr,uint16_t serverPort,String filePath )
	{
		CNetConnectionParam p;
		p.m_protocolVersion = Guid(DumpProtocolVersion);
		p.m_serverIP=serverAddr;
		p.m_serverPort=serverPort;

		m_fileHandle = ::CreateFile(filePath.GetString(), FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (m_fileHandle == INVALID_HANDLE_VALUE)
		{
			Exception exception("Failed to Open the Dump File");
			m_dg->OnException(exception);
			return;
		}

		if (!m_client->Connect(p))
		{
			Exception exception("Failed to connect the Dump Server");
			m_dg->OnException(exception);
		}
	}

	void CDumpClientImpl::FrameMove()
	{
		m_client->FrameMove();
	}

	void CDumpClientImpl::OnJoinServerComplete(ErrorInfo *info, const ByteArray &/*replyFromServer*/)
	{
		if (info->m_errorType != ErrorType_Ok)
		{
			Exception exception("Failed to connect the Dump Server");
			m_dg->OnException(exception);
		}
		else
		{
			m_state = Sending;
			m_c2sProxy.Dump_Start(HostID_Server, g_ReliableSendForPN);
			m_sendProgress = 0;

			ULARGE_INTEGER fileSize;
			fileSize.LowPart = ::GetFileSize(m_fileHandle, &fileSize.HighPart);
			if (fileSize.LowPart != INVALID_FILE_SIZE)
				m_sendTotal = (int)fileSize.QuadPart;

			SendNextChunk();
		}
	}

	bool CDumpClientImpl::Dump_ChunkAck(HostID /*remote*/, Proud::RmiContext &/*rmiContext*/)
	{
		SendNextChunk();
		return true;
	}

	bool CDumpClientImpl::SendNextChunk()
	{
		ByteArray ba;
		int chunkLength = PNMAX(m_client->GetMessageMaxLength() - 100, 4096);
		ba.SetCount(chunkLength);

		DWORD bytesRead;
		if (::ReadFile(m_fileHandle, ba.GetData(), (DWORD)ba.GetCount(), &bytesRead, NULL) == FALSE)
		{
			Exception exception("Failed to Read the Dump File");
			m_dg->OnException(exception);
			return false;
		}

		ba.SetCount(bytesRead);
		if (bytesRead == 0)
		{
			m_dg->OnComplete();
			m_c2sProxy.Dump_End(HostID_Server,g_ReliableSendForPN);
			m_state=Closing;
			m_client->Disconnect();
			m_state=Stopped;
			return false;
		}
		else
		{
			m_c2sProxy.Dump_Chunk(HostID_Server,g_ReliableSendForPN,ba);
			m_sendProgress += bytesRead;
			return true;
		}
	}

	void CDumpClientImpl::OnError(ErrorInfo *errorInfo)
	{
		m_dg->OnException(Exception(errorInfo->ToString().GetString()));
	}

	void CDumpClientImpl::OnLeaveServer(ErrorInfo*)
	{
		m_state = Stopped;
	}

	void CDumpClientImpl::OnP2PMemberJoin(HostID /*memberID*/, HostID /*groupID*/, int /*memberNum*/, const ByteArray &/*message*/)
	{
	}

	void CDumpClientImpl::OnP2PMemberLeave(HostID /*memberID*/, HostID /*groupID*/, int /*memberNum*/)
	{
	}

	CDumpClient::State CDumpClientImpl::GetState()
	{
		return m_state;
	}

	int CDumpClientImpl::GetSendProgress()
	{
		return m_sendProgress;
	}

	int CDumpClientImpl::GetSendTotal()
	{
		return m_sendTotal;
	}

	CDumpClient* CDumpClient::Create(IDumpClientDelegate* dg)
	{
		return new CDumpClientImpl(dg);
	}
#endif
}
