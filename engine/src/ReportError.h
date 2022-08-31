#pragma once

#include "../include/Singleton.h"
#include "FastSocket.h"

namespace Proud
{
#ifdef _WIN32
	class CReportSocketDg//:public IFastSocketDelegate
	{
	public:
		virtual void OnSocketWarning(CFastSocket* socket, String msg);
	};
#endif // _WIN32

	// 클라,서버 모두 쓴다.
	class CErrorReporter_Indeed:public CSingleton<CErrorReporter_Indeed>
	{
#ifdef _WIN32
		enum LogSocketState
		{
			LSS_Init,
			LSS_Connecting,
			LSS_Sending,
			LSS_Finished,
		};

		static const PNTCHAR* LogServerName;
		static int LogServerPort;
		static const char* LogHttpMsgFmt;
		static const char* LogHttpMsgFmt2;

		LogSocketState m_logSocketState;
		uint32_t m_lastReportedTime;

		// 유저가 쌓은 메시지
		String m_text;

		shared_ptr<CFastSocket> m_logSocket;
		CReportSocketDg m_logSocketDg;

		// overlapped IO 중이므로 이 버퍼를 유지하고 있어야 한다.
		StringA m_logHttpTxt;

		int m_logSendProgress;

		void Heartbeat_LogSocket_SendingCase();
		void Heartbeat_LogSocket_ConnectingCase();

		void Heartbeat_LogSocket_IssueSend();
		void Heartbeat_LogSocket_NoneCase();

		bool Heartbeat_LogSocket_IssueConnect();

		void IssueCloseAndWaitUntilNoIssueGuaranteed();

		CriticalSection m_cs;
		volatile bool m_stopThread;

		void RequestReport(const String &text);

		Thread m_worker;
		volatile int32_t m_sendIoFlag; // 0:no issued, 1: issued, 2:stop acked
		static void StaticWorkerProc(void* ctx);
		void WorkerProc();
	public:
		CErrorReporter_Indeed();
		~CErrorReporter_Indeed();

		void Heartbeat_LogSocket();
#endif // _WIN32

	public:
		PROUD_API static void Report( String text );
	};

	class CErrorReporter_Dummy:public CSingleton<CErrorReporter_Dummy>
	{
	public:
		static void Report( String text )
		{
			// do nothing
		}
	};

	#ifdef NO_USE_ERROR_REPORTER
		typedef CErrorReporter_Dummy CErrorReporter;
	#else
		typedef CErrorReporter_Indeed CErrorReporter;
	#endif
}
