#pragma once 

#include "../include/Enums.h"
#include "../include/Singleton.h"
#include "FastMap2.h"

namespace Proud 
{

#ifdef _DEBUG
#define PNTRACE CTracer::Instance().Trace
#else
#define PNTRACE __noop
#endif

	// 로그를 분류해서 표기하기 위한 목적
	// Enable()로 필요한 것을 켠다. 그리고 Trace()로 로그를 출력한다.
	class CTracer:public CSingleton<CTracer>
	{
		struct Context
		{};

		CFastMap2<LogCategory, Context, int> m_enabledCategories;

		CriticalSection m_critSec;
	public:
		CTracer(void);
		~CTracer(void);

		void Trace(LogCategory logCategory, const char* format, ...);
		void Enable(LogCategory logCategory);
	};
}