#pragma once

#include "../include/FastArray.h"
#include "../include/ThreadUtil.h"
#include "../include/FakeClrBase.h"
#include "../include/ReaderWriterMonitor.h"

namespace Proud
{
	class CNetServerImpl;

#if defined (_WIN32)
	// $$$; == > 본 작업이 완료되면 반드시 ifdef를 제거해주시기 바랍니다.
	// dev003에서 흘러온 것임. 이승철님 선에서 해결되어야.
	class CReaderWriterMonitorTest
	{
		static const int WorkerCount = 10;
		CFastArray<ThreadPtr> m_workers;
		CReaderWriterMonitor_NORECURSE m_cs;
		static const int ArrayLength = 1000;
		int m_array[ArrayLength];

		volatile bool m_stopThreads;
		volatile bool m_multipleReaderAccessed;

		static void StaticWorkerProc(void *ctx);
		void WorkerProc();

		enum TestPlan
		{
			TP_Read,
			TP_Write,
		};
	public:
		CNetServerImpl* m_main;

		CReaderWriterMonitorTest();
		void RunTest();
	};
#endif // _WIN32
}
