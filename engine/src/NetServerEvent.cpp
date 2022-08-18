#include "stdafx.h"
#include "NetServer.h"

// 원칙적으로는 PN client dll에서 이것을 export해야 한다.
// 그러나 그래봤자 해커만 좋아한다. 
// 따라서 그냥 이렇게 중복 include한다.
#ifdef PROUDNETSERVER_EXPORTS
#include "pidl/NetS2C_common.cpp"
#include "pidl/NetC2S_common.cpp"
#include "pidl/NetC2C_common.cpp"
#endif


namespace Proud
{
/*
	// 발생하는 RMI callback이나 event callback이 현재 모두 실제 일어나지 않고 보류되어라~~~라는 상태인지? 그리고 현재 어떠한 콜백도 진행중이지 않은지?
	bool CNetServerImpl::IsEventCallbackPaused()
	{
		// 서버가 종료 상태이면 당연히...
		if (!m_netThreadPool)
			return true;
		
		throw Exception("Not implemented yet!");
		return false;
	}

	// 위 보류 상황을 켜고, 이벤트 콜백을 처리하는 스레드가 전혀 없음이 보장될 때까지 리턴 안 한다.
	void CNetServerImpl::PauseEventCallback()
	{
		throw Exception("Not implemented yet!");
	}

	// 위 함수의 반대.
	void CNetServerImpl::ResumeEventCallback()
	{
		// 서버가 종료 상태이면 당연히...
		if (!m_netThreadPool)
			return;

		throw Exception("Not implemented yet!");
	}*/
}
