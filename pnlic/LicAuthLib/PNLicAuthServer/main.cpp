#include "stdafx.h"
#include "PNLicAuthServer.h"

#if defined(__linux__)
#else
#include "ServiceEvent.h"
#endif

// Windows Service 등록
CServiceEvent serverEvent;

int main(int argc, char *argv[], char *envp[])
{
	int nRetCode = 0;
	
	CNTServiceStartParameter sp;
	sp.m_serviceName = L"PN License AuthServer 2";
	sp.m_serviceEvent = &serverEvent;
	CNTService::WinMain(argc, argv, envp, sp);

//	CHeldPtr<CPNLicAuthServer> server;
//
//	CStartServerParameter cp;
//	cp.m_tcpPorts.Add(g_port);
//	//cp.m_localNicAddr = g_serverIp;
//	cp.m_serverAddrAtClient = g_serverIp;
//	cp.m_localNicAddr = _PNT("0.0.0.0");
//	cp.m_protocolVersion = g_protocolVersion;
//	cp.m_encryptedMessageKeyLength = EncryptLevel_High;
//	cp.m_fastEncryptedMessageKeyLength = FastEncryptLevel_High;
//	
//	server.Attach(new CPNLicAuthServer());
//	server->Start(cp);
//	printf("success to start running server.. \n");
//
//	while (1)
//	{
//		Sleep(10);
//	}
//
//END:
//	server->Stop();
//	//server.Free();
//	server.Detach();
//
//	printf("success to finish running server.. bye.. \n");
//	getchar();

	return nRetCode;
}