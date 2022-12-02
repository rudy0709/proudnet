/* ProudNet
이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.
** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#if defined(_WIN32)
#include <dbghelp.h>
#include <ASSERT.h>
#include <tchar.h>
#include <shellapi.h>
#endif

#include "../include/Exception.h"
#include "../include/MiniDumper.h"
#include "../include/sysutil.h"
#include "../include/ThreadUtil.h"
#include "../include/PnTime.h"
#include "../include/FastArray.h"
#include "../include/PnThread.h"


// 변수 이름을 바꾸지 말것. Test/UnitTest에서 이걸 쓰고 있다.
const PNTCHAR* g_ProudNet_CrashDumpArgText = _PNT("-ProudNetDumpXYZ");
const PNTCHAR* g_ProudNet_ManualDumpArgText = _PNT("-ProudNetDumpXXX");

namespace Proud
{
#if defined(_WIN32)


	CMiniDumpParameter::CMiniDumpParameter()
		: m_miniDumpType(SmallMiniDumpType)
	{
	}

	CMiniDumper::CMiniDumper()
	{
		m_hitCount = 0;
	}

	CMiniDumper::~CMiniDumper()
	{
	}

	// 직접 호출하지 말것
	// 프로그램 오류 발생시 호출되는 함수
	LONG CMiniDumper::TopLevelFilter(_EXCEPTION_POINTERS *pExceptionInfo)
	{
		RefCount<CMiniDumper> e = CMiniDumper::GetSharedPtr();
		if (e)
		{
			return e->TopLevelFilter_(pExceptionInfo);
		}
		else
		{
			// 이미 singleton lost. 할 수 있는게 없다.
			return EXCEPTION_CONTINUE_EXECUTION; // 예전에는 EXCEPTION_EXECUTE_HANDLER
		}
	}

	// 직접 호출하지 말것
	// 프로그램 오류 발생시 호출되는 함수
	LONG CMiniDumper::TopLevelFilter_(_EXCEPTION_POINTERS *pExceptionInfo)
	{
		CriticalSectionLock lock(m_filterWorking, 1);

		// 만약 아래 구문도 실패할 경우를 고려, 문제 재발시 무시하도록 한다.
		int32_t a = InterlockedIncrement(&m_hitCount);
		if (a > 1)
			return EXCEPTION_CONTINUE_SEARCH;
		
		if (CreateProcessAndWaitForExit(g_ProudNet_CrashDumpArgText, m_parameter.m_miniDumpType, pExceptionInfo) == 0)
		{
			// CreateProcess 실패
		}
		//return EXCEPTION_CONTINUE_SEARCH;
		return EXCEPTION_CONTINUE_EXECUTION; // 예전에는 EXCEPTION_EXECUTE_HANDLER
	}

	/* Flags 값에 따라서 덤프 파일을 생성한다. */
#if defined(_WIN64)
	bool CMiniDumper::DumpWithFlags(const PNTCHAR *dumpFileName, const uint32_t processID, const MINIDUMP_TYPE minidumpFlags, const uint32_t threadID, const uint64_t exceptionInfoAddr)
#else
	bool CMiniDumper::DumpWithFlags(const PNTCHAR *dumpFileName, const uint32_t processID, const MINIDUMP_TYPE minidumpFlags, const uint32_t threadID, const uint32_t exceptionInfoAddr)
#endif
	{
		CriticalSectionLock lock(m_filterWorking, 1);
		// create the file
		HANDLE hFile = ::CreateFile(dumpFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			// 프로세스 ID로 프로세스 핸들을 얻어옵니다.
			/*
			윈도우 XP에서는 PROCESS_ALL_ACCESS 플래그의 사이즈가 문제가 된다.
			윈도우 7에서는 가능
			그렇기 때문에 최소 권한으로 요청하는 것이 낫다.
			http://koronaii.tistory.com/279
			*/

			MINIDUMP_EXCEPTION_INFORMATION exInfo;
			exInfo.ThreadId = threadID;
			exInfo.ExceptionPointers = (EXCEPTION_POINTERS*)exceptionInfoAddr;
			exInfo.ClientPointers = TRUE;

			PMINIDUMP_EXCEPTION_INFORMATION exInfoPointer = (0 != exceptionInfoAddr ? &exInfo : NULL);

			HANDLE processHandle = ::OpenProcess(MAXIMUM_ALLOWED, 0, processID);
			// write the dump
			// 다른 프로세스의 덤프를 뜨는 것이지만, 그 프로세스로가 보내온 exInfoPointer 주소값을 여기서 쓸 수 있다!
			BOOL bOK = MiniDumpWriteDump(processHandle, processID, hFile, minidumpFlags, exInfoPointer, NULL, NULL);
			::CloseHandle(hFile);
			return bOK ? true : false;
		}
		return false;
	}

	/* 기존에 Init()내부에서 하던 SetUnhandledExceptionFilter를 따로 함수로 뺌 */
	void CMiniDumper::SetUnhandledExceptionHandler()
	{
		::SetUnhandledExceptionFilter( TopLevelFilter );
		//void* a = CKernel32Api::AddVectoredExceptionHandler(0,TopLevelFilter);
	}

	/* 유저 호출에 의해 미니 덤프 파일을 생성한다. 단, 메모리 전체 덤프를 하므로 용량이 큰 파일이 생성된다. */
	void CMiniDumper::ManualFullDump()
	{
		if (CreateProcessAndWaitForExit(g_ProudNet_ManualDumpArgText, MiniDumpWithFullMemory) == 0)
		{
			// CreateProcess 실패
		}
	}

	/* 유저 호출에 의해 미니 덤프 파일을 생성한다. */
	void CMiniDumper::ManualMiniDump()
	{
		if (CreateProcessAndWaitForExit(g_ProudNet_ManualDumpArgText, SmallMiniDumpType) == 0)
		{
			// CreateProcess 실패
		}
	}

	int GetCurrentThreadExecutionPoint(unsigned int /*code*/, _EXCEPTION_POINTERS *ep,HANDLE hFile) 
	{
		_MINIDUMP_EXCEPTION_INFORMATION ExInfo;
		ExInfo.ThreadId = ::GetCurrentThreadId();
		ExInfo.ClientPointers = NULL;
		ExInfo.ExceptionPointers = ep;
		// write the dump
		MiniDumpWriteDump( GetCurrentProcess(),
			(DWORD)GetCurrentProcessId(), hFile,
			MiniDumpNormal,
			&ExInfo, NULL, NULL );
		return EXCEPTION_EXECUTE_HANDLER;
	}

	void CMiniDumper::WriteDumpFromHere( const PNTCHAR* fileName, bool /*fullDump*/ )
	{
		// 덤프 파일을 일단 쌓기 시작하면 굳이 오류 창을 표시하게 할 필요가 없다. 따라서 막아버린다.
		SetErrorMode(SEM_NOGPFAULTERRORBOX);
		// create the file
		HANDLE hFile = ::CreateFile( fileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL, NULL );
		if (hFile != INVALID_HANDLE_VALUE)
		{
			// 현재 스레드의 콜스택 수집
			__try
			{
				char* a=0;
				*a=1; //일부러
			}
			__except(GetCurrentThreadExecutionPoint(GetExceptionCode(), GetExceptionInformation(), hFile))
			{
			}
			::CloseHandle(hFile);
		}
	}

	void CreateDumpProcessDelegate(void* ctx)
	{
		String *cmdLine = (String *)ctx;
		if (NULL == cmdLine)
		{
			return;
		}

		STARTUPINFO startupInfo;
		::memset(&startupInfo, 0, sizeof(startupInfo));
		PROCESS_INFORMATION processInformation;
		::memset(&processInformation, 0, sizeof(processInformation));
		// 덤프를 남길 프로세스를 생성한다. 부모로부터 핸들을 상속받지는 않는다.
		StrBuf cmdLineBuf(*cmdLine, 32768);

		if (::CreateProcess(NULL, cmdLineBuf, NULL, NULL, false, 0, NULL, NULL, &startupInfo, &processInformation) == 0)
		{
			return;
		}

		// 프로세스가 종료될때까지 대기한다.
		::WaitForSingleObject(processInformation.hProcess, PN_INFINITE);
		::CloseHandle(processInformation.hProcess);
	}

	/* 새로운 프로세스를 생성하고 종료될 때까지 대기한다.	*/
	 uint32_t CMiniDumper::CreateProcessAndWaitForExit(
		const PNTCHAR *args, MINIDUMP_TYPE miniDumpType, _EXCEPTION_POINTERS *pExceptionInfo)
	{
		// WERS(Windows Error Reporting Service)이 실행 중일 경우에 덤프 프로세스를 생성하기 전에 SetErrorMode(SEM_NOGPFAULTERRORBOX) 호출을 해야  WerFault.exe를 띄우지 않습니다.
		// 덤프 파일을 일단 쌓기 시작하면 굳이 오류 창을 표시하게 할 필요가 없습니다.
		::SetErrorMode(SEM_NOGPFAULTERRORBOX);

		// ModuleFileName
		PNTCHAR moduleFileName[MAX_PATH] = { 0, };
		::GetModuleFileName(NULL, moduleFileName, MAX_PATH);

		// 명령줄을 생성(thread id는 exception을 catch한 스레드의 id를 넣어야 합니다.
		String cmdLine;
		cmdLine.Format(_PNT("\"%s\" %s %d \"%s\" %d %d %p"), moduleFileName, args, ::GetCurrentProcessId(), m_parameter.m_dumpFileName.GetBuffer(), miniDumpType, ::GetCurrentThreadId(), pExceptionInfo);

		// 64bit의 경우 32bit와는 다르게 stackoverflow가 발생 한 후 CreateProcess 함수를 실행하면 CreateProcess 함수 내부에서 사용하는 
		// stack 메모리가 부족해 추가적으로 exception이 발생해서 덤프 과정이 실행이 되지 않습니다. 덤프 생성용 스레드를 별도로 생성하도록 합니다.
		Proud::Thread createProcessThread(CreateDumpProcessDelegate, &cmdLine);
		createProcessThread.Start();
		createProcessThread.Join();

		return 0;
	}

	// 이 프로세스를 실행할때 입력되었던 argc,argv를 얻는다.
	// main()으로부터 받지 않았어도 이렇게 얻을 수 있다.
	void GetCommandLine(CFastArray<String> &tokenArray)
	{
		LPWSTR *arglist = NULL;
		int args = 0;

		arglist = ::CommandLineToArgvW(::GetCommandLineW(), &args);
		if (NULL == arglist)
		{
			return;
		}

		for (int i = 1; i < args; ++i)
		{
			tokenArray.Add(String(arglist[i]));
		}

		LocalFree(arglist);
	}

	// 사용자가 호출하는 함수
	MiniDumpAction CMiniDumper::Startup(const CMiniDumpParameter &parameter)
	{
		return CMiniDumper::GetUnsafeRef().Startup_(parameter);
	}

	MiniDumpAction CMiniDumper::Startup_(const CMiniDumpParameter &parameter)
	{
		{
			m_parameter = parameter;

			// 사용자가 입력한 ""를 제거 합니다.
			m_parameter.m_dumpFileName.Replace(_PNT("\""), _PNT(""));

			CFastArray<String> tokenArray;
			GetCommandLine(tokenArray);

			// cmdLine에서 잘라온 문자열의 갯수가 4개보다 작으면 일반앱으로 처리한다.
			if (6 > tokenArray.GetCount())
			{
				goto NormalApp;
			}

			// 두번 째 인자로부터 ProcessID를 DWORD로 변환한다.
			const uint32_t processID = _ttoi(tokenArray[1].GetString());

			// 네번 째 인자로부터 MINIDUMP_TYPE으로 변환한다.
			MINIDUMP_TYPE miniDumpType = (MINIDUMP_TYPE)_ttoi(tokenArray[3].GetString());
			
			// 다섯 째 인자로 부터 예외가 발생된 thread id를 변환한다.
			const uint32_t excepThreadId = _ttoi(tokenArray[4].GetString());

			// x64에서는 포인터 주소 값이 4바이트보다 더 클 수도 있습니다.
			PNTCHAR *addrEnd;
#if defined(_WIN64)
			const uint64_t exceptionInfoAddr = _tcstoui64(tokenArray[5].GetBuffer(), &addrEnd, 16);
#else
			const uint32_t exceptionInfoAddr = _tcstoul(tokenArray[5].GetBuffer(), &addrEnd, 16);
#endif
			// 특수구문이 ProudNetDumpXXX인 경우
			if (tokenArray[0] == g_ProudNet_ManualDumpArgText)
			{
				// ManualFullDump등의 함수로 생성된 프로세스인 경우
				DumpWithFlags(tokenArray[2].GetString(), processID, miniDumpType, excepThreadId, exceptionInfoAddr);
				return MiniDumpAction_DoNothing;
			}
			// 덤프 뜨라는 의미의 구문이 들어가 있으면
			else if (tokenArray[0] == g_ProudNet_CrashDumpArgText)
			{
				// Exception이 발생하여 생성된 프로세스인 경우이다.
				// 덤프를 뜨자.
				DumpWithFlags(tokenArray[2].GetString(), processID, miniDumpType, excepThreadId, exceptionInfoAddr);
				// 뜬 덤프를 강제로 종료.
				HANDLE processHandle = ::OpenProcess(PROCESS_TERMINATE, false, processID);
				DWORD exitCode;
				// 프로세스의 상태를 확인한다.
				::GetExitCodeProcess(processHandle, &exitCode);
				if (exitCode == STILL_ACTIVE)
				{
					// 여전히 프로세스가 살아있는 경우 강제 종료.
					::TerminateProcess(processHandle, 0);
				}
				return MiniDumpAction_AlarmCrash;
			}
		}
	NormalApp:
		//ProudNetDump 특수구문과 다른 경우 일반앱으로 처리한다.
		SetUnhandledExceptionHandler();
		return MiniDumpAction_None;
	}
	// 	void CMiniDumper::WriteDumpAtExceptionThrower( LPCWSTR fileName, bool fullDump )
	// 	{
	// 		// 덤프 파일을 일단 쌓기 시작하면 굳이 오류 창을 표시하게 할 필요가 없다. 따라서 막아버린다.
	// 		SetErrorMode(SEM_NOGPFAULTERRORBOX);
	// 
	// 		// create the file
	// 		HANDLE hFile = ::CreateFile( fileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
	// 			FILE_ATTRIBUTE_NORMAL, NULL );
	// 
	// 		if (hFile != INVALID_HANDLE_VALUE)
	// 		{
	// 			_MINIDUMP_EXCEPTION_INFORMATION ExInfo;
	// 			ExInfo.ThreadId = ::GetCurrentThreadId();
	// 			ExInfo.ClientPointers = NULL;
	// 
	// 			ExInfo.ExceptionPointers = GetExceptionInformation();
	// 
	// 			// write the dump
	// 			BOOL bOK = MiniDumpWriteDump( GetCurrentProcess(),
	// 				GetCurrentProcessId(), hFile,
	// 				MiniDumpNormal,
	// 				&ExInfo, NULL, NULL );
	// 
	// 			::CloseHandle(hFile);
	// 		}
	// 
	// 	}

	LONG CALLBACK CExceptionLogger::ExceptionLoggerProc(PEXCEPTION_POINTERS pExceptionInfo)
	{
		RefCount<CExceptionLogger> e = CExceptionLogger::GetSharedPtr();
		if (e)
		{
			return e->ExceptionLoggerProc_(pExceptionInfo);
		}
		else
		{
			// 이미 singleton이 파괴되었다. 할 수 있는게 없다.
			return EXCEPTION_CONTINUE_SEARCH;
		}
	}
		
	// 현재 시간을 file name으로 표현하는 프로세스 덤프 파일을 남긴다.
	LONG CExceptionLogger::ExceptionLoggerProc_(PEXCEPTION_POINTERS pExceptionInfo)
	{
#ifdef _DEBUG
		// 디버깅을 하고자 할 때는 non-debugger로 실행 후 아래 라인에서 멈추면 debugger attach를 하면 땡.
		//::MessageBox(NULL,_PNT("XXX"),_PNT("XXX"),MB_OK);
#endif

		// NOTE: Non-Debugger 상태에서 OutputDebugString이나 ATLTRACE 함수 사용할 경우, Dump파일이 생성되는 문제에 대한 해결법입니다.
		// OutputDebugString의 경우 Debug, Release의 상관없이 Dump파일이 생성되어 별도의 전처리문을 넣지 않았습니다.
		// 참고로 ATLTRACE는 자체적으로 Debug가 아니면 동작하지 않도록 구현이 되어있습니다.

		// NOTE: DBG_PRINTEXCEPTION_C는 Visual Studio 2010 부터 정의되어있어서, Visual Studio에 따라서 처리하도록 하였습니다.
#if (_MSC_VER>=1600)
		if (pExceptionInfo->ExceptionRecord->ExceptionCode == DBG_PRINTEXCEPTION_C)
#else
		if (pExceptionInfo->ExceptionRecord->ExceptionCode == 0x40010006L/*DBG_PRINTEXCEPTION_C*/)
#endif
			return EXCEPTION_CONTINUE_SEARCH;

		// 예외가 발생한 순간의 호출 스택을 담은 DMP 파일을 생성한다.
		CriticalSectionLock lock(m_cs,true);
		// 가장 마지막에 덤프한 이후 너무 짧은 시간 내면 덤프를 무시한다. 이게 없으면 지나치게 많은 예외
		// 발생시 정상적인 실행이 불가능할 정도로 느려진다.
		uint32_t currTick = CFakeWin32::GetTickCount();
		if(currTick - m_lastLoggedTick >= 10000)
		{
			timespec ts;
			timespec_get_pn(&ts, TIME_UTC);

			tm t;
			localtime_pn(&ts.tv_sec, &t);

			String strDumpTime = String::NewFormat(_PNT("[%04d-%02d-%02d(%02d.%02d.%02d)]"),
				t.tm_year + 1900,
				t.tm_mon + 1,
				t.tm_mday,
				t.tm_hour,
				t.tm_min,
				t.tm_sec
			);

			m_lastLoggedTick = currTick;
			m_dumpSerial++;
			Tstringstream ss;
			ss << m_directory.GetString()
				<< _PNT("\\")
				<< m_dumpName.GetString()
				<< strDumpTime.GetString()
				<< m_dumpSerial 
				<< _PNT(".DMP");

			String dmpFilePath = ss.str().c_str();
			
			// create the file
			HANDLE hFile = ::CreateFile( dmpFilePath.GetString(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL, NULL );
			if (hFile != INVALID_HANDLE_VALUE)
			{
				_MINIDUMP_EXCEPTION_INFORMATION ExInfo;
				ExInfo.ThreadId = ::GetCurrentThreadId();
				ExInfo.ExceptionPointers = pExceptionInfo;
				ExInfo.ClientPointers = NULL;
				// 덤프 파일을 일단 쌓기 시작하면 굳이 오류 창을 표시하게 할 필요가 없다. 따라서 막아버린다.
				SetErrorMode(SEM_NOGPFAULTERRORBOX);
				// write the dump
				MiniDumpWriteDump( GetCurrentProcess(),
					(DWORD)GetCurrentProcessId(), hFile,
					MiniDumpNormal,
					&ExInfo, NULL, NULL );
				::CloseHandle(hFile);
			}
		}
		return EXCEPTION_CONTINUE_SEARCH;
	}

	void CExceptionLogger::Init(IExceptionLoggerDelegate* dg, Proud::String dumpName)
	{
		// 덤프 파일이 쌓일 폴더를 생성한다.
		m_dg=dg;
		m_dumpName = dumpName;
		m_dumpSerial=0;
		m_lastLoggedTick=0;
		CreateFullDirectory(m_dg->GetDumpDirectory().GetString(), m_directory);

		if (m_directory.GetLength() > 0 && m_directory[m_directory.GetLength() - 1] == _PNT('\\'))
			m_directory = m_directory.Left(m_directory.GetLength() - 1);
		// 이 기능은 Windows 2003 또는 Windows XP 이상에서만 작동한다.
		::AddVectoredExceptionHandler(1, ExceptionLoggerProc);
	}
#endif // __MACH__
}
