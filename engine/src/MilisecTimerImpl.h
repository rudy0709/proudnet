/*
ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.

ProudNet

This program is soley copyrighted by Nettention.
Any use, correction, and distribution of this program are subject to the terms and conditions of the License Agreement.
Any violated use of this program is prohibited and will be cause of immediate termination according to the License Agreement.

** WARNING : PLEASE DO NOT REMOVE THE LEGAL NOTICE ABOVE.

*/

#pragma once

#include "../include/MilisecTimer.h"

namespace Proud
{
#if (_MSC_VER>=1400)
// 아래 주석처리된 pragma managed 전처리 구문은 C++/CLI 버전이 있었을 때에나 필요했던 것입니다.
// 현재는 필요없는 구문이고, 일부 환경에서 C3295 "#pragma managed는 전역 또는 네임스페이스 범위에서만 사용할 수 있습니다."라는 빌드에러를 일으킵니다.
//#pragma managed(push,off)
#endif

	/* 리눅스 포팅시: 이건 없애버려도 된다. 대신 CMMMilisecTimer를 쓰자. */
	class CMilisecTimerImpl:public CMilisecTimer
	{
	public:
		CMilisecTimerImpl();

		virtual ~CMilisecTimerImpl(){}

		// 타이머를 초기화한다.
		void Reset();
		// 타이머를 시작한다.
		void Start();
		// 타이머를 일시 정지한다.
		void Stop();
		// 타이머를 0.1초만큼 진행되게 한다.
		void Advance();

		//퇴역
		//double GetAbsoluteTime();

		int64_t GetTimeMs();

		int64_t GetElapsedTimeMs();

		bool IsStopped();

	protected:
		bool m_bTimerStopped;

		int64_t m_llStopTime;
		int64_t m_llLastElapsedTime;
		int64_t m_llBaseTime;
	};

	/**  @} */
#if (_MSC_VER>=1400)
//#pragma managed(pop)
#endif
}
