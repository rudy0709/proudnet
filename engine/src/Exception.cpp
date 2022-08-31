﻿/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "../include/sysutil.h"
#include "../include/Exception.h"
#include "../include/ErrorInfo.h"
#include "ExceptionImpl.h"

using namespace std;

namespace Proud
{
	Exception::Exception(const char* text)
	{
		StringA s;
		chMsg = text;

		m_remote = HostID_None;
		m_exceptionType = ExceptionType_String;
		m_userCallbackName = _PNT("");
		m_delegateObject = NULL;
		NTTNTRACE("Exception occurred: %s\n", chMsg.GetString());
	}

	Exception::Exception(const wchar_t* text)
	{
		StringW s;

		chMsg = StringW2A(text);

		m_remote = HostID_None;
		m_exceptionType = ExceptionType_String;
		m_userCallbackName = _PNT("");
		m_delegateObject = NULL;

		NTTNTRACE("Exception occurred: %s\n", chMsg.GetString());
	}

	Exception::~Exception(void) throw()
	{

	}

	const char *Exception::what( ) const throw()
	{
		// return CT2A(chMsg); 이건 제대로 된 리턴값을 안주는 듯
		return chMsg.GetString();
	}

	Exception::Exception( void )
	{
		m_remote = HostID_None;
		m_exceptionType = ExceptionType_None;
		m_userCallbackName = _PNT("");
		m_delegateObject = NULL;
		m_pVoidSource = NULL;
		m_pStdSource = NULL;

	}

	Exception::Exception(std::exception& src)
	{
		chMsg = src.what();
		m_pStdSource = &src;
		m_remote = HostID_None;
		m_exceptionType = ExceptionType_Std;
		m_userCallbackName = _PNT("");
		m_delegateObject = NULL;

		//ATLTRACE("Exception 발생: %s\n", chMsg);
	}

	Exception::Exception(ErrorInfo* src)
	{
		chMsg = StringT2A(src->ToString());
		// ErrorInfoPtr을 외부에서 사용할 수 있기 때문에 ErrorInfo 객체 포인터가 아닌 새로운 복제본을 사용합니다.
		m_errorInfoSource = ErrorInfoPtr(src->Clone());
		if(m_errorInfoSource)
			m_remote = m_errorInfoSource->m_remote;
		m_exceptionType = ExceptionType_ErrorInfo;
		m_userCallbackName = _PNT("");
		m_delegateObject = NULL;
		NTTNTRACE("Exception 발생: %s\n", chMsg.GetString());
	}

	Exception::Exception(const Exception& src)
	{
		m_remote = src.m_remote;
		m_pVoidSource = src.m_pStdSource;
		m_pStdSource = src.m_pStdSource;
		m_exceptionType = src.m_exceptionType;
		m_userCallbackName = src.m_userCallbackName;
		m_delegateObject = src.m_delegateObject;
		chMsg = src.chMsg;

		// 깊은 복사 수행합니다. (Exception 소멸자에서delete를 수행)
		if (src.m_errorInfoSource)
			m_errorInfoSource = ErrorInfoPtr(src.m_errorInfoSource->Clone());
	}

#ifdef _WIN32
	// _com_error는 windows 전용이다. 따라서 Exception class 자체가 _com_error를 가지지 않고 본 함수로 대체.
	void Exception_UpdateFromComError(Exception& e, _com_error& src)
	{
		_bstr_t desc = src.Description();
		if (((const PNTCHAR*)desc) != NULL)
		{
			e.chMsg = StringT2A(desc);
		}
		else
		{
			e.chMsg = StringT2A(src.ErrorMessage());
		}
		e.m_remote = HostID_None;
		e.m_exceptionType = ExceptionType_ComError;
		e.m_userCallbackName = _PNT("");
		e.m_delegateObject = NULL;
		NTTNTRACE("Exception occurred: %s\n", e.chMsg.GetString());
	}
#endif // _WIN32


	void ThrowInvalidArgumentException()
	{
		throw Exception("An invalid argument is detected!");
	}

	void ThrowArrayOutOfBoundException()
	{
		throw Exception("Array index out of range!");
	}

	void ThrowInt32OutOfRangeException(const char* where)
	{
		stringstream part;
		part << "32-bit integer out of range! " << where;
		throw Exception(part.str().c_str());
	}

	void ThrowBadAllocException()
	{
		throw std::bad_alloc();
	}

	void ThrowException(const char* text)
	{
		throw Exception(text);
	}

	void XXXHeapChkImpl(int /*index*/)
	{
#if defined(CHECK_HEAP_CORRUPTION) && defined(_WIN32)
		if (index == 0)
		{
			if (_heapchk() != _HEAPOK)
				throw Exception("XXXCheckHeap() failure!");

			uint8_t *a = new uint8_t[10000]; // 이렇게 테스트해보면 _heapchk()에서도 못찾는 write to already freed memory 에러를 검증할 수 있다.
			delete []a;
		}

		{

			if (!HeapValidate(GetProcessHeap(),0,NULL) )
				throw Exception("XXXCheckHeap() failure!");

			uint8_t *a = new uint8_t[10000]; // 이렇게 테스트해보면 _heapchk()에서도 못찾는 write to already freed memory 에러를 검증할 수 있다.
			delete []a;
		}
#endif
	}
}
