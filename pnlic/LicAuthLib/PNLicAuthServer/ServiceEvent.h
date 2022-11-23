
#include "../../AuthNet/include/NTService.h"
#include "PNLicAuthServer.h"

#define EVENTLOG_SUCCESS                0x0000
#define EVENTLOG_ERROR_TYPE             0x0001
#define EVENTLOG_WARNING_TYPE           0x0002
#define EVENTLOG_INFORMATION_TYPE       0x0004
#define EVENTLOG_AUDIT_SUCCESS          0x0008
#define EVENTLOG_AUDIT_FAILURE          0x0010

void EventLog(int type, LPCWSTR txt)
{
	HANDLE system_event_handle;
	system_event_handle = RegisterEventSource(NULL, L"PN License Auth Server");
	if (system_event_handle != NULL)
	{
		ReportEvent(system_event_handle,        // �ڵ�   
			type,				        // Ÿ��   
			0,                          // ī�װ��   
			0,                          // �̺�Ʈ ID   
			NULL,                       // ����� SID �ּ�   
			1,                          // ���ڿ� ��   
			0,                          // ������ ũ��   
			&txt,                       // ���ڿ�����  ���� �ּ�    
			NULL                        // ������ �ּ�   
			);

		DeregisterEventSource(system_event_handle);
	}
}

class CServiceEvent : public INTServiceEvent
{
private:
	CHeldPtr<CPNLicAuthServer> m_server;

public:
	CServiceEvent(){}
	~CServiceEvent(){}

	virtual void Log(int type, LPCWSTR text) PN_OVERRIDE
	{
		_tprintf(L"LOG[Type : %d] : %s", type, text);
		EventLog(type, text);
	}

	virtual void Pause() PN_OVERRIDE
	{
		m_server.Free();
	}

	virtual void Stop() PN_OVERRIDE
	{
		m_server.Free();
	}

	virtual void Run() PN_OVERRIDE
	{
		//���� ��ü ����
		CStartServerParameter p;
		p.m_tcpPorts.Add(g_port);
		p.m_serverAddrAtClient = g_serverIp;
		p.m_localNicAddr = _PNT("0.0.0.0");
		p.m_protocolVersion = g_protocolVersion;
		p.m_encryptedMessageKeyLength = EncryptLevel_High;
		p.m_fastEncryptedMessageKeyLength = FastEncryptLevel_High;

		m_server.Attach(new CPNLicAuthServer);
		m_server->Start(p);

		while (1)
		{
			MSG msg;
			// �ִ� ���� ª�� �ð�����, �ܼ� �Է�, ���� �޽��� ����, ���� ������ ���� �� �ϳ��� ��ٸ���.
			MsgWaitForMultipleObjects(0, 0, TRUE, 100, QS_ALLEVENTS);
			if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
			{
				if (!GetMessage(&msg, NULL, NULL, NULL))
					break;

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	virtual void Continue() override
	{
		//���� ��ü ����
		CStartServerParameter p;
		p.m_tcpPorts.Add(g_port);
		p.m_serverAddrAtClient = g_serverIp;
		p.m_localNicAddr = _PNT("0.0.0.0");
		p.m_protocolVersion = g_protocolVersion;
		p.m_encryptedMessageKeyLength = EncryptLevel_High;
		p.m_fastEncryptedMessageKeyLength = FastEncryptLevel_High;
		
		m_server->Start(p);
	}
};
