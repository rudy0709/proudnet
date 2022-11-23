#include "stdafx.h"
#include "PNLicInformer.h"

#include "../PNLicenseManager/LicenseMessage.inl"

#include "../LicAuthLib/PNLicAuthCommon/pidl/C2S_common.cpp"
#include "../LicAuthLib/PNLicAuthCommon/pidl/S2C_common.cpp"
#include "../LicAuthLib/PNLicAuthCommon/pidl/C2S_stub.cpp"
#include "../LicAuthLib/PNLicAuthCommon/pidl/S2C_proxy.cpp"

#include "../LicAuthLib/PNLicAuthCommon/pidl/C2S_proxy.cpp"
#include "../LicAuthLib/PNLicAuthCommon/pidl/S2C_stub.cpp"
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

CPNLicInformer::CPNLicInformer(void)
{
	m_didSendRMIToAuthServer = false;
	m_ccuNumber = 0;

	//m_logWriter = CLogWriter::New(StringT2A("D:\\Temp\\AuthLog\\AuthClientLog"), 10000);
}

CPNLicInformer::~CPNLicInformer(void)
{
	if (m_client != NULL) {
		m_client->Disconnect();
		m_client.Free();
	}
}

DEFRMI_LicenseS2C_NotifyRecordServerInfo(CPNLicInformer)
{
	OnReplyHandler((LicenseMessageType)result);
	return true;
}

void CPNLicInformer::OnReplyHandler(LicenseMessageType resultType)
{
	if (resultType == LMType_RegSuccess) {
		OnReplySucceed(resultType);
	}
	else {
		OnReplyFailed(resultType);
	}
}

void CPNLicInformer::OnReplySucceed(LicenseMessageType authResultType)
{
#if defined(_DEBUG) || defined(DEBUG) 
	cout << "success to activate online authorization. (" << authResultType << ")" << endl;
#endif
};

void CPNLicInformer::OnReplyFailed(LicenseMessageType authResultType)
{
#if defined(_DEBUG) || defined(DEBUG) 
	cout << "failed to activate online authorization. (errorcode: " << authResultType << ")" << endl;
#endif
};

// 인증 서버와의 접속 성공 혹은 실패
void CPNLicInformer::OnJoinServerComplete(ErrorInfo *info, const ByteArray &replyFromServer)
{
#if defined(_DEBUG) || defined(DEBUG) 
	cout << "success to get OnJoinServerComplete(" << info->m_errorType << ")" << endl;
#endif

	if (info->m_errorType != ErrorType_Ok) {
		// 접속 실패
		OnReplyHandler(LMType_AuthServerConnectionFailed);
	}
	else
	{
		// 접속 성공
		String strCpuId;
		String strComName;
		String strUserName;
		String strProcessName;

		// CPU ID를 얻는다.
		PNCPUID cpuid(0);
		strCpuId.Format(_PNT("%d %d %d %d"), cpuid.eax(), cpuid.ebx(), cpuid.ecx(), cpuid.edx());

		// 사용자 정보와 컴퓨터 이름을 얻는다.
#if defined(__linux__)
		char *userName;
		userName = getlogin();
		strUserName = StringA2T(userName);

		char comName[128];
		gethostname(comName, sizeof(comName));
		strComName = StringA2T(comName);
#else
		wchar_t userName[128];
		DWORD bufferLength = 128;
		::GetUserNameW(userName, &bufferLength);
		strUserName = StringW2T(userName);

		wchar_t comName[MAX_COMPUTERNAME_LENGTH + 1];
		bufferLength = sizeof(comName) / sizeof(wchar_t);
		::GetComputerNameW(comName, &bufferLength);
		strComName = StringW2T(comName);

#endif
		// 인증 서버에 실행중인 서버 정보를 보낸다.
		this->RequestRecordServerInfo(strCpuId, strComName, strUserName, this->GetProcessName(), this->GetInternalIP(), this->GetCcuNumber());
		this->m_didSendRMIToAuthServer = true;
	}
}

void CPNLicInformer::OnLeaveServer(ErrorInfo *info)
{
#if defined(_DEBUG) || defined(DEBUG) 
	printf("success to get OnLeaveServer. \n");
#endif
}

void CPNLicInformer::SetLicenseInfo(CPNLicenseInfo licenseInfo)
{
	m_licenseInfo = licenseInfo;
}

void CPNLicInformer::SetCcuNumber(int32_t ccuNumber)
{
	m_ccuNumber = ccuNumber;
}

int32_t CPNLicInformer::GetCcuNumber()
{
	return m_ccuNumber;
}

Proud::String CPNLicInformer::GetProcessName()
{
	return m_processName;
}

// 프로세스 아이디를 가지고 프로세스 이름을 설정한다.  
void CPNLicInformer::SetProcessNameByPID(int64_t pid)
{
#if defined(_WIN32)
	HANDLE proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	if (proc == NULL) {
		m_processName = _PNT("");
	}
	else
	{
		wchar_t processName[Proud::LongMaxPath + 1];
		GetModuleBaseNameW(proc, NULL, processName, Proud::LongMaxPath + 1);
		m_processName = processName;
		CloseHandle(proc);
	}
#else
	TSTRING strProcFilePath;
	Tstringstream strProcFilePathStream;
	strProcFilePathStream << _PNT("/proc/");
	strProcFilePathStream << pid;
	strProcFilePathStream << _PNT("/cmdline");
	strProcFilePath = strProcFilePathStream.str();

	CPNErrorInfo errorInfo;
	CPNFileSystem fs;
	String strReadFileData;
	fs.OpenFile(strProcFilePath.c_str(), CPNFileSystem::READONLY, errorInfo);
	fs.ReadLine(strReadFileData, errorInfo);
	fs.CloseFile(errorInfo);
	m_processName = basename(strReadFileData.GetString());
#endif
}

bool CPNLicInformer::DidSendRMIToAuthServer()
{
	return m_didSendRMIToAuthServer;
}

void CPNLicInformer::Connect(Proud::String serverIP, int serverPort)
{
	CNetConnectionParam cp;
	cp.m_serverIP = serverIP;
	cp.m_serverPort = serverPort;
	cp.m_protocolVersion = g_protocolVersion;
	cp.m_netWorkerThreadModel = ThreadModel_SingleThreaded;
	cp.m_userWorkerThreadModel = ThreadModel_SingleThreaded;

	m_client = CNetClient::Create();
	if (m_client == NULL) {
		return;
	}

	m_client->SetEventSink(this);
	m_client->AttachProxy(&m_c2sProxy);
	m_client->AttachStub(this);
	m_client->Connect(cp);
}

void CPNLicInformer::Disconnect()
{
	if (m_client != NULL) {
		m_client->Disconnect();
	}
#if defined(_DEBUG) || defined(DEBUG) 
	printf("success to disconnect. \n");
#endif
}

void CPNLicInformer::RequestRecordServerInfo(Proud::String cpuId, Proud::String comName, Proud::String userName, Proud::String processName, Proud::String internalIP, int32_t ccuNumber)
{
	m_c2sProxy.RequestRecordServerInfo(HostID_Server, RmiContext::SecureReliableSend, cpuId, m_licenseInfo.m_companyName, m_licenseInfo.m_projectName, m_licenseInfo.m_licenseType, m_licenseInfo.GetSignatureText(), comName, userName, processName, internalIP, ccuNumber);
#if defined(_DEBUG) || defined(DEBUG) 
	cout << "success to request to record server info. " << endl;
#endif
}

String CPNLicInformer::GetInternalIP()
{
	if (m_client == NULL) {
		return _PNT("");
	}
	return m_client->GetTcpLocalAddr().ToDottedIP();
}

void CPNLicInformer::ConnectToAuthServer(String serverIP, int port)
{
	this->Connect(serverIP, port);
}

void CPNLicInformer::FrameMove()
{
	m_client->FrameMove();
}

void CPNLicInformer::OnError(ErrorInfo *errorInfo)
{
}

void CPNLicInformer::OnWarning(ErrorInfo *errorInfo)
{
}

void CPNLicInformer::OnInformation(ErrorInfo *errorInfo)
{
}

void CPNLicInformer::OnException(Proud::Exception &e)
{
}
