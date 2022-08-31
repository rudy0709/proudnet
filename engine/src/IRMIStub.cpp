/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "../include/IRMIStub.h"
#include "../include/sysutil.h"
#include "../include/MessageSummary.h"

namespace Proud
{
	void IRmiStub::AfterRmiInvocation(const AfterRmiSummary& /*summary*/)
	{
	}

	void IRmiStub::BeforeRmiInvocation(const BeforeRmiSummary& /*summary*/)
	{
	}

	void IRmiStub::NotifyCallFromStub(HostID /*remote*/, RmiID /*RmiID*/, String /*methodName*/, String /*parameters*/)
	{
	}

	bool IRmiStub::BeforeDeserialize(HostID /*remote*/, RmiContext& /*rmiContext*/, CMessage& /*message*/)
	{
		return true;
	}

	void IRmiStub::ShowUnknownHostIDWarning(HostID remoteHostID)
	{
		_pn_unused(remoteHostID);
		NTTNTRACE("경고: ProcessReceivedMessage에서 unknown HostID %d!\n", remoteHostID);
	}

	IRmiStub::IRmiStub()
	{
		m_core = NULL;
		m_enableNotifyCallFromStub = false;
		m_internalUse = false;
		m_enableStubProfiling = false;
	}

	IRmiStub::~IRmiStub()
	{
		if(m_core)
		{
			ShowUserMisuseError(_PNT("RMI Stub which is still in use by ProudNet core cannot be destroyed! Destroy CNetClient or CNetServer instance first."));
		}
	}

	const PNTCHAR* DecryptFailedError = _PNT("msg.Decrypt failed");
}
