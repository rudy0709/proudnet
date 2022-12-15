/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "../include/IRmiHost.h"
#include "../include/ClassBehavior.h"

namespace Proud
{
	

	CDisconnectArgs::CDisconnectArgs()
	{
		m_gracefulDisconnectTimeoutMs = CNetConfig::DefaultGracefulDisconnectTimeoutMs;
		m_disconnectSleepIntervalMs = CNetConfig::HeartbeatIntervalMs;
	}

	void IRmiHost::AttachProxy(IRmiProxy *proxy, ErrorInfoPtr& outError)
	{
		try {
			outError = ErrorInfoPtr();
			AttachProxy(proxy);
		}
		NOTHROW_FUNCTION_CATCH_ALL
	}

	void IRmiHost::AttachStub(IRmiStub* stub, ErrorInfoPtr& outError)
	{
		try {
			outError = ErrorInfoPtr();
			AttachStub(stub);
		}
		NOTHROW_FUNCTION_CATCH_ALL
	}

	void IRmiHost::DetachProxy(IRmiProxy* proxy, ErrorInfoPtr& outError)
	{
		try {
			outError = ErrorInfoPtr();
			DetachProxy(proxy);
		}
		NOTHROW_FUNCTION_CATCH_ALL
	}

	void IRmiHost::DetachStub(IRmiStub* stub, ErrorInfoPtr& outError)
	{
		try {
			outError = ErrorInfoPtr();
			DetachStub(stub);
		}
		NOTHROW_FUNCTION_CATCH_ALL
	}

}