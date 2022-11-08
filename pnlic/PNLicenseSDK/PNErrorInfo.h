#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <stdint.h>

enum PNLicErrorCode
{
	OK = 0,

	// 30001~ 39999 : PNRegistry 오류
	NotInitializedRegistry = 30001,
	NotCreatedRegistry,
	NotLoadedRegistry,
	NotSavedRegistry,
	NotMadeRegistry,
	NotInstalledRegistry,
	NotUpdatedRegistry,
	NotLoadedRegistryMemory,
	NotActivatedRegistryMemory,
	NotVerifiedKey,
	NotPassedValidity,
	NotCreatedSignature,
	NotGeneratedDSA,
	InvalidRegistryKey,
	RSADecryptionError,
	RegistryFileNotExists,

	// 40001~ 49999 : PNLicenseSDK 오류 
	InvalidLicenseInfo = 40001,
	InvalidLicenseKey,
	NotInstalledLicenseKey,
	NotLoadLicenseInfo,

	// 50001~ 59999 : App 오류
	PNErrorInvalidArgCount = 50001,
	PNErrorInvalidMagicKey,
	PNErrorInvalidMagicNum,
	PNErrorCreateThread,
	PNErrorThreadJoin,
	PNErrorGetLicense,
	PNErrorOnlineAuthFail
};

// 아래 에러 정보의 list item.
class CPNErrorCode
{
public:
	CPNErrorCode(uint64_t code = 0, std::string comment = "") : m_code(code), m_comment(comment) {};
	~CPNErrorCode() {};

	// 에러 코드
	uint64_t m_code;
	// 코멘트
	std::string m_comment;
};

// Code Virtualizer의 특성상 throw를 못한다. 따라서 실패 상황에서 실패 지점을 여러 군데를 담을 수 있어야 한다.
// 이를 위해 에러 정보를 여러개를 한꺼번에 담는 에러 정보이다.
class CPNErrorInfo
{
public:
	CPNErrorInfo() {};
	~CPNErrorInfo() {};

	void AppendErrorCode(uint64_t code, std::string comment) {
		std::shared_ptr<CPNErrorCode> ob(new CPNErrorCode(code, comment));
		m_errorStack.push_back(ob);
	};

	uint64_t GetErrorcode() {
		return m_errorStack.back()->m_code;
	}

	std::string ToString() {
		std::vector<std::shared_ptr<CPNErrorCode>>::iterator pos;
		std::string strErrorCode;
		std::ostringstream strstream;
		for (pos = m_errorStack.begin(); pos != m_errorStack.end(); ++pos) {
			strstream << " " << (*pos)->m_code;
		}

		strErrorCode.assign(strstream.str());
		return strErrorCode;
	};

	void Clear() {
		m_errorStack.clear();
	}

public:
	std::vector<std::shared_ptr<CPNErrorCode>> m_errorStack;
};


