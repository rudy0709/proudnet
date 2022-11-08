#include "stdafx.h"
#include "PNLicenseInfo.h"


Proud::String CPNLicenseInfo::ToString()
{
	Proud::String output;
	output += _PNT("Company: ") + m_companyName + _PNT("\n");
	output += _PNT("Project: ") + m_projectName + _PNT("\n");
	output += _PNT("Type: ") + m_licenseType + _PNT("\n");
	output += _PNT("Issued(UTC+0): ") + String::NewFormat(_PNT("%04d-%02d-%02d"), (m_features.IssueDate.year >= 0) ? m_features.IssueDate.year + 1900 : 0, (m_features.IssueDate.mon >= 0) ? m_features.IssueDate.mon + 1 : 0, m_features.IssueDate.mday) + _PNT("\n");
	output += _PNT("Expires(UTC+0): ") + String::NewFormat(_PNT("%04d-%02d-%02d"), (m_features.ExpDate.year >= 0) ? m_features.ExpDate.year + 1900 : 0, (m_features.ExpDate.mon >= 0) ? m_features.ExpDate.mon + 1 : 0, m_features.ExpDate.mday) + _PNT("\n");
	
	return output;
}

void CPNLicenseInfo::ToBinary(ByteArray &outBinary)
{
	int m_version = g_pnlicenseSDK_version; // 추후 하위 호환성을 위해.

	CPNLicenseFeaturesBuffer lfBuf;
	lfBuf << m_features;

	CMessage msg;
	msg.UseInternalBuffer();
	msg << m_version << StringT2A(m_companyName) << StringT2A(m_projectName) << StringT2A(m_licenseType) << lfBuf.ToByteArray();
	msg.CopyTo(outBinary);
}

bool CPNLicenseInfo::Equals(const CPNLicenseInfo& rhs) const
{
	if (m_version != rhs.m_version)
		return false;
	if (m_companyName != rhs.m_companyName)
		return false;
	if (m_projectName != rhs.m_projectName)
		return false;
	if (m_licenseType != rhs.m_licenseType)
		return false;
	if (std::memcmp((void*)&(m_features.IssueDate), (void*)&(rhs.m_features.IssueDate), sizeof(SYSTEMTIME_t)) != 0)
		return false;
	if (std::memcmp((void*)&(m_features.ExpDate), (void*)&(rhs.m_features.ExpDate), sizeof(SYSTEMTIME_t)) != 0)
		return false;

	return true;
}

void CPNLicenseInfo::AddNewLineOnNeed(Proud::String& str)
{
	if (str.GetLength() > 0 && str[str.GetLength() - 1] != _PNT('\n'))
		str += _PNT("\n");
}

void CPNLicenseInfo::BinaryToText(ByteArray &binary, Proud::String &outText)
{
	StringA textA;
	Base64::Encode(binary, textA);

#ifdef _WIN32
	CHeldPtr<CStringEncoder> Utf8ToUtf16 = CStringEncoder::Create("UTF-8", "UTF-16LE");
	outText = StringA2W(textA, Utf8ToUtf16);
#else
	outText = textA;
#endif
}

void CPNLicenseInfo::TextToBinary(Proud::String text, ByteArray &outBinary)
{
	Proud::StringA textA;
#ifdef _WIN32
	// UTF8로 변환. unix는 애당초 utf8이므로 이런게 불필요
	CHeldPtr<CStringEncoder> Utf16ToUtf8 = CStringEncoder::Create("UTF-16LE", "UTF-8");
	textA = StringW2A(text, Utf16ToUtf8);
#else
	textA = text;
#endif
	Base64::Decode(textA, outBinary);
}

void CPNLicenseInfo::CreateDocumentHash(ByteArray& document, ByteArray &outHash)
{
	const int HashLength = 80;

	outHash.SetCount(HashLength);
	std::memset(outHash.GetData(), 0, outHash.GetCount());

	// 첫byte부터 끝까지 80byte 단위로 해시한다. 
	int length = document.GetCount();
	for (int i = 0; i < length; i++)
	{
		int partLength = min(length - i, HashLength);
		for (int j = 0; j < partLength; j++)
		{
			outHash[j] ^= document[i + j];
		}
	}
}

void CPNLicenseInfo::CreateLicenseKey(String &signedDocument, CPNErrorInfo& errorInfo)
{
	// license 정보를 binary로 serialize 후 서명 처리.
	// binary로 serialize할 때 text encoding이 고려되어 저장되므로 safe.

	ByteArray licenseBinary;
	ToBinary(licenseBinary);

	ByteArray licenseSignature;
	CreateSignature(licenseBinary, licenseSignature, errorInfo);

	// license 정보를 text로 뽑는다. 
	signedDocument += ToString();

	// text와 위 서명을 조합한다.
	AddNewLineOnNeed(signedDocument);
	Proud::String licenseSignatureText;
	BinaryToText(licenseSignature, licenseSignatureText);
	signedDocument += _PNT("Key: ") + licenseSignatureText;
}

void CPNLicenseInfo::SetPublicKey(ByteArray &binary)
{
	// base64 encode를 한다. public key는 인증앱에 하드코딩되어 들어가고 private key는 키생성 앱에 하드코딩되어 들어간다.
	Base64::Encode(binary, m_strBase64GenerationKey);
}

void CPNLicenseInfo::SetPrivateKey(ByteArray &binary)
{
	// base64 encode를 한다. public key는 인증앱에 하드코딩되어 들어가고 private key는 키생성 앱에 하드코딩되어 들어간다.
	Base64::Encode(binary, m_strBase64VerifyKey);
}

void CPNLicenseInfo::SetSignature(String &signature)
{
	m_strSignature = signature;
}

void CPNLicenseInfo::SetSignature(ByteArray &binary)
{
	BinaryToText(binary, m_strSignature);
}

void CPNLicenseInfo::GetSignatureBinary(ByteArray& outBinary)
{
	TextToBinary(m_strSignature, outBinary);
}

String CPNLicenseInfo::GetSignatureText()
{
	return m_strSignature;
}

StringA CPNLicenseInfo::GetVerifyKeyText()
{
	return m_strBase64VerifyKey;
}

StringA CPNLicenseInfo::GetGenerationKeyText()
{
	return m_strBase64GenerationKey;
}

bool CPNLicenseInfo::CreateSignature(ByteArray& document, ByteArray& signature, CPNErrorInfo& errorInfo)
{
	// text document의 hash를 생성한다.
	// TODO: 이 hash는 매우 단순한 해시다. 이것이 아니라 SHA같은 고난도 해시로 하는게 바람직하다.
	// ProudNet/src/libtom에 있는 것을 사용하는 것이 좋다.
	ByteArray documentHash;
	CreateDocumentHash(document, documentHash);

	// hash를 공개키로 암호화한다.
	ByteArray publicKeyBlob;

	if (m_strBase64GenerationKey.GetLength() < 1) {
		errorInfo.AppendErrorCode(NotCreatedSignature, "");
		return false;
	}

	Base64::Decode(m_strBase64GenerationKey, publicKeyBlob);
	if (m_rsa.EncryptSessionKeyByPublicKey(signature, documentHash, publicKeyBlob) == false)
	{
		errorInfo.AppendErrorCode(NotCreatedSignature, "");
		return false;
	}

	return true;
}

bool CPNLicenseInfo::ParseLine(const Proud::String &line, const Proud::String& name, Proud::String &outValue)
{
	int foundIndex = line.Find(name);
	if (foundIndex < 0)
		return false;
	String right = line.Right(line.GetLength() - name.GetLength());
	right.TrimLeft(_PNT(": "));
	outValue = right;
	return true;
}

bool CPNLicenseInfo::ParseLicenseKey(String &licenseKeyText, CPNLicenseInfo &outLicenseInfo, String &outSignatureText)
{
	Proud::String extractedDocument;
	Proud::String signatureText;

	// 본문의 맨 마지막 줄 즉 signature을 얻는다.
	int offset = 0;
	Proud::String line, prevLine;
	CPNLicenseInfo licenseInfo;

	// license 정보와 signature를 추출한다.
	while (1)
	{
		line = licenseKeyText.Tokenize(_PNT("\n"), offset);
		if (offset < 0)
			break;

		Proud::String value;
		if (ParseLine(line, _PNT("Key"), value))
			signatureText = value;

		if (ParseLine(line, _PNT("Project"), value))
			licenseInfo.m_projectName = value;

		if (ParseLine(line, _PNT("Company"), value))
			licenseInfo.m_companyName = value;

		if (ParseLine(line, _PNT("Expires(UTC+0)"), value)) {
			licenseInfo.m_features.ExpDate.year = pnttoi(value.Mid(0, 4)) - 1900;
			licenseInfo.m_features.ExpDate.mon  = pnttoi(value.Mid(5, 2)) - 1;
			licenseInfo.m_features.ExpDate.mday = pnttoi(value.Mid(8, 2));
			licenseInfo.m_features.ExpDate.hour = 23;
			licenseInfo.m_features.ExpDate.min  = 59;
			licenseInfo.m_features.ExpDate.sec = 59;
			licenseInfo.m_features.ExpDate.isdst = 0;
		}

		if (ParseLine(line, _PNT("Issued(UTC+0)"), value))	{
			licenseInfo.m_features.IssueDate.year = pnttoi(value.Mid(0, 4)) - 1900;
			licenseInfo.m_features.IssueDate.mon  = pnttoi(value.Mid(5, 2)) - 1;
			licenseInfo.m_features.IssueDate.mday = pnttoi(value.Mid(8, 2));
			licenseInfo.m_features.IssueDate.hour = 0;
			licenseInfo.m_features.IssueDate.min  = 0;
			licenseInfo.m_features.IssueDate.sec  = 0;
			licenseInfo.m_features.IssueDate.isdst = 0;
		}

		if (ParseLine(line, _PNT("Type"), value))
			licenseInfo.m_licenseType = value;
	}

	licenseInfo.m_version = g_pnlicenseSDK_version;

	if (signatureText.GetLength() == 0)
		return false;

	outLicenseInfo = licenseInfo;
	outSignatureText = signatureText;

	return true;
}

bool CPNLicenseInfo::VerifyLicenseKey(String licenseKeyText, CPNErrorInfo& errorInfo)
{
	// 본문의 맨 마지막 줄 즉 signature을 얻는다.
	CPNLicenseInfo outLicenseInfo;
	Proud::String extractedDocument;
	Proud::String outSignatureText;

	CCryptoRsaKey privateKey;
	ByteArray publicKeyBlob, privateKeyBlob;
	
	if (licenseKeyText.GetLength() < 1)
	{
		errorInfo.AppendErrorCode(NotVerifiedKey, "");
		return false;
	}

	if (ParseLicenseKey(licenseKeyText, outLicenseInfo, outSignatureText) == false)	{
		errorInfo.AppendErrorCode(NotVerifiedKey, "");
		return false;
	}

	// signature를 binary로 바꾼다.
	ByteArray signatureBinary;
	TextToBinary(outSignatureText, signatureBinary);
	SetSignature(signatureBinary);

	if (m_strBase64VerifyKey.GetLength() < 1) {
		errorInfo.AppendErrorCode(NotVerifiedKey, "");
		return false;
	}

	// binary로 바꾼 것을 개인키로 복호화한다.
	Base64::Decode(m_strBase64VerifyKey, privateKeyBlob);
	if (privateKey.FromBlob(privateKeyBlob) == false) {
		errorInfo.AppendErrorCode(NotVerifiedKey, "");
		return false;
	}

	ByteArray documentHash2;
	ErrorInfoPtr err = m_rsa.DecryptSessionKeyByPrivateKey(documentHash2, signatureBinary, privateKey);
	if (err) {
		errorInfo.AppendErrorCode(NotVerifiedKey, "");
		return false;
	}

	// 추출한 라이선스 정보를 binary로 바꾼다.
	ByteArray licenseBinary;
	outLicenseInfo.ToBinary(licenseBinary);

#if 0
	// sgno : licenseBinary.txt 파일 생성
	CPNFileSystem fs3;
	CPNFileInfo fileInfo3;
	fs3.CreateAllUserWritableRegistryFileAndOpen(_PNT("../verify_licenseBinary.txt"), CPNFileSystem::READWRITE, errorInfo);
	fs3.WriteFileData(licenseKeyText, errorInfo);
	fs3.WriteFileData(licenseBinary, licenseBinary.GetCount(), errorInfo);
	fs3.WriteFileData(outSignatureText, errorInfo);
	fs3.WriteFileData(signatureBinary, signatureBinary.GetCount(), errorInfo);
	fs3.CloseFile(errorInfo);
#endif

	// 라이선스 정보 binary를 hash로 뽑는다.
	ByteArray documentHash;
	CreateDocumentHash(licenseBinary, documentHash);

	// 복호화한 것과 hash가 동일한지?
	if (documentHash.Equals(documentHash2) == false) {
		errorInfo.AppendErrorCode(NotVerifiedKey, "");
		return false;
	}
		
	m_version	  = outLicenseInfo.m_version;
	m_companyName = outLicenseInfo.m_companyName;
	m_projectName = outLicenseInfo.m_projectName;
	m_licenseType = outLicenseInfo.m_licenseType;
	m_features    = outLicenseInfo.m_features;
	return true;
}

bool CPNLicenseInfo::CheckValidity(CPNErrorInfo& errorInfo)
{
	CCryptoRsaKey privateKey;
	ByteArray publicKeyBlob, privateKeyBlob;

	if (m_strBase64VerifyKey.GetLength() < 1) {
		errorInfo.AppendErrorCode(NotPassedValidity, "");
		return false;
	}

	if (m_strSignature.GetLength() < 1) {
		errorInfo.AppendErrorCode(NotPassedValidity, "");
		return false;
	}

	// signature를 binary로 바꾼다.
	ByteArray signatureBinary;
	TextToBinary(m_strSignature, signatureBinary);
		
	// binary로 바꾼 것을 개인키로 복호화한다.
	Base64::Decode(m_strBase64VerifyKey, privateKeyBlob);
	if (privateKey.FromBlob(privateKeyBlob) == false) {
		errorInfo.AppendErrorCode(NotPassedValidity, "");
		return false;
	}

	ByteArray documentHash2;
	ErrorInfoPtr err = m_rsa.DecryptSessionKeyByPrivateKey(documentHash2, signatureBinary, privateKey);
	if (err) {
		errorInfo.AppendErrorCode(NotPassedValidity, "");
		return false;
	}

	// 추출한 라이선스 정보를 binary로 바꾼다.
	ByteArray licenseBinary;
	ToBinary(licenseBinary);

	// 라이선스 정보 binary를 hash로 뽑는다.
	ByteArray documentHash;
	CreateDocumentHash(licenseBinary, documentHash);

	// 복호화한 것과 hash가 동일한지?
	if (documentHash.Equals(documentHash2) == false) {
		errorInfo.AppendErrorCode(NotPassedValidity, "");
		return false;
	}

	return true;
}