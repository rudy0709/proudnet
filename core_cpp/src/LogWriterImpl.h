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

#include "../include/Tracer.h"
#include "../include/FakeClrBase.h"
#include "../include/PNSemaphore.h"
#include "FastList2.h"

//#pragma pack(push,8)

namespace Proud
{
	// 기존의 LogWriter가 CAtlFile을 사용했었는데
	// CAtlFile은 소멸자에서 자동 close를 해주기 때문에
	// close를 해줘야 할 부분을 찾는 수고를 덜기 위해 래핑하여 사용.
	class CFileWrapper
	{
		FILE* m_filePointer;

	public:

		CFileWrapper();
		~CFileWrapper();
		void Close();
		bool Open(const String& path, const String& option = _PNT("a+c, ccs=UTF-8"));
		int Puts(const String& str);
		int Putc(const PNTCHAR c);
		void Flush();
	};
	// 소멸자에서 Close를 하기 때문에 레퍼런스 카운팅이 필요함 
	typedef RefCount<CFileWrapper> CFileWrapperPtr;

	class CLogWriterImpl:public CLogWriter
	{
	private:
		friend class CLogWriter;

		enum eLogType
		{
			// 새 파일에 기록
			LogType_NewFile,
			// logcat format으로 기록
			LogType_Logcat,
			// 형식 없이 쌩 text로 기록
			LogType_Raw,
		};

		CFileWrapperPtr m_file;

		// producer-consumer용
		Semaphore m_writeSemaphore;

		// mutex
		CriticalSection m_CS;

		// 파일교체중 에러가 있는지를 체크.
		// Exception을 워커쓰레드가 아닌 유저쓰레드에서 발생시키도록 하기 위함.
		volatile bool m_changeFileFailed;

		//파일 교체를 위한 최대 라인수.
		// 추후 바이트 제한으로 변경 요망.
		unsigned int m_newFileForLineLimit;

		//현재 라인수.
		unsigned int m_currentLineCount;

		// 사용자가 입력한 확장자 포함된 파일경로.
		// 실제 로그파일명은 m_baseFileName에 _날짜시간값이 더해진다.
		String m_baseFileName;

		// 로그를 지속적으로 기록하는 스레드
		Thread m_workerThread;

		// 쓰레드 종료 플래그
		volatile bool m_stopThread;

		// 이 플래그가 true이면 종료 시 출력하지 못한 로그가 남아있어도 무시한다.
		volatile bool m_ignorePendedWriteOnExit;

		// 이 플래그가 true이면 로그파일 생성 시 현재 날짜시간정보를 파일명에 붙여준다.
		bool m_putFileTimeString;

		static void StaticThreadProc(void* ctx);
		void ThreadProc();

		struct LogData
		{
			eLogType	m_type;
			String		m_addedDateAndTime;	// for example -> 05(month) - 18(day) 11(hour):09(min):02(sec)
			int			m_logLevel;
			LogCategory m_logCategory;
			HostID		m_hostID;
			String		m_message; // LogType_NewFile인 경우 file name이 들어감
			String		m_function;
			int			m_line;
		};

		typedef RefCount<LogData> LogDataPtr;
		typedef CFastList2<LogDataPtr, intptr_t> LogList;
		LogList m_logList;

		void WriteLine_Internal(LogDataPtr& newLog);
		void ProcessLogData(LogDataPtr& logData);

	public:
		CLogWriterImpl(const String& logFileName, unsigned int newFileForLineLimit, bool putFileTimeString);
		~CLogWriterImpl();

		void StartThread();

		virtual void SetFileName(const String& logFileName) PN_OVERRIDE;

		virtual void WriteLine(
			int				logLevel,
			LogCategory		logCategory,
			HostID			logHostID,
			const String&	logMessage,
			const String&	logFunction = _PNT(""),
			int				logLine = 0
			) PN_OVERRIDE;


		virtual void WriteLine(const String& logMessage) PN_OVERRIDE;

		virtual void SetIgnorePendedWriteOnExit(bool flag) PN_OVERRIDE;
	};
}

//#pragma pack(pop)
