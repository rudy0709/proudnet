/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "../include/Tracer.h"
#include "IntraTracer.h"

namespace Proud
{
	CTracer::CTracer(void)
	{
		m_critSec.m_neverCallDtor = true;
	}

	CTracer::~CTracer(void)
	{
	}

	// 로그를 출력한다.
	void CTracer::Trace(LogCategory logCategory, const char* pFormat, ...)
	{
		StringA s;

		va_list	pArg;
		va_start(pArg, pFormat);
		s.FormatV(pFormat, pArg);
		va_end(pArg);

		Trace_(logCategory, s.GetString());
	}

	// 로그를 출력한다.
	void CTracer::Trace_(LogCategory logCategory, const char* text)
	{
		_pn_unused(text);

		CriticalSectionLock lock(m_critSec, true);
		Context context;

		if (m_enabledCategories.TryGetValue(logCategory, context))
		{
			NTTNTRACE("%s\n", text);
		}
	}

	// 로그를 출력한다.
	void Trace(LogCategory logCategory, const char* format, ...)
	{
		StringA s;

		va_list	pArg;
		va_start(pArg, format);
		s.FormatV(format, pArg);
		va_end(pArg);

		CTracer::PtrType p = CTracer::GetSharedPtr();
		// singleton lost가 아니면 시행
		if(p)
			p->Trace(logCategory, s.GetString());
		
	}

	// 특정 category에 대한 로그 기능을 켠다.
	void CTracer::Enable(LogCategory logCategory)
	{
		CriticalSectionLock lock(m_critSec, true);
		Context context;
		m_enabledCategories.Add(logCategory, context);
	}

	CLogWriter::~CLogWriter()
	{
		// none
	}

	CLogWriter::CLogWriter()
	{
		// none
	}


}//end of namespace Proud


