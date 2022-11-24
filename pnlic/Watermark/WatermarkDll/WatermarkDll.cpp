// CommonDll.cpp : DLL 응용 프로그램을 위해 내보낸 함수를 정의합니다.
//

#include "stdafx.h"
#include "../../AuthNetLib/include/ProudNetClient.h"
#include "WatermarkDll.h"

#include <thread>
#include <memory.h>
#include <mutex>
#include <condition_variable>

//////////////////////////////////////////////////////////////////////////

DLLEXPORT void WriteCommentWaterMark(char *pFilePath,char* pVersion)
 {
	 VIRTUALIZER_FISH_BLACK_START
	 Proud::String strProjectName;
	 Proud::String strCompanyName;
	 Proud::String strLicenseType;

	if (CPNLicense::GetLicenseInfo(strProjectName, strCompanyName, strLicenseType))
	{
		Proud::String strWaterMark;
		strWaterMark.Format(_PNT("%s %s %s %s"), strProjectName.GetString(), strCompanyName.GetString(), strLicenseType.GetString(), StringA2T(pVersion));

		CWaterMarkMgr WaterMarkMgr;
		WaterMarkMgr.Init((CStringA)strWaterMark);

		Proud::String strFilePath;
		strFilePath.Format(_PNT("%s"), StringA2T(pFilePath));
		WaterMarkMgr.WriteWaterMark((CString)strFilePath);
	}
	VIRTUALIZER_FISH_BLACK_END
 }
 
 // 소스안에 메모리 공간만 할당해줍니다.
 DLLEXPORT void WriteSourceWaterMark(char *pFilePath,char *pFileName)
 {
	 Proud::String strFilePath;
	 strFilePath.Format(_PNT("%s"), StringA2T(pFilePath));

	 int size = strFilePath.GetLength();
     if( strFilePath[size-1] != '\\')
         strFilePath.Append(_PNT("\\"));
 
	 Proud::String strFileName;
	 strFileName.Format(_PNT("%s"), StringA2T(pFileName));

     CWaterMarkMgr WaterMarkMgr;
	 WaterMarkMgr.WriteSrcWaterMark((CStringA)strFilePath, (CStringA)strFileName);
 }
 
 // 위에서 할당된 메모리 공간안에 데이터를 씁니다.
 DLLEXPORT void WriteBinaryWaterMark_ChangeLibBinary(char *pFilePath ,char* pVersion)
 {
	 VIRTUALIZER_FISH_BLACK_START
	 Proud::String strFilePath;
	 strFilePath.Format(_PNT("%s"), StringA2T(pFilePath));
	 strFilePath.MakeUpper();

	Proud::String strProjectName;
	Proud::String strCompanyName;
	Proud::String strLicenseType;

	if (CPNLicense::GetLicenseInfo(strProjectName, strCompanyName, strLicenseType))
	{
		//Proud::String strName; // CW2A(szName);
		//Proud::String strCompany; // CW2A(szCompany);
		//Proud::String strCustomData; // CW2A(szCustomData);
		//Proud::String strVersion;
		//Proud::String strWaterMark = strName + " " + strCompany + " " + strCustomData + " " + strVersion;
		Proud::String strWaterMark;
		strWaterMark.Format(_PNT("%s %s %s %s"), strProjectName.GetString(), strCompanyName.GetString(), strLicenseType.GetString(), StringA2T(pVersion));

		CWaterMarkMgr WaterMarkMgr;
		WaterMarkMgr.ChangeWaterMarkBinary((CStringA) strFilePath, (CStringA) strWaterMark);
	}
	VIRTUALIZER_FISH_BLACK_END
 }

 DLLEXPORT int CheckNetworkAuthentication(HWND hwnd, const char* licenseKey, const char * licensetype)
 {
	 VIRTUALIZER_FISH_BLACK_START
	 LicenseMessageType authorized = LMType_Nothing; //성공 실패 값 리턴
	 Proud::String strResultMessage;
	 Proud::String strLicenseKey = StringA2T(licenseKey);
	 Proud::String strLicenseType = StringA2T(licensetype);

	 CPNLicAuthClient licClient;
	 bool isComplete;
	 licClient.AuthorizationBlocking(strLicenseKey, strLicenseType, strResultMessage, authorized, isComplete);

	 if (authorized == LMType_RegSuccess)
	 {
		 Proud::String strAppFullPath;
		 Proud::String strAppDataPath;
		 Proud::StrBuf pPathBuf(strAppDataPath, MAX_PATH_LENGTH);
		 SHGetSpecialFolderPath(NULL, pPathBuf, CSIDL_COMMON_APPDATA, TRUE);
		 strAppFullPath.Format(_PNT("%s\\pnrglic.cab"), strAppDataPath.GetString());
		 DeleteFile(strAppFullPath.GetString());
		 
		 if (CPNLicense::InstallSmartkeyToRegistry(strLicenseKey) == false) {
			 authorized = LMType_LicenseInstallError;
		 }
	 }

	 VIRTUALIZER_FISH_BLACK_END
		 return (int)authorized;
 }


 DLLEXPORT int CheckLicenseStatus()
 {
  	// 라이선스 상태 확인 (만료되었는지 확인)
	return CPNLicense::GetStatus();
 }

 DLLEXPORT bool CheckLicenseType(char const* LicenseType)
 {
	 //라이선스 타입 확인
	 return CPNLicense::CheckLicenseType(StringA2T(LicenseType));
 }
