#pragma once

#include "../PNLicenseSdk/PNLicenseSdk.h"
#pragma comment(lib, "PNLicenseSdk")

class CPNLicense
{
public:
	//PC에 입력된 라이선스 타입과 입력된 라이선스를 비교한다
	static bool CheckLicenseType(String& licenseType);
	//PC에 입력된 라이선스의 정보를 가져온다.
	static int  GetStatus(int* extendedInfo = NULL );
	//라이선스의 만료일을 가져온다.
	static int  GetExpirationDate(SYSTEMTIME_t * expdate);
	//라이선스의 정보를 가져온다.
	static bool GetLicenseInfo(String& projectName, String& companyName, String& libtype);
	//라이선스키를 레지스트리에 등록한다.
	static bool InstallSmartkeyToRegistry(String& licenseKey);
	//라이선스키를 확인한다.
	static bool SmartKeyCheck(String& projectName, String& companyName, String& libtype, String& licenseKey);
	//실행중인 앱을 재시작한다.
	//static bool RestartAppArgs(char* args = NULL);
	//라이선스키를 확인뒤 레지스트리에 등록한다.
	static bool CheckAndInstallToRegisry(String& licenseKey);
};
