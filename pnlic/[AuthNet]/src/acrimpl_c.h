#pragma once

#include "../include/Ptr.h"
#include "../include/FastArray.h"
#include "SuperSocket.h"

namespace Proud
{
	// 연결 재접속 단계를 관리한다.
	// ACR 진행중일 때 이 객체가 생성되고 ACR 이 끝나면 사라진다.
	class CAutoConnectionRecoveryContext
	{
	public:
		// 네트워크 망이 전환됬는지를 검사하는 단계
		// true 시에는 ACR 시작 후, 망 전환이 이루어졌는지 체크
		// false 시에는 실제 재접속을 시작
		bool m_isTransitionNetworkDetectStep;

		/* ACR 연결이 진행중인 async io가 진행중인 소켓.
		iOS, Android는 1개 NIC만 잡힌다. 따라서 여러개가 불필요. */
		CSuperSocketPtr m_tcpSocket;

		// 소유주
		CNetClientImpl* m_owner;

		// ACR이 시작하는 시간. ACR 실패 한계 대기 시간을 위함.
		// 현재 시간보다 더 미래가 세팅되는 경우가 있다.
		// ACR 과정을 테스트하려면, TCP 연결이 끊어져도 일정 시간 일부러 냅뒀다가 시행해야 하니까.
		int64_t m_startTime;

		// 최초의 시간. m_startTime은 재접속을 반복할 때마다 갱신되나, 이건 아님.
		int64_t m_firstStartTime;

		// 재접속이 실패한 횟수. 이 횟수가 너무 많으면 총 제한 시간 이내더라도 최종 실패 처리를 한다.
		int m_failCount;

		CAutoConnectionRecoveryContext();
		~CAutoConnectionRecoveryContext();
	};

	typedef RefCount<CAutoConnectionRecoveryContext> CAutoConnectionRecoveryContextPtr;
}