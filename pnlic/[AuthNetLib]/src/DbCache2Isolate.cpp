#if defined(_WIN32)

#include "stdafx.h"
#include "../include/variant-marshaler.h"
#include "DbCacheClient2Impl.h"
#include "RmiContextImpl.h"
#include "DbCacheServer2Impl.h"

// Isolate를 통해 Cache중인 data를 직접 DB가 안전 억세스하는 기능.pptx
namespace Proud
{
	bool CDbCacheClient2Impl::RequestIsolateData(Guid rootUUID, String rootTableName, Guid &outSessionGuid)
	{
		outSessionGuid = Win32Guid::NewGuid();
		rootTableName.MakeUpper();

		// 서버에 요청한다.
		m_c2sProxy.RequestIsolateData(HostID_Server, g_ReliableSendForPN, rootUUID, rootTableName, outSessionGuid);
		return true;
	}

	bool CDbCacheClient2Impl::RequestDeisolateData(Guid rootUUID, String filterText, Guid &outSessionGuid)
	{
		outSessionGuid = Win32Guid::NewGuid();
		filterText.MakeUpper();

		// 서버에 요청한다.
		m_c2sProxy.RequestDeisolateData(HostID_Server, g_ReliableSendForPN, rootUUID, filterText, outSessionGuid);
		return true;
	}
	
	DEFRMI_DB2S2C_NotifyDeisolateDataSuccess(CDbCacheClient2Impl)
	{
		CPropNodePtr data = CPropNodePtr(new CPropNode(_PNT("")));
		data->FromByteArray(dataBlock);

		IDbCacheClientDelegate2::CCallbackArgs args;
		args.m_items.SetCount(1);

		args.m_items[0].m_rootUUID = rootUUID;
		args.m_items[0].m_data = data;
		args.m_items[0].m_sessionGuid = sessionGuid;
		m_delegate->OnDeisolateDataSuccess(args);
		return true;
	}

	DEFRMI_DB2S2C_NotifyDeisolateDataFailed(CDbCacheClient2Impl)
	{
		IDbCacheClientDelegate2::CCallbackArgs args;
		args.m_items.SetCount(1);

		args.m_items[0].m_sessionGuid = sessionGuid;
		args.m_items[0].m_reason = reason;
		args.m_items[0].m_comment = comment;
		m_delegate->OnDeisolateDataFailed(args);
		return true;
	}

	DEFRMI_DB2S2C_NotifyIsolateDataSuccess(CDbCacheClient2Impl)
	{
		CLoadedData2Ptr data = CLoadedData2Ptr(new CLoadedData2);
		data->FromByteArray(dataBlock);

		IDbCacheClientDelegate2::CCallbackArgs args;
		args.m_items.SetCount(1);

		args.m_items[0].m_rootUUID = rootUUID;
		args.m_items[0].m_loadedData = data;
		args.m_items[0].m_sessionGuid = sessionGuid;
		m_delegate->OnIsolateDataSuccess(args);
	
		return true;
	}

	DEFRMI_DB2S2C_NotifyIsolateDataFailed(CDbCacheClient2Impl)
	{
		IDbCacheClientDelegate2::CCallbackArgs args;
		args.m_items.SetCount(1);

		args.m_items[0].m_sessionGuid = sessionGuid;
		args.m_items[0].m_reason = reason;
		args.m_items[0].m_comment = comment;
		m_delegate->OnIsolateDataFailed(args);
		return true;
	}

	DEFRMI_DB2C2S_RequestIsolateData(CDbCacheServer2Impl)
	{
		CriticalSectionLock lock(m_cs, true);

		CDbRemoteClientPtr_S rc = GetAuthedRemoteClientByHostID(remote);
		if (!rc)
		{
			return true;
		}

		// root table name이 유효한지 검사
		if (!IsValidRootTable(rootTableName))
		{
			m_s2cProxy.NotifyIsolateDataFailed(
				remote, 
				g_ReliableSendForPN, 
				sessionGuid, 
				ErrorType_PermissionDenied, 
				_PNT("invalid table name")
				);
			return true;
		}

		// 이미 격리 처리된 데이터인지 확인
		if (CheckIsolateDataList(rootUUID))
		{
			m_s2cProxy.NotifyIsolateDataFailed(
				remote, 
				g_ReliableSendForPN,
				sessionGuid,
				ErrorType_AlreadyExists,
				_PNT("already request of another client")
				);
			return true;
		}

		// isolate 요청한 데이터를 이미 로드한 유저가 있다면 unload 시킨다. 
		CLoadedData2Ptr_S loadData = GetLoadedDataByRootGuid(rootUUID);
		if (loadData != NULL && loadData->GetLoaderClientHostID() != HostID_None)
		{
			// 사용자가 isolate를 하는 경우 그걸 갖고 있던 클라는 얄짤 없이 소유권을 잃는다. 이를 알림.
			m_s2cProxy.NotifyDataForceUnloaded(remote, g_ReliableSendForPN, sessionGuid, rootUUID);

			UnloadLoadeeByRootUUID(loadData->GetLoaderClientHostID(), rootUUID, true);
			// NOTE: isolated LD list에 추가되면 다른 루틴에서 그 리스트를 뒤져
			// 즉시 DB에 pend write가 모두 실행되게 하고 있다.
		}

		// isolateDataList 에 추가
		CIsolateRequestPtr requestContext = CIsolateRequestPtr(new CIsolateRequest);
		requestContext->m_requester = remote;
		requestContext->m_session = sessionGuid;
		m_isolatedDataList.Add(rootUUID, requestContext);

		return true;
	}
	// RequestIsolateData의 인자가 root UUID가 아니라 filter text인 경우도 넣어야 할텐데.	
	DEFRMI_DB2C2S_RequestDeisolateData(CDbCacheServer2Impl)
	{
		CriticalSectionLock lock(m_cs, true);

		if (!CheckIsolateDataList(rootUUID))
		{
			// isolate Data List 에 없는데 deisolate 를 하는 것은 문제
			m_s2cProxy.NotifyDeisolateDataFailed(
				remote, 
				g_ReliableSendForPN, 
				sessionGuid, 
				ErrorType_ValueNotExist, 
				_PNT("is not isolated data") );
			return true;
		}

		// 삭제
		m_isolatedDataList.RemoveKey(rootUUID);

		ByteArray deIsolateDataBlock;
		m_s2cProxy.NotifyDeisolateDataSuccess(
			remote, 
			g_ReliableSendForPN, 
			sessionGuid, 
			rootUUID, 
			deIsolateDataBlock );
		return true;
	}

	// rootUUID 가 가리키는 data tree 가 이미 isolate 된 것인지 검사합니다.
	// 격리되어 있으면 true.
	bool CDbCacheServer2Impl::CheckIsolateDataList(Guid rootUUID)
	{
		//이 함수를 각 API에서 호출하는데, 나중에 병합시 추가된 API에서 놓치는 문제 존재. 
		// 이 브랜치에서는 말고, 추후 다른 브랜치가 병합될 때 새로 추가된 함수가 있을 떄 주의해야 (지금은 없는 듯)


		CriticalSectionLock lock(m_cs, true);

		if (m_isolatedDataList.GetCount() <= 0)
			return false;

		if (!m_isolatedDataList.ContainsKey(rootUUID))
			return false;

		return true;
	}

}

#endif // _WIN32
