#include "stdafx.h"
#include "PNLicAuthClient.h"


#include "../../PNLicenseManager/LicenseMessage.inl"

#include "../PNLicAuthCommon/pidl/C2S_common.cpp"
#include "../PNLicAuthCommon/pidl/S2C_common.cpp"
#include "../PNLicAuthCommon/pidl/C2S_stub.cpp"
#include "../PNLicAuthCommon/pidl/S2C_proxy.cpp"

#include "../PNLicAuthCommon/pidl/C2S_proxy.cpp"
#include "../PNLicAuthCommon/pidl/S2C_stub.cpp"
#include <regex>

#if defined(__linux__)
#define LANG_UNKNOWN 0x00
#define LANG_KOREAN 0x12
int GetAppLang()
{
	char appLang[3] = { 0 };
	char *sysLang = getenv("LANG");

	if (sysLang == NULL)
	{
		return LANG_UNKNOWN;
	}

	strncpy(appLang, sysLang, 2);
	if (strcmp(appLang, "ko") == 0)
	{
		return LANG_KOREAN;
	}

	return LANG_UNKNOWN;
}
#endif

CPNLicAuthClient::CPNLicAuthClient(void)
{
	m_isConnectedToAuthServer = false;
	//m_logWriter = CLogWriter::New(StringT2A("D:\\Temp\\AuthLog\\AuthClientLog"), 10000);
}

CPNLicAuthClient::~CPNLicAuthClient(void)
{
	if (m_clientLM != NULL)
		m_clientLM.Free();
}

DEFRMI_LicenseS2C_NotifyAuthenticationComment(CPNLicAuthClient)
{
	OnAuthenticationHandler((LicenseMessageType)result);
	return true;
}

DEFRMI_LicenseS2C_NotifyAuthentication(CPNLicAuthClient)
{
	OnAuthenticationHandler((LicenseMessageType)result);
	return true;
}

void CPNLicAuthClient::OnAuthenticationHandler(LicenseMessageType resultType)
{
	if (resultType == LMType_RegSuccess) {
		OnAuthenticationSucceed(resultType);
	}
	else {
		OnAuthenticationFailed(resultType);
	}
}

bool CPNLicAuthClient::IsConnectedToAuthServer()
{
	return m_isConnectedToAuthServer;
}

void CPNLicAuthClient::Connect(Proud::String serverIP, int serverPort)
{
	CNetConnectionParam cp;
	cp.m_serverIP = serverIP;
	cp.m_serverPort = serverPort;
	cp.m_protocolVersion = g_protocolVersion;

	/*@@주의 사항
	윈도우즈에서는 이노셋업을 통한 온라인 인증시 설치 프로세스가 종료 되지 않는 문제가 있기 때문에 싱글 모드로 동작을 시킵니다.
	*/
#if defined(_WIN32)
	cp.m_netWorkerThreadModel = ThreadModel_SingleThreaded;
	cp.m_userWorkerThreadModel = ThreadModel_SingleThreaded;
#endif

	m_clientLM = CNetClient::Create();
	if (m_clientLM == NULL) {
		return; // throw Exception("cannot create authclient.");
	}

	m_clientLM->SetEventSink(this);
	m_clientLM->AttachProxy(&m_c2sProxy);
	m_clientLM->AttachStub(this);
	m_clientLM->Connect(cp);
}

void CPNLicAuthClient::OnJoinServerComplete(ErrorInfo *info, const ByteArray &replyFromServer)
{
#if defined(_DEBUG) || defined(DEBUG) 
	cout << "success to get OnJoinServerComplete(" << info->m_errorType << ")" << endl;
#endif

	if (info->m_errorType != ErrorType_Ok) {
		OnAuthenticationHandler(LMType_AuthServerConnectionFailed);
		return;
	}

	this->m_isConnectedToAuthServer = true;

	OnRequestAuthentication();
}

void CPNLicAuthClient::OnLeaveServer(ErrorInfo *info)
{
	//if (info->m_errorType != ErrorType_Ok) {

	//	//throw Exception("closed by authserver. (errorcode: %d)", info->m_errorType);

	//}
#if defined(_DEBUG) || defined(DEBUG) 
	printf("success to get OnLeaveServer. \n");
#endif

}

void CPNLicAuthClient::Disconnect()
{
	if (m_clientLM != NULL) {
		m_clientLM->Disconnect();
	}
#if defined(_DEBUG) || defined(DEBUG) 
	printf("success to disconnect. \n");
#endif
}

void CPNLicAuthClient::RequestAuthentication(String cpuId, String computerName, String userName, String key, String internalIP, String licenseType)
{
	m_c2sProxy.RequestAuthentication(HostID_Server, RmiContext::SecureReliableSend, cpuId, computerName, userName, key, internalIP, licenseType);
#if defined(_DEBUG) || defined(DEBUG) 
	cout << "success to request to activate online authorization. " << endl;
#endif
}

String CPNLicAuthClient::GetInternalIP()
{
	if (m_clientLM == NULL) {
		return _PNT("");
	}
	return m_clientLM->GetTcpLocalAddr().ToDottedIP();
}

bool CPNLicAuthClient::AuthorizationServer(String serverIP, int port)
{
	this->Connect(serverIP, port);
	return true;
}

void CPNLicAuthClient::FrameMove()
{
	m_clientLM->FrameMove();
}

void CPNLicAuthClient::AuthorizationBlocking(String licensekey, String licensetype, String &resultMessage, LicenseMessageType &authorized, bool &isComplete)
{
	/* 이 함수는 장시간 블러킹하면서 작동한다. 하지만 내부에 메시지 루프가 존재한다.
	메시지 루프는 SC_CLOSE 윈도메시지를 받으면 처리를 cancel하고 이 함수를 나간다.
	modeless dialog box로 만들고 처리하는 것이 더 나은 방식일 수도 있지만,
	설치앱(.iss로 컴파일되는 앱)에서 델파이 기반 컨트롤을 생성 후 이 함수를 DLL함수로서 호출하는데,
	그 설치앱이 윈도 메시지를 처리해줘야 하므로 이 함수가 내부에 메시지 루프가 존재한다.
	즉 설치앱 자체가 가진 메시지 루프가 호출되지 못하므로 여기서 대신 호출하는 것이다.
	흥미롭게도 잘 작동한다.
	*/

	// 언어 설정
#if defined(__linux__)
	int langid = GetAppLang();
#else
	LANGID id = ::GetSystemDefaultLangID();
	LANGID langid = PRIMARYLANGID(id);
	setlocale(LC_ALL, "");
#endif

	authorized = LMType_Incorrect;
	isComplete = false;
	bool isFirst = true; //호스트네임 실패시  

	this->OnAuthenticationSucceed = [&isComplete, &licensekey, &licensetype, &authorized, &resultMessage](LicenseMessageType authResultType)
	{
		//// 서버에서 인증이 성공했다고 응답이 오면, 비로소 키를 설치한다.

		CPNErrorInfo errorInfo;

		//if (PNRegSmartKeyInstallToRegistry(licensekey, errorInfo) == false) {
		//	//throw Exception("online activated, but cannot install activation key.");
		//	printf("failed to activate online authorization, but cannot install activation key. \n");
		//	return false;
		//}

		authorized = authResultType;
		isComplete = true;
#if defined(_DEBUG) || defined(DEBUG) 
		cout << "success to activate online authorization. (" << authorized << ")" << endl;
#endif
	};

	this->OnAuthenticationFailed = [&isComplete, this, &isFirst, &licensekey, &licensetype, &authorized, &resultMessage](LicenseMessageType authResultType)
	{
		// 인증 과정이 실패했는데, 연결 실패로 인한 것이라면, IP로 재시도를 한다.
		// 그래도 안되면 말짱 꽝 처리.
		if (authResultType == LMType_AuthServerConnectionFailed)
		{
			if (isFirst == true) {
				isFirst = false;
				this->AuthorizationServer(g_serverIp, g_port);
			}
			else {
				authorized = LMType_AuthServerConnectionFailed;
				isComplete = true;
			}
		}
		else
		{
			authorized = authResultType;
			isComplete = true;
		}
#if defined(_DEBUG) || defined(DEBUG) 
		cout << "failed to activate online authorization. (errorcode: " << authResultType << ")" << endl;
#endif
	};

	this->OnRequestAuthentication = [this, &licensekey, &licensetype]()
	{
		String strCpuId = _PNT("");
		String strComName = _PNT("");
		String strUserName = _PNT("");

		PNCPUID cpuid(0);
		strCpuId.Format(_PNT("%d %d %d %d"), cpuid.eax(), cpuid.ebx(), cpuid.ecx(), cpuid.edx());

#if defined(__linux__)
		char *userName;
		userName = getlogin();
		strUserName = StringA2T(userName);

		char comName[128];
		gethostname(comName, sizeof(comName));
		strComName = StringA2T(comName);
#else
		wchar_t userName[128];
		DWORD bufferlength = 128;
		::GetUserNameW(userName, &bufferlength);
		strUserName = StringW2T(userName);

		wchar_t comName[MAX_COMPUTERNAME_LENGTH + 1];
		bufferlength = sizeof(comName) / sizeof(comName[0]);
		::GetComputerNameW(comName, &bufferlength);
		strComName = StringW2T(comName);
#endif
		this->RequestAuthentication(strCpuId, strComName, strUserName, licensekey, this->GetInternalIP(), licensetype);
	};

	this->AuthorizationServer(g_serverHostName, g_port);

	cout << endl << "Authorizing";
	while (!isComplete)
	{

#if defined(_WIN32)
		// innoset
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if ((msg.message & 0xFFF0) == SC_CLOSE)
			{
				isComplete = true;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
#endif

		this->FrameMove();
		Proud::Sleep(10);
		cout << ".";
	}
	cout << endl;

	class CDisconnectHelper
	{
	public:
		explicit CDisconnectHelper(CPNLicAuthClient& authClient) : m_authClient(authClient)
		{}
		~CDisconnectHelper()
		{
			m_authClient.Disconnect();
		}

	private:
		CPNLicAuthClient& m_authClient;

	} disconnectHelper(*this);

	if (authorized == LMType_RegSuccess)
	{
		resultMessage.Format(StringA2T("%s\n"), GetLicenseMessageString(langid, authorized));
	}
	else
	{
		resultMessage.Format(StringA2T("%s\n(Error = %d)"), GetLicenseMessageString(langid, authorized), authorized);
	}
}

void CPNLicAuthClient::OnError(ErrorInfo *errorInfo)
{
}

void CPNLicAuthClient::OnWarning(ErrorInfo *errorInfo)
{
}

void CPNLicAuthClient::OnInformation(ErrorInfo *errorInfo)
{
}

void CPNLicAuthClient::OnException(Proud::Exception &e)
{
}
