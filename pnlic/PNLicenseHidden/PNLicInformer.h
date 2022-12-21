#pragma once

typedef unsigned char BYTE;


#include "../AuthNetLib/include/ProudNetClient.h"
#include "../PNLicenseManager/LicenseMessage.h"
#include "../PNLicenseSdk/PNLicenseSdk.h"

#include "../LicAuthLib/PNLicAuthClient/CPUID.h"
#include "../LicAuthLib/PNLicAuthCommon/Vars.h"
#include "../LicAuthLib/PNLicAuthCommon/pidl/S2C_stub.h"
#include "../LicAuthLib/PNLicAuthCommon/pidl/C2S_proxy.h"
#include <functional>

#if defined(_WIN32)
#include <psapi.h>
#else
#endif

using namespace Proud;

// 정보 전달자 클래스 - 인디/퍼스널,5CCU,10CCU 라이선스 사용시 동접자 300을 넘으면 동접자수와 서버PC정보를 인증서버에 전달하는 역할을 한다. 
class CPNLicInformer : public INetClientEvent, LicenseS2C::Stub
{
public:
	CPNLicInformer(void);
	~CPNLicInformer(void);

	void OnError(ErrorInfo *errorInfo) PN_OVERRIDE;
	void OnWarning(ErrorInfo *errorInfo) PN_OVERRIDE;
	void OnInformation(ErrorInfo *errorInfo) PN_OVERRIDE;
	void OnException(Proud::Exception &e) PN_OVERRIDE;
	void OnJoinServerComplete(ErrorInfo *info, const ByteArray &replyFromServer);
	void OnLeaveServer(ErrorInfo *info);

	void OnP2PMemberJoin(HostID memberHostID, HostID groupHostID, int memberCount, const ByteArray &customField) {};
	void OnP2PMemberLeave(HostID memberHostID, HostID groupHostID, int memberCount) {};
	void OnChangeP2PRelayState(HostID remoteHostID, ErrorType reason) {};
	void OnSynchronizeServerTime() {};
	void OnNoRmiProcessed(Proud::RmiID rmiID) {};

	Proud::String GetInternalIP();
	bool DidSendRMIToAuthServer();
	void Connect(Proud::String serverIP, int serverPort);
	void Disconnect();
	void FrameMove();
	void SetLicenseInfo(CPNLicenseInfo licenseInfo);
	void SetCcuNumber(int32_t ccuNumber);
	void SetProcessNameByPID(int64_t pid);

	int32_t GetCcuNumber();
	Proud::String GetProcessName();

	void ConnectToAuthServer(Proud::String serverIP, int port);
	void RequestRecordServerInfo(Proud::String cpuId, Proud::String comName, Proud::String userName, Proud::String processName, Proud::String internalIP, int32_t ccuNumber);

	void OnReplyHandler(LicenseMessageType result);
	void OnReplySucceed(LicenseMessageType authResultType);
	void OnReplyFailed(LicenseMessageType authResultType);

	DECRMI_LicenseS2C_NotifyRecordServerInfo;
	std::function<void()> OnJoinServerHandler;

private:
	CHeldPtr<CNetClient> m_client;
	LicenseC2S::Proxy m_c2sProxy;

	// 서버와의 접속 성공시 true가 된다.
	bool m_didSendRMIToAuthServer;

	// hidden app 등으로부터 입력받는 라이선스 정보
	CPNLicenseInfo m_licenseInfo;

	// 역시, 입력받는 동시접속자수
	int32_t m_ccuNumber;

	// 서버 앱 이름
	String m_processName;
};
