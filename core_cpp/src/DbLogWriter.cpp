/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/


#include "stdafx.h"
#if defined(_WIN32)
#include "../include/DbLogWriter.h"
#include "DbLogWriterImpl.h"

namespace Proud
{
	CDbLogWriter* CDbLogWriter::New( CDbLogParameter& logparameter, ILogWriterDelegate *pDelegate )
	{
		assert(pDelegate);
		CDbLogWriterImpl* ret = new CDbLogWriterImpl;
		ret->m_logParameter = logparameter;
		ret->m_delegate = pDelegate;
		ret->m_workerThread.Start();
		return (CDbLogWriter*)ret;
	}

}

#endif // _WIN32