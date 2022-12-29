

#include "stdafx.h"

CHttpConnector::CHttpConnector() :m_HttpText(),
m_Socket(),
m_SocketDg(),
m_httpProgress(eHttp_End)
{

}

CHttpConnector::~CHttpConnector()
{

}

bool CHttpConnector::Request(String strServerName, StringA requestMethod, StringA strSend)
{
	if (eHttp_End != m_httpProgress)
		return false;

	m_strServerName = strServerName;
	m_HttpText = requestMethod + " http://" + StringW2A(strServerName) + strSend + "\n";
	m_httpProgress = eHttp_None;

	return true;
}

// Update http connection by level.
// http ������ �ܰ躰�� ������Ʈ�Ѵ�.
void CHttpConnector::Update(float fDelta)
{
	switch (m_httpProgress)
	{
	case eHttp_None:
		Progress_None();
		break;
	case eHttp_Connecting:
		Progress_Connecting();
		break;;
	case eHttp_Sending:
		Progress_Sending();
		break;
	case eHttp_Receiving:
		Progress_Receiving();
		break;
	case eHttp_End:
		break;
	}
}

// Try Connect.
// Connect�� �õ��Ѵ�.
bool CHttpConnector::TryConnect()
{
	SocketErrorCode r = m_Socket->Connect(m_strServerName, 80);

	if (r != SocketErrorCode_Ok && r != SocketErrorCode_WouldBlock)
	{
		printf("Connect failed.");
		return false;
	}
	return true;
}

void CHttpConnector::Send()
{
	m_Socket->IssueSend((BYTE*)((m_HttpText.GetString() + m_sendProgress)), m_HttpText.GetLength() + 1 - m_sendProgress);
}

// Create Socket and Try Bind, Connect
// Socket�� �����ϰ� Bind, Connect �õ��� �Ѵ�.
void CHttpConnector::Progress_None()
{
	m_Socket = SocketPtr(CSocket::New(SocketType_Tcp, &m_SocketDg));

	if (!m_Socket->Bind())
	{
		printf("Bind failed.");
		m_Socket = SocketPtr();
		m_httpProgress = eHttp_End;
		return;
	}

	m_Socket->SetBlockingMode(false);

	if (!TryConnect())
	{
		m_Socket = SocketPtr();
		m_httpProgress = eHttp_End;
		return;
	}

	printf("Connect succeed.\n");
	m_httpProgress = eHttp_Connecting;
}

// If it connected then send a packet otherwise send a packet again.
// ������ �Ǿ��ٸ� ��Ŷ�� ������, ������ �ȵǾ��ٸ� �ٽ� ��Ŷ�� ������.
void CHttpConnector::Progress_Connecting()
{
	SocketSelectContext* selectContext = SocketSelectContext::New();
	selectContext->AddWriteWaiter(*m_Socket);
	selectContext->AddExceptionWaiter(*m_Socket);

	selectContext->Wait(0);

	SocketErrorCode code;
	bool did = selectContext->GetConnectResult(*m_Socket, code);
	if (did)
	{
		if (code == SocketErrorCode_Ok)
		{
			printf("Connect complete.\n");
			// Send a packet because it is connected.
			// ����Ǿ����Ƿ� ��Ŷ�� ������.
			m_Socket->SetBlockingMode(true);

			m_sendProgress = 0;

			m_httpProgress = eHttp_Sending;

			// Do Send
			// Send �Ѵ�.
			Send();
		}
		else
		{
			if (!TryConnect())
			{
				printf("Connection re-try is failed.");
				m_Socket = SocketPtr();
				m_httpProgress = eHttp_End;
			}
		}
	}

	delete selectContext;
}

// Do receiving process
// �۽� ó���� �Ѵ�.
void CHttpConnector::Progress_Sending()
{
	OverlappedResult oresult;
	if (m_Socket->GetSendOverlappedResult(false, oresult) == false)
	{
		// Still waiting
		//���� �����
		return;
	}

	if (oresult.m_errorCode != SocketErrorCode_Ok || oresult.m_completedDataLength == 0)
	{
		// It is meaning of disconnecting TCP. So process it as error.
		// TCP������ �������� �ǹ��Ѵ�. ���� ����ó���Ѵ�.
		printf("Socket Error.");
		m_httpProgress = eHttp_End;
		m_Socket = SocketPtr();
		return;
	}

	// Try next sending.
	// ���� �۽��� �Ǵ�.
	m_sendProgress += oresult.m_completedDataLength;
	if (m_sendProgress >= m_HttpText.GetLength() + 1)
	{
		m_ReceiveBuffer.Clear();
		m_ReceiveBuffer.SetCount(4096);
		ZeroMemory(m_ReceiveBuffer.GetData(), m_ReceiveBuffer.Count);

		// Process it as received.
		//����ó���� �Ѵ�.
		if (m_Socket->IssueRecv(m_ReceiveBuffer.Count - 1) == SocketErrorCode_Ok)
		{
			printf("Send succeed. Now receiving.\n");
			m_httpProgress = eHttp_Receiving;
		}
		else
		{
			printf("Receive failed.");
			m_Socket = SocketPtr();
			m_httpProgress = eHttp_End;
		}
	}
	else
		Send();

}

// Process it as received.
// ���� ó���� �Ѵ�.
void CHttpConnector::Progress_Receiving()
{
	OverlappedResult oresult;
	if (m_Socket->GetRecvOverlappedResult(false, oresult) == false)
	{
		return;
	}

	if (oresult.m_errorCode != SocketErrorCode_Ok || oresult.m_completedDataLength == 0)
	{
		printf("Socket Error.");
		m_httpProgress = eHttp_End;
		m_Socket = SocketPtr();
		return;
	}

	StringA strRecv;
	StrBufA RecvBuf(strRecv, oresult.m_completedDataLength + 2);
	memcpy(RecvBuf, m_Socket->GetRecvBufferPtr(), oresult.m_completedDataLength);
	RecvBuf[oresult.m_completedDataLength] = 0;

	printf("receive complete.\n");
	printf("=============================================================\n");
	printf("%s\n", strRecv.GetString());
	printf("=============================================================\n");

	m_httpProgress = eHttp_End;
	m_Socket = SocketPtr();
}
