// 성능에 민감한 함수를 inline화.

#pragma once

namespace Proud
{
	// hostID to hostid ptr 변환을 한다.
	// 못 찾으면 sendFailedRemotes에 채운다.
	inline void CNetServerImpl::HostIDArrayToHostPtrArray_NOLOCK(SendDestInfoArray& dest, HostIDArray& src, const SendOpt& sendOpt)
	{
		AssertIsLockedByCurrentThread();

		// shared_ptr의 복사 비용이 커서, 일단 한번 청소 후
		// 클리어한 상태로 하나 준비한다.
		dest.SetCount(0);
		dest.SetCount(src.GetCount());

		// 속도 최적화
		HostID* pSrc = src.GetData();
		SendDestInfo* pDest = dest.GetData();
		int srcCount = src.GetCount();

		// FastArray.Add의 부하가 커서, 직접 배열에 채우는걸로 바뀜.
		int c = 0;

		for (int i = 0; i < srcCount; i++)
		{
			SendDestInfo& a = pDest[c]; // 이미 위에서 배열 크기를 한번 0으로 바꾼 바 있으므로, a는 clean하다.

			// shared_ptr 복사가 비싸므로, 바로 배열 항목에 채워 버린다.
			if(TryGetSendDestByHostID_NOLOCK(pSrc[i], a.mObject)) // garbaged host는 제외된다.
			{
				// 성공적으로 가져왔다. 나머지를 채우자.
				a.mHostID = pSrc[i];
				c++;
			}
			else
			{
				// 아무것도 안한다. 굳이 a.mObject=null을 할 필요는 없다. c++; 을 안하니까.
				FillSendFailListOnNeed(sendOpt, &pSrc[i], 1, ErrorType_InvalidHostID);
			}
		}
		dest.SetCount(c); // 크기 재조정해서 마무리
	}

	// HostBase 객체를 얻는다. garbaged인 것은 제외된다.
	inline shared_ptr<CHostBase> CNetServerImpl::GetSendDestByHostID_NOLOCK(HostID peerHostID)
	{
		AssertIsLockedByCurrentThread();

		shared_ptr<CHostBase> ret;
		if (TryGetSendDestByHostID_NOLOCK(peerHostID, ret))
			return ret;
		else
			return shared_ptr<CHostBase>();
	}

	// 성능 때문에 추가된 함수. 나중에 위 함수와 교체해 버리자.
	// code profile 후 추가됨.
	inline bool CNetServerImpl::TryGetSendDestByHostID_NOLOCK(HostID peerHostID, shared_ptr<CHostBase>& output)
	{
		AssertIsLockedByCurrentThread();
		//CriticalSectionLock clk(GetCriticalSection(), true);
		if (peerHostID == HostID_Server)
		{
			if (m_loopbackHost)
			{
				output = m_loopbackHost;
				return true;
			}
			return false;
		}
		if (peerHostID == HostID_None)
		{
			return false;
		}

		shared_ptr<CHostBase> temp;
		if (!AuthedHostMap_TryGet(peerHostID, temp))
			return false;

		// 찾았지만 폐기예정이면 즐.
		if (temp->m_disposeWaiter)
			return false;

		output = MOVE_OR_COPY(temp);
		return true;
	}

	// remote 객체를 얻되, garbaged 상태가 아닌 경우에만 얻는다.
	inline shared_ptr<CRemoteClient_S> CNetServerImpl::GetAuthedClientByHostID_NOLOCK(HostID clientHostID)
	{
		AssertIsLockedByCurrentThread();
		//CriticalSectionLock clk(GetCriticalSection(), true);

		if (clientHostID == HostID_None)
			return shared_ptr<CRemoteClient_S>();

		if (clientHostID == HostID_Server)
			return shared_ptr<CRemoteClient_S>();

		shared_ptr<CRemoteClient_S> rc = LeanDynamicCast_RemoteClient_S(AuthedHostMap_Get(clientHostID));
		//		m_authedHosts.TryGetValue(clientHostID, rc);

		if (rc && rc->m_disposeWaiter == nullptr)
			return rc;

		return shared_ptr<CRemoteClient_S>();
	}

// remote client들만 찾는다. loopback은 안 찾는다.
	inline bool CNetServerImpl::TryGetAuthedClientByHostID(HostID clientHostID, shared_ptr<CRemoteClient_S>& output)
	{
		AssertIsLockedByCurrentThread();
		//CriticalSectionLock clk(GetCriticalSection(), true);

		if (clientHostID == HostID_None)
		{
			return false;
		}

		if (clientHostID == HostID_Server)
		{
			return false;
		}

		shared_ptr<CHostBase> output0;
		if (!AuthedHostMap_TryGet(clientHostID, output0))
			return false;

		output = LeanDynamicCast_RemoteClient_S(output0);
		if (!output)
			return false;

		if(output->m_disposeWaiter)
		{
			// 위에서 세팅된거를 리셋
			output.reset();

			return false;
		}

		// 성공
		return true;
	}
}
