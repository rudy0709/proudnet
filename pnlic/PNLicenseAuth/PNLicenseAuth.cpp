#include "stdafx.h"
#include "PNLicenseAuth.h"
#include <fstream>

const char *g_pnVersionText = "HERE_SHALL_BE_EDITED_BY_BUILD_HELPER";

#include "../PNLicenseManager/LicenseType.h"

void GetProudNetVer(String &outProudNetVersion)
{
	outProudNetVersion = StringA2T(g_pnVersionText);
}

void ShowUsage()
{
	cout << endl << "Usage: PNLicenseAuth [ License key path ]" << endl;
}

String ShowActivationValidity(const int state)
{
	String validity = _PNT("");

	switch (state)
	{
	case PNNoKey:
		validity = _PNT("No Key.");
		break;
		
	case PNIsNotInstalled:
		validity = _PNT("Not Installed.");
		break;

	case PNIsRegisteredNotActivated:
		validity = _PNT("Not Activated.");
		break;

	case PNLicenseExpired:
		validity = _PNT("LicenseKey Expired!");
		break;

	case PNInvalidLicenseKey:
		validity = _PNT("Invalid Licensekey!!");
		break;

	case PNInvalidSystemTime:
		validity = _PNT("Time Fraud Detected!!");
		break;

	case PNIsRegistered:
		validity = _PNT("OK");
		break;
		
	default:
		validity = _PNT("NONE");
		break;
	}
	
	return validity;
}

int ShowLicenseInfo(CPNErrorInfo& errorInfo)
{
	VIRTUALIZER_FISH_RED_START
	int status = 0;
	String strProudNetVersion;

	CPNLicenseInfo licenseInfo;
	PNRegGetLicenseInfoAndUpdateLastAccessTime(licenseInfo.m_projectName, licenseInfo.m_companyName, licenseInfo.m_licenseType, errorInfo);

	status = PNRegGetStatus(NULL, errorInfo);
	GetProudNetVer(strProudNetVersion);

	SYSTEMTIME_t expDate;
	memset(&expDate, 0x00, sizeof(expDate));
	PNRegExpirationDate(&expDate, errorInfo);

	SYSTEMTIME_t issueDate;
	memset(&issueDate, 0x00, sizeof(issueDate));
	PNRegLicIssuedDate(&issueDate, errorInfo);

	printf("\n=< ProudNet License Info >=========================================\n");
	printf(" - Version : %s\n", StringT2A(strProudNetVersion).GetString());
	printf(" - Project Name : %s\n", StringT2A(licenseInfo.m_projectName).GetString());
	printf(" - Company Name : %s\n", StringT2A(licenseInfo.m_companyName).GetString());
	printf(" - License Type : %s\n", StringT2A(licenseInfo.m_licenseType).GetString());
	printf(" - IssuedDate(UTC+0) : %04d-%02d-%02d \n", (issueDate.year >= 0) ? issueDate.year + 1900 : 0, (issueDate.mon >= 0) ? issueDate.mon + 1 : 0, issueDate.mday);
	printf(" - ExpiryDate(UTC+0) : %04d-%02d-%02d \n", (expDate.year >= 0) ? expDate.year + 1900 : 0, (expDate.mon >= 0) ? expDate.mon + 1 : 0, expDate.mday);
	printf(" - Validity : %s\n", StringT2A(ShowActivationValidity(PNRegGetStatus(NULL, errorInfo))).GetString());
	printf("===================================================================\n");
	VIRTUALIZER_FISH_RED_END;

	return status;
}

// 라인선스 키에 대한 인터넷 인증&오프라인 인증을 한다. 키 설치는 아님.
// 단, Pro 라이선스의 경우 인터넷 인증만 실패하는 것만큼은 용인한다. (Tencent 등 때문)
bool ActivateOnlineLicense(String strLicensekey)
{
	VIRTUALIZER_FISH_RED_START
	Proud::String resultMessage;
	Proud::String strProjectName;
	Proud::String strCompanyName;
	Proud::String strLicenseType;
	CPNErrorInfo errorInfo;
	LicenseMessageType errorcode = LMType_Nothing;
	bool isComplete = false;
	bool ret = false;

	// 인터넷 인증을 한다.
	CPNLicAuthClient authnet;
	authnet.AuthorizationBlocking(strLicensekey.GetString(), _PNT("NotCheck"), resultMessage, errorcode, isComplete);

	if (isComplete == true) {
		switch (errorcode)
		{
		case LMType_RegSuccess:
			// 온라인 인증이 성공하여 true 를 반환한다. 
			ret = true;
			break;

		case LMType_AuthServerConnectionFailed:
			// 네트웍 장애로 온라인 인증을 할 수 없는 상태로 이 경우에만 특별히 오프라인 설치를 허용하기 위해 true 로 반환한다.
			// 단, Indie/Personal/5CCU/10CCU 라이선스의 경우 반드시 온라인 인증이 되어야 하므로 제외된다. 
			ret = true; 

			if (PNRegGetLicenseInfoFromSmartKey(strLicensekey, strProjectName, strCompanyName, strLicenseType, errorInfo) == true) {
				if (strLicenseType.Compare(PNINDIELICENSE) == 0 ||
					strLicenseType.Compare(PNPERSONALLICENSE) == 0 ||
					strLicenseType.Compare(PN5CCU) == 0 ||
					strLicenseType.Compare(PN10CCU) == 0
					) {
					ret = false;
				}
			}
			else
			{
				ret = false;
			}

			break;

		default:
			//cout << "Failed to activate online license (Reason: " << StringT2A(resultMessage).GetString() << ". errorcode: " << errorcode << ")" << endl;
			ret = false;
			break;
		}
	}
	else
	{
		ret = false;
	}
	
	authnet.Disconnect();
	//cout << "success to activate online." << endl;
	return ret;
	VIRTUALIZER_FISH_RED_END;
}

#if defined(WIN32)
int _tmain(int argc, TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
	VIRTUALIZER_FISH_RED_START

	int errorcode = 0;
	CPNErrorInfo errorInfo;

	if (argc > 2) {
		cout << "Failed to execute PNLicenseAuth. (Reason: too many arguments. errorcode: " << PNErrorInvalidArgCount << ")" << endl;
		return -1;
	}

	if (argc == 1) {
		errorcode = ShowLicenseInfo(errorInfo);
		if (errorcode != PNIsRegistered) {
			cout << "Failed to execute PNLicenseAuth. (Reason: the ProudNet license key is not installed. errorcode: " << errorInfo.ToString().c_str() << ")" << endl;
			return -1;
		}

		return 0;
	}

	Proud::String strTempPath = Proud::String(argv[1]); 
	if (strTempPath.GetLength() < 1) {
		ShowUsage();
		return 0;
	}

	if (argc == 2) {
		if (Tstrcmp(strTempPath.GetString(), _PNT("--help")) == 0) {
			ShowUsage();
			return 0;
		}
	}

	// 라이선스 키 파일 내용을 로드한다.
	String strReadFileData;

	std::vector<char> stringBuffer;
	std::ifstream myFile(strTempPath.GetString(), std::ios::in | std::ios::binary);

	if (false == myFile.is_open())
	{
		cout << "Failed to open your license file '" << strTempPath << "'." << endl;
		return -1;
	}

	while (false == myFile.eof())
	{
		char tmpChar;

		// 주의 : myFile >> tmpChar 이런식으로 사용하면 \r\n과 같은 개행문자를 무시해버린다.
		myFile.read(&tmpChar, 1);

		stringBuffer.push_back(tmpChar);
	}

	int offset = 0;
	if ('\xEF' == stringBuffer[0] && '\xBB' == stringBuffer[1] && '\xBF' == stringBuffer[2])
	{
		offset = 3;
	}

	Proud::StringA tmpStr(&stringBuffer[offset], stringBuffer.size() - offset - 1);

#if defined(_PNUNICODE)
	strReadFileData = Proud::StringA2W(tmpStr);
#else
	strReadFileData = tmpStr;
#endif

	myFile.close();

	// 인터넷으로 인증 후 라이선스 키를 설치한다. (단, 네트웍 단절 시에만 오프라인 모드로 키 설치가 진행 된다.)
	if (ActivateOnlineLicense(strReadFileData) == false) {
		cout << "Failed to execute PNLicenseAuth. (errorcode: " << PNIsNotOnlineActivated << ")" << endl;
		return -1;
	}

	// 오프라인으로 인증 후 라이선스 키를 설치한다. 
	if (PNRegSmartKeyInstallToRegistry(strReadFileData, errorInfo) == false) {
		errorcode = ShowLicenseInfo(errorInfo);
		if (errorcode != PNIsRegistered) {
			cout << "Failed to execute PNLicenseAuth. (Reason: Cannot install the ProudNet license key. errorcode: " << errorcode << " " << errorInfo.ToString().c_str() << ")" << endl;
			return -1;
		}
		else
		{
			cout << "Failed to execute PNLicenseAuth. (Reason: Already installed. errorcode: " << errorcode << " " << errorInfo.ToString().c_str() << ")" << endl;
			return -1;
		}
	}

	errorcode = ShowLicenseInfo(errorInfo);
	if (errorcode != PNIsRegistered) {
		cout << "Failed to execute PNLicenseAuth. (Reason: Cannot load license information. errorcode: " << errorcode << " " << errorInfo.ToString().c_str() << ")" << endl;
		return -1;
	}

	VIRTUALIZER_FISH_RED_END
	return 0;
}