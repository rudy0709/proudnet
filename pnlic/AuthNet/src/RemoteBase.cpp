#include "stdafx.h"

#include <stack>

#include "RemoteBase.h"
#include "CriticalSectImpl.h"
#include "NetUtil.h"


namespace Proud
{
	// true이면 CUserTaskQueue에 아직 등록되어 있음을 의미
	bool CHostBase::IsInUserTaskQueue()
	{
		return m_taskSubjectNode.GetListOwner() != NULL;
	}

	CHostBase::CHostBase() 
		: m_taskSubjectNode(this), m_HostID(HostID_None), m_hostTag(NULL), m_overloadPacketState(false), m_overloadPacketTime(0)

/*//3003!!!
		, m_MessageAnalyzeLog(false)*/
	{
		m_purgeRequested = false;
		m_userTaskRunning = false;
		m_garbaged = false;

		m_recentPingMs = 0;

		m_coalesceIntervalMs = CNetConfig::ClientHeartbeatIntervalMs;
		m_autoCoalesceInterval = true;
		
		m_useCount = 0;
	}

	bool CHostBase::IsFinalReceiveQueueEmpty()
	{
		return m_finalUserWorkItemList.GetCount() == 0;
	}


	bool CHostBase::CheckMessageOverloadAmount()
	{
		if(m_finalUserWorkItemList.GetCount() >= CNetConfig::MessageOverloadWarningLimit)
		{
			return true;
		}
		return false;
	}



	// 이 host가 가진 final user work item 중 가장 오래된 것을 꺼낸다.
	bool CHostBase::PopFirstUserWorkItem(CFinalUserWorkItem &output)
	{
		//AssertIsLockedByCurrentThread();

		if (m_finalUserWorkItemList.GetCount() == 0)
			return false;

		m_finalUserWorkItemList.GetHead(&output);
		m_finalUserWorkItemList.RemoveHeadNoReturn();
		output.m_unsafeMessage.m_remoteHostID = GetHostID();

		return true;
	}

	bool CHostBase::IsTaskRunning()
	{
		return m_userTaskRunning;
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