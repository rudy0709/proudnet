#pragma once

#include "../include/Enums.h"
#include "../include/Singleton.h"
#include "FastMap2.h"

namespace Proud
{

#ifdef _DEBUG
#define PNTRACE Trace
#else
#define PNTRACE __noop
#endif

	// 로그를 분류해서 표기하기 위한 목적
	// Enable()로 필요한 것을 켠다. 그리고 Trace()로 로그를 출력한다.
	class CTracer:public CSingleton<CTracer>
	{
		struct Context
		{};

		// 로그를 켬이 허용된 category들.
		CFastMap2<LogCategory, Context, int> m_enabledCategories;

		// 멀티스레드 보호
		CriticalSection m_critSec;
	public:
		CTracer(void);
		~CTracer(void);

		void Trace(LogCategory logCategory, const char* format, ...);
		void Trace_(LogCategory logCategory, const char* text);
		void Enable(LogCategory logCategory);
	};

	void Trace(LogCategory logCategory, const char* format, ...);
}
