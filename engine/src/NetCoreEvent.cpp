#include "stdafx.h"
#include <new>
#include "../include/INetCoreEvent.h"
#include "../include/RMIContext.h"
#include "NetCore.h"

namespace Proud
{
	INetCoreEvent::~INetCoreEvent()
	{
	}

	void INetCoreEvent::OnReceiveUserMessage(HostID /*sender*/, const RmiContext& /*rmiContext*/, uint8_t* /*payload*/, int /*payloadLength*/)
	{
	}

	bool CNetCoreImpl::RunAsyncInternal(HostID taskOwner, LambdaBase_Param0<void>* func)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		// actor를 찾는다. 없으면 false를 리턴한다.
		shared_ptr<CHostBase> actor = AuthedHostMap_Get(taskOwner);
		if (!actor)
			return false;

		// 새 항목
		CFinalUserWorkItem item;
		item.Internal().m_type = UWI_UserFunction;
		item.Internal().m_userFunction = RefCount<LambdaBase_Param0<void> >(func);
		item.Internal().m_unsafeMessage.m_remoteHostID = taskOwner;
		TryGetReferrerHeart(item.Internal().m_netCoreReferrerHeart);

		// 그것을 task subject에 넣는다.
		if (item.Internal().m_netCoreReferrerHeart)
			m_userTaskQueue.Push(actor, item);

		return true;
	}


	void CNetCoreImpl::UserWork_FinalReceiveUserFunction(CFinalUserWorkItem& UWI, const shared_ptr<CHostBase>& /*subject*/, CWorkResult*)
	{
		AssertIsNotLockedByCurrentThread();

		try
		{
			// 람다식을 실행한다.
			UWI.Internal().m_userFunction->Run();
		}
		catch (std::bad_alloc &e)
		{
			Exception ex(e);
			ex.m_remote = UWI.Internal().m_unsafeMessage.m_remoteHostID;
			ex.m_userCallbackName = _PNT("OnReceiveUserMessage");
			ex.m_delegateObject = GetEventSink_NOCSLOCK();
			BadAllocException(ex);
		}
		catch (Exception& e)
		{
			e.m_remote = UWI.Internal().m_unsafeMessage.m_remoteHostID;
			e.m_userCallbackName = _PNT("OnReceiveUserMessage");
			e.m_delegateObject = GetEventSink_NOCSLOCK();

			if (Get_HasCoreEventFunctionObjects().OnException.m_functionObject)
				Get_HasCoreEventFunctionObjects().OnException.m_functionObject->Run(e);

			if (GetEventSink_NOCSLOCK())
				GetEventSink_NOCSLOCK()->OnException(e);
		}
		catch (std::exception &e)
		{
			Exception ex(e);
			ex.m_remote = UWI.Internal().m_unsafeMessage.m_remoteHostID;
			ex.m_userCallbackName = _PNT("OnReceiveUserMessage");
			ex.m_delegateObject = GetEventSink_NOCSLOCK();

			if (Get_HasCoreEventFunctionObjects().OnException.m_functionObject)
				Get_HasCoreEventFunctionObjects().OnException.m_functionObject->Run(ex);

			if (GetEventSink_NOCSLOCK())
				GetEventSink_NOCSLOCK()->OnException(ex);
		}
#ifdef _COMDEF_NOT_WINAPI_FAMILY_DESKTOP_APP
		catch (_com_error &e)
		{
			Exception ext; Exception_UpdateFromComError(ext, e);
			ext.m_remote = UWI.Internal().m_unsafeMessage.m_remoteHostID;
			ext.m_userCallbackName = _PNT("OnReceiveUserMessage");
			ext.m_delegateObject = GetEventSink_NOCSLOCK();

			if (Get_HasCoreEventFunctionObjects().OnException.m_functionObject)
				Get_HasCoreEventFunctionObjects().OnException.m_functionObject->Run(ext);

			if (GetEventSink_NOCSLOCK())
				GetEventSink_NOCSLOCK()->OnException(ext);
		}
#endif // _COMDEF_NOT_WINAPI_FAMILY_DESKTOP_APP
#ifdef ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
		catch (...)
		{
			Exception ex;
			ex.m_exceptionType = ExceptionType_Unhandled;
			ex.m_remote = UWI.m_unsafeMessage.m_remoteHostID;
			ex.m_userCallbackName = _PNT("OnReceiveUserMessage");
			ex.m_delegateObject = GetEventSink_NOCSLOCK();

			if (Get_HasCoreEventFunctionObjects().OnException.m_functionObject)
				Get_HasCoreEventFunctionObjects().OnException.m_functionObject->Run(ex);

			if (GetEventSink_NOCSLOCK())
				m_owner->GetEventSink_NOCSLOCK()->OnException(ex);
		}
#endif // ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
	}
}
