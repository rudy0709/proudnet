#include "stdafx.h"
#include "../AuthNetLib/include/ProudNetClient.h"
#include "PNLicense.h"

// 사용자가 입력하는 라이선스키를 확인한다.
bool CPNLicense::SmartKeyCheck(/*out*/String& projectName /*= NULL*/, /*out*/String& companyName/*=NULL*/, /*out*/String& libtype/*=NULL*/, String& licenseKey)
{
	CPNErrorInfo errorInfo;
	return PNRegSmartKeyCheck(projectName, companyName, libtype, licenseKey, errorInfo) ? true : false;
}

//라이선스의 정보를 가져온다.
bool CPNLicense::GetLicenseInfo(String& projectName, String& companyName, String& libtype)
{
	CPNErrorInfo errorInfo;
	return PNRegGetLicenseInfoAndUpdateLastAccessTime(projectName, companyName, libtype, errorInfo) ? true : false;
}

//라이선스키를 레지스트리에 등록한다
bool CPNLicense::InstallSmartkeyToRegistry(String& licenseKey)
{
	CPNErrorInfo errorInfo;

	return PNRegSmartKeyInstallToRegistry(licenseKey, errorInfo) ? true : false;
}

//라이선스키를 확인뒤 레지스트리에 등록한다.
bool CPNLicense::CheckAndInstallToRegisry(String& licenseKey)
{
	String strCompanyName;
	String strProjectName;
	String strLibType;
	String strLicenseKey;

	if (SmartKeyCheck(strCompanyName, strProjectName, strLibType, licenseKey) == false)
	{
		return false;
	} 

	return InstallSmartkeyToRegistry(licenseKey) ? true : false;
}

//PC에 입력된 라이선스의 정보를 가져온다.
int CPNLicense::GetStatus( int* extendedInfo /*= NULL*/)
{
	CPNErrorInfo errorInfo;
	return PNRegGetStatus(extendedInfo, errorInfo);
}

//라이선스의 만료일을 가져온다.
int CPNLicense::GetExpirationDate( SYSTEMTIME_t* expdate )
{
	CPNErrorInfo errorInfo;
	return PNRegExpirationDate(expdate, errorInfo);
}

////실행중인 앱을 재시작한다.
//bool __stdcall CPNLicense::RestartAppArgs( char* args )
//{
//	if(args != NULL)
//	{
//		return PNRestartApplicationArgs(args) ? true : false;
//	} 
//	else 
//	{
//		return PNRestartApplication() ? true : false;
//	}
//}

//PC에 입력된 라이선스 타입과 입력된 라이선스를 비교한다
bool CPNLicense::CheckLicenseType(String& licenseType)
{
	String strLicenseType;
	String strProjectName;
	String strCompanyName;

	if (GetLicenseInfo(strProjectName, strCompanyName, strLicenseType) == false)	{
		return false;
	}

	if (strLicenseType.Compare(licenseType) != 0) {
		return false;
	}

	return true;
}
