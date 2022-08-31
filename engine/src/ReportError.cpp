/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include <stack> // for xcode
#include "ReportError.h"
#include "../include/ThreadUtil.h"
//#include "Networker_C.h"
#include "URI.h"
#include "SocketUtil.h"

namespace Proud
{
#ifdef _WIN32
	const PNTCHAR* CErrorReporter_Indeed::LogServerName=_PNT("www.nettention.com");
	int CErrorReporter_Indeed::LogServerPort=80;
	const char* CErrorReporter_Indeed::LogHttpMsgFmt = "GET /PNReport/ReportError.php?Text=";
	const char* CErrorReporter_Indeed::LogHttpMsgFmt2 = " HTTP/1.1\r\n"
		"User-Agent: Mozilla/5.0 (Windows; U; Windows NT 6.0; en-US) AppleWebKit/525.19 (KHTML, like Gecko) Chrome/0.3.154.9 Safari/525.19\r\n"
		"Accept-Language: ko-KR,ko,en-US,en\r\n"
		"Accept: text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5\r\n"
		"Cache-Control: max-age=0\r\n"
		"Accept-Charset: EUC-KR,*,utf-8\r\n"
		"Accept-Encoding: gzip,deflate,bzip2\r\n"
		"Host: %s\r\n"
		"Connection: Keep-Alive\r\n\r\n";

	void CErrorReporter_Indeed::Heartbeat_LogSocket()
	{
		try
		{
			CriticalSectionLock clk(m_cs, true); // 한 개의 스레드에서만 이것을 처리해야 하므로

			switch(m_logSocketState)
			{
			case LSS_Init:
				Heartbeat_LogSocket_NoneCase();
				break;
			case LSS_Connecting:
				Heartbeat_LogSocket_ConnectingCase();
				break;
			case LSS_Sending:
				Heartbeat_LogSocket_SendingCase();
				break;
			case LSS_Finished:
				// 아무것도 안함
				break;
			}
		}
		catch(std::exception &)
		{
			if(m_logSocket)
				m_logSocket->CloseSocketOnly();

			m_logSocketState = LSS_Finished;
		}
	}

	void CErrorReporter_Indeed::Heartbeat_LogSocket_NoneCase()
	{
		if(m_text.IsEmpty())
			return;

		// 소켓 생성 및 연결 시도를 한다.
		SocketCreateResult r = CFastSocket::Create(SocketType_Tcp/*,&m_logSocketDg*/);
		if (!r.socket)
		{
			return;
		}
		m_logSocket = r.socket;

		if (SocketErrorCode_Ok  != m_logSocket->Bind())
		{
			m_logSocket=shared_ptr<CFastSocket>();
			m_logSocketState=LSS_Finished;
			return;
		}

		// 소켓을 넌블럭킹 모드로 바꾼 후 연결 시도를 한다. 연결 시도가 끝나면 블럭킹 모드로 다시 바꾼다.
		m_logSocket->SetBlockingMode(false);

		// 연결 시도를 건다.
		if(!Heartbeat_LogSocket_IssueConnect())
		{
			m_logSocket=shared_ptr<CFastSocket>();
			m_logSocketState=LSS_Finished;
		}
		else
			m_logSocketState=LSS_Connecting;
	}

	void CErrorReporter_Indeed::Heartbeat_LogSocket_ConnectingCase()
	{
		FastSocketSelectContext selectContext;
		selectContext.AddWriteWaiter(*m_logSocket);
		selectContext.AddExceptionWaiter(*m_logSocket);

		// 기다리지 않고 폴링한다. 어차피 이 함수는 타이머에 의해 일정 시간마다 호출되니까.
		selectContext.Wait(0);

		SocketErrorCode code;
		bool did = selectContext.GetConnectResult(*m_logSocket, code);
		if (did)
		{
			if (code == SocketErrorCode_Ok)
			{
				// 오케바리. 이제 패킷을 보내기를 걸자.
				m_logSocket->SetBlockingMode(true);

				m_logSendProgress=0;

				Proud::CUri cannedText;
				cannedText.SetUrlPath((const PNTCHAR*)&m_text);
				cannedText.Canonicalize();

				//				CUrl cannedText;
				//				cannedText.SetUrlPath(m_text);
				//				cannedText.Canonicalize();
				m_text = _PNT(""); //이래야 다음 로그 요청도 받는다.

				StringA s1 = LogHttpMsgFmt;
				StringA s2 = StringT2A(cannedText.GetUrlPath());
				StringA s3; s3.Format(LogHttpMsgFmt2,StringT2A(LogServerName).GetString());

				m_logHttpTxt = s1+s2+s3;
				Heartbeat_LogSocket_IssueSend();

				m_logSocketState=LSS_Sending;
			}
			else
			{
				// nonblock socket의 connect에서는 blocked socket에서는 없던
				// '아직 연결 결과가 안나왔는데도 연결 실패라고 오는' 경우가
				// 종종 있다. 이런 것도 이렇게 막아야 한다.
				if (!Heartbeat_LogSocket_IssueConnect())
				{
					m_logSocket=shared_ptr<CFastSocket>();
					m_logSocketState=LSS_Finished;
				}
			}
		}
		else
		{
			// 아직 연결 결과를 얻지 못했다. 더 기다리자.
		}
	}

	bool CErrorReporter_Indeed::Heartbeat_LogSocket_IssueConnect()
	{
		AddrPort serverAddrPort;
		AddrInfo addrInfo;
		if (DnsForwardLookupAndGetPrimaryAddress(LogServerName, (uint16_t)LogServerPort, addrInfo) != SocketErrorCode_Ok)
		{
			return false;
		}

		serverAddrPort.FromNative(addrInfo.m_sockAddr);

		SocketErrorCode r = m_logSocket->Connect(serverAddrPort);
		if (r != SocketErrorCode_Ok && !CFastSocket::IsWouldBlockError(r))
		{
			return false;
		}

		return true;
	}

	void CErrorReporter_Indeed::Heartbeat_LogSocket_SendingCase()
	{
		OverlappedResult overlappedResult;
		if (m_logSocket->GetSendOverlappedResult(false, overlappedResult) == false)
		{
			// 아직 대기중
			return;
		}

		AtomicCompareAndSwap32(1, 0, &m_sendIoFlag); // 결과: 0 or 2

		if (overlappedResult.m_errorCode != 0 || overlappedResult.m_completedDataLength == 0)
		{
			// TCP 연결이 끊어졌음을 의미한다. 따라서 에러 처리한다.
			m_logSocketState = LSS_Finished;
			m_logSocket = shared_ptr<CFastSocket>();
			return;
		}

		// 다음 송신을 건다.
		m_logSendProgress += overlappedResult.m_completedDataLength;
		if (m_logSendProgress >= m_logHttpTxt.GetLength() + 1)
		{
			// 완료!
			m_logSocketState = LSS_Finished;
			m_logSocket = shared_ptr<CFastSocket>();
		}
		else
			Heartbeat_LogSocket_IssueSend();

	}

	void CErrorReporter_Indeed::Heartbeat_LogSocket_IssueSend()
	{
		if(!AtomicCompareAndSwap32(0,1,&m_sendIoFlag))
		{
			return;
		}

		SocketErrorCode e = m_logSocket->IssueSend(
			(uint8_t*)(m_logHttpTxt.GetString() + m_logSendProgress),
			m_logHttpTxt.GetLength()+1-m_logSendProgress);

		if(e!=SocketErrorCode_Ok)
		{
			AtomicCompareAndSwap32(1,0,&m_sendIoFlag);
		}
	}

	CErrorReporter_Indeed::CErrorReporter_Indeed():m_worker(StaticWorkerProc,this)
	{
		m_sendIoFlag = 0;
		m_lastReportedTime = 0;
		m_logSocketState = LSS_Finished;
		m_stopThread = false;

		m_worker.Start();
	}

	// 항상 받아들이면 곤란.
	void CErrorReporter_Indeed::RequestReport( const String &text )
	{
		// CriticalSectionLock.Lock 자체가 error report를 유발할 수 있으므로, 무한호출을 막기 위해서
		if(!m_cs.IsValid())
			return;

		CriticalSectionLock lock(m_cs,true);

		if(m_text.IsEmpty() && m_logSocketState == LSS_Finished && GetTickCount() - m_lastReportedTime > 1000*30 )
		{
			m_text = text;
			m_logSocketState = LSS_Init;
			m_lastReportedTime = CFakeWin32::GetTickCount();

			String txt2 = _PNT("**ErrorReport** "); // text가 너무 길 수 있으므로
			txt2 += text;
			txt2 += _PNT("\n");

			CFakeWin32::OutputDebugString(txt2.GetString());
		}
	}

	void CErrorReporter_Indeed::IssueCloseAndWaitUntilNoIssueGuaranteed()
	{
		CriticalSectionLock lock(m_cs,true);

		m_stopThread = true;
		m_worker.Join();

		if(m_logSocket)
		{
			// 소켓 닫기
			m_logSocket->CloseSocketOnly();

			// io가 끝날때까지 대기. 금방 끝날거다.
			while(1)
			{
				OverlappedResult ov;
				if(AtomicCompareAndSwap32(0,2,&m_sendIoFlag) || m_logSocket->GetSendOverlappedResult(0,ov) || Thread::m_dllProcessDetached_INTERNAL)
				{
					break;
				}
				Sleep(10);
			}
		}
	}
#endif // _WIN32

	void CErrorReporter_Indeed::Report( String text )
	{
#ifdef _WIN32 // CErrorReporter_Indeed가 non-block을 지원하게 수정 후 이 제한을 풀자.
		if(CNetConfig::EnableErrorReportToNettention)
		{
			// 보내기 주기 제한해야 함
			CErrorReporter_Indeed::GetUnsafeRef().RequestReport(text);
		}
#else
		CFakeWin32::OutputDebugStringT(text.GetString());
#endif
	}


#ifdef _WIN32
	CErrorReporter_Indeed::~CErrorReporter_Indeed()
	{
		IssueCloseAndWaitUntilNoIssueGuaranteed();
	}

	void CErrorReporter_Indeed::WorkerProc()
	{
		SetThreadName(::GetCurrentThreadId(), "PnErrReporter");
		while(!m_stopThread)
		{
			Heartbeat_LogSocket();
			Sleep(5); // 어차피 대량으로 보낼건 아니므로
		}
	}

	void CErrorReporter_Indeed::StaticWorkerProc( void* ctx )
	{
		CErrorReporter_Indeed* r = (CErrorReporter_Indeed*)ctx;
		r->WorkerProc();
	}

	void CReportSocketDg::OnSocketWarning(CFastSocket* /*socket*/, String /*msg*/)
	{
	}
#endif // _WIN32
}
