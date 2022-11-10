#include "StdAfx.h"
#include "WaterMarkStrMgr.h"
#include <xstring>

CWaterMarkString::CWaterMarkString(void)
{
}

CWaterMarkString::~CWaterMarkString(void)
{
}

CString CWaterMarkString::GetWaterMarkHeader() 
{
	assert(m_originNettentionInfo.GetLength()!=0);	
	return STRING_CONVERT_A_TO_T(m_encrypedNettentionInfomation); 
}

CString CWaterMarkString::GetWaterMarkString()
{
	assert(m_encryptedCustomerInfomation.GetLength()!=0);	
	CStringA Header = "/* " + m_encrypedNettentionInfomation+m_encryptedCustomerInfomation + " */";
	return STRING_CONVERT_A_TO_T(Header); 
}

void CWaterMarkString::init()
{
	m_originNettentionInfo = "넷텐션 ProudNet";
	CStringA nettenInto = m_originNettentionInfo;

	// 키 생성
	/*******************************************************************************************/	
	// 	BYTE *encryptKey = new BYTE[MaxKeylen];
	// 	for(int i =0; i<MaxKeylen; ++i)
	// 	{
	// 		encryptKey[i] = CSymmetricEncrypt::RandBYTE();
	// 	}
	// 	
	// 	
	// 	CString str;
	// 	CString strEncryptKey;
	// 	for(int i=0;i<MaxKeylen;++i)
	// 	{
	// 		str.Format(_T("%d"),encryptKey[i]);
	// 		strEncryptKey += str;
	// 	}
	// 	
	// 	CFileRW::emptyFileWrite("enckey.txt", CStrTostr(strEncryptKey.GetBuffer()));
	/*******************************************************************************************/

	// 여기 까지가 암호를 Txt로 저장하는 부분
	
	m_Encrypt.Init(MaxKeylen, (BYTE*)g_EncryptKey);
	m_encrypedNettentionInfomation = ConvertStringToWaterMark(nettenInto);
}


void CWaterMarkString::SetCustomerInfo( CStringA customInfo)
{
	m_originCustomerInfo = customInfo;
	m_encryptedCustomerInfomation = ConvertStringToWaterMark(customInfo);
}


CString CWaterMarkString::ConvertWatermarkToString( CString waterMark )
{
	//먼저 주석 문자열을 제거한다.
	{
		int startPos=0;
		int endPos=0;
		
		// 실질 위치는 나오는 pos값 -1
		startPos = waterMark.Find(_T("/*"));
		if(startPos==-1)
			startPos =0;
		else
			startPos +=2;
		
		endPos = waterMark.Find(_T("*/"));	
		if(endPos==-1 || endPos == 0)
			endPos = waterMark.GetLength();
		else
			endPos -=2;
			
		waterMark = waterMark.Mid(startPos, endPos);
		waterMark.Trim();
	}
	
	// 스트링이 워터마크를 포함하는지 체크
	CStringA convertedWaterMark = STRING_CONVERT_T_TO_A(waterMark);
	int pos = convertedWaterMark.Find(m_encrypedNettentionInfomation);
	CStringA companyWatermark;
	if(pos != -1)
	{
		pos += m_encrypedNettentionInfomation.GetLength();
		companyWatermark = convertedWaterMark.Mid(pos,convertedWaterMark.GetLength()-pos );
	}
	else
		companyWatermark = convertedWaterMark;
	
	// 숫자로 이루어진 문자열의 수들을 Byte 배열에 담는다
	CStringA decryptedString = companyWatermark;
	int Size;
	BYTE* Buffer = ConvertNumberStringToBytes(decryptedString, Size);
	

	// 암호화를 풀어준다.
	m_Encrypt.DecryptBuffer(Buffer, Size);
	decryptedString = (LPCSTR)Buffer;
	
	delete[] Buffer;
	return STRING_CONVERT_A_TO_T(decryptedString);
}


//privete
//------------------------------------------------------------------------



CStringA CWaterMarkString::ConvertStringToWaterMark( CStringA informationString)
{
	CStringA returnStr;
	
	int size = informationString.GetLength();
	BYTE *Buffer = new BYTE[size];
	memcpy(Buffer, informationString.GetString(), size);
	
	m_Encrypt.EncryptBuffer(Buffer,size);
	returnStr = ConvertBytesToString(Buffer, size);			//숫자로 변환해 주어야 마크를 찍을 때 깨진 문자열이 나타나지 않는다.
	
	delete[] Buffer;
	return returnStr;
}

CStringA CWaterMarkString::ConvertBytesToString(BYTE* buffer, int size)
{
	CStringA strConvert;
	CStringA strNum;
	int Num;
	for(int i=0; size > i;++i)	// 숫자들을 구분할 수 있게 한칸씩 띈다
	{
		Num = (int)buffer[i];
		strNum.Format("%d", Num);
		strConvert += strNum;
		if(size != i)	//마지막에는 들어가지 않게 함
			strConvert += " ";
	}
	return strConvert;
}

BYTE* CWaterMarkString::ConvertNumberStringToBytes(CStringA inputString, int &outSize)
{
	int size = inputString.GetLength();
	if(size == 0)
	{
		LOGWRITER(L"ConvertNumberStringToBytes에 들어온 스트링의 사이즈가 0입니다.");
		return NULL;
	}
	BYTE *buffer = new BYTE[size];
	memset(buffer, 0, size);
	outSize=0;
	int Num;
	int pos=0;
	
	CString token;
	while(   ( token=inputString.Tokenize(" ", pos ) ) != ""   )
	{
		token.Trim();
		Num = atoi(   STRING_CONVERT_T_TO_A(token)   );
		buffer[outSize] = Num;
		++outSize;
		inputString.Trim();
	}
	return buffer;
}

bool CWaterMarkString::functionTest()
{
	return true;
	
}

CStringA CWaterMarkString::ConvertANSIToUTF8( CStringA ansi)
{
	CStringW multi = /*(LPCWSTR)*/CA2W(ansi);
	return (LPCSTR)CW2A(multi,CP_UTF8);
}