#include "stdafx.h"
#include "PNLicenseSdk.h"

CPNRegistry g_registry;

// 지금까지 설치된 적이 있는 라이선스키의 signature들. 
// 라이선스 크래킹을 시도한 유저는, 기 갖고 있던 라이선스키를 이용해서, 크래킹으로 인한 차단을 해제할 권리가 없다. 이를 위함.
// 라이선스키 A 설치 => 크래킹 시도 => 차단 => 라이선스키 A 다시 설치시도 => 이 변수 때문에 설치 실패!
std::set<String> g_usedKeySignatures;

int g_pnlicenseSDK_version = 1;

//라이선스키를 확인한다.
bool PNRegSmartKeyCheck(String& ProjectName, String& CompanyName, String& LicenseType, String& LicenseKey, CPNErrorInfo& errorInfo)
{
	VIRTUALIZER_FISH_RED_START
	String strCompanyName;
	String strProjectName;
	String strLicenseType;

	LicenseKey.TrimLeft();
	LicenseKey.TrimRight();
	LicenseKey.Replace(_PNT("\r\n"), _PNT("\n"));
	LicenseKey.Replace(_PNT("\r"), _PNT("\n"));

	if (g_registry.ActivateRegistryMemoryWithLicensekey(LicenseKey, errorInfo) == false) {
		errorInfo.AppendErrorCode(NotInstalledLicenseKey, "");
		return false;
	}

	g_registry.GetCompanyName(strCompanyName);
	if (CompanyName.Compare(strCompanyName.GetString()) != 0) {
		return false;
	}

	g_registry.GetProjectName(strProjectName);
	if (ProjectName.Compare(strProjectName.GetString()) != 0) {
		return false;
	}

	g_registry.GetLicenseType(strLicenseType);
	if (LicenseType.Compare(strLicenseType.GetString()) != 0) {
		return false;
	}

	VIRTUALIZER_FISH_RED_END
	return true;
}

bool PNRegGetLicenseInfoFromSmartKey(String& LicenseKey, String& ProjectName, String& CompanyName, String& LicenseType, CPNErrorInfo& errorInfo)
{
	VIRTUALIZER_FISH_RED_START
	LicenseKey.TrimLeft();
	LicenseKey.TrimRight();
	LicenseKey.Replace(_PNT("\r\n"), _PNT("\n"));
	LicenseKey.Replace(_PNT("\r"), _PNT("\n"));

	if (g_registry.ActivateRegistryMemoryWithLicensekey(LicenseKey, errorInfo) == false) {
		errorInfo.AppendErrorCode(NotInstalledLicenseKey, "");
		return false;
	}

	g_registry.GetProjectName(ProjectName);
	g_registry.GetCompanyName(CompanyName);
	g_registry.GetLicenseType(LicenseType);
	VIRTUALIZER_FISH_RED_END
	return true;
}

//#### 라이선스키를 레지스트리에 등록한다
bool  PNRegSmartKeyInstallToRegistry(String &LicenseKey, CPNErrorInfo& errorInfo)
{
	VIRTUALIZER_FISH_RED_START

	LicenseKey.TrimLeft();
	LicenseKey.TrimRight();
	LicenseKey.Replace(_PNT("\r\n"), _PNT("\n"));
	LicenseKey.Replace(_PNT("\r"), _PNT("\n"));

	CPNLicenseInfo licenseInfo;
	ByteArray bytePrivateBlob;
	StringA strPrivateKey = DSA_AUTHENTICATIONKEY;
	Base64::Decode(strPrivateKey, bytePrivateBlob);
	licenseInfo.SetPrivateKey(bytePrivateBlob);

	// 라이선스 키를 인증키로 검증합니다. 
	if (licenseInfo.VerifyLicenseKey(LicenseKey, errorInfo) == false)
	{
		errorInfo.AppendErrorCode(NotActivatedRegistryMemory, "");
		return false;
	}

	// 라이선스 정보의 유효성을 확인합니다. 
	if (licenseInfo.CheckValidity(errorInfo) == false) {
		errorInfo.AppendErrorCode(NotActivatedRegistryMemory, "");
		return false;
	}

	// CreateEmptyRegistryFileIfNotExists의 로직은 CPNRegistry.LoadRegistryFile 내부에 있던 것이다.
	// 그런데 레지스트리 파일이 없는 경우, 퍼스널 라이선스로 간주되게 하기로 하였다.
	// 때문에 더 이상 LoadRegistryFile에 있으면 안되는 로직이 되었다.
	// 그렇지만 CreateEmptyRegistryFileIfNotExists의 로직은 이 지점에서는 수행되어야만 한다.
	// 그 이유는 레지스트리 파일이 없다면, LoadRegistryFile의 수정사항 때문에 인증이 실패하게 되기 때문이다.
	if (false == g_registry.CreateEmptyRegistryFileIfNotExists(errorInfo))
	{
		return false;
	}

	g_registry.LoadRegistryFile(errorInfo);
	g_registry.SetLicenseInfo(licenseInfo);
	g_registry.SaveRegistryFile(errorInfo);
	VIRTUALIZER_FISH_RED_END
		return true;
}

//#### 라이선스의 발급일을 가져온다.
int  PNRegLicIssuedDate(SYSTEMTIME_t* pIssueDate, CPNErrorInfo& errorInfo)
{
	uint64_t issueTime;
	SYSTEMTIME_t *_ptm;

	// Return Values
	// 0: function succeds
	// wlPermKey : a permanent key
	// wlNoKey : a license key has not ben installed

	g_registry.GetIssueDate(&issueTime);

	_ptm = (SYSTEMTIME_t*)gmtime((time_t*)&issueTime);
	memcpy_s(pIssueDate, sizeof(SYSTEMTIME_t), _ptm, sizeof(SYSTEMTIME_t));

	return 0;
}

//#### 라이선스의 만료일을 가져온다.
int  PNRegExpirationDate(SYSTEMTIME_t* pExpDate, CPNErrorInfo& errorInfo)
{
	uint64_t expTime;
	SYSTEMTIME_t *_ptm;

	// Return Values
	// 0: function succeds
	// wlPermKey : a permanent key
	// wlNoKey : a license key has not ben installed

	g_registry.GetExpirationDate(&expTime);

	_ptm = (SYSTEMTIME_t*)gmtime((time_t*)&expTime);
	memcpy_s(pExpDate, sizeof(SYSTEMTIME_t), _ptm, sizeof(SYSTEMTIME_t));

	return 0;
}

//#### PC에 입력된 라이선스의 정보를 가져온다.
int  PNRegGetStatus(int* pExtendedInfo, CPNErrorInfo& errorInfo)
{
	// pExtendedInfo [out] extended information about the app status.
	// wlLicenseDateExpired when a license is expired on date

	// Return Values
	// wlIsRegistered when an app is registered with a valid license key.
	// wlInvalidLicense when a license key is invalid
	// wlLicenseExpired when a license key has expired. 
	// wlInstallLicenseDateExpired when a license has been installed after a specific installation date

	uint32_t isValid = 0;

	g_registry.GetStatus(&isValid);

	if (isValid & ExpiredRegKey) {
		return PNLicenseExpired;
	}
	else if (isValid & InValidRegKeyMTime) {
		return PNInvalidSystemTime;
	}
	else if (isValid & InValidSysTime) {
		return PNInvalidSystemTime;
	}
	else if (isValid & ExistRegKey) {
		return PNIsRegisteredNotActivated;
	}
	else if (isValid & BoundaryErrorRegKey) {
		return PNInvalidLicenseDate;
	}
	else if (isValid & InValidRegKey) {
		return PNInvalidLicenseKey;
	}
	else if (isValid & ValidRegKey) {
		return PNIsRegistered;
	}
		
	return PNIsNotInstalled;
}

//#### 라이선스의 정보를 가져온다.
//#### [out] pName: ProjectName
//#### [out] pCompanyName: CompanyName
//#### [out] pCustomDate: LicenseType
PNLicErrorCode PNRegGetLicenseInfoAndUpdateLastAccessTime(String &strProjectName, String &strCompanyName, String &strLicenseType, CPNErrorInfo& errorInfo)
{
	PNLicErrorCode loadResult = g_registry.LoadRegistryFile(errorInfo);

	if (PNLicErrorCode::RegistryFileNotExists == loadResult)
	{
		return loadResult;
	}

	assert(PNLicErrorCode::RegistryFileNotExists != loadResult);

	if (PNLicErrorCode::OK != loadResult)
	{
		errorInfo.AppendErrorCode(NotActivatedRegistryMemory, "");
		return loadResult;
	}

	g_registry.GetCompanyName(strCompanyName);
	g_registry.GetProjectName(strProjectName);
	g_registry.GetLicenseType(strLicenseType);
	g_registry.SaveRegistryFile(errorInfo);

	assert(PNLicErrorCode::OK == loadResult);

	return loadResult;
}
