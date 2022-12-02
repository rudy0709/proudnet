/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "../include/PNString.h"
#include "../include/CriticalSect.h"
#include "../include/Singleton.h"
#include "DefaultStringEncoder.h"

#if !defined(_WIN32)
#include <wchar.h>
#include <cwctype>    
#include "swprintf.h"
#else
#include <mbstring.h>
#endif

#include "ctype.h"
#include <stdlib.h> // wcstombs
#include <stdio.h>
#include "../include/sysutil.h"
#include "PnIconv.h"

extern "C"
{
	size_t	pnwcslen(const wchar_t *s)
	{
		const wchar_t *p;

		p = s;
		while (*p)
			p++;

		return p - s;
	}
};

namespace Proud
{

	int UnicodeStrTraits::SafeStringLen(const CharType* str)
	{
		if (!str)
			return 0;

		return (int)pnwcslen(str);
	}

	// ikpil.choi 2016-11-10 : memcpy_s 로 변경, destSize(2번째 인자) 값이 항상 올바른 값이여야 합니다.
	//void UnicodeStrTraits::CopyString(CharType* dest, const CharType* src, int length)
	//{
	//	memcpy(dest, src, sizeof(CharType)*(length + 1));
	//	dest[length] = 0; // \0 처리
	//}

	// dest : 복사 받을 대상, destNumOfElements : 복사 받을 대상의 요소 갯수('\0' 자리 포함)
	void UnicodeStrTraits::CopyString(CharType* dest, int destNumOfElements, const CharType* src, int length)
	{
		// 멀티 플랫폼 방어까지 포함
		if (0 > destNumOfElements)
			return;

		int destByteSize = sizeof(CharType) * destNumOfElements;
		int srcByteSize = sizeof(CharType) * (length + 1);
		memcpy_s(dest, destByteSize, src, srcByteSize);

		int zeroIndex = PNMIN(length, destNumOfElements - 1);
		dest[zeroIndex] = 0; // \0 처리
	}

	int UnicodeStrTraits::StringCompare(const CharType* a, const CharType*b)
	{
		return wcscmp(a, b);
	}

	int UnicodeStrTraits::StringCompareNoCase(const CharType* a, const CharType*b)
	{
#if defined(_WIN32)
		return _wcsicmp(a, b);
#else
		// 누가 이딴 코드를 ㅡㅡ
		// 없으면 platform not support debug assert 코드를 넣어놔야지 ㅡㅡ
		// 언렬에서는 UNICODE를 사용하므로 무조건 false가 나와버림 따라서 수정

		assert(0);

		//return wcscasecmp(a,b);
		return 0;
#endif
	}

	int UnicodeStrTraits::GetFormattedLength(const wchar_t* pszFormat, va_list args)
	{
#if defined(_WIN32)
		return _vscwprintf(pszFormat, args);
#else
#if defined(__ANDROID__)
		return 10000;	// 클라이언트는 더미 파일도 사용할 수 없음. 컨피그를 이용하여 조절하는게 어떨지?
#else
		FILE* dummy = fopen("/dev/null", "wb");
		va_list argsClone;	// 리눅스에서는 함수 내부에서 va_list 클론을 생성하여 사용하지 않기 때문에 이 작업을 해주어야함.
		va_copy(argsClone, args);
		int ret = vfwprintf(dummy, pszFormat, argsClone);	// 리눅스에서는 wchar_t에 대한 가변 문자열 길이를 구하는 함수가 없어 더미 파일을 이용함.
		va_end(argsClone);
		fclose(dummy);
		return ret;
#endif
#endif
	}

	int __cdecl UnicodeStrTraits::Format(wchar_t* pszBuffer, size_t nLength, const wchar_t* pszFormat, va_list args) throw()
	{
#if !defined(_WIN32)
#if defined(__ANDROID__)
		return pnvswprintf(pszBuffer, nLength, pszFormat, args);
#else
		return vswprintf(pszBuffer, nLength, pszFormat, args);
#endif
#else
#if (_MSC_VER>=1400)
		return vswprintf_s(pszBuffer, nLength, pszFormat, args);
#else
		return vswprintf(pszBuffer, pszFormat, args);
#endif
#endif
	}

#if defined(_WIN32)    // std string 을 쓸수있는 방법을 찾아야할듯.. 일단 막는다.

#if !defined(_WIN32)
	struct ToLower
	{
		char operator() (char c) const { return std::tolower(c); }
	};

	struct ToUpper
	{
		char operator() (char c) const { return std::toupper(c); }
	};

	struct ToWLower
	{
		wchar_t operator() (wchar_t c) const { return std::towlower(c); }
	};

	struct ToWUpper
	{
		wchar_t operator() (wchar_t c) const { return std::towupper(c); }
	};
#endif

	wchar_t* UnicodeStrTraits::StringUppercase(wchar_t* psz, size_t size) throw()
	{
#if !defined(_WIN32)
		std::wstring wstr(psz);
		std::transform(wstr.begin(), wstr.end(), wstr.begin(), ToWUpper());
		return (wchar_t*)wstr.c_str();
#else
#if (_MSC_VER>=1400)
		errno_t err = _wcsupr_s(psz, size);
		return (err == 0) ? psz : NULL;
#else
		return _wcsupr(psz);
#endif
#endif
	}

	wchar_t* UnicodeStrTraits::StringLowercase(wchar_t* psz, size_t size) throw()
	{
#if !defined(_WIN32)
		std::wstring wstr(psz);
		std::transform(wstr.begin(), wstr.end(), wstr.begin(), ToWLower());
		return (wchar_t*)wstr.c_str();
#else
#if (_MSC_VER>=1400)
		errno_t err = _wcslwr_s(psz, size);
		return (err == 0) ? psz : NULL;
#else
		return _wcslwr(psz);
#endif
#endif
	}

#endif // std string 을 쓸수있는 방법을 찾아야할듯.. 일단 막는다.

	const wchar_t* __cdecl UnicodeStrTraits::StringFindString(const wchar_t* pszBlock, const wchar_t* pszMatch) throw()
	{
		return std::wcsstr(pszBlock, pszMatch);
	}

	wchar_t* __cdecl UnicodeStrTraits::StringFindString(wchar_t* pszBlock, const wchar_t* pszMatch) throw()
	{
		return(const_cast<wchar_t*>(StringFindString(const_cast<const wchar_t*>(pszBlock), pszMatch)));
	}

	const wchar_t* __cdecl UnicodeStrTraits::StringFindChar(const wchar_t* pszBlock, wchar_t chMatch) throw()
	{
		return std::wcschr(pszBlock, chMatch);
	}

	int __cdecl UnicodeStrTraits::StringSpanIncluding(const wchar_t* pszBlock, const wchar_t* pszSet) throw()
	{
		return (int)wcsspn(pszBlock, pszSet);
	}

	int __cdecl UnicodeStrTraits::StringSpanExcluding(const wchar_t* pszBlock, const wchar_t* pszSet) throw()
	{
		return (int)wcscspn(pszBlock, pszSet);
	}

	wchar_t* __cdecl UnicodeStrTraits::CharNext(const wchar_t* psz) throw()
	{
		return const_cast<wchar_t*>(psz + 1);
	}

	int __cdecl UnicodeStrTraits::IsDigit(wchar_t ch) throw()
	{
		return iswdigit(static_cast<unsigned short>(ch));
	}

	int __cdecl UnicodeStrTraits::IsSpace(wchar_t ch) throw()
	{
		return iswspace(static_cast<unsigned short>(ch));
	}

	// char은 인코딩 상관없이 바이트수를 리턴한다. STL string도 마찬가지.
	int AnsiStrTraits::SafeStringLen(const CharType* str)
	{
		if (!str)
			return 0;

		return (int)strlen(str);
	}

	// ikpil.choi 2016-11-10 : memcpy_s 로 변경, destSize(2번째 인자) 값이 항상 올바른 값이여야 합니다.
	//void AnsiStrTraits::CopyString(CharType* dest, const CharType* src, int length)
	//{
	//	memcpy(dest, src, sizeof(CharType)*length);
	//	dest[length] = 0; // mark sz
	//}

	// dest : 복사 받을 대상, destNumOfElements : 복사 받을 대상의 요소 갯수('\0' 자리 포함)
	void AnsiStrTraits::CopyString(CharType* dest, int destNumOfElements, const CharType* src, int length)
	{
		// 멀티 플랫폼 방어까지 포함
		if (0 > destNumOfElements)
			return;

		int destByteSize = sizeof(CharType) * destNumOfElements;
		int srcByteSize = sizeof(CharType) * (length + 1);
		memcpy_s(dest, destByteSize, src, srcByteSize);

		int zeroIndex = PNMIN(length, destNumOfElements - 1);
		dest[zeroIndex] = 0; // \0 처리
	}


	int AnsiStrTraits::StringCompare(const CharType* a, const CharType*b)
	{
		return strcmp(a, b);
	}

	int AnsiStrTraits::StringCompareNoCase(const CharType* a, const CharType*b)
	{
#if !defined(_WIN32)
		return strcasecmp(a, b);
#else
		return _stricmp(a, b);
#endif
	}

	int AnsiStrTraits::GetFormattedLength(const char* pszFormat, va_list args)
	{
#if defined(_WIN32)
		return _vscprintf(pszFormat, args);
#else
		va_list argsClone;	// 리눅스에서는 함수 내부에서 va_list 클론을 생성하여 사용하지 않기 때문에 이 작업을 해주어야함.
		va_copy(argsClone, args);
		int ret = vsnprintf(NULL, 0, pszFormat, argsClone);
		va_end(argsClone);
		return ret;
#endif
	}

	int __cdecl AnsiStrTraits::Format(char* pszBuffer, size_t nlength, const char* pszFormat, va_list args) throw()
	{
#if (_MSC_VER>=1400) && defined(_WIN32)
		return vsprintf_s(pszBuffer, nlength, pszFormat, args);
#else
		return vsprintf(pszBuffer, pszFormat, args);
#endif
	}


	char* AnsiStrTraits::StringUppercase(char* psz, size_t size) throw()
	{
#if !defined(_WIN32)
		std::string str(psz);
		// 만약 여기서 빌드에러 나면, android,xcode에 한해서는 막으셔도 됩니다. 가령 #if defined(_WIN32) || defined(__linux__)라던지.
		std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c)->unsigned char { return std::toupper(c); });
		return (char*)str.c_str();
#else
#if (_MSC_VER>=1400)
		errno_t err = _strupr_s(psz, size);
		return (err == 0) ? psz : NULL;
#else
		return _strupr(psz);
#endif
#endif
	}

	char* AnsiStrTraits::StringLowercase(char* psz, size_t size) throw()
	{
#if !defined(_WIN32)
		std::string str(psz);
		std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c)->unsigned char { return std::tolower(c); });
		return (char*)str.c_str();
#else
#if (_MSC_VER>=1400)
		int err = _strlwr_s(psz, size);
		return (err == 0) ? psz : NULL;
#else
		return _strlwr(psz);
#endif
#endif
	}

	const char* __cdecl AnsiStrTraits::StringFindString(const char* pszBlock, const char* pszMatch) throw()
	{
		return strstr(pszBlock, pszMatch);
	}

	char* __cdecl AnsiStrTraits::StringFindString(char* pszBlock, const char* pszMatch) throw()
	{
		return(const_cast<char*>(StringFindString(const_cast<const char*>(pszBlock), pszMatch)));
	}

	const char* __cdecl AnsiStrTraits::StringFindChar(const char* pszBlock, char chMatch) throw()
	{
		return strchr(pszBlock, chMatch);
	}

	int __cdecl AnsiStrTraits::StringSpanIncluding(const char* pszBlock, const char* pszSet) throw()
	{
#if !defined(_WIN32)        
		return (int)strspn(pszBlock, pszSet);
#else
		return (int)_mbsspn(reinterpret_cast<const unsigned char*>(pszBlock), reinterpret_cast<const unsigned char*>(pszSet));
#endif
	}

	int __cdecl AnsiStrTraits::StringSpanExcluding(const char* pszBlock, const char* pszSet) throw()
	{
#if !defined(_WIN32)        
		return (int)strcspn(pszBlock, pszSet);
#else
		return (int)_mbscspn(reinterpret_cast<const unsigned char*>(pszBlock), reinterpret_cast<const unsigned char*>(pszSet));
#endif		
	}

	char* __cdecl AnsiStrTraits::CharNext(const char* p) throw()
	{
		unsigned char* p2 = (unsigned char*)p;

#if defined(__linux__) || defined(__MACH__) || defined(__FreeBSD__) || defined(__GUNC__)// UTF-8

		if (*p2 < 0x80)
			return (char *)(p2 + 1);

		if (((*p2 & 0xE0) == 0xC0) && ((*(p2 + 1) & 0xC0) == 0x80))
			return (char *)(p2 + 1);
		else if (((*p2 & 0xF0) == 0xE0) && ((*(p2 + 1) & 0xC0) == 0x80) && ((*(p2 + 2) & 0xC0) == 0x80))
			return (char *)(p2 + 2);
		else if (((*p2 & 0xF0) == 0xE0) && ((*(p2 + 1) & 0xC0) == 0x80) && ((*(p2 + 2) & 0xC0) == 0x80) && ((*(p2 + 3) & 0xC0) == 0x80))
			return (char *)(p2 + 3);
		return NULL;

#elif defined(_WIN32)

		if (*p2 < 0x80)
			return (char *)(p2 + 1);
		else
			return (char *)(p2 + 2);

#endif
	}

	int __cdecl AnsiStrTraits::IsDigit(char ch) throw()
	{
#if !defined(_WIN32)
		return isdigit(ch);
#else
		return _ismbcdigit(ch);
#endif
	}

	int __cdecl AnsiStrTraits::IsSpace(char ch) throw()
	{
#if !defined(_WIN32)
		return isspace(ch);
#else
		return _ismbcspace(ch);
#endif
	}

	StringA StringW2A(const wchar_t *src, CStringEncoder* encoder)
	{
		int srclen = (int)pnwcslen(src); // 글자 갯수가 아니라 문자열을 차지하는 byte수임

		if (srclen <= 0)
			return StringA();

		StringA str;

		int destCapacity = (srclen + 1) * 3; // MS949 한글: 2byte, UTF-8 한글 : 3byte
		StrBufA out(str, destCapacity);
		out[0] = 0;

		size_t inbytesleft = sizeof(wchar_t)*(srclen)+sizeof(wchar_t);
		size_t outbytesleft = destCapacity;

		CDefaultStringEncoder::PtrType t; // 여기 있어야 아래 가는 동안 안 없어짐
		if (encoder == NULL)
		{
			t = CDefaultStringEncoder::GetSharedPtr();
			if (!t)
				return "Single lost before StringW2A!";
			encoder = t->m_W2AStringEncoder;
		}

		iconv_string_convert(encoder,
			(const char*)src, &inbytesleft, (char*)out.GetBuf(), &outbytesleft);

		return str;
	}

	StringW StringA2W(const char *src, CStringEncoder* encoder)
	{
		int srclen = (int)strlen(src); // 글자 갯수가 아니라 문자열을 차지하는 byte수임

		if (srclen <= 0)
			return StringW();

		StringW str;
		int destCapacity = sizeof(wchar_t)*(srclen + 1); // 넉넉하게 잡음
		StrBufW out(str, destCapacity);
		out[0] = 0;

		size_t inbytesleft = srclen + 1;
		size_t outbytesleft = destCapacity;

		CDefaultStringEncoder::PtrType t;
		if (!encoder)
		{
			t = CDefaultStringEncoder::GetSharedPtr();
			if(!t)
				return L"Single lost before StringA2W!";
			encoder = t->m_A2WStringEncoder;
		}

		iconv_string_convert(encoder,
			src, &inbytesleft, (char*)out.GetBuf(), &outbytesleft);

		return str;
	}

	
}
