#pragma once 

#include "../include/Enums.h"

namespace Proud
{
	// Fallback 함수에 쓰이는 파라메터.
	struct FallbackParam
	{
		// 왜 릴레이로 전환 하는지
		ErrorType m_reason;
		// 릴레이로 전환했음을 서버에 노티할건지
		bool m_notifyToServer;
		// 릴레이 전환 횟수를 리셋할 것인지? 
		// (ACR 재접속 시에는 0으로 세팅해주어야, 홀펀칭을 시작부터 안하는 사태가 안 생김)
		bool m_resetFallbackCount;

		FallbackParam()
			: m_reason(ErrorType_Unexpected)
			, m_notifyToServer(true)
			, m_resetFallbackCount(false)
		{}
	};
}