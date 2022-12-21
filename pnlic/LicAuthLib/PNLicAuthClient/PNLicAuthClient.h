#pragma once

typedef unsigned char BYTE;

#include "CPUID.h"
#include "../../AuthNetLib/include/ProudNetClient.h"
#include "../../PNLicenseManager/LicenseMessage.h"
#include "../../PNLicenseSdk/PNLicenseSdk.h"

// License Manager Client
#include "../PNLicAuthCommon/Vars.h"
#include "../PNLicAuthCommon/pidl/S2C_stub.h"
#include "../PNLicAuthCommon/pidl/C2S_proxy.h"
#include <functional>

using namespace Proud;

class CPNLicAuthClient : public INetClientEvent, LicenseS2C::Stub
{
private:
	//## LM Client Part
	CHeldPtr<CNetClient> m_clientLM;
	LicenseC2S::Proxy m_c2sProxy;
	//Proud::CLogWriter *m_logWriter;
	bool m_isConnectedToAuthServer;

public:
	CPNLicAuthClient(void);
	~CPNLicAuthClient(void);
	
	//## Common Part
	void OnError(ErrorInfo *errorInfo) PN_OVERRIDE;
	void OnWarning(ErrorInfo *errorInfo) PN_OVERRIDE;
	void OnInformation(ErrorInfo *errorInfo) PN_OVERRIDE;
	void OnException(Proud::Exception &e) PN_OVERRIDE;

	//## LM Client Part - Event
	void OnJoinServerComplete(ErrorInfo *info, const ByteArray &replyFromServer);
	void OnLeaveServer(ErrorInfo *info);

	//## 서버 인증기능엔 필요하지 않음
	void OnP2PMemberJoin(HostID memberHostID, HostID groupHostID, int memberCount, const ByteArray &customField) {};
	void OnP2PMemberLeave(HostID memberHostID, HostID groupHostID, int memberCount) {};
	void OnChangeP2PRelayState(HostID remoteHostID, ErrorType reason) {};
	void OnSynchronizeServerTime() {};
	void OnNoRmiProcessed(Proud::RmiID rmiID) {};

	//## LM Client Part
	Proud::String GetInternalIP();
	bool IsConnectedToAuthServer();
	void Connect(Proud::String serverIP, int serverPort);
	void Disconnect();
	void FrameMove();

	void OnAuthenticationHandler(LicenseMessageType result);
	bool AuthorizationServer(Proud::String serverIP, int port);
	void AuthorizationBlocking(Proud::String licensekey, Proud::String licensetype, Proud::String &resultMessage, LicenseMessageType &authorized, bool &isComplete);
	void RequestAuthentication(Proud::String cpuId, Proud::String computerName, Proud::String userName, Proud::String key, Proud::String internalIP, Proud::String licenseType);
	
	//LM Client Part - S2C Stub
	DECRMI_LicenseS2C_NotifyAuthentication;
	DECRMI_LicenseS2C_NotifyAuthenticationComment;
			
	std::function<void()> OnJoinServerHandler;
	std::function<void(LicenseMessageType resultType)> OnAuthenticationSucceed;
	std::function<void(LicenseMessageType resultType)> OnAuthenticationFailed;
	std::function<void()> OnRequestAuthentication; // 인증서버에 정상적으로 접속 후, 라이선스 인증 Rmi 보내는 function
};
