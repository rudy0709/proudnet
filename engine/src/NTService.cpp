/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "../include/NTService.h"
#include "../include/NTServiceStartParameter.h"
#include "../include/ThreadUtil.h"
#include "IntraTracer.h"

#ifdef _WIN32
#include <tchar.h>
#endif

using namespace std;

namespace Proud
{

	class CNTServiceInternal
	{
	public:

		CFastArray<String> m_args,m_argsFromScm,m_envParams;

		bool m_actualService;
		PNTCHAR m_name[1000];

		uint32_t m_lastFrequentWarning;

		SERVICE_STATUS_HANDLE m_hServiceStatus;
		SERVICE_STATUS m_status;
		INTServiceEvent *m_eventSink;
		uint32_t m_dwThreadID;

		CNTServiceInternal()
		{
			m_eventSink = NULL;
			m_lastFrequentWarning = 0;
		}
	};

	/** 외부 DLL을 쓰는 경우 R6002 floating point not loaded라는 에러가 발생하는 경우가 있다.
	그것을 예방하고자, 무조건 아래와 같은 루틴을 넣도록 한다. */
	void EnforceLoadFloatingPointModule()
	{
		const char *s = "1.2";
		float a = 1.2f;
#if (_MSC_VER>=1400)
		sscanf_s(s, "%f", &a);
#else
		sscanf(s, "%f", &a);
#endif
	}

	CNTService::CNTService(void)
	{
		m_internal=new CNTServiceInternal;
	}

	CNTService::~CNTService(void)
	{
		delete m_internal;
	}

	void CNTService::WinMain(int argc, char* argv[], char** /*envp[]*/, CNTServiceStartParameter &param)
	{
		RefCount<CNTService> core = CNTService::GetSharedPtr();
		if(core)
		{
			for (int i = 0; i < argc; i++)
				core->m_internal->m_args.Add(StringA2T(argv[i]));
			//m_internal->m_envp = envp;

			core->WinMain_Internal(param);
		}
		else
		{
			// singleton lost.
		}
	}

	void CNTService::WinMain(int argc, wchar_t* argv[], wchar_t** /*envp[]*/, CNTServiceStartParameter &param)
	{
		RefCount<CNTService> core = CNTService::GetSharedPtr();
		if (core)
		{
			for (int i = 0; i < argc; i++)
				core->m_internal->m_args.Add(StringW2T(argv[i]));
			//m_internal->m_envp = envp;

			core->WinMain_Internal(param);
		}
		else
		{
			// singleton lost.
		}
	}

	void CNTService::WinMain_ActualService()
	{
		SERVICE_TABLE_ENTRY st[] =
		{
			{ m_internal->m_name, _ServiceMain },
			{ NULL, NULL }
		};

		m_internal->m_actualService = true;

		// StartServiceCtrlDispatcher가 성공하면, 이것을 호출하는 프로세스는 SCM에 연결되고 프로세스가 SERVICE_STOPPED 상태로 진입할 때까지 리턴하지 않는다.
		if (!::StartServiceCtrlDispatcher(st))
		{
			DWORD dw = GetLastError();
			m_internal->m_actualService = false;

			String text;
			// ERROR_FAILED_SERVICE_CONTROLLER_CONNECT 1063 이 에러는 콘솔 응용프로그램에서 서비스 모드를 실행하였을 때 나는 에러이다.
			if(dw == 1063L)
			{
				printf("Running in console mode. \n");
				WinMain_Console();
			}
			else
			{
				text.Format(_PNT("StartServiceCtrlDispatcher Error=%d Please send inquiries to Nettention Support"), dw);
				Log(EVENTLOG_INFORMATION_TYPE, text.GetString());
				// 1063이 아닐 때에는 콘솔 응용프로그램이 아니기 때문에 printf를 쓸 필요가 없다.
			}
		}
	}

	void CNTService::WinMain_Console()
	{
		Run();
	}

	void WINAPI CNTService::_ServiceMain(DWORD dwArgc, PNTCHAR** lpszArgv)
	{
		EnforceLoadFloatingPointModule();

		RefCount<CNTService> core = CNTService::GetSharedPtr();
		if(core)
			core->ServiceMain(dwArgc, lpszArgv);
	}

	/** 이 함수는 이 프로그램이 서비스 매니저에 의해 실행될 때 호출된다. 즉 시작 버튼을 누를떄 호출된다.
	(서비스 모드에서는 main()이 호출되지 않는다.)
	\argc 서비스 시작 메시지. .exe를 실행할 떄의 파라메터가 아니다.
	\argv 서비스 시작 메시지. .exe를 실행할 떄의 파라메터가 아니다.
	*/
	void CNTService::ServiceMain(int argc, PNTCHAR* argv[])
	{
		for(int i=0;i<argc;i++)
		{
			m_internal->m_argsFromScm.Add(argv[i]);
		}

		// Register the control request handler
		m_internal->m_status.dwCurrentState = SERVICE_START_PENDING;
		m_internal->m_hServiceStatus = RegisterServiceCtrlHandler(m_internal->m_name, _Handler);
		if (m_internal->m_hServiceStatus == NULL)
		{
			Log(EVENTLOG_INFORMATION_TYPE, _PNT("Handler not installed"));
			return;
		}
		SetServiceStatus(SERVICE_START_PENDING);

		m_internal->m_status.dwWin32ExitCode = S_OK;
		m_internal->m_status.dwCheckPoint = 0;
		m_internal->m_status.dwWaitHint = 0;

		// When the Run function returns, the service has stopped.
		Run();

		SetServiceStatus(SERVICE_STOPPED);
		Log(EVENTLOG_INFORMATION_TYPE, _PNT("Service has stopped."));
	}

	void CNTService::Handler(uint32_t dwOpcode)
	{
		switch (dwOpcode)
		{
		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			Log(EVENTLOG_INFORMATION_TYPE, _PNT("Service request to stop..."));
			SetServiceStatus(SERVICE_STOP_PENDING);
			if (m_internal->m_eventSink)			// 사용자에게 이벤트를 우선 날리고...
				m_internal->m_eventSink->Stop();

			// 프로그램 종료 메시지 보냄
			// Service 중지 상태가 SCM에게 오고나서 프로그램이 중지 되지 않으면
			// Service에서 '중지하는중' 이라는 상태로 계속 남아 있게 된다.
			// 이 코드가 없으면 유저 레벨(고객)에서 프로그램을 종료해야한다.
			// 서비스 중지가 왔다는건 해당 프로그램이 종료되야 한다는것이고 고객사의
			// 실수를 방지하기 위해 우리가 직접 프로그램 종료 메시지를 보낸다
			// (고객사에서 우리의 가이드대로 윈도우 메시지 처리루틴을 만들었다면 이것은 정상적인 코드이다.)
			// by. hyeonmin.yoon
			::PostThreadMessage(m_internal->m_dwThreadID, WM_QUIT, 0, 0);
			break;
		case SERVICE_CONTROL_PAUSE:
			SetServiceStatus(SERVICE_PAUSE_PENDING);
			if(m_internal->m_eventSink)
				m_internal->m_eventSink->Pause();
			SetServiceStatus(SERVICE_PAUSED);
			break;
		case SERVICE_CONTROL_CONTINUE:
			SetServiceStatus(SERVICE_CONTINUE_PENDING);
			if(m_internal->m_eventSink)
				m_internal->m_eventSink->Continue();
			SetServiceStatus(SERVICE_RUNNING);
			break;
		case SERVICE_CONTROL_INTERROGATE:
			break;
		default:
			Log(EVENTLOG_INFORMATION_TYPE, _PNT("Invalid service request"));
			break;
		}
	}

	void WINAPI CNTService::_Handler(DWORD dwOpcode)
	{
		RefCount<CNTService> core = CNTService::GetSharedPtr();
		core->Handler(dwOpcode);
	}

	void CNTService::SetServiceStatus(uint32_t dwState)
	{
		if (m_internal->m_actualService)
		{
			m_internal->m_status.dwCurrentState = dwState;
			::SetServiceStatus(m_internal->m_hServiceStatus, &m_internal->m_status);
		}
	}

	/** 실행 파라메터에 특정 단어가 있나 찾는다.
	\param name 찾을 단어 */
	bool CNTService::FindArg(const PNTCHAR* name)
	{
		for (int i = 0;i < (int)m_internal->m_args.GetCount();i++)
		{
			String name_(name);
			if (name_.CompareNoCase(m_internal->m_args[i]) == 0)
			//if (Tstrcmp(name, m_internal->m_args[i]) == 0)
			{
				return true;
			}
		}

		return false;
	}

	/** 프라우드넷에서 사용하는 arg들을 제외한 나머지 arg들을 합쳐서 뽑아준다.
	*/
	Proud::String CNTService::CreateArg()
	{
		String ret;
		for(int i = 0; i < (int)m_internal->m_args.GetCount();i++)
		{
			String strAR(_PNT("-AR"));
			String strConsole(_PNT("-console"));
			String strInstall(_PNT("-install"));
			String strUninstall(_PNT("-uninstall"));

			if( strAR.CompareNoCase(m_internal->m_args[i]) == 0 ||
				strConsole.CompareNoCase(m_internal->m_args[i]) == 0 ||
				strInstall.CompareNoCase(m_internal->m_args[i]) == 0 ||
				strUninstall.CompareNoCase(m_internal->m_args[i]) == 0 )
			/*if( Tstrcmp(_PNT("-AR"), m_internal->m_args[i]) == 0 ||
				Tstrcmp(_PNT("-console"), m_internal->m_args[i]) == 0 ||
				Tstrcmp(_PNT("-install"), m_internal->m_args[i]) == 0 ||
				Tstrcmp(_PNT("-uninstall"), m_internal->m_args[i]) == 0)*/
				continue;

			ret += m_internal->m_args[i];
			ret += _PNT(" ");
		}

		return ret;
	}

	/** Log 문자열을 printf()처럼 남긴다. INTServiceEvent.Log()를 내부에서 호출한다. */
	void CNTService::Log(int type, const PNTCHAR* fmt, ...)
	{
		if (m_internal->m_eventSink)
		{
			String text;
			va_list	pArg;
			va_start(pArg, fmt);
			text.FormatV(fmt, pArg);
			va_end(pArg);

			m_internal->m_eventSink->Log(type, text.GetString());
		}
	}

	void CNTService::Run()
	{
		m_internal->m_dwThreadID = ::GetCurrentThreadId();

		SetServiceStatus(SERVICE_RUNNING);

		Log(EVENTLOG_INFORMATION_TYPE, _PNT("Start service. Process ID=%d"), GetCurrentProcessId() );

		// 2016.3.18 dwThreadID 인자는 uint32_t 타입이지만 SetThreadName 함수내에서 아무것도 안하기 때문에 dwThreadID 의 값이 0 이여도 상관없다.
		SetThreadName(0, "ProudNet Service main");

		if (m_internal->m_eventSink)
			m_internal->m_eventSink->Run();
	}

	BOOL CNTService::Install(CServiceParameter &param)
	{
		// 기 동명 서비스를 제거.
		Uninstall();

		SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (hSCM == NULL)
		{
			// 유니코드를 wprintf하여 한글이 출력되기 위해서는 _wsetlocale(LC_ALL, L"korean"); 되어 있어야 한다.
			cout << StringT2A(m_internal->m_name).GetString() <<
				": Cannot open Service Control Manager. Please run this app in Administrator Mode.";
			return FALSE;
		}

		// Get the executable file path
		PNTCHAR szFilePath[LongMaxPath];
		::GetModuleFileName(NULL, szFilePath, LongMaxPath);

		String binarypathname = String(szFilePath) + _PNT(" ") + CreateArg();

		SC_HANDLE hService = ::CreateService(
			hSCM, m_internal->m_name,  m_internal->m_name,
			param.m_desiredAccess /*SERVICE_ALL_ACCESS*/,
			param.m_serviceType /*SERVICE_WIN32_OWN_PROCESS*/,
			param.m_startType /*SERVICE_DEMAND_START*/,
			param.m_errorControl /*SERVICE_ERROR_NORMAL*/,
			binarypathname.GetString(), NULL, NULL, NULL,
			param.m_serviceStartName.IsEmpty() ? NULL : param.m_serviceStartName.GetString(),
			param.m_password.IsEmpty() ? NULL : param.m_password.GetString());

		if (hService == NULL)
		{
			::CloseServiceHandle(hSCM);
			String str;
			cout << "Error: Cannot register " <<
				StringT2A(m_internal->m_name).GetString() <<
				"as Windows Service. GetLastError=" << GetLastError() << ".\n";

			return FALSE;
		}

		// 서비스 실패(예: 크래쉬)시 자동 재시작을 하게 한다.
		if (FindArg(_PNT("-AR")))
		{
			SC_ACTION a;
			a.Type = SC_ACTION_RESTART;
			a.Delay = 60000;

			SERVICE_FAILURE_ACTIONS failureActions;
			// ikpil.choi 2016-11-07 : memset_s 로 변경
			//ZeroMemory(&failureActions, sizeof(failureActions));
			memset_s(&failureActions, sizeof(failureActions), 0, sizeof(failureActions));
			failureActions.dwResetPeriod = 60 * 60 * 24;
			failureActions.cActions = 1;
			failureActions.lpsaActions = &a;

			if (!ChangeServiceConfig2(hService, SERVICE_CONFIG_FAILURE_ACTIONS, &failureActions))
			{
				cout << StringT2A(m_internal->m_name).GetString() <<
					": Service registration done. But, we cannot config failure actions.";
			}
		}

		::CloseServiceHandle(hService);
		::CloseServiceHandle(hSCM);

		return TRUE;
	}

	/** 이 NT 서비스가 SCM에 등록되어 있는가? */
	BOOL CNTService::IsInstalled()
	{
		BOOL bResult = FALSE;

		SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

		if (hSCM != NULL)
		{
			SC_HANDLE hService = ::OpenService(hSCM, m_internal->m_name, SERVICE_QUERY_CONFIG);
			if (hService != NULL)
			{
				bResult = TRUE;
				::CloseServiceHandle(hService);
			}
			::CloseServiceHandle(hSCM);
		}
		return bResult;
	}

	BOOL CNTService::Uninstall()
	{
		if (!IsInstalled())
			return TRUE;

		SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

		if (hSCM == NULL)
		{
			cout<< StringT2A(m_internal->m_name).GetString()<<
				"Couldn't open Service Manager.";
			return FALSE;
		}

		SC_HANDLE hService = ::OpenService(hSCM, m_internal->m_name, SERVICE_STOP | DELETE);

		if (hService == NULL)
		{
			::CloseServiceHandle(hSCM);
			cout << StringT2A(m_internal->m_name).GetString() <<
				": Couldn't open Service.";
			return FALSE;
		}
		SERVICE_STATUS status;
		::ControlService(hService, SERVICE_CONTROL_STOP, &status);

		BOOL bDelete = ::DeleteService(hService);
		::CloseServiceHandle(hService);
		::CloseServiceHandle(hSCM);

		if (bDelete)
			return TRUE;

		cout << StringT2A(m_internal->m_name).GetString() <<
			": Service could not be deleted.";

		return FALSE;
	}

	/** FrequentWarningWithCallStack과 달리 콜 스택을 남기지 않는다. */
	void CNTService::FrequentWarning(const PNTCHAR* text)
	{
		uint32_t currTime = CFakeWin32::GetTickCount();

		if (!m_internal->m_lastFrequentWarning || currTime - m_internal->m_lastFrequentWarning > 500)
		{
			m_internal->m_lastFrequentWarning = currTime;

			String ee;
			ee.Format(_PNT("%s\n(Consecutive Appearance of Warnings for Short Time Skipped)"), text);
			if (m_internal->m_eventSink)
			{
				m_internal->m_eventSink->Log(EVENTLOG_WARNING_TYPE, ee.GetString());
			}
			NTTNTRACE("%s\n", StringT2A(ee).GetString());
		}
	}

	/** 자주 발생할 수 있는 경고를 출력한다. 이 경고는 너무 많이 발생해서 서버가 오히려 과부하가 걸리는걸 막는 기능이 있다.
	막는 방법은, 잦은 메시지는 버린다. */
	void CNTService::FrequentWarningWithCallStack(const PNTCHAR* text)
	{
		uint32_t currTime = CFakeWin32::GetTickCount();

		if (!m_internal->m_lastFrequentWarning || currTime - m_internal->m_lastFrequentWarning > 500)
		{
			m_internal->m_lastFrequentWarning = currTime;

			String ee;
			ee.Format(_PNT("%s\n(Consecutive Appearance of Warnings for Short Time Skipped)"), text);

			//StringA dmp;
			//g_CallStackDumper.GetDumped_ForGameServer(dmp);
			//String ee;
			//ee.Format(_PNT("%s\n%s\n(짧은 시간동안 연속 발생 경고는 생략)"),text,StringA2T(dmp));

			if (m_internal->m_eventSink)
			{
				m_internal->m_eventSink->Log(EVENTLOG_WARNING_TYPE, ee.GetString());
			}
			NTTNTRACE("%s\n", StringT2A(ee).GetString());
		}
	}

	bool CNTService::IsStartedBySCM() const
	{
		return m_internal->m_actualService;
	}

	const PNTCHAR* CNTService::GetName()
	{
		return m_internal->m_name;
	}

//	int CNTService::GetArgcFromSCM_Internal()
//	{
//		return m_internal->m_argsFromScm.GetCount();
//	}

//	int CNTService::GetArgc_Internal()
//	{
//		return m_internal->m_args.GetCount();
//	}

	void CNTService::GetArgvFromSCM_Internal(CFastArray<String>& output)
	{
		output.Clear();
		for (int i = 0; i < m_internal->m_argsFromScm.GetCount(); ++i)
		{
			output.Add(m_internal->m_argsFromScm[i]);
		}
	}

	void CNTService::GetArgv_Internal(CFastArray<String>& output)
	{
		output.Clear();
		for (int i = 0; i < m_internal->m_args.GetCount(); ++i)
		{
			output.Add(m_internal->m_args[i]);
		}
	}

	void CNTService::GetEnvp_Internal(CFastArray<String>& output)
	{
		output.Clear();
		for (int i = 0; i < m_internal->m_envParams.GetCount(); ++i)
		{
			output.Add(m_internal->m_envParams[i]);
		}
	}

	void CNTService::WinMain_Internal( CNTServiceStartParameter &param )
	{
		m_internal->m_eventSink = param.m_serviceEvent;

		// ikpil.choi 2016-11-29 : _tcscpy_s 추가
		_tcscpy_s(m_internal->m_name, param.m_serviceName.GetString());

		// set up the initial service status
		m_internal->m_hServiceStatus = NULL;
		m_internal->m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
		m_internal->m_status.dwCurrentState = SERVICE_STOPPED;
		m_internal->m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE;
		m_internal->m_status.dwWin32ExitCode = 0;
		m_internal->m_status.dwServiceSpecificExitCode = 0;
		m_internal->m_status.dwCheckPoint = 0;
		m_internal->m_status.dwWaitHint = 0;

		if (FindArg(_PNT("-install")) == true)
		{
			if (!Install(param.m_serviceParam))
			{
				cout << StringT2A(m_internal->m_name).GetString() <<
					": Service registration failed.";
			}

			return;
		}
		else if (FindArg(_PNT("-uninstall")))
		{
			if (!Uninstall())
			{
				cout << StringT2A(m_internal->m_name).GetString() <<
					": Service uninstall failed.";
			}

			return;
		}

		printf("NT Service를 시작합니다. \n");
			WinMain_ActualService();
		}

	void CNTService::GetArgv(CFastArray<String>& output)
	{
		RefCount<CNTService> core = CNTService::GetSharedPtr();
		core->GetArgv_Internal(output);
	}

	void CNTService::GetEnvp(CFastArray<String>& output)
	{
		RefCount<CNTService> core = CNTService::GetSharedPtr();
		core->GetEnvp_Internal(output);
	}

	void CNTService::GetArgvFromSCM(CFastArray<String>& output)
	{
		RefCount<CNTService> core = CNTService::GetSharedPtr();
		core->GetArgvFromSCM_Internal(output);
	}
}
