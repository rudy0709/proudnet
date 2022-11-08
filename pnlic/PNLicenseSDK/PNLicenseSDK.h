// LinuxLicenseSDK.cpp : Defines the exported functions for the DPN application.
//

#pragma once

#include "stdafx.h"
#include "PNExport.h"
#include "PNErrorInfo.h"
#include "PNRegistry.h"
#include "PNLicenseInfo.h"

enum PNRegistryState
{
	PNNoKey = 3001,
	PNIsRegisteredNotActivated,
	PNLicenseExpired,
	PNIsRegistered,
	PNInvalidLicenseKey,
	PNInvalidSystemTime, 
	PNIsNotInstalled, 
	PNIsNotOnlineActivated,
	PNInvalidLicenseDate
};

// ***********************************************
// LinLicense functions prototype
// ***********************************************

#ifdef __cplusplus
extern "C" {
#endif

	//라이선스키를 확인하고 개인키를 셋해준다.
	bool  PNRegSmartKeyCheck(String& ProjectName, String& CompanyName, String& LicenseType, String& LicenseKey, CPNErrorInfo& errorInfo);

	//라이선스키에서 라이선스 타입정보를 추출한다. 
	bool  PNRegGetLicenseInfoFromSmartKey(String& LicenseKey, String& ProjectName, String& CompanyName, String& LicenseType, CPNErrorInfo& errorInfo);

	//라이선스의 정보를 가져온다.
	PNLicErrorCode PNRegGetLicenseInfoAndUpdateLastAccessTime(String &strName, String &strCompanyName, String &strLicenseType, CPNErrorInfo& errorInfo);

	//라이선스키를 레지스트리에 등록한다
	bool  PNRegSmartKeyInstallToRegistry(String &LicenseKey, CPNErrorInfo& errorInfo);

	//PC에 입력된 라이선스의 정보를 가져온다.
	int  PNRegGetStatus(int* pExtendedInfo, CPNErrorInfo& errorInfo);

	//라이선스의 발급일을 가져온다.
	int  PNRegLicIssuedDate(SYSTEMTIME_t* pIssueDate, CPNErrorInfo& errorInfo);

	//라이선스의 만료일을 가져온다.
	int  PNRegExpirationDate(SYSTEMTIME_t* pExpDate, CPNErrorInfo& errorInfo);

#ifdef __cplusplus
} // extern "C"
#endif

