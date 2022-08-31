/*
ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.

ProudNet

This program is soley copyrighted by Nettention.
Any use, correction, and distribution of this program are subject to the terms and conditions of the License Agreement.
Any violated use of this program is prohibited and will be cause of immediate termination according to the License Agreement.

** WARNING : PLEASE DO NOT REMOVE THE LEGAL NOTICE ABOVE.

*/

#pragma once

#include "../include/PNSemaphore.h"
#include "../include/DbLogWriter.h"
#include "FastList2.h"

//#pragma pack(push,8)

namespace Proud
{
#if (_MSC_VER>=1400)
// 아래 주석처리된 pragma managed 전처리 구문은 C++/CLI 버전이 있었을 때에나 필요했던 것입니다.
// 현재는 필요없는 구문이고, 일부 환경에서 C3295 "#pragma managed는 전역 또는 네임스페이스 범위에서만 사용할 수 있습니다."라는 빌드에러를 일으킵니다.
//#pragma managed(push,off)
#endif

	/**
	\~korean
	DbLogWriter.h 파일을 Include해야 합니다.
	DBMS에 로그를 기록합니다. ( <a target="_blank" href="http://guide.nettention.com/cpp_ko#logwriter_db" >데이터베이스에 로그를 기록하기</a> 참고)

	일반적 용도
	- CDbLogWriter를 사용하기 전에 ProudNet/Sample/DbmsSchema/LogTable.sql 을 실행하여 DbLog 테이블을 생성해야 합니다.
	- CDbLogWriter.New를 써서 이 객체를 생성합니다.
	- WriteLine, WriteLine를 써서 로그를 기록합니다. 저장된 로그는 비동기로 저장됩니다.
	- 기본적으로 LoggerName, LogText, DateTime 이 기록됩니다. 유저가 원하는 컬럼을 넣으려면 WriteLine의 CPropNode를 사용하면 됩니다.
	+
	\~english
	Write log to DBMS (Please refer to <a target="_blank" href="http://guide.nettention.com/cpp_en#logwriter_db" >Recording a log in database</a>)

	General usage
	- Before using CDbLogWriter, you must create Dblog table by running ProudNet/Sample/DbmsSchema/LogTable.sql
	- Create this object by using CDbLogWriter.New
	- Record log by using WriteLine, WriteLine. (Note: maybe a typo) The recorded log will be regarded as asynchronous.
	- Basically LoggerName, LogText and DateTime will be recorded. In order to add a column that user wants, use CPropNode of WriteLine
	\~
	*/
	class CDbLogWriterImpl : public CDbLogWriter
	{
		// mutex
		CriticalSection m_CS;

		// producer-consumer
		Semaphore m_writeSemaphore;

	public:
		CDbLogParameter m_logParameter;
		ILogWriterDelegate *m_delegate;

		// 로그를 지속적으로 기록하는 스레드
		Thread m_workerThread;

	private:
		volatile bool m_stopThread;

		static void StaticThreadProc(void* ctx);
		void ThreadProc();
		void CreateProcedure(CAdoConnection& conn);

		struct LogData
		{
			String m_LogText;
			CProperty m_properties;
		};

		typedef CFastList2<LogData*, intptr_t> LogList;
		LogList m_LogList;

	public:
		CDbLogWriterImpl();
		virtual ~CDbLogWriterImpl();

		void WriteLine( String logText, CProperty* const pProperties=NULL);
	};

	/**  @} */

#if (_MSC_VER>=1400)
//#pragma managed(pop)
#endif
}

//#pragma pack(pop)
