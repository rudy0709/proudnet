#pragma once

#include "../../AuthNetLib/include/ProudNetServer.h"
#include "../../AuthNetLib/include/Tracer.h" 
#include "../../AuthNetLib/include/AdoWrap.h"

#include "../../PNLicenseManager/LicenseMessage.h"

#include "../PNLicAuthCommon/Vars.h"
#include "../PNLicAuthCommon/pidl/C2S_stub.h"
#include "../PNLicAuthCommon/pidl/S2C_proxy.h"

#include <functional>
#include <sstream>

using namespace Proud;

// 라이선스 인증 서버.
class CPNLicAuthServer : public INetServerEvent, public LicenseC2S::Stub
{
private:
	CHeldPtr<CNetServer> m_server;
	LicenseS2C::Proxy m_s2cProxy;

	// 서버에서 일어나는 이벤트 로그를 기록하는 역할.
	CLogWriter *m_logWriter;

public:
	CPNLicAuthServer(void);
	~CPNLicAuthServer(void);

	bool Start(CStartServerParameter p);
	void Stop();

	void OnError(ErrorInfo *errorInfo) override;
	void OnWarning(ErrorInfo *errorInfo) override;
	void OnInformation(ErrorInfo *errorInfo) override;
	void OnException(Exception &e) override;
	void OnNoRmiProcessed(RmiID rmiID) override;
	
	void OnClientJoin(CNetClientInfo *clientInfo) override;
	void OnClientLeave(CNetClientInfo *clientInfo, ErrorInfo *errorinfo, const ByteArray& comment) override;
	void OnP2PGroupJoinMemberAckComplete(HostID groupHostID, HostID memberHostID, ErrorType result) override {}
	void OnUserWorkerThreadBegin() override {}
	void OnUserWorkerThreadEnd() override {}
	
	String GetExternalIP(HostID remote) const;
	void NotifyRecordServerInfo(HostID remote, LicenseMessageType resultType);
	void NotifyAutherized(HostID remote, LicenseMessageType resultType);
	void NotifyAutherized(HostID remote, LicenseMessageType resultType, const Proud::String &comment );
	LicenseMessageType DBCheckAuthentication(const String &cpuId, const String &comName, const String &userName, const String &key_2, const String &externalIP, const String &internalP, const String &licensetype);
	LicenseMessageType DBRecordServerInfo(const String &cpuId, const String &companyName, const String &projectName, const String &licenseType, const String &sigKey, const String &comName, const String &userName, const String &processName, const String &externalIP, const String &internalIP, const int8_t ccuNumber);

	DECRMI_LicenseC2S_RequestAuthentication;
	DECRMI_LicenseC2S_RequestRecordServerInfo;

	//std::function<void(CNetClientInfo * clientInfo)> OnClientJoinHandler;
	//std::function<void(CNetClientInfo *clientInfo, ErrorInfo *errorinfo, const ByteArray& comment)> OnClientLeaveHandler;

	//std::function<bool(Proud::HostID remote, const Proud::StringW &cpuId,const Proud::StringW &comName,const Proud::StringW &userName,const Proud::StringW &key, const Proud::StringW &internalIP, const Proud::StringW &licenseType)> RequestAuthenticationHandler_V2;
	//std::function<bool(Proud::HostID remote, const Proud::StringA &cpuId,const Proud::StringA &comName,const Proud::StringA &userName,const Proud::StringA &key)> RequestAuthenticationHandler;
	//
	//std::function<void(ErrorInfo * errorInfo)> OnErrorHandler;
	//std::function<void(ErrorInfo * errorInfo)> OnWarningHandler;
	//std::function<void(ErrorInfo * errorInfo)> OnInformationHandler;
	//std::function<void(Exception &e)> OnExceptionHandler;
};

