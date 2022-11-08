#include "stdafx.h"
#include "PNRegistry.h"

/* 레지스트리 파일 구조
1. 350바이트의 동일 값으로 구성된 헤더 - 하위호환성 감지를 위해 존재. 입력했던 키 내역을 갖고 있지 않던 과거 버전에서는 이 350바이트 헤더가 없다. 그걸 감지하기 위함.
초기 버전의 레지스트리 파일은 버전 정보가 잘못 들어가서, 제 기능을 하지 못했다. 그래서 이렇게 꼼수를 쳐야 했음.
2. 레지스트리 파일의 32바이트짜리 해시 - 레지스트리 파일을 크래킹하는 것을 감지하기 위함.
3. 레지스트리 파일 전체 (라이선스 정보, 과거 입력했던 키 내역 등)
*/


// 파일 헤더를 구성하는 350바이트 데이터의 값
const uint8_t g_fileHeaderKey = 0x33; 
// 파일 헤더 크기
const int g_fileHeaderSize = 350;
// 해시 크기(바이트)
const int g_hashBlobSize = 32;

#include "../PNLicenseManager/KernelObjectNames.h"

CPNRegistry::~CPNRegistry() {
	if (m_fs.IsOpen() == true) {
		CPNErrorInfo errorInfo;
		m_fs.CloseFile(errorInfo);
	}
	m_isLoaded = false;
};

void CPNRegistry::CreateHash(ByteArray &inBlob, ByteArray &outHash)
{
	//char msg[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
	//unsigned char hash[32] = { 0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8,
	//	0xe5, 0xc0, 0x26, 0x93, 0x0c, 0x3e, 0x60, 0x39,
	//	0xa3, 0x3c, 0xe4, 0x59, 0x64, 0xff, 0x21, 0x67,
	//	0xf6, 0xec, 0xed, 0xd4, 0x19, 0xdb, 0x06, 0xc1 };

	hash_state md;
	outHash.SetCount(g_hashBlobSize);
	std::memset(outHash.GetData(), 0, outHash.GetCount());

	//inBlob.AddRange((uint8_t*)msg, strlen(msg));
	sha256_init(&md);
	sha256_process(&md, (unsigned char*)inBlob.GetData(), (unsigned long)inBlob.GetCount());
	sha256_done(&md, outHash.GetData());
	
	//if (XMEMCMP(outHash.GetData(), hash, 32) != 0) {
	//	return false;
	//}

	//return true;

	//int i = 0;
	//for (i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++) {
		//sha256_init(&md);
		//sha256_process(&md, (unsigned char*)tests[i].msg, (unsigned long)strlen(tests[i].msg));
		//sha256_done(&md, tmp);
		//if (XMEMCMP(tmp, tests[i].hash, 32) != 0) {
		//	return CRYPT_FAIL_TESTVECTOR;
		//}
	//}
}

bool CPNRegistry::LoadLicenseInfoFromRegistryFile_Ex(ByteArray &fileReadDataBlob, CPNErrorInfo& errorInfo)
{
	CMessage regKeyMsg;
	regKeyMsg.UseExternalBuffer(fileReadDataBlob.GetData(), fileReadDataBlob.GetCount());
	regKeyMsg.SetLength(fileReadDataBlob.GetCount());

	ByteArray aesCipherBlob;
	ByteArray aesKeyBlob;
	if (regKeyMsg.Read(aesCipherBlob) == false) {
		return false;
	}
	if (regKeyMsg.Read(aesKeyBlob) == false) {
		return false;
	}

	ByteArray aesPlainBlob;
	CCryptoAesKey aesKey;
	CCryptoAes::ExpandFrom(aesKey, aesKeyBlob.GetData(), aesKeyBlob.GetCount());
	CCryptoAes::DecryptByteArray(aesKey, aesCipherBlob, aesPlainBlob);

	CMessage aesPlainMsg;
	aesPlainMsg.UseExternalBuffer(aesPlainBlob.GetData(), aesPlainBlob.GetCount());
	aesPlainMsg.SetLength(aesPlainBlob.GetCount());

	ByteArray regDataBlob;
	ByteArray regDataSignBlob;
	if (aesPlainMsg.Read(regDataBlob) == false) {
		errorInfo.AppendErrorCode(InvalidRegistryKey, "RLLIFRFEAPMR");
		return false;
	}
	if (aesPlainMsg.Read(regDataSignBlob) == false) {
		errorInfo.AppendErrorCode(InvalidRegistryKey, "RLLIFRFEAPMR2");
		return false;
	}

	ByteArray byteFeaturesBlob;
	byteFeaturesBlob.SetCount(sizeof(PNLicenseFeatures));

	CMessage regDataMsg;
	regDataMsg.UseInternalBuffer();
	regDataMsg.AppendByteArray(regDataBlob.GetData(), regDataBlob.GetCount());

	if (regDataMsg.Read(m_licenseInfo.m_version) == false) {
		errorInfo.AppendErrorCode(InvalidRegistryKey, "RLLIFRFERDMRLIV");
		return false;
	}
	if (regDataMsg.ReadString(m_licenseInfo.m_companyName) == false) {
		errorInfo.AppendErrorCode(InvalidRegistryKey, "RLLIFRFERDMRLIC");
		return false;
	}
	if (regDataMsg.ReadString(m_licenseInfo.m_projectName) == false) {
		errorInfo.AppendErrorCode(InvalidRegistryKey, "RLLIFRFERDMRLIP");
		return false;
	}
	if (regDataMsg.ReadString(m_licenseInfo.m_licenseType) == false) {
		errorInfo.AppendErrorCode(InvalidRegistryKey, "RLLIFRFERDMRLIL");
		return false;
	}
	if (regDataMsg.Read(byteFeaturesBlob) == false) {
		errorInfo.AppendErrorCode(InvalidRegistryKey, "RLLIFRFERDMRLIB");
		return false;
	}

	CPNLicenseFeaturesBuffer buf;
	buf << byteFeaturesBlob;
	buf >> m_licenseInfo.m_features;

	CRegInfoEx regInfoEx;
	memcpy_s(&regInfoEx, sizeof(CRegInfoEx), aesKeyBlob.GetData(), sizeof(CRegInfoEx));
	m_licenseInfo.m_status = (RegState)regInfoEx.regKeyValid;
	m_licenseInfo.m_lastAccessTime = regInfoEx.timeModified;

	ByteArray bytePrivateBlob;
	StringA strPrivateKey = DSA_AUTHENTICATIONKEY;
	Base64::Decode(strPrivateKey, bytePrivateBlob);
	m_licenseInfo.SetPrivateKey(bytePrivateBlob);
	m_licenseInfo.SetSignature(regDataSignBlob);
	if (m_licenseInfo.CheckValidity(errorInfo) == false) {
		errorInfo.AppendErrorCode(InvalidRegistryKey, "RLLIFRFELICV");
		return false;
	}

	return true;
}

bool CPNRegistry::LoadLicenseInfoFromRegistryFile(ByteArray &fileReadDataBlob, CPNErrorInfo& errorInfo)
{
	ByteArray hashBlob;
	ByteArray licenseBlob;

	// file header 부분을 건너뛰고, BLOB만큼을 읽어들인다.
	fileReadDataBlob.CopyRangeTo(hashBlob, g_fileHeaderSize, g_hashBlobSize);
	fileReadDataBlob.CopyRangeTo(licenseBlob, g_fileHeaderSize + g_hashBlobSize, fileReadDataBlob.GetCount() - g_fileHeaderSize - g_hashBlobSize);

	//if (IsValidRegistryFile(hashBlob, licenseBlob) == false) {
	//	errorInfo.AppendErrorCode(InvalidRegistryKey, "");
	//	return false;
	//}

	int version = 0;
	uint8_t usedKeyCount = 0;
	ByteArray aesEncryptBlob;
	ByteArray aesKeyBlob;
	String strSignature;

	CMessage msg;
	msg.UseExternalBuffer(licenseBlob.GetData(), licenseBlob.GetCount());
	msg.SetLength(licenseBlob.GetCount());
	if (msg.Read(version) == false) {
		errorInfo.AppendErrorCode(InvalidRegistryKey, "RLLIFRFMRV");
		return false;
	}
	if (msg.Read(aesEncryptBlob) == false) {
		errorInfo.AppendErrorCode(InvalidRegistryKey, "RLLIFRFMRAEB");
		return false;
	}
	if (msg.Read(aesKeyBlob) == false) {
		errorInfo.AppendErrorCode(InvalidRegistryKey, "RLLIFRFMRAKB");
		return false;
	}
	if (msg.Read(usedKeyCount) == false) {
		errorInfo.AppendErrorCode(InvalidRegistryKey, "RLLIFRFMRUKC");
		return false;
	}

	// 과거 입력했던 라이선스키의 signature들을 채운다.
	g_usedKeySignatures.clear();
	for (int i = 0; i < usedKeyCount; i++) {
		msg.ReadString(strSignature);
		g_usedKeySignatures.insert(strSignature);
	}

	ByteArray aesDecryptedBlob;
	CCryptoAesKey aesKey;
	CCryptoAes::ExpandFrom(aesKey, aesKeyBlob.GetData(), aesKeyBlob.GetCount());
	CCryptoAes::DecryptByteArray(aesKey, aesEncryptBlob, aesDecryptedBlob);

	CPNLicenseFeaturesBuffer featuresBuffer;
	String strLicenseSignature;
	ByteArray featuresBlob;
	CMessage licenseMsg;
	licenseMsg.UseExternalBuffer(aesDecryptedBlob.GetData(), aesDecryptedBlob.GetCount());
	licenseMsg.SetLength(aesDecryptedBlob.GetCount());
	if (licenseMsg.ReadString(m_licenseInfo.m_companyName) == false) {
		errorInfo.AppendErrorCode(InvalidRegistryKey, "RLLIFRFLRC");
		return false;
	}
	if (licenseMsg.ReadString(m_licenseInfo.m_projectName) == false) {
		errorInfo.AppendErrorCode(InvalidRegistryKey, "RLLIFRFLRP");
		return false;
	}
	if (licenseMsg.ReadString(m_licenseInfo.m_licenseType) == false) {
		errorInfo.AppendErrorCode(InvalidRegistryKey, "RLLIFRFLRL");
		return false;
	}
	if (licenseMsg.Read(m_licenseInfo.m_lastAccessTime) == false) {
		errorInfo.AppendErrorCode(InvalidRegistryKey, "RLLIFRFLRLAT");
		return false;
	}
	if (licenseMsg.ReadString(strLicenseSignature) == false) {
		errorInfo.AppendErrorCode(InvalidRegistryKey, "RLLIFRFLRSLS");
		return false;
	}
	if (licenseMsg.Read(featuresBlob) == false) {
		errorInfo.AppendErrorCode(InvalidRegistryKey, "RLLIFRFLRFB");
		return false;
	}
		
	featuresBuffer << featuresBlob;
	featuresBuffer >> m_licenseInfo.m_features;

	ByteArray bytePrivateBlob;
	StringA strPrivateKey = DSA_AUTHENTICATIONKEY;
	Base64::Decode(strPrivateKey, bytePrivateBlob);
	m_licenseInfo.SetPrivateKey(bytePrivateBlob);
	m_licenseInfo.SetSignature(strLicenseSignature);
	if (m_licenseInfo.CheckValidity(errorInfo) == false) {
		errorInfo.AppendErrorCode(InvalidRegistryKey, "RLLIFRFLICV");
		return false;
	}

	return true;
}

bool CPNRegistry::CreateEmptyRegistryFileIfNotExists(CPNErrorInfo& errorInfo)
{
	VIRTUALIZER_TIGER_RED_START

	String regFilePath;
	if (false == GetRegistryFilePath(regFilePath)) {
		errorInfo.AppendErrorCode(NotLoadedRegistry, "CERFNEGRFP");
		return false;
	}

	if (false == m_fs.Exists(regFilePath, errorInfo))
	{
		if (false == m_fs.CreateAllUserWritableRegistryFile(regFilePath, CPNFileSystem::WRITEONLY, errorInfo))
		{
			errorInfo.AppendErrorCode(NotLoadedRegistry, "CERFNEFSCAWF");
			return false;
		}
	}
	VIRTUALIZER_TIGER_RED_END
	return true;
}

PNLicErrorCode CPNRegistry::LoadRegistryFile(CPNErrorInfo& errorInfo)
{
	VIRTUALIZER_TIGER_RED_START
	String regFilePath;
	if (false == GetRegistryFilePath(regFilePath)) {
		errorInfo.AppendErrorCode(NotLoadedRegistry, "LRFGRFP");
		return NotLoadedRegistry;
	}

	// 레지스트리 파일을 오픈한다. 
	if (false == m_fs.Exists(regFilePath, errorInfo)) {
		// 레지스트리 파일이 없다.
		// 이럴 경우에는 퍼스널 라이선스로 동작하게 만들기로 했으니
		// 빈 레지스트리 파일도 생성하면 안된다.
		errorInfo.AppendErrorCode(NotLoadedRegistry, "LRFFSEEIAEC");
		return RegistryFileNotExists;
	}
	else
	{
		if (m_fs.OpenFile(regFilePath, CPNFileSystem::READWRITE, errorInfo) == false)
		{
			errorInfo.AppendErrorCode(NotLoadedRegistry, "LRFFSOF");
			return NotLoadedRegistry;
		}
	}

	CPNFileInfo fileInfo;
	m_fs.GetFileState(fileInfo, errorInfo);

	// 레지스트리 키를 로드합니다.
	ByteArray fileReadDataBlob;
	fileReadDataBlob.SetCount(PNMAX(fileInfo.st_size, 1));
	if (m_fs.ReadBinary(fileReadDataBlob, fileReadDataBlob.GetCount(), errorInfo) == false) {
		m_fs.CloseFile(errorInfo);
		errorInfo.AppendErrorCode(NotLoadedRegistry, "LRFFSRB");
		return InvalidRegistryKey;
	}
	
	if (fileReadDataBlob.GetCount() < g_fileHeaderSize)
	{
		//옛날 포맷으로 파일을 로딩한다. 
		// 레지스트리 키를 역직렬화(Deserialize) 합니다. 
		if (LoadLicenseInfoFromRegistryFile_Ex(fileReadDataBlob, errorInfo) == false) {
			m_fs.CloseFile(errorInfo);
			errorInfo.AppendErrorCode(NotLoadedRegistry, "LRFLLIFRFE");
			return InvalidRegistryKey;
		}
	}
	else
	{
		//ByteArray fileVerifyBlob;
		ByteArray fileHeaderBlob;
		fileHeaderBlob.CopyFrom(fileReadDataBlob.GetData(), g_fileHeaderSize);
		//for (int i = 0; i < g_fileHeaderSize; i++) {
		//	fileVerifyBlob.Add(g_fileHeaderKey);
		//}

		// if(fileVerifyBlob.Equals(fileHeaderBlob) == true) 구문은 1.7.28714 버전 이하 PNLicenseAuth.exe 실행시 파일 앞부분 바이너리가 달라질 수 있어 적용하지 못함
		if (fileHeaderBlob[g_fileHeaderSize - 5] == g_fileHeaderKey && fileHeaderBlob[g_fileHeaderSize - 1] == g_fileHeaderKey)
		{
			if (LoadLicenseInfoFromRegistryFile(fileReadDataBlob, errorInfo) == false) {
				m_fs.CloseFile(errorInfo);
				errorInfo.AppendErrorCode(NotLoadedRegistry, "LRFLLIFRF");
				return InvalidRegistryKey;
			}
		}
		else
		{
			m_fs.CloseFile(errorInfo);
			errorInfo.AppendErrorCode(NotLoadedRegistry, "LRFFSCF");
			return InvalidRegistryKey;
		}
	}

	m_fs.CloseFile(errorInfo);
	UpdateLastAccessTimeAndStatus();
	m_isLoaded = true;
	VIRTUALIZER_TIGER_RED_END
	return PNLicErrorCode::OK;
}

// 사용된 적이 없는 새 키인가?
bool CPNRegistry::IsUnusedLicenseKey(const String &strSignature)
{
	// 있으면
	if (g_usedKeySignatures.find(strSignature) != g_usedKeySignatures.end()) {
		return false;
	}

	return true;
}

bool CPNRegistry::IsValidLicenseStatus(const uint32_t status)
{
	if ((status & BoundaryErrorRegKey) ||
		(status & ExpiredRegKey) ||
		(status & InValidRegKeyMTime) ||
		(status & InValidSysTime) ||
		(status & InValidRegKey)) {
		return false;
	}

	return true;
}

void CPNRegistry::UpdateLastAccessTimeAndStatus()
{
	time_t curSystemTime; time(&curSystemTime);

	// 레지스트리 메모리에서 레지스트리 파일 변경시간 값을 읽어옵니다.
	uint64_t licExpTime = 0; GetExpirationDate(&licExpTime);
	uint64_t licIssuedTime = 0; GetIssueDate(&licIssuedTime);
	uint64_t modifiedTime = 0; modifiedTime = m_licenseInfo.m_lastAccessTime;

	const time_t csCurSystemTime = curSystemTime;
	const time_t csLicExpTime = licExpTime;
	const time_t csLicIssuedTime = licIssuedTime;
	const String strSignature = m_licenseInfo.GetSignatureText();

	// 레지스트리 파일의 마지막 변경시간을 알 수 없으므로, 신규 설치로 판단합니다.
	if (modifiedTime == 0 && IsUnusedLicenseKey(strSignature) == true) {
		if (csCurSystemTime > csLicIssuedTime) {
			modifiedTime = csCurSystemTime;
		}
		else{
			modifiedTime = csLicIssuedTime;
		}
	}
	else
	{
		if (csCurSystemTime > modifiedTime) {
			modifiedTime = csCurSystemTime;
		}
	}

	const time_t csRegKeyModifiedTime = modifiedTime;
	
	// 라이선스 유효성여부를 진단합니다. 
	// 가령 시간 롤백 공격을 했다면 현재까지 입력된 모든 라이선스는 무효 처리됩니다.
	GetLicenseStatus(csCurSystemTime, csRegKeyModifiedTime, csLicIssuedTime, csLicExpTime, &m_licenseInfo.m_status);
		
	if (IsUnusedLicenseKey(strSignature) == true) 
	{
		if (IsValidLicenseStatus(m_licenseInfo.m_status) == false)
		{
			// 무효화키를 감지하면 기 입력했던 라이선스 키 내역에 기존 키도 등록합니다. 
			// 현재까지는 유효 라이선스 키였으나 무효화 상태가 된 경우, 무효화 키로 등록합니다. 
			g_usedKeySignatures.insert(strSignature);
		}
	}
	else
	{
		uint32_t tmpStatus = m_licenseInfo.m_status;
		tmpStatus &= ~ValidRegKey;
		tmpStatus |= InValidRegKey;
		m_licenseInfo.m_status = (RegState)tmpStatus;
	}

	m_licenseInfo.m_lastAccessTime = csRegKeyModifiedTime;
}

bool CPNRegistry::SaveRegistryFile(CPNErrorInfo& errorInfo)
{
	VIRTUALIZER_TIGER_BLACK_START

	String regFilePath;
	if (GetRegistryFilePath(regFilePath) == false) {
		errorInfo.AppendErrorCode(NotLoadedRegistry, "RSRFGRF");
		return false;
	}

	//// 레지스트리 파일을 오픈한다. 
	//CPNFileSystem fs;
	if (m_fs.Exists(regFilePath, errorInfo) == false) {
		if (m_fs.CreateAllUserWritableRegistryFileAndOpen(regFilePath, CPNFileSystem::WRITEONLY, errorInfo) == false) {
			errorInfo.AppendErrorCode(NotLoadedRegistry, "RSRFFSCAWF");
			return false;
		}
	}

	if (m_fs.IsOpen() == false) {
#if defined(WIN32)
		if (m_fs.OpenFile(regFilePath, CPNFileSystem::READWRITE, errorInfo) == false) {
#else
		if (m_fs.OpenFile(regFilePath, CPNFileSystem::WRITEONLY, errorInfo) == false) {
#endif
			errorInfo.AppendErrorCode(NotLoadedRegistry, "RSRFFSOF");
			return false;
		}
	}

	UpdateLastAccessTimeAndStatus();
	
	CPNLicenseFeaturesBuffer featuresBuffer;
	featuresBuffer << m_licenseInfo.m_features;

	ByteArray aesPlainBlob;
	CMessage aesPlainMsg;
	aesPlainMsg.UseInternalBuffer();
	aesPlainMsg.WriteString(m_licenseInfo.m_companyName);
	aesPlainMsg.WriteString(m_licenseInfo.m_projectName);
	aesPlainMsg.WriteString(m_licenseInfo.m_licenseType);
	aesPlainMsg.Write(m_licenseInfo.m_lastAccessTime);
	aesPlainMsg.WriteString(m_licenseInfo.GetSignatureText());
	aesPlainMsg.Write(featuresBuffer.ToByteArray());
	aesPlainMsg.CopyTo(aesPlainBlob);

	CCryptoAesKey aesKey;
	ByteArray aesKeyBlob;
	MakeRandomByteArray(aesKeyBlob, 32); 
	CCryptoAes::ExpandFrom(aesKey, aesKeyBlob.GetData(), aesKeyBlob.GetCount());

	ByteArray aesEncryptBlob;
	CCryptoAes::EncryptByteArray(aesKey, aesPlainBlob, aesEncryptBlob, 0);

	ByteArray fileBlob;
	uint8_t usedKeyCount = g_usedKeySignatures.size();
	CMessage fileMsg;
	fileMsg.UseInternalBuffer();
	fileMsg.Write(g_pnlicenseSDK_version);
	fileMsg.Write(aesEncryptBlob);
	fileMsg.Write(aesKeyBlob);
	fileMsg.Write(usedKeyCount);

	for (auto i : g_usedKeySignatures) {
		String strSignature = i.GetString();
		fileMsg.WriteString(strSignature);
	}

	fileMsg.CopyTo(fileBlob);
	
	ByteArray hashBlob;
	CreateHash(fileBlob, hashBlob);

	ByteArray fileHeaderBlob;
	fileHeaderBlob.SetCount(g_fileHeaderSize);
	for (int i = 0; i < fileHeaderBlob.GetCount(); i++)
	{
		fileHeaderBlob[i] = g_fileHeaderKey;
	}

	// 여러 히든앱들이 ProgramData\pnrglic.cab (윈도우) 혹은 etc/.pnrglic (리눅스)에 
	// 쓰기행위를 하는 것을 막기 위한 처리. 윈도우는 뮤텍스로 리눅스는 세마포어로 배타적 접근이 이루어지도록 만듬.
#if defined(WIN32)
		HANDLE hMutex = CreateMutex(NULL, FALSE, PN_MUTEX_NAME);
		if (hMutex == NULL)
		{
			NTTNTRACE("CreateMutex failed");
			return false;
		}
		DWORD result = WaitForSingleObject(hMutex, INFINITE);
		if (result == WAIT_FAILED)
		{
			NTTNTRACE("WaitForSingleObject failed");
			return false;
		}
#else
		sem_t *sem;
		sem = sem_open(LINUX_SEMAPHORE_NAME, O_CREAT, 0777, 1);
		if (sem == SEM_FAILED)
		{
			int e = errno;
			std::cout << std::endl << "semaphore open failed."
				<< "strerror(e) : " << strerror(e)
				<< "errno : " << e 
				<< std::endl;
			return false;
		}
		
		int result = sem_wait(sem);
		if (result == -1)
		{
			std::cout << "sem_wait failed." << std::endl;
			sem_close(sem);
			sem_unlink(LINUX_SEMAPHORE_NAME);
			return false;
		}
#endif

	m_fs.WriteBinary(fileHeaderBlob, g_fileHeaderSize, errorInfo);
	m_fs.WriteBinary(hashBlob, hashBlob.GetCount(), errorInfo);
	m_fs.WriteBinary(fileBlob, fileBlob.GetCount(), errorInfo);
	m_fs.CloseFile(errorInfo);

#if defined(WIN32)
		if(!CloseHandle(hMutex))
		{
			NTTNTRACE(std::to_string(::GetLastError()).c_str());
			NTTNTRACE("CloseHandle failed");
			return false;
		}
#else
		// 이 sem_post는 세마포어 카운트를 1늘린다.
		result = sem_post(sem);
		if (result == -1)
		{
			std::cout << "sem_post failed." << std::endl;
			sem_close(sem);
			sem_unlink(LINUX_SEMAPHORE_NAME);
			return false;
		}
		result = sem_close(sem);
		if (result == -1)
		{
			std::cout << "sem_close failed." << std::endl;
			sem_unlink(LINUX_SEMAPHORE_NAME);
			return false;
		}
		// 리눅스에서는 세마포어를 close만 해서는 해당 세마포어를 사용하고 있는 프로세스가 없더라도 안 없어진다.
		// sem_unlink를 해도 세마포어가 바로 없어지는 것은 아니고 세마포어를 사용하고 있는 프로세스가 없어야
		// 커널이 제거를 해준다. unlink를 해주면 커널이 관리하고 있는 명부에서 세마포어의 이름만 사라질 뿐이다.
		// 때문에 unlink를 해도 이 세마포어로 sem_wait를 하고 있는 프로세스들은 정상적으로 동작한다.
		sem_unlink(LINUX_SEMAPHORE_NAME);
#endif
		
		VIRTUALIZER_TIGER_BLACK_END

	return true;
}

bool CPNRegistry::IsLoaded()
{
	return m_isLoaded;
}

void CPNRegistry::GetCompanyName(String &outstr) {
	outstr = m_licenseInfo.m_companyName;
}

void CPNRegistry::GetProjectName(String &outstr) {
	outstr = m_licenseInfo.m_projectName;
}

void CPNRegistry::GetLicenseType(String &outstr) {
	outstr = m_licenseInfo.m_licenseType;
}

void CPNRegistry::GetIssueDate(uint64_t *outval) {
	*outval = mkUTCTime(m_licenseInfo.m_features.IssueDate);
}

void CPNRegistry::GetExpirationDate(uint64_t *outval) {
	*outval = mkUTCTime(m_licenseInfo.m_features.ExpDate);
}

void CPNRegistry::GetStatus(uint32_t *outval) {
	*outval = m_licenseInfo.m_status;
}

// 라이선스 키를 레지스트리 메모리에 활성화합니다. 
// #PNLIC_REFACTOR 이 함수의 콜러는, "입력한 키가 유효한가?"이다. 그런데 이 함수는 키를 메모리에 활성화하는, 
// 즉 전역 상태에 뭔가 변화를 가한다. 현재 오동작은 없으나, 혼란을 유발하는 부분이라, 유지보수 중 위험할 수 있다. 리팩토링 필요.
// 자세한건 https://trello.com/c/Y5aIO61Z/135-111 에 명세함.
bool CPNRegistry::ActivateRegistryMemoryWithLicensekey(String strLicensekey, CPNErrorInfo& errorInfo)
{
	CPNLicenseInfo licenseInfo;
	ByteArray bytePrivateBlob;

	// 암호키를 복호화하기 위한 공개키를 준비한다.
	// DSA_AUTHENTICATIONKEY는 하드코딩 되어 있고, 이 준비는 여러번 호출하지만, 내부적으로 바뀌는 것은 없다.
	// #PNLIC_REFACTOR m_licenseInfo는, 설치된 라이선스키의 정보인데, 왜 여기에 복호화용 공개키가 들어가는가? 리팩토링을 해야 한다.
	StringA strPrivateKey = DSA_AUTHENTICATIONKEY;
	Base64::Decode(strPrivateKey, bytePrivateBlob);
	m_licenseInfo.SetPrivateKey(bytePrivateBlob);

	// 라이선스 키를 인증키로 검증합니다. 
	if (m_licenseInfo.VerifyLicenseKey(strLicensekey, errorInfo) == false) {
		errorInfo.AppendErrorCode(NotActivatedRegistryMemory, "RARMWLLIVLK");
		return false;
	}

	// 라이선스 정보의 유효성을 확인합니다. 
	if(m_licenseInfo.CheckValidity(errorInfo) == false) {
		errorInfo.AppendErrorCode(NotActivatedRegistryMemory, "RARMWLLICV");
		return false;
	}
	
	return true;
}

// 레지스트리 파일 경로를 가져옵니다.  
bool CPNRegistry::GetRegistryFilePath(String &filePath)
{
#ifdef _WIN32
	String strRootPath;
	{
		StrBuf pStrRootPath(strRootPath, MAX_PATH_LENGTH);
		if (SHGetSpecialFolderPath(NULL, pStrRootPath, CSIDL_COMMON_APPDATA, TRUE) == FALSE) {
			return false;
		};
	}
	filePath.Format(_PNT("%s\\%s"), strRootPath, StringA2T(PATH_REGISTRY_FILE));
#else
	//StringA temp = StringT2A(PATH_REGISTRY_FILE);
	filePath.Format(_PNT("%s"), PATH_REGISTRY_FILE /*temp.GetString()*/);
#endif
	return true;
}

// 레지스트리 파일이 존재하는지 체크하여 존재하지 않으면 레지스트리 파일을 생성합니다. 
bool CPNRegistry::CreateRegistryFile(CPNErrorInfo& errorInfo)
{
	VIRTUALIZER_TIGER_BLACK_START
		String regFilePath;
	if (GetRegistryFilePath(regFilePath) == false) {
		errorInfo.AppendErrorCode(NotCreatedRegistry, "RCRFGRF");
		return false;
	}

	CPNFileSystem fs;
	if (fs.Exists(regFilePath, errorInfo) == false)	{
		if (fs.CreateAllUserWritableRegistryFileAndOpen(regFilePath, CPNFileSystem::WRITEONLY, errorInfo) == false) {
			errorInfo.AppendErrorCode(NotCreatedRegistry, "RCRFFSCAWF");
			return false;
		}
	}

	fs.CloseFile(errorInfo);
	VIRTUALIZER_TIGER_BLACK_END
	return true;
}

// 레지스트리 메모리(레지스트리 파일을 메모리로 불러온 데이터)의 유효성을 판단합니다. 
// 현재 시간도 체크해서, 시간 롤백 공격을 가했으면 무효라고 리턴합니다.
void CPNRegistry::GetLicenseStatus(const time_t curSysTime, const time_t regkeyMTime, /*const time_t regFileCTime, const time_t regFileMTime,*/ const time_t licIssuedTime, const time_t licExpTime, RegState *outValid)
{
	VIRTUALIZER_TIGER_BLACK_START
	uint32_t tmpValid = *outValid;
	tmpValid = ValidRegKey;

	if (licIssuedTime < 1) {
		tmpValid &= ~ValidRegKey;
		tmpValid |= BoundaryErrorRegKey;
	}

	// 1. 라이선스 발급일자와 만료일자가 뒤 바뀐 경우, 발급 오류로 판단
	if (licExpTime <= licIssuedTime) {
		tmpValid &= ~ValidRegKey;
		tmpValid |= BoundaryErrorRegKey;
	}

	// 2. 현재시간이 발급일자보다 과거일 경우, 시스템시간조작으로 판단
	if (curSysTime < licIssuedTime) {
		tmpValid &= ~ValidRegKey;
		tmpValid |= InValidSysTime;
	}

	// 3. 현재시간이 만료일자보다 미래일 경우, 라이선스 기간 만료!
	if (curSysTime > licExpTime) {
		tmpValid &= ~ValidRegKey;
		tmpValid |= ExpiredRegKey;
	}

	// 4. 현재시간이 레지스트리 변경시간 보다 과거일 경우, 시스템시간조작으로 판단
	if (regkeyMTime - curSysTime > 60*10) { // 그래도 10분 정도의 오차는 인정해주자. 중앙 서버에서 시간을 조금씩 깨먹는 경우 때문이다.
		tmpValid &= ~ValidRegKey;
		tmpValid |= InValidSysTime;
	}

	// 10. 레지스트리 변경시각이 0 이하 일 때, 레지스트리키 시간조작으로 판단
	if (regkeyMTime < 1) {
		tmpValid &= ~ValidRegKey;
		tmpValid |= InValidRegKeyMTime;
	}

	*outValid = (RegState)tmpValid;

	VIRTUALIZER_TIGER_BLACK_END
}

void CPNRegistry::MakeRandomByteArray(ByteArray &byteBlob, int byteLength)
{
	Proud::CRandom rand;
	rand.InitializeSeed();

	byteBlob.Clear();
	for (int i = 0; i < byteLength; i++) {
		byteBlob.Add(rand.GetInt());
	}
}

void CPNRegistry::GetLastError(CPNErrorInfo &errorInfo) {
	errorInfo = m_errorInfo;
}

void CPNRegistry::SetLicenseInfo(const CPNLicenseInfo& info){
	m_licenseInfo = info;
}