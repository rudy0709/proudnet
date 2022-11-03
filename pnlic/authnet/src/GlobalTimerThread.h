#pragma once

#include "../include/SysTime.h"
#include "../include/Singleton.h"
#include "FastMap2.h"
#include "../include/PnThread.h"
#include "../include/TimeUtil.h"



namespace Proud
{
	typedef void(*TaskFunction)(void* context);

	/* 멀티플랫폼 정밀 시간 얻기 기능과 1ms마다의 타이머 콜백에 사용된다.
	timeGetTime,timeSetEvent,멀티플랫폼 시간 얻기 기능을 대체한다.
	현재 시간을 얻는 기능이나, heartbeat에서 사용된다

	배경
	- timeGetTime,timeSetEvent는 win32전용이라 멀티플랫폼에서 곤란.
	- timeGetTime,timeSetEvent는 내부적으로 per-process thread를 하나 암묵적으로 만들어 거기서 실행된다. 이를 따라 만들면 된다.
	- timeGetTime의 정밀도는 5ms인데, 1ms heartbeat에서 시간을 얻기에는 정밀도 불충분. 
	  정밀도를 높이기 위해 timeBeginPeriod를 쓰더라도 결국 이 클래스처럼 스레드 하나 만들어 콜백되는 구조로 귀결됨.
	- QPC/QPF가 정밀도, 호출 시간은 최고지만, CPU0에서만 호출해야 함(BIOS 버그때문)
	- 1ms 정밀도의 timeSetEvent가 heartbeat 등에서 사용되지만, 안타깝게도 비 win32에서는 그러한 것이 없기 때문에 자체 개발해야 함

	이렇게 작동
	- win32: Sleep(1)이 걸려 있는 스레드에서 QPC 호출로 현재시간 구하고 등록된 함수들을 콜백한다.
	- ios: 위와 동일. 단, usleep, gettimeofday가 쓰임
	- linux: 위와 동일. 단, usleep, clock_gettime이 쓰임 
	- marmalade: 위와 동일. 단, s3eTimerGetMs가 쓰임.

	스레드에 대해
	- CPU 0 affinity를 가진다. QPC가 일부 머신에서 affinity가 없으면 오류를 내기 때문이다.
	  이 오류는 BIOS관련이기 때문에 OS 공통일 것이라 예상된다. 따라서 linux,ios도 마찬가지로 CPU 0 affinity를 가져야 하지 않을까? (sched_setaffinity)
	  아직은 확신하지 말자. Galaxy S4의 경우 서로 다른 클럭수를 가진 CPU들이 들어있기 때문에 무슨 일이 생길지 모른다.

	TODO: 스마트폰에서 배터리 사용량을 줄이기 위해, FrameMove나 RMI 송신이 매우 뜸한 경우, 이 스레드의 처리 주기를 일시적으로 증가시키는 기능도 들어가야 한다.
	*/
	class CGlobalTimerThread:public CSingleton<CGlobalTimerThread>
	{
		class CTask 
		{
		public:
			int64_t m_timeToDo; // 등록된 task를 실행할 미래 시간
			int64_t m_interval; // 등록된 task를 실행하는 주기
			TaskFunction m_callback; // 실행할 task. 최대한 빨리 리턴해야 한다.
			void* m_callbackContext; // m_callback 실행시 들어가는 인자
		};
		CriticalSection m_critSec; // 아래 멤버들을 보호

		// 여기에 등록된 사용자 콜백들
		CFastMap2<CTask*, CTask*, int> m_tasks;

		// CPU 0 affinity.
		// m_tasks들을 뒤지면서 수행할 것들을 찾아 처리한다.
		Thread m_thread; 
		volatile bool m_stopThread;

		// 아래 시간값을 보호. m_critSec과 분리되어 있어야 task가 실행되는 '긴 시간'으로 인한 contention을 예방한다.
		CriticalSection m_timeCritSec;

		// 		// 측정된 시간. 이 값은 GetPreciseCurrentTimeMs()에 의해 사용된다.
		// 		// 코트 프로필링 결과 꽤 잦게 사용된다.
		// 		// 따라서, x64에서는 더 나은 성능을 위해 GetPreciseCurrentTime은 critsec을 쓰지 않고 atomic op을 쓴다. 
		// 		// x86에서 64bit atomic op 함수가 쓸 수 없더라.
		// #if defined(_WIN64) || defined(__LP64__)
		// 		volatile int64_t m_measuredTimeMs;
		// #else
		int64_t m_measuredTimeMs;
		//#endif

#if !defined(_WIN32)
	private:
		// unix에서 사용하는 정밀 시간값. 마이크로초 단위로 나온다.
		// pthread conditional variable의 timed wait 등에서 사용하기 때문에 이 값을 뽑아낸다.
		timeval m_absoluteTimeVal;
		timespec m_absoluteTimeSpec;
	public:
		void GetAbsoluteTimeSpec(timespec* ts);
#endif

		// 이것이 존재하는 이유: thread가 시작도 하기 전에 사용자가 시간을 얻으려고 하는 경우 0이 리턴되어야 하며, 
		// 직후 얻는 시간값은 0을 기준으로 시작해야 하기 때문이다.
		// 게다가 ios에서는 아예 음수로 시작하기도 하므로 이렇게 한다. 이것보다는 위의 이유가 더 중요.
		int64_t m_baseTimeMs;

		static void ThreadProc(void* context);
		void ThreadProc2();
		int64_t UpdateCachedTime();

	public:
		CGlobalTimerThread(void);
		~CGlobalTimerThread(void);

		TimerEventHandle TimerMiniTask_Add(const int64_t &interval, TaskFunction callback, void* callbackContext);
		void TimerMiniTask_Remove(TimerEventHandle event);
		void TimerMiniTask_SetInterval(TimerEventHandle event, const int64_t &interval);

		int64_t GetCachedTime();
	};
}
