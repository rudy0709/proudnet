/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.

*/

//////////////////////////////////////////////////////////////////////////
// DB cache에서 data load/unload를 하는 함수,변수는 여기다 모으자.

#include "stdafx.h"
#include "DbCacheClient2Impl.h"
#include "dbconfig.h"
#include "RmiContextImpl.h"

namespace Proud
{
	//////////////////////////////////////////////////////////////////////////
	// req - ack method
	void CDbCacheClient2Impl::RequestExclusiveLoadData(String rootTableName, String fieldName, CVariant cmpValue, intptr_t tag, ByteArray& message)
	{
		rootTableName.MakeUpper();

		// 서버에 ExclusiveLoad Field-Value를 요청
		m_c2sProxy.RequestExclusiveLoadDataByFieldNameAndValue(HostID_Server, g_ReliableSendForPN, rootTableName, fieldName, cmpValue, (int64_t)tag, message);
	}

	void CDbCacheClient2Impl::RequestExclusiveLoadDataByGuid(String rootTableName, Guid rootUUID, intptr_t tag, ByteArray& message)
	{
		rootTableName.MakeUpper();

		// 서버에 ExclusiveLoad Guid를 요청
		m_c2sProxy.RequestExclusiveLoadDataByGuid(HostID_Server, g_ReliableSendForPN, rootTableName, rootUUID, (int64_t)tag, message);
	}

	void CDbCacheClient2Impl::RequestExclusiveLoadDataByQuery(String rootTableName, String queryString, intptr_t tag, ByteArray &message)
	{
		rootTableName.MakeUpper();

		// 서버에 ExclusiveLoad Query를 요청
		m_c2sProxy.RequestExclusiveLoadDataByQuery(HostID_Server, g_ReliableSendForPN, rootTableName, queryString, (int64_t)tag, message);
	}

	void CDbCacheClient2Impl::RequestExclusiveLoadNewData(String rootTableName, CPropNodePtr addData, intptr_t tag, bool transaction)
	{
		rootTableName.MakeUpper();

		// Root Node의 TableName 설정
		addData->SetTypeName(rootTableName);

		// Root Node이기 때문에 모든 UUID가 같게 설정된다.
		addData->m_UUID = Win32Guid::NewGuid();
		addData->m_ownerUUID = addData->UUID;
		addData->m_RootUUID = addData->UUID;

		Proud::ByteArray block;
		addData->ToByteArray(block);

		// 서버에 ExclusiveLoad NewData를 요청
		m_c2sProxy.RequestExclusiveLoadNewData(HostID_Server, g_ReliableSendForPN, rootTableName, block, (int64_t)tag, transaction);
	}

	// PIDL의 설명 참고
	DEFRMI_DB2S2C_NotifyLoadDataComplete(CDbCacheClient2Impl)
	{
		rmiContext;
		remote;

		IDbCacheClientDelegate2::CCallbackArgs args;
		intptr_t successCount = successList_sessionGuid.GetCount();
		intptr_t failCount = failList_reason.GetCount();
		intptr_t resultCount = successCount + failCount;
		intptr_t i;

		args.m_tag = (intptr_t)tag;

		if ( resultCount == 0 )
		{
			// 요청 조건에 맞는 데이터가 DB에 존재하지 않는 경우
			args.m_items.SetCount(1);
			args.m_items[0].m_reason = ErrorType_LoadedDataNotFound;
			args.m_items[0].m_comment = _PNT("The data does not exist.");
		}
		else
		{
			args.m_items.SetCount(resultCount);

			// 비독점 요청인 경우 내부 데이터를 접근하지 않으므로 락을 걸 필요가 없다.
			CriticalSectionLock lock(m_cs, false);
			if ( isExclusive )
				lock.Lock();

			// 성공 목록 
			for ( i = 0; i < successCount; ++i )
			{
				// 데이터 추출
				CLoadedData2Ptr loadedData(new CLoadedData2);
				loadedData->FromByteArray(successList_loadedData[i]);
				loadedData->sessionGuid = successList_sessionGuid[i];

				if ( isExclusive )
				{
					// 스냅샷이 아니므로, 디비클라 메모리에 등록
					m_loadedDataList.Add(loadedData->RootUUID, loadedData);
					m_loadedDataListBySession.Add(loadedData->sessionGuid, loadedData);
				}

				// 기타 파라메터
				args.m_items[i].m_loadedData = loadedData->Clone();
				args.m_items[i].m_sessionGuid = loadedData->sessionGuid;
				args.m_items[i].m_rootUUID = loadedData->GetRootUUID();
				args.m_items[i].m_data = args.m_items[i].m_loadedData->GetRootNode();
				args.m_items[i].m_UUID = args.m_items[i].m_data->GetUUID();
				args.m_items[i].m_message = successList_message[i];
			}

			if ( isExclusive )
				lock.Unlock();

			// 실패 목록
			for ( i = 0; i < failCount; ++i )
			{
				intptr_t offset = i + successCount;
				args.m_items[offset].m_UUID = failList_uuid[i];
				args.m_items[offset].m_reason = failList_reason[i];
				args.m_items[offset].m_comment = failList_comment[i];
				args.m_items[offset].m_hResult = (HRESULT)failList_hresult[i];
				args.m_items[offset].m_message = failList_message[i];
			}
		}

		if (isExclusive)
		{
			m_delegate->OnExclusiveLoadDataComplete(args);
		}
		else
		{
			m_delegate->OnNonExclusiveSnapshotDataComplete(args);
		}
		return true;
	}

	DEFRMI_DB2S2C_NotifyDataUnloadRequested(CDbCacheClient2Impl)
	{
		rmiContext;
		remote;

		IDbCacheClientDelegate2::CCallbackArgs args;
		args.m_items.SetMinCapacity(sessionList.GetCount());

		CriticalSectionLock lock(m_cs, true);
		m_unloadRequestTimeoutTimeMs = unloadRequestTimeoutTimeMs;

		intptr_t count = 0;
		for (intptr_t i = 0; i < sessionList.GetCount(); ++i)
		{
			// 내가 이미 가진 LD만 찾아서 그것에 대해서 "저쪽에서 언로딩하라는데?" 항목을 채운다.
			CLoadedData2Ptr loadedData = GetOrgLoadedDataBySessionGuid(sessionList[i]);
			if (loadedData != NULL)
			{
				args.m_items.Add(IDbCacheClientDelegate2::CCallbackArgs::CItem());
				IDbCacheClientDelegate2::CCallbackArgs::CItem& item = args.m_items[count++];
				item.m_loadedData = loadedData->Clone();
				item.m_sessionGuid = sessionList[i];
				item.m_rootUUID = loadedData->GetRootUUID();
				if (commonMessage)
					item.m_message = messageList[0];
				else
					item.m_message = messageList[i];

				// 언로드 요청 목록에 추가.
				m_unloadRequests[item.m_sessionGuid].AddTail(GetPreciseCurrentTimeMs());
			}
		}

		lock.Unlock(); // 유저 콜백 주기 전에 데드락 방지차

		// count가 0이면 이 알림을 받기 전에 로더가 먼저 UnloadDataBySessionGuid를 호출한 경우이다.
		// 따라서 하나도 없으면 아무 노티도 줄 필요가 없다.
		if (count > 0)
		{
			// 사용자 콜백을 실행한다. 사용자는 이 안에서 unload or deny를 하는 함수를 호출해야 한다.
			m_delegate->OnDataUnloadRequested(args);
		}

		return true;
	}

	DEFRMI_DB2S2C_NotifyDataForceUnloaded(CDbCacheClient2Impl)
	{
		rmiContext;
		remote;

		CriticalSectionLock lock(m_cs, true);

		// rootGuid에 대응하는 '로딩된 정보'를 찾아 제거한다.
		assert(rootGuid != Guid());
		CLoadedData2* loadedData = GetOrgLoadedDataByRootUUID(rootGuid);

		if (loadedData != NULL)
		{
			// 노티
			IDbCacheClientDelegate2::CCallbackArgs args;
			args.m_items.SetCount(1);

			args.m_items[0].m_loadedData = loadedData->Clone();
			args.m_items[0].m_sessionGuid = sessionGuid;
			args.m_items[0].m_rootUUID = loadedData->GetRootUUID();

			m_loadedDataList.Remove(loadedData->RootUUID);
			lock.Unlock();

			m_delegate->OnDataForceUnloaded(args);
		}
		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// unload method
	bool CDbCacheClient2Impl::UnloadDataBySessionGuid(Guid sessionGuid, ByteArray& messageToNextLoader)
	{
		CriticalSectionLock lock(m_cs, true);

		bool ret;

		// 불러온 객체를 제거하고 서버에도 요청을 보낸다.
		CLoadedData2Ptr object = GetOrgLoadedDataBySessionGuid(sessionGuid);
		if (object != NULL)
		{
			ret = true;
			m_loadedDataList.Remove(object->RootUUID);
			m_loadedDataListBySession.Remove(sessionGuid);

			// 이제 이 sessionGuid는 볼일이 없으므로 
			// 언로드 요청 목록에서 모두 제거해준다.
			m_unloadRequests.Remove(sessionGuid);
		}
		else
		{
			assert(m_unloadRequests.ContainsKey(sessionGuid) == false);
			ret = false;
		}

		// 여기서 한다. 랙 때문에 로컬에만 없을 수 있으므로.
		m_c2sProxy.RequestUnloadDataBySessionGuid(HostID_Server, g_ReliableSendForPN, sessionGuid, messageToNextLoader);
		return ret;
	}

	void CDbCacheClient2Impl::DenyUnloadData(Guid sessionGuid, ByteArray& messageToRequester)
	{
		CriticalSectionLock lock(m_cs, true);

		CLoadedData2Ptr object = GetOrgLoadedDataBySessionGuid(sessionGuid);

		if (object != NULL)
		{
			// 언로드 요청 목록에서 해당 세션에 대한 요청을 1회 제거한다.
			{
				UnloadRequests::iterator it = m_unloadRequests.find(sessionGuid);
				if ( it == m_unloadRequests.end() )
				{
					// 언로드 요청을 받지도 않았는데 Deny를 하는것은 잘못된 행동이다.
					throw Exception("Unload요청을 받은 Session이 아닙니다.");
				}

				RequestedTimeQueue& reqs = it.GetSecond();
				assert(reqs.IsEmpty() == false);

				reqs.RemoveHead();
				if ( reqs.IsEmpty() )
				{
					// 요청을 모두 처리한 경우.
					m_unloadRequests.erase(it);
				}
			}

			m_c2sProxy.DenyUnloadData(HostID_Server, g_ReliableSendForPN, sessionGuid, messageToRequester);
			return;
		}
		else
		{
			assert(m_unloadRequests.ContainsKey(sessionGuid) == false);
		}

		ErrorInfo info;
		info.m_errorType = ErrorType_LoadedDataNotFound;
		info.m_detailType = ErrorType_LoadedDataNotFound;
		info.m_comment = _PNT("DenyUnloadData is called for not-loaded data.");

		lock.Unlock();

		m_delegate->OnWarning(&info);
	}

	void CDbCacheClient2Impl::ForceCompleteUnload(Guid rootUUID)
	{
		m_c2sProxy.RequestForceCompleteUnload(HostID_Server, g_ReliableSendForPN, rootUUID);
	}

}
