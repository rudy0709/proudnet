#pragma once 

#include "stdafx.h"
#include "../include/BasicTypes.h"
#include "../include/Singleton.h"
#include "../include/StringEncoder.h"

namespace Proud
{
	// 플랫폼별 char,wchar_t의 기본 string converter.
	// 윈도,ios,marmalade 등은 char,wchar_t이 서로 다른 인코딩이다.
	// 이를 StringA2W,W2A에서 통합하고자 하는 용도임.
	class CDefaultStringEncoder : public CSingleton<CDefaultStringEncoder>
	{
	public:		
		CStringEncoder* m_A2WStringEncoder;
		CStringEncoder* m_W2AStringEncoder;

	#if (WCHAR_LENGTH == 4)
		// 유닉스에서는 wchar_t=utf32이고 잘 안 쓴다.
		// char 즉 utf8을 많이 쓴다.
		CStringEncoder* m_UTF8toUTF16LEEncoder;
		CStringEncoder* m_UTF16LEtoUTF8Encoder;
		// wchar_t가 utf32인 플랫폼에서는 CMessage를 위한 utf16 변환이 필요.
		// CMessage에서는 문자열이 utf16으로 다루어지기 때문.
		CStringEncoder* m_UTF32LEtoUTF16LEEncoder;
		CStringEncoder* m_UTF16LEtoUTF32LEEncoder;
	#endif	
		CDefaultStringEncoder();
		~CDefaultStringEncoder();
	};	
}