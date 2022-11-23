#pragma once
#include "SymmetricEncrypt.h"

using namespace std;

class CWaterMarkString
{
//private:
public:
	// 중요! 우리의 정보 문자열 역시 키값과 같이 절대 바뀌어선 안됨 
	// 때문에 하드코딩 되어있음
	CStringA m_originNettentionInfo;
	CStringA m_originCustomerInfo;

	// 중요! 우리의 정보 문자열 역시 키값과 같이 절대 바뀌어선 안됨 
	// !!!! 이 문자열로 워터마크를 검색해 낸다. !!!!
	CStringA m_encrypedNettentionInfomation;
	CStringA m_encryptedCustomerInfomation;
	CSymmetricEncrypt m_Encrypt;
	
public:
	CWaterMarkString(void);
	~CWaterMarkString(void);
	
	void init();
	
	// 이정보를 중심으로 문자열 검색을 함
	// 모든 업체에 같은 스트링이 들어감
	CString GetWaterMarkHeader();		
	
	// 고객 정보에 대한 문자열을 세팅
	void SetCustomerInfo( CStringA customInfo );
	
	// 주석 워터마크에 기입될 헤더 스트링
	CString GetWaterMarkString();		//"/* " + m_NettentionInfoStr+m_CustomerInfoStr + " */"
	
	// 워터마크에 기입된 암호화된 문자열을 풀어줌 주의 우리 측 정보는 빼고 리턴됨
	CString ConvertWatermarkToString(CString);
	
	CStringA ConvertANSIToUTF8(CStringA);


	bool functionTest();

	
private:
	// 스트링의 모든 글자들을 숫자로 변환 후 문자열로 만들어줌
	CStringA ConvertBytesToString(BYTE*, int);
	
	// 숫자로 이루어진 문자열의 수들을 Byte 배열에 담는다
	BYTE* ConvertNumberStringToBytes(CStringA numberString, int &OutSize);
	
	// 문자열을 워터마크용 문자열로 변화해줌
	CStringA ConvertStringToWaterMark(CStringA);
	
};
