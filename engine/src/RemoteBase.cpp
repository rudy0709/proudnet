#include "stdafx.h"

#include <stack>

#include "RemoteBase.h"
#include "CriticalSectImpl.h"
#include "NetUtil.h"
#include "NetCore.h"

namespace Proud
{

	CHostBase::CHostBase()
		: m_overloadPacketState(false)
		, m_overloadPacketTime(0)
		, m_HostID(HostID_None)
		, m_hostTag(NULL)
		, m_runtimePlatform(RuntimePlatform_Last)
	{
		m_purgeRequested = false;
		m_UserTaskQueueUseOnly_WorkState = NotInWorkList;
		m_garbaged = false;

		m_recentPingMs = 0;

		m_coalesceIntervalMs = CNetConfig::EveryRemoteIssueSendOnNeedIntervalMs;
		m_autoCoalesceInterval = true;

		m_rttTotalTimeMs = 0;
		m_rttTestCount = 0;
		m_rttIntervalMs = 300;
		m_rttLastPingMs = 0;
		m_rttEndTimeMs = 0;
		m_rttStartTimeMs = 0;
		m_isTestingRtt = false;
	}

	bool CHostBase::CheckMessageOverloadAmount()
	{
		if(GetFinalUserWotkItemCount_STALE() >= CNetConfig::MessageOverloadWarningLimit)
		{
			return true;
		}
		return false;
	}

	// host가 user work item을 하나라도 갖고 있거나 설령 없더라도 item이 worker thread에서 실행중에 있으면 true를 리턴한다.
	bool CUserTaskQueue::DoesHostHaveUserWorkItem(CHostBase* host)
	{
		// delete is safe 를 검사하는 곳에서 호출되는 함수이다.
		// 따라서 main lock이 이미 되어있어야 한다.
		AssertIsLockedByCurrentThread(host->GetOwnerCriticalSection());

		// 아래 변수들은 UserTaskQueue의 lock에서 액세스된다.
		// 여기도 예외가 아니다.
		// 이게 없어서 ndn 이슈 4179.
		SpinLock lock(m_critSec, true);
		return host->m_UserTaskQueueUseOnly_WorkState != CHostBase::NotInWorkList
			|| host->m_UserTaskQueueUseOnly_finalUserWorkItemList.GetCount() > 0;
	}


	bool CHostBase::MessageOverloadChecking(int64_t currentTime)
	{
		//LockMain_AssertIsLockedByCurrentThread();

		if (!m_overloadPacketState)
		{
			if (CheckMessageOverloadAmount())
			{
				m_overloadPacketState = true;
				m_overloadPacketTime = currentTime;
			}
			return false;
		}
		// 전 프레임에서 packet이 많을 때
		if (CheckMessageOverloadAmount())
		{
			// Packet이 일정 시간 이상 Overload 상태인지 확인
			if ((currentTime - m_overloadPacketTime)
				>= CNetConfig::MessageOverloadWarningLimitTimeMs)
			{
				m_overloadPacketTime = currentTime;
				return true;
			}
			return false;
		}
		m_overloadPacketState = false;
		return false;
	}

	CHostBase::~CHostBase()
	{
		ClearFinalUserWorkItemList();
	}

	// auto가 켜져있으면 인자를 무시하고 RTT 근거로 설정된다.
	// 꺼져있으면 인자를 수용한다.
	void CHostBase::SetManualOrAutoCoalesceInterval(int interval)
	{
		m_coalesceIntervalMs = NetUtil::SetManualOrAutoCoalesceInterval(m_autoCoalesceInterval, m_recentPingMs, interval);
	}
}
