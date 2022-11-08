#pragma once

#include <iostream>
#include <new>
#include <time.h>
#include <string>
#include <stdio.h>	

#if defined(WIN32)
#include <shlobj.h>
#include "stdafx.h"
#else
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include "PNErrorInfo.h"
#include "PNSecure.h"
#include "PNTime.h"
#include "PNFileSystem.h"
#include "PNLicenseInfo.h"
#include "PNRegistryInfo.h"
#include "PNLicenseSDK.h"

#include "../AuthNet/include/CryptoAes.h"
#include "../AuthNet/OpenSources/libtom/crypto/headers/tomcrypt.h"
#include "../CodeVirtualizer/Include/C/VirtualizerSDK.h"

#include "../AuthNet/src/PooledObjects.h"
#include "../AuthNet/src/DefaultStringEncoder.h"

using namespace std;
using namespace Proud;

// DSA Private Key 라이선스키 무결성 검증키로 사용
#define DSA_AUTHENTICATIONKEY "MIICXAIBAAKBgQDPQS4/2Ko7wkHOOWXc3WIh/UQpL423m6zgLF0oyROBQiinLOOdMVditrqkzlvWtoybosSSsxThHrBWEb65qG4yqJRK1nTeP7HjfSVD22ZE1fI0Xhcz7CdRN89+OpjGr6VKjdNjg2uTrttDH4+x5B6gCr2Fo0dPlfq7SXakgG52JQIDAQABAoGAJNdbkw5e7moA3BmAZbM1wUB92Atjt/Z0k9HXCou5y6GYy+TTHit58up0AZ1MHn4LPxQ/OKucQ8s6gcY8PtD1q3o+vh8zLys5XMkdUOkGrK+UG6Hqb7tMGlqV06DInFqbveTkGnn/VqpO0prr/NOnUheCvZhq5loWvfCeXTBrq3kCQQDoCzFtlmZOkAs+nwBDnXL7hVRK2yzQEGflDuUyONs80TCF7/CicdmT5t5IiqhzhYjxTF86lUNFfao8jCMZ97BZAkEA5KbSr225gAFywSalX3Yc/BbwQk2Fm4kYIBG1EIWltPs3dSFe9N3e1TOgyX4Ak08weRAsWureajrYW9ZJltVarQJASbSseaPJUXEdsUFuIwwTJuOd970QyfI8Hh0SHlbDBNlpsVGavO6u7vTpbF9mzHMBIaxhn0kkOiGFfoAA8lGj2QJBAIkE81JPNY9gzsyyhP1cwWfLszR4Ui1vjTaChfedrzxyIrydP9MLNiKbKqo0SNH97XVO3NWq05fjJY57LmQl/I0CQH2/1lZHzOesARb9fe0RLYVOlI6L0k/RG6fnWf5HBna+iYRHluP0uf6DyPxa/7YLN5Rs79l/2CCZgi49YgsNh5M=";
// :: DSA_AUTHENTICATIONKEY 값을 정의 하는 방법 :: 
// PNlicenseSDKDll 프로젝트에서 PNGenPublickeyAndPrivatekey() API를 실행하여 privatekey.txt 파일에 생성된 값을 넣으면 됩니다.

#if defined(DEBUG) || defined(_DEBUG)
// PATH_PROUDNET_LICENSE_FILE: 테스트용으로 임시 사용하는 라이선스키 파일경로변수로 릴리즈시 사용안함 (테스트용으로 암호화 하지 않습니다.)
#define PATH_PROUDNET_LICENSE_FILE _PNT("../proudnet-licensekey.txt");
#endif

#ifdef __linux__
#define NEWLINE_CHAR _PNT("\n");
#define PATH_REGISTRY_FILE "/etc/.pnrglic" //_PNT("L2V0Yy8ucG5yZ2xpYwA=");
#else
#define NEWLINE_CHAR _PNT("\r\n");
#define PATH_REGISTRY_FILE "pnrglic.cab" // _PNT("QzpccHJvZ3JhbWRhdGFccG5yZ2xpYy5jYWIA");
#endif

#include "../PNLicenseManager/Defines.h"

#if defined(WIN32)
typedef struct _stat64i32 CPNFileInfo;
#else
typedef struct stat CPNFileInfo;
#endif

// 사용자가 입력한 라이선스 키는, 시스템 액세스가 가능한 숨은 파일에 저장된다. 이를 레지스트리 파일이라고 명명하자.
// 레지스트리 파일은 라이선스 설치앱에 의해 만들어지고, NetServer를 실행할 떄 액세스된다.
// 라이선스 설치앱은 루트 권한이 있는 계정으로 실행되는데, 이떄 시스템 폴더에 레지스트리 파일을 만든다.
// NetServer는 루트 권한이 없는 계정으로 실행되기도 하는데, 이미 레지스트리 파일은 all user access 상태이기 떄문에 읽기,쓰기가 모두 가능하다.
class CPNRegistry
{
public:
	~CPNRegistry();

	void GetCompanyName(String &outstr);
	void GetProjectName(String &outstr);
	void GetLicenseType(String &outstr);
	void GetIssueDate(uint64_t *outval);
	void GetExpirationDate(uint64_t *outval);
	void GetStatus(uint32_t *outval);

	bool CreateEmptyRegistryFileIfNotExists(CPNErrorInfo& errorInfo);

	// 라이선스 정보를 얻기위해 레지스트리 파일을 로드합니다. 
	PNLicErrorCode LoadRegistryFile(CPNErrorInfo& errorInfo);

	// 갱신된 라이선스 정보를 레지스트리 파일에 저장합니다. 
	bool SaveRegistryFile(CPNErrorInfo& errorInfo);

	// 라이선스 키를 레지스트리 메모리에 활성화합니다. 
	bool ActivateRegistryMemoryWithLicensekey(String strLicensekey, CPNErrorInfo& errorInfo);

	// 라이선스 인포를 셋합니다.
	void SetLicenseInfo(const CPNLicenseInfo& info);

private:

	// 레지스트리 파일에서 라이선스 정보를 로드합니다. (1.7.28714 이하 버전)
	bool LoadLicenseInfoFromRegistryFile_Ex(ByteArray &fileReadDataBlob, CPNErrorInfo& errorInfo);

	// 레지스트리 파일에서 기 설치되어 있던 라이선스 정보를 로드합니다.
	bool LoadLicenseInfoFromRegistryFile(ByteArray &fileReadDataBlob, CPNErrorInfo& errorInfo);

	// 레지스트리 파일이 로드되었는지 확인합니다. 
	bool IsLoaded();

	// 레지스트리 파일 액세스시간과 라이선스 상태를 갱신합니다. 
	void UpdateLastAccessTimeAndStatus();

	// 라이선스 유효상태를 검사합니다. 
	bool IsValidLicenseStatus(const uint32_t status);

	// 라이선스 시크니처 유효성을 검사합니다. 
	bool IsUnusedLicenseKey(const String &strSignature);

	// 레지스트리 파일 경로를 가져옵니다.  
	bool GetRegistryFilePath(String &filePath);

	// 레지스트리 파일이 존재하는지 체크하여 존재하지 않으면 레지스트리 파일을 생성합니다. 
	bool CreateRegistryFile(CPNErrorInfo& errorInfo);

	// 라이선스 유효성을 판단합니다. 
	void GetLicenseStatus(const time_t curSysTime, const time_t regkeyMTime, const time_t licIssuedTime, const time_t licExpTime, RegState *outValid);
	
	// 랜덤블럭을 생성합니다. 
	void MakeRandomByteArray(ByteArray &byteBlob, int byteLength);

	// 마지막 오류를 얻습니다. 
	void GetLastError(CPNErrorInfo &errorInfo);

	// 해시키를 생성합니다. 
	void CreateHash(ByteArray &inBlob, ByteArray &outHash);

private:
	bool    m_isLoaded = false;
	CPNFileSystem m_fs;
	CPNErrorInfo m_errorInfo;
	CPNLicenseInfo m_licenseInfo;
};


extern CPNRegistry g_registry;
extern std::set<String> g_usedKeySignatures;
extern int g_pnlicenseSDK_version;