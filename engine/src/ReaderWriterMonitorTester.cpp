/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "ReaderWriterMonitorTester.h"
#include "NetServer.h"
#include "sysutil-private.h"

namespace Proud 
{
#if defined (_WIN32)
	// $$$; == > 본 작업이 완료되면 반드시 ifdef를 제거해주시기 바랍니다.0
	// dev003에서 흘러온 것임. 이승철님 선에서 해결되어야. 
	void CReaderWriterMonitorTest::RunTest()
	{
		// 난잡 접근 스레드를 여럿 생성
		for(int i=0;i<WorkerCount;i++)
		{
			m_workers.Add(ThreadPtr(new Thread(StaticWorkerProc,this)));
		}
		
		for(int i=0;i<WorkerCount;i++)
		{
			m_workers[i]->Start();
		}

		// 대기
		Sleep(1000);
		while(!m_multipleReaderAccessed)
		{
			Sleep(50);
		}

		// 종료
		m_stopThreads = true;
		for(int i=0;i<WorkerCount;i++)
		{
			m_workers[i]->Join();
		}

		// 최종결과 체크
		for(int i=0;i<ArrayLength-1;i++)
		{
			if(m_array[0] == 0 || m_array[i] != m_array[i+1])
				m_main->EnqueueUnitTestFailEvent(_PNT("Reader Writer Lock Did Not Properly Conduct the Lock Policy!"));
		}
	}

	CReaderWriterMonitorTest::CReaderWriterMonitorTest()
	{
		m_multipleReaderAccessed = false;
		m_stopThreads = false;

		// ikpil.choi 2016-11-07 : memset_s 로 변경
		//ZeroMemory(m_array, sizeof(m_array));
		memset_s(m_array, sizeof(m_array), 0, sizeof(m_array));
	}

	void CReaderWriterMonitorTest::StaticWorkerProc( void *ctx )
	{
		CReaderWriterMonitorTest* m = (CReaderWriterMonitorTest*)ctx;
		m->WorkerProc();
	}

	void CReaderWriterMonitorTest::WorkerProc()
	{
		Random tlsRandom;
		while(!m_stopThreads)
		{
			// 시행할 행동을 결정한다
			// 재귀를 지금은 불허하므로 생략
			double ratio = tlsRandom.NextDouble();

			TestPlan plan = TP_Read;
			if(ratio>0.9)  // 10%의 확률로만 writer lock을 하게 한다. 그래야 다중 reader lock 확률이 생기므로! 
			{
				plan = TP_Write;
			}

			if(plan == TP_Write)
			{
				CWriterLock_NORECURSE lock(m_cs,true);

				// 배열을 접근
				int val = m_array[0];
				for(int i=0;i<ArrayLength;i++)
				{
					m_array[i]++;
					if(val+1 != m_array[i])
					{
						m_main->EnqueueUnitTestFailEvent(_PNT("During the Approach of Writer Lock, the Writer Lock is in Use in Other Places. "));
						m_stopThreads = true;
						return;
					}
				}				
			}
			else
			{
				CReaderLock_NORECURSE lock(m_cs,true);
				// 배열을 접근
				int val = m_array[0];
				for(int i=0;i<ArrayLength;i++)
				{
					 if(val != m_array[i])
					 {
						 m_main->EnqueueUnitTestFailEvent(_PNT("During the Approach of Reader lock, the Writer Lock is in Use in Other Places. "));
						 m_stopThreads = true;
						 return;
					 }
				}

				if(lock.GetMultipleReaderCount() > 1)
					m_multipleReaderAccessed = true;
			}
			YieldThread();
		}
	}
#endif // _WIN32
}