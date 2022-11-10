#pragma once

#include <ctime>
#include <cstring>

#if defined(_WIN32)
#include <atlstr.h>
#include <tchar.h>
#else 
#include <stdlib.h>
#include "PNSecure.h"
#endif

#include "PNTime.h"
#include "PNFileSystem.h"

#include "../[AuthNet]/src/Base64.h"
#include "../[AuthNet]/include/ByteArray.h"
#include "../[AuthNet]/include/CryptoRsa.h"
#include "../[AuthNet]/include/StringEncoder.h"
#include "../[AuthNet]/include/ProudNetCommon.h"

#ifdef _WIN32
#define pnttoi(x) _ttoi(x)
#else
#define pnttoi(x) atoi(x)
#endif

#ifdef _MSC_VER
#define ATTRIBUTE_ALIGN(n) __declspec(align(n))
#else
#define ATTRIBUTE_ALIGN(n) __attribute__ ((aligned(n)))
#endif

extern int g_pnlicenseSDK_version;

using namespace Proud;

enum RegState
{
	StartRegState = 0,
	ExistRegKey = 1,					// Registrykey exists on pc
	NotExistRegKey = 2,					// Registrykey does not exist on pc
	BoundaryErrorRegKey = 4,					// Registrykey is over the permissible range
	ExpiredRegKey = 8,					// Registrykey is Expired 
	InValidSysTime = 16,					// System Time Fraud detected
	InValidRegKeyMTime = 32,					// The last modified time of the registrykey is invalid
	InValidRegFileCTime = 64,					// The create time of the registry file is invalid 
	InValidRegFileMTime = 128,					// The last modified time of the registry file is invalid 
	InValidRegKey = 0x4000,				// Registrykey is invalid
	ValidRegKey = 0x8000,				// Registrykey is valid
	EndRegState = 0xFFFF
};

// ***********************************************
// LinLicense typedef definition
// ***********************************************

// 발급일, 만료일 등을 저장하는 구조체. C# code에서 호출할때 인자로 넘겨진다.
ATTRIBUTE_ALIGN(4) 
struct PNLicenseFeatures 
{
	SYSTEMTIME_t	ExpDate;                // expiration date 
	SYSTEMTIME_t	IssueDate;				// Date to issue the license since it was created
};

class CPNLicenseFeaturesBuffer {
private:
	ByteArray m_byteBuffer;

public:

	ByteArray ToByteArray()
	{
		return m_byteBuffer;
	}

	void AddRange(uint8_t* data, int length)
	{
		m_byteBuffer.AddRange(data, length);
	}

	void CopyRangeTo(ByteArray &data, int size)
	{
		memcpy_s((void*)data.GetData(), size, m_byteBuffer.GetData(), sizeof(PNLicenseFeatures));
	}

	void CopyRangeTo(PNLicenseFeatures &data)
	{
		memcpy_s((void*)&data, sizeof(PNLicenseFeatures), m_byteBuffer.GetData(), sizeof(PNLicenseFeatures));
	}

	template<class T>
	friend T& operator<<(T &a, const ByteArray &b)
	{
		a.AddRange((uint8_t*)b.GetData(), b.GetCount());
		return a;
	}

	template<class T>
	friend T& operator<<(T &a, const PNLicenseFeatures &b)
	{
		a.AddRange((uint8_t*)&b.ExpDate, sizeof(SYSTEMTIME_t) / sizeof(uint8_t));
		a.AddRange((uint8_t*)&b.IssueDate, sizeof(SYSTEMTIME_t) / sizeof(uint8_t));

		return a;
	}

	template<class T>
	friend T& operator>>(T &a, ByteArray &b)
	{
		a.CopyRangeTo(b, sizeof(PNLicenseFeatures));
		return a;
	}

	template<class T>
	friend T& operator>>(T &a, PNLicenseFeatures &b)
	{
		a.CopyRangeTo(b);
		return a;
	}
};

class CPNLicenseInfo
{
public:
	PNLicenseFeatures m_features;
	time_t m_lastAccessTime;  // last access time
	// BoundaryErrorRegKey, ExpiredRegKey, InValidRegKeyMTime, InValidSysTime, InValidRegKey
	// 상기의 열거형 상수값들 중 하나를 인증상태로 갖는 함수입니다.
	RegState m_status;		  // license status

	String m_projectName;
	String m_companyName;
	String m_licenseType;
	// (ProgramData\pnrglic.cab(윈도우)이나 etc\.pnrglic(리눅스)에서 추출한 라이선스 버전이 들어갑니다.)
	int m_version;			  // license version 

private:
	// 이건 내부 키생성 앱, 외부 설치앱에 모두 사용됨
	CCryptoRsa m_rsa;
	// base64를 encode할 때 쓰이는 private key로서 키생성 앱에 하드코딩되어 들어가집니다.
	StringA m_strBase64VerifyKey;
	// base64를 encode할 때 쓰이는 public key로서 인증앱에 하드코딩되어 들어가집니다.
	StringA m_strBase64GenerationKey;
	// 서명자가 인증정보를 해쉬해서 얻은 값을 private key로 암호화해서 보낸것을 가지고 있는 변수입니다.
	String m_strSignature;

public:
	CPNLicenseInfo() {
		m_lastAccessTime = 0;
		m_status = RegState::StartRegState;
		m_version = 0;
		memset((void*)&m_features, 0x00, sizeof(PNLicenseFeatures));
	};
	~CPNLicenseInfo() {};

	String ToString();
	bool Equals(const CPNLicenseInfo& rhs) const;
	void ToBinary(ByteArray &outBinary);
	void SetPublicKey(ByteArray &binary);
	void SetPrivateKey(ByteArray &binary);
	void SetSignature(ByteArray &binary);
	void SetSignature(String &signature);

	void CreateLicenseKey(String &signedDocument, CPNErrorInfo& errorInfo);
	bool ParseLicenseKey(String &licenseKeyText, CPNLicenseInfo &outLicenseInfo, String &outSignatureText);
	bool CreateSignature(ByteArray& document, ByteArray& signature, CPNErrorInfo& errorInfo);
	bool VerifyLicenseKey(String licenseKey, CPNErrorInfo& errorInfo);
	bool CheckValidity(CPNErrorInfo& errorInfo);

	void GetSignatureBinary(ByteArray& outBinary);
	String GetSignatureText();
	StringA GetVerifyKeyText();
	StringA GetGenerationKeyText();

private:
	void AddNewLineOnNeed(String &str);
	void CreateDocumentHash(ByteArray& document, ByteArray &outHash);
	void BinaryToText(ByteArray &binary, Proud::String &outText);
	void TextToBinary(Proud::String text, ByteArray &outBinary);
	bool ParseLine(const Proud::String &line, const Proud::String& name, Proud::String &outValue);
};

