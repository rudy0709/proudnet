#include "stdafx.h"
#include "../[AuthNetLib]/include/ProudNetClient.h"
#include <iostream>
#include <thread>
#include <stdio.h>

#if defined(__linux__)
#include <unistd.h>
#include <sys/stat.h>
#endif

#include "../PNLicenseManager/LicenseMessage.inl"
#include "PNLicenseWarning.h"

#pragma comment(lib, "PNLicenseSDK")
using namespace std;

#if defined(__linux__)
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

int main(int argc, char *argv[])
{
	VIRTUALIZER_FISH_RED_START

	// 언어 설정
#if defined(__linux__)
	int langid = GetAppLang();
#else
	LANGID id = ::GetSystemDefaultLangID();
	LANGID langid = PRIMARYLANGID(id);
	setlocale(LC_ALL, "");
#endif

	int extendedInfo = 0;
	int regState = 0;

		if (argc != 4) {
			cout << endl << StringT2A(GetLicenseMessageString(langid, LMType_ExpiredDateKey)).GetString() << " (errorcode:" << PNErrorInvalidArgCount << ")" << endl;
			return -1;
		}

		if (strcmp(argv[1], "tqxc2") != 0) {
			cout << endl << StringT2A(GetLicenseMessageString(langid, LMType_ExpiredDateKey)).GetString() << " (errorcode:" << PNErrorInvalidMagicKey << ")" << endl;
			return -1;
		}

		//#ifdef _DEBUG
		//	printf("argv[0]:%s\n argv[1]:%s\n argv[2]:%s\n", argv[0], argv[1], argv[2]);
		//#endif

		// 경고앱이 떳다는거 자체가 인증이 안되었다는거지만 확인사살한다.
		// 어차피 인증정보를 얻어오려면 winlicense SDK를 사용해야하므로...
		// 키가 등록 되어있는지 체크한다.
		CPNErrorInfo errorInfo;
		if ((regState = PNRegGetStatus(&extendedInfo, errorInfo)) != PNIsRegistered)
		{
			cout << endl << StringT2A(GetLicenseMessageString(langid, LMType_ExpiredDateKey)).GetString() << " (REG:" << regState << ", EXT:" << extendedInfo << ")" << endl;
			return -1;
		}
		
	VIRTUALIZER_FISH_RED_END
	return 0;
}

