/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "../include/Exception.h"
#include "../include/Marshaler.h"
#include "../include/Message.h"
#include "../include/P2PGroup.h"
#include <wchar.h>
#include "../include/BasicTypes.h"

#if !defined(_WIN32)
	#include "swprintf.h"
#endif

#define _COUNTOF(x) (sizeof(x)/sizeof(x[0]))

namespace Proud
{

CMessage& operator<<(CMessage& a, const NamedAddrPort &b)
{
	a.Write(b);
	return a;
}

CMessage& operator>>(CMessage& a, NamedAddrPort &b)
{
	a.Read(b);
	return a;
}

template<typename T>
void AppendTextOutT(String &a, const PNTCHAR* fmt, const T& b)
{
	String p;
	p.Format(fmt, b);
		a += p;
}

// AppendTextOut 함수들은 RMI callback notification의 파라메터 처리에서 사용된다.
void AppendTextOut(String &a, const bool &b)
{
	a += b ? _PNT("true") : _PNT("false");
}

void AppendTextOut(String &a, const char &b)
{
	AppendTextOutT(a,_PNT("%d"),b);
}
void AppendTextOut(String &a, const unsigned char &b)
{
	AppendTextOutT(a,_PNT("%d"),b);
}

void AppendTextOut(String &a, const short &b)
{
	AppendTextOutT(a,_PNT("%d"),b);
}
void AppendTextOut(String &a, const unsigned short& b)
{
	AppendTextOutT(a,_PNT("%d"),b);
}
void AppendTextOut(String &a, const int &b)
{
	AppendTextOutT(a,_PNT("%d"),b);
}
void AppendTextOut(String &a, const unsigned int& b)
{
	AppendTextOutT(a,_PNT("%d"),b);
}

#if defined(_WIN32)
void AppendTextOut(String &a, const long &b)
{
	AppendTextOutT(a,_PNT("%ld"),b);
}
void AppendTextOut(String &a, const unsigned long & b)
{
	AppendTextOutT(a,_PNT("%ld"),b);
}
#endif

void AppendTextOut(String &a, const int64_t &b)
{
	AppendTextOutT(a,_PNT("%I64d"),b);
}

void AppendTextOut(String &a, const uint64_t &b)
{
	AppendTextOutT(a,_PNT("%I64d"),b);
}


void AppendTextOut(String &a, const float  &b)
{
	AppendTextOutT(a,_PNT("%f"),b);
}

void AppendTextOut(String &a, const double &b)
{
	AppendTextOutT(a,_PNT("%lf"),b);
}

void AppendTextOut(String &a, const char* &b)
{
	a += _PNT("'");
	a += StringA2T(b);
	a += _PNT("'");
}

void AppendTextOut(String &a, const wchar_t* &b)
{
	a += _PNT("'");
	a += StringW2T(b);
	a += _PNT("'");
}

void AppendTextOut(String &a, const StringA &b)
{
	a += _PNT("'");
	a += StringA2T(b);
	a += _PNT("'");
}

void AppendTextOut(String &a, const StringW &b)
{
	a += _PNT("'");
	a += StringW2T(b);
	a += _PNT("'");
}

#if defined(_WIN32) 
void AppendTextOut(String &a, const std::string & b)
{
	a += _PNT("'");
	a += StringA2T(b.c_str());
	a += _PNT("'");
}

void AppendTextOut(String &a, const std::wstring & b)
{
	a += _PNT("'");
	a += StringW2T(b.c_str());
	a += _PNT("'");
}

void AppendTextOut(String &a, const POINT &b)
{
	String p;
	p.Format(_PNT("(%d,%d)"), b.x, b.y);
	a += p;
}
#endif

void AppendTextOut(String &a, ByteArray &b)
{
	a += String::NewFormat(_PNT("<ByteArray length=%d>"), b.GetCount());
}

void AppendTextOut( String &a,HostIDArray &b )
{
	Proud::String x;
	x.Format(_PNT("<HostIDArray %d>"), b.GetCount());
	a += x;
}

void ThrowExceptionOnReadString(int length)
{
	throw Exception("%s: Length Assignment of %d Requested", __FUNCTION__ , length);
}

void ThrowExceptionOnReadArray(int64_t length)
{
	throw Exception(": Length Assignment of %d Requested", __FUNCTION__, length);
}


}