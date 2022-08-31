/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>

#include "CriticalSectImpl.h"
#include "LogWriterImpl.h"
#include "IntraTracer.h"
#include "../include/PnTime.h"
#include "../include/Message.h"
#include "../include/PNPath.h"
#if defined(_WIN32)
#include <tchar.h>
#else

#define _SH_DENYWR 0x20

// ikpil.choi 2017-02-20 : pntime 만 분리시키면, tchar.h 를 pntchar.h 로 대응시키고 _t 시리즈 중 파일관련을 이동해야 깔끔해 보입니다. 아래 함수는 미리 만들어 둡니다.

// ikpil.choi 2017-02-20 : 크로스 플랫폼을 위한 fputws 대응 함수
inline size_t _fputts(const wchar_t *_Src, FILE *_File)
{
	return fputws(_Src, _File);
}

// ikpil.choi 2017-02-20 : 크로스 플랫폼을 위한 fputs 대응 함수
inline size_t _fputts(const char *_Src, FILE *_File)
{
	return fputs(_Src, _File);
}

// ikpil.choi 2017-02-20 : 크로스 플랫폼을 위한 fputwc 대응 함수
inline size_t _fputtc(wchar_t _Ch, FILE *_File)
{
	return fputwc(_Ch, _File);
}

// ikpil.choi 2017-02-20 : 크로스 플랫폼을 위한 fputc 대응 함수
inline size_t _fputtc(char _Ch, FILE *_File)
{
	return fputc(_Ch, _File);
}

// ikpil.choi 2017-02-20 : 크로스 플랫폼을 위한 fopen 대응 함수
inline FILE* _tfsopen(const wchar_t* _Filename, const wchar_t * _Mode, int /*_ShFlag*/)
{
	Proud::String strFileName = Proud::StringW2A(_Filename);
	Proud::String strMode = Proud::StringW2A(_Mode);

	return fopen(strFileName.GetString(), strMode.GetString());
}

// ikpil.choi 2017-02-20 : 크로스 플랫폼을 위한 fopen 대응 함수
inline FILE* _tfsopen(const char * _Filename, const char * _Mode, int /*_ShFlag*/)
{
	return fopen(_Filename, _Mode);
}
#endif


namespace Proud
{
	String GetLogTimeString()
	{
		timespec ts;
		timespec_get_pn(&ts, TIME_UTC);

		tm t;
		localtime_pn(&ts.tv_sec, &t);

		// foramt YYYY-mm-dd HH:MM:SS
		PNTCHAR buf[256] = { 0, };
		_tcsftime(buf, ArraySize(buf), _PNT("%Y-%m-%d %H:%M:%S"), &t);

		String ret = String::NewFormat(_PNT("%s.%d"), buf, ts.tv_nsec / 1000000);
		return ret;
	}

	String GetFileTimeString()
	{
		timespec ts;
		timespec_get_pn(&ts, TIME_UTC);

		tm t;
		localtime_pn(&ts.tv_sec, &t);

		// foramt YYYYmmddHHMMSS
		PNTCHAR buf[256] = { 0, };
		_tcsftime(buf, ArraySize(buf), _PNT("%Y%m%d%H%M%S"), &t);

		String ret = buf;
		return ret;
	}

	String GetNewFileName(const String& orgFileName, bool putFileTimeString)
	{
		if (putFileTimeString == false)
		{
			return orgFileName;
		}

		String ret;
		String fileName, extension;

		// orgFileName 중에서 경로+파일명부분만을 fileName으로, .확장자 부분을 extension으로 반환
		int temp;

		// 마지막 .의 위치를 구해서 dotPos에 저장한다.
		int dotPos;
		temp = -1;
		do
		{
			dotPos = temp;
			temp = orgFileName.Find(_PNT("."), dotPos + 1);
		} while (temp != -1);

		// 마지막 /의 위치를 구해서 slashPos1에 저장한다
		int slashPos1;
		temp = -1;
		do
		{
			slashPos1 = temp;
			temp = orgFileName.Find(_PNT("/"), slashPos1 + 1);
		} while (temp != -1);

		// 마지막 \의 위치를 구해서 slashPos2에 저장한다
		int slashPos2;
		temp = -1;
		do
		{
			slashPos2 = temp;
			temp = orgFileName.Find(_PNT("\\"), slashPos2 + 1);
		} while (temp != -1);

		// 만약 마지막 슬래시의 위치가 점보다 뒤에 있으면
		// 해당 점은 경로명 중에 있는 것이고 파일명에는 확장자가 없는 경우이다.
		if (max(slashPos1, slashPos2) > dotPos || dotPos == -1)
		{
			// 확장자가 없는 경우
			fileName = orgFileName;
			extension = _PNT("");
		}
		else
		{
			// 파일명 추출
			fileName = orgFileName.Left(dotPos);

			// 확장자 추출
			extension = orgFileName.Mid(dotPos, orgFileName.GetLength() - dotPos);
		}

		ret.Format(_PNT("%s_%s%s"),
			fileName.GetString(),
			GetFileTimeString().GetString(),
			extension.GetString()
		);
		return ret;
	}

	CFileWrapper::CFileWrapper()
	{
		m_filePointer = NULL;
	}
	CFileWrapper::~CFileWrapper()
	{
		Close();
	}
	void CFileWrapper::Close()
	{
		if (m_filePointer != NULL)
		{
			fclose(m_filePointer);
			m_filePointer = NULL;
		}
	}
	bool CFileWrapper::Open(const String& path, const String& option/*= _PNT("a, ccs=UTF-8")*/)
	{
		Close();
		m_filePointer = _tfsopen(path.GetString(), option.GetString(), _SH_DENYWR);
		return m_filePointer != NULL;
	}

	int CFileWrapper::Puts(const String& str)
	{
		return _fputts(str.GetString(), m_filePointer);
	}
	int CFileWrapper::Putc(const PNTCHAR c)
	{
		return _fputtc(c, m_filePointer);
	}

	void CFileWrapper::Flush()
	{
		fflush(m_filePointer);
	}

	CLogWriter* CLogWriter::New(const String& logFileName, unsigned int newFileForLineLimit /*= 0*/, bool putFileTimeString /*=true*/)
	{
		// 객체 생성
		CLogWriterImpl* ret = new CLogWriterImpl(logFileName, newFileForLineLimit, putFileTimeString);

		if (ret->m_file == NULL)
		{
			int err = errno;
			String fn = GetNewFileName(logFileName, putFileTimeString);
			delete ret;

			stringstream ss;
			ss << "Log file creation failed! file name=" << StringT2A(fn).GetString() << ",errno=" << err;
			throw Exception(ss.str().c_str());
		}
		// 모든게 준비됐다. 스레드도 시작한다.
		ret->StartThread();

		return (CLogWriter*)ret;
	}

	CLogWriterImpl::CLogWriterImpl(const String& logFileName, unsigned int newFileForLineLimit, bool putFileTimeString) :
		m_writeSemaphore(0, INT_MAX),
		m_changeFileFailed(false),
		m_newFileForLineLimit(newFileForLineLimit),
		m_currentLineCount(0),
		m_workerThread(StaticThreadProc, this),
		m_ignorePendedWriteOnExit(false),
		m_putFileTimeString(putFileTimeString)
	{
		m_stopThread = false;
		m_baseFileName = logFileName;

		String fn = GetNewFileName(m_baseFileName, m_putFileTimeString);

		// 로그 파일 생성
		m_file = CFileWrapperPtr(new CFileWrapper);
		m_file->Open(fn);
	}

	CLogWriterImpl::~CLogWriterImpl()
	{
		// 스레드가 종료할 때까지 기다린다.
		m_stopThread = true;
		m_writeSemaphore.Release();
		m_workerThread.Join();
	}

	void CLogWriterImpl::StartThread()
	{
		m_workerThread.Start();
	}

	void CLogWriterImpl::StaticThreadProc(void* ctx)
	{
		if (ctx == NULL)
			throw Exception("StaticThreadProc Parameter is NULL");

		((CLogWriterImpl*)ctx)->ThreadProc();
	}

	void CLogWriterImpl::ThreadProc()
	{
		// 새로 파일을 기록하기 시작함을 의미하는 구분자를 첫 1회만 기록한다.
		m_file->Puts(String::NewFormat(_PNT("-------- Log start at %s --------\n"), GetLogTimeString().GetString()));

		// 메인루프
		while (m_stopThread == false)
		{
			// 이벤트 시그널이 있으면 쓰기 동작을 한다. 만약을 위해 대기시간을 infinity로 설정하진 않는다.
			if (m_writeSemaphore.WaitOne(300))
			{
				// 파일에 기록할 것들을 걷어낸다.
				// 걷어낼 데이터를 빨리 걷어낸 후 즉시 mutex unlock을 해야 다른 스레드에서 병목을 최소화한다.
				LogList logList;
				{
					CriticalSectionLock lock(m_CS, true);

					while (m_logList.GetCount() > 0)
					{
						logList.AddTail(m_logList.RemoveHead());
					}
				}

				// 걷어낸 것들을 모두 파일에 기록한다.
				Proud::AssertIsNotLockedByCurrentThread(m_CS);
				while (logList.GetCount() > 0)
				{
					LogDataPtr logData = logList.RemoveHead();
					ProcessLogData(logData);
				}
				m_file->Flush();
			}
		}

		// 종료 처리
		if (m_ignorePendedWriteOnExit == false)
		{
			CriticalSectionLock lock(m_CS, true);
			while (m_logList.GetCount() > 0)
			{
				// 아직 출력하지 못한 로그가 있는 경우 처리해줍니다.
				LogDataPtr logData = m_logList.RemoveHead();
				ProcessLogData(logData);
			}
		}
	}

	void CLogWriterImpl::ProcessLogData(LogDataPtr& logData)
	{
		//		// 콘솔에도 출력하자. 개발 편하게.
		//		// TODO: 본 내용을 더 편하게 그때그때마다 더 고치세요.
		//		cout << "NetServer Log for client " << logData->m_hostID << ": " << logData->m_message.GetString() << endl;

				// 메인
		switch (logData->m_type)
		{
		case LogType_NewFile:
		{
			//파일을 교체 한다.
			m_file->Close();
			String fn = GetNewFileName(logData->m_message, m_putFileTimeString);
			if (m_file->Open(fn) == false)
			{
				m_changeFileFailed = true;
				break;
			}
			else
			{
				// 파일 생성 실패를 인지한 유저가 다시 다른 파일을 생성하여 복구하려는 경우를 위함.
				m_changeFileFailed = false;
			}
			m_file->Puts(String::NewFormat(_PNT("-------- Log start at %s --------\n"), GetLogTimeString().GetString()));
		}
		break;

		case LogType_Logcat:
		{
			// 파일 교체에 실패했다면 이 로그는 쓸 수 없으므로 날린다.
			if (m_changeFileFailed)
				break;

			// 메시지 내에 있는 개행문자들을 공백으로 바꾼다.
			// 추후 라인수 제한 개념이 사라지고 바이트 제한으로 변경된다면 이 코드는 사라져야 한다.
			logData->m_message.Replace(_PNT("\n"), _PNT(" "));

			String log, codeInfo;
			if (logData->m_function.IsEmpty() == false)
			{
				codeInfo.Format(_PNT("{%s(%d)}"),
					logData->m_function.GetString(),
					logData->m_line
				);
			}

			log.Format(_PNT("%s: %d / %s(%d): %s %s\n"),
				logData->m_addedDateAndTime.GetString(),
				logData->m_logLevel,
				ToString(logData->m_logCategory),
				logData->m_hostID,
				logData->m_message.GetString(),
				codeInfo.GetString()
			);

			m_file->Puts(log);
		}
		break;

		case LogType_Raw:
		{
			// 파일 교체에 실패했다면 이 로그는 쓸 수 없으므로 날린다.
			if (m_changeFileFailed)
				break;

			// 메시지 내에 있는 개행문자들을 공백으로 바꾼다.
			// 추후 라인수 제한 개념이 사라지고 바이트 제한으로 변경된다면 이 코드는 사라져야 한다.
			logData->m_message.Replace(_PNT("\n"), _PNT(" "));

			logData->m_message += _PNT("\n");
			m_file->Puts(logData->m_message);
		}
		break;

		default:
			break;
		}
	}

	// 어느 스레드에서나 호출해도 된다. 새 파일 기록하라는 명령을 큐에 쌓을 뿐이니까.
	void CLogWriterImpl::SetFileName(const String& logFileName)
	{
		if (m_stopThread)
			return;

		LogDataPtr newLog(new LogData);
		newLog->m_type = LogType_NewFile;
		newLog->m_message = logFileName;

		CriticalSectionLock lock(m_CS, true);

		//파일이 바뀌므로.
		m_currentLineCount = 0;
		m_baseFileName = logFileName;

		m_logList.AddTail(newLog);

#if defined(_WIN32)
		NTTNTRACE("NewFileName: %s\n", StringT2A(logFileName).GetString());
#endif

		// Write이벤트 시그널을 넣는다.
		m_writeSemaphore.Release();
	}

	void CLogWriterImpl::WriteLine(
		int				logLevel,
		LogCategory		logCategory,
		HostID			logHostID,
		const String&	logMessage,
		const String&	logFunction /* = _PNT("") */,
		int				logLine /* = 0 */
	)
	{
		if (m_stopThread)
			return;

		if (m_changeFileFailed)
			throw Exception(_PNT("LogFile Creation Failed"));

		// 콜러의 부하를 조금이라도 줄이고자
		// 구조체만 넘기고 포맷팅은 IO Thread에서 수행한다.
		LogDataPtr newLog(new LogData);
		newLog->m_type = LogType_Logcat;
		newLog->m_logLevel = logLevel;
		newLog->m_logCategory = logCategory;
		newLog->m_hostID = logHostID;
		newLog->m_message = logMessage;
		newLog->m_function = logFunction;
		newLog->m_line = logLine;

		timespec ts;
		timespec_get_pn(&ts, TIME_UTC);

		tm tformat;
		localtime_pn(&ts.tv_sec, &tformat);

		newLog->m_addedDateAndTime.Format(_PNT("%d-%d %d:%d:%d.%d"),
			tformat.tm_mon + 1,
			tformat.tm_mday,
			tformat.tm_hour,
			tformat.tm_min,
			tformat.tm_sec,
			ts.tv_nsec / 1000000);

		WriteLine_Internal(newLog);
	}

	void CLogWriterImpl::WriteLine(const String& logMessage)
	{
		if (m_stopThread)
			return;

		if (m_changeFileFailed)
			throw Exception(_PNT("LogFile Creation Failed"));

		LogDataPtr newLog(new LogData);

		newLog->m_message = logMessage.GetString();

		newLog->m_type = LogType_Raw;

#if defined(_WIN32)
		NTTNTRACE("Log: %s\n", StringT2A(newLog->m_message).GetString());
#endif

		WriteLine_Internal(newLog);
	}

	void CLogWriterImpl::WriteLine_Internal(LogDataPtr& newLog)
	{
		CriticalSectionLock lock(m_CS, true);

		//파일 바꿔치기. - 일정 라인이 되면 바꿔치기 합니다.
		if ((m_newFileForLineLimit != 0) && (++m_currentLineCount > m_newFileForLineLimit))
		{
			// 기준파일명을 넘기면 자동으로 _날짜시간이 파일명에 추가된다.
			// 내부에서 m_baseFileName = m_baseFileName; 와 같은 연산이 일어나겠지만,
			// 이 함수는 사용자에게도 노출되는 함수이고 어짜피 Proud::String은 레퍼런스카운팅을 하므로 이와같이 호출한다.
			SetFileName(m_baseFileName);
		}

		m_logList.AddTail(newLog);

		// Write이벤트 시그널을 넣는다.
		m_writeSemaphore.Release();
	}

	void CLogWriterImpl::SetIgnorePendedWriteOnExit(bool flag)
	{
		m_ignorePendedWriteOnExit = flag;
	}
}//end of namespace Proud
