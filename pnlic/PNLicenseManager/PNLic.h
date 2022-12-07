// PN 서버 모듈에서 include되는 소스 파일이다.
// 따라서 이 모듈이 사용하는 타 모듈들은 AuthNet이 아닌, PN 최신 버전이다.

#pragma once 

#include "PNTypes.h"

#if defined(__linux__)
#include <unistd.h>
 #ifndef __ANDORID__
#include <wait.h>
#endif
#endif

#include <thread>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "../../core/src/enumimpl.h"	// RuntimePlatform enum
#include "../../core/include/CriticalSect.h"
#include "../../core/include/AddrPort.h"
#include "../../core/src/FastMap2.h"

#include "../../core/include/LambdaWrap.h"

using namespace Proud;

namespace Proud
{
	// 랜덤 클래스. PN에 있는 것을 복사해왔다. 안그러면 해커가 구 클래스에 브레이크 걸고 디버깅할 터이니.
	class CLicRandom
	{
	private:
		uint32_t m_dwState[625];
		uint32_t *m_pdwNext;
		long m_lLeft;

		uint32_t LoadMersenneTwister();
	public:
		CLicRandom();
		~CLicRandom() {}

		void SetSeed(uint32_t dwSeed);
		int GetInt();
		double NextDouble();
	};

	extern CLicRandom g_licRandom;

	// 디버거에 안보이게 하려고 의도적으로 #define 사용
#define BeforeHiddenAppExecution 0
#define HiddenAppRunning 1
#define HiddenAppExited 2
#define HiddenAppNotExecuted 3
//#define STILL_ACTIVE 103

    class CPNLicSingleton final
    {
    public:
        // AuthNet or ProudNet을 사용할 것이다. 이 모듈이 어디에서 include되건간에.
        // 컴파일이 잘 되려면 이 모듈을 #include 하기 전에 필요한 것을 미리 include 하라.
        CriticalSection m_critSec;

        // Hidden App State
        int m_HAS;

        // Hidden Read pipe
        //int m_readFD;

        // Deffered Hidden App Execution Time(ms)
        int64_t m_DHAET;

        // Executed Process Handle
        HANDLE m_EPA;

        // License Type. LicenseType enum의 OR 조합이다.
        uint32_t m_LT;

        CPNLicSingleton();

        ~CPNLicSingleton();

        void POSH(int64_t currTimeMs);

        // Hidden app을 실행->실행완료 대기->실행파일 삭제 과정을 하는 스레드 모음.
        // 중첩실행되는 경우를 위해 이렇게 되어있다.
        std::vector<std::thread> m_hiddenAppCleanTaskList;
    };

	// PN src에서 이 객체를 void*에 넣어서 다룬다. 따라서 디버거로 내용물이 보이지 않을 것이다.
	class CPNLic final
	{
	private:
		// AuthNet or ProudNet을 사용할 것이다. 이 모듈이 어디에서 include되건간에.
		// 컴파일이 잘 되려면 이 모듈을 #include 하기 전에 필요한 것을 미리 include 하라.
		CriticalSection m_critSec;

		// Platform to IPv4 to Count Map. 각 플랫폼별 접속 IP 분포. Count는 같은 외부IP에서 접속한 중복 갯수를 의미한다.
		CFastMap2<uint32_t, CFastMap2<uint32_t, int, int>*, int > m_PICMap;

		bool IsAllowedConcurrentUserNumbers(const int ccuNum);
		bool IsLicensedPlatform(RuntimePlatform clientPlatform, const AddrPort & clientAddr, const int ccuNum);
		void ReportConcurrentUserNumbersOverLogDatum(const int ccuNum);

		//////////////////////////////////////////////////////////////////////////
		// 아래 함수들은 모두 inline이다. 그래야 해커가 해킹이 더 어려워지니까.
		inline void AddPlatformCount(RuntimePlatform type, const AddrPort & addr);
		inline void RemovePlatformCount(RuntimePlatform type, const AddrPort & addr);
		
		inline int GetIPCountByPlatform(RuntimePlatform clientPlatform);

        // 동접 300 넘긴것이 보고된 적이 있는지 여부
        // 서버 프로세스 라이프타임 통틀어 딱 한번만 이루어지면 되기에 이런 플래그를 둔다.
        static bool m_isReported; // = false ( 초기화는 PNLic.inl 최하단에 있다. )
		
	public:
		CPNLic() {}; // 주의 : = default 이런 형태는 VS2012에서 사용할 수 없다.
		inline ~CPNLic();

		bool POCJ(const int ccuNum);
		bool POCJ(uint32_t clientPlatform0, const AddrPort &clientAddr, const int ccuNum);
		void POCL(uint32_t clientPlatform0, const AddrPort & clientAddr);

		inline void POSS();
		inline void POSH(int64_t currTimeMs);

		// Process On Server End
		void POSE()
		{
			CriticalSectionLock lock(m_critSec, true);

			// 플랫폼별 접속 통계표를 청소한다. 서버를 껐으니까 다시 켜더라도 방어적 코딩.
			for (CFastMap2<uint32_t, CFastMap2<uint32_t, int, int>*, int >::iterator i = m_PICMap.begin(); i != m_PICMap.end(); i++)
			{
				CFastMap2<uint32_t, int, int>* iVal = i.GetSecond();
				delete iVal;
			}
			m_PICMap.Clear();
		}
	};

    extern CPNLicSingleton g_PNLicSingleton;
}
