/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "../include/AdoWrap.h"
#include "../include/coinit.h"
#include "../include/Exception.h"
#include "../include/ProudNetServer.h"
#include "../include/threadutil.h"
#include "DbConfig.h"
#include "RmiContextImpl.h"
#include "AsyncWork.h"
#include "DBCacheJob.h"
#include "FavoriteLV.h"
#include "DbCacheServer2Impl.h"
#include "ExceptionImpl.h"
#include "CriticalSectImpl.h"

#ifdef _MSC_VER
#pragma warning(disable:4840)
#endif


namespace Proud
{
	extern const PNTCHAR* g_notSameUUIDErrorText;

	// child table들을 로딩한다.
	bool CDbCacheServer2Impl::LoadDataForChildTablesFromDbms(CAdoConnection &conn, String rootTableName, Guid rootguid, CLoadedData2* data)
	{
		String strFmt;
		CFastList2<CPropNodePtr, int> tempList;
		// 모든 low table을 검사해 root node과 관련있는 모든 노드를 data에 추가한다.

		strFmt.Format(_PNT("pn_Get%sAllData"), rootTableName);

		CAdoCommand cmd;
		cmd->CommandTimeout = m_commandTimeoutTime;
		CAdoRecordset rs;
		cmd.Prepare(conn, strFmt);

		if (MsSql == m_dbmsType)
		{
			cmd.AppendReturnValue();
		}

		//cmd.Parameters[1]=rootguid;가 너무 느려서(0.2초) 아래와 같이 수정
		cmd.AppendInputParameter(_PNT("inRootGuid"), rootguid);
		cmd.Execute(rs);

		if (MsSql == m_dbmsType)
		{
			if (long(cmd.Parameters[0]) < 0L)
				return false;
		}

		if (!rs.IsOpened())
		{
			rs.Open();
		}

		int rootTableNum = 0;

		for (rootTableNum; rootTableNum < m_tableNames.GetCount(); rootTableNum++)
		{
			if (rootTableName == m_tableNames[rootTableNum].m_rootTableName)
				break;
		}

		CFastArray<Proud::String>::iterator childTableName = m_tableNames[rootTableNum].m_childTableNames.begin();

		while (true)
		{
			while (!rs.IsEOF())
			{
				CPropNodePtr newNode = CPropNodePtr(new CPropNode(*childTableName));
				CLoadedData2_S::DbmsLoad(rs, *newNode);

				if (newNode->UUID == Guid() || newNode->OwnerUUID == Guid() || newNode->RootUUID == Guid())
					throw Exception("low table Record has NULL GUID!");

				// 새노드의 owner노드를 찾는다.   
				CPropNodePtr owner = data->GetNode(newNode->OwnerUUID);
				if (owner)
				{
					data->InsertChild(owner, newNode);
				}
				else
				{
					tempList.AddTail(newNode);
				}

				rs->MoveNext();
			}

			childTableName++;

			// 해당 비교 구문이 없으면 MySql에서 MoveNextRecordset 호출시 에러 발생 
			if (childTableName == m_tableNames[rootTableNum].m_childTableNames.end())
				break;

			if (!rs.MoveNextRecordset())
				break;
		}
		rs.Close();  // less DB lock을 위해


		// 무한루프 방지를 위한 안전장치
		int nCount = (int)tempList.GetCount();
		while (tempList.GetCount() > 0)
		{
			CPropNodePtr node = tempList.RemoveHead();
			CPropNodePtr owner = data->GetNode(node->OwnerUUID);
			if (owner)
			{
				data->InsertChild(owner, node);
			}
			else
			{
				tempList.AddTail(node);
				--nCount;
				if (nCount <= 0)
					break;
			}
		}

		if (tempList.GetCount() > 0)
		{
			//일단은 예외처리...나중에는 봐서 로딩안한듯 한것도 좋을듯..
			//throw Exception(_PNT("유저 정의 테이블중 아무것도 참조치 않는 데이터가 있습니다."));
			stringstream ss;
			ss << "One or more orphaned record is found. Root UUID=" << StringT2A(rootguid.ToString()).GetString();
			m_delegate->OnException(Exception(ss.str().c_str()));
		}
		data->_INTERNAL_ClearChangeNode();

		return true;
	}


	DEFRMI_DB2C2S_RequestExclusiveLoadDataByFieldNameAndValue(CDbCacheServer2Impl)
	{
		RequestExclusiveLoadDataByFieldNameAndValue_Param* params = new RequestExclusiveLoadDataByFieldNameAndValue_Param(
			this,
			remote,
			rmiContext,
			rootTableName,
			fieldName,
			cmpValue,
			tag,
			message
			);

		m_dbJobQueue->PushJob(CDbCacheJobQueue::JobType_ExclusiveLoadDataByFieldNameAndValue, params);

		return true;
	}

	DWORD WINAPI RequestExclusiveLoadDataByFieldNameAndValue_Core_Static(void* context)
	{
		CHeldPtr<RequestExclusiveLoadDataByFieldNameAndValue_Param> params((RequestExclusiveLoadDataByFieldNameAndValue_Param*)context);

		try
		{
			params->m_server->RequestExclusiveLoadDataByFieldNameAndValue_Core(
				params->remote,
				params->rmiContext,
				params->rootTableName,
				params->fieldName,
				params->cmpValue,
				params->tag,
				params->message
				);
		}
		catch (std::bad_alloc &e)
		{
			Exception ex(e);
			ex.m_remote = params->remote;
			ex.m_userCallbackName = _PNT("RequestExclusiveLoadDataByFieldNameAndValue");

			// void CNetCoreImpl::FreePreventOutOfMemory()를 호출해야 하는데...
			params->m_server->m_delegate->OnException(ex);
			params->m_server->m_netServer->Stop();
		}
		catch (Proud::Exception&e)
		{
			e.m_remote = params->remote;
			e.m_userCallbackName = _PNT("RequestExclusiveLoadDataByFieldNameAndValue");
			params->m_server->m_delegate->OnException(e);
		}
		catch (std::exception &e)
		{
			Exception ex(e);
			ex.m_remote = params->remote;
			ex.m_userCallbackName = _PNT("RequestExclusiveLoadDataByFieldNameAndValue");
			params->m_server->m_delegate->OnException(ex);
		}
		catch (_com_error &e)
		{
			Exception ext; Exception_UpdateFromComError(ext, e);
			ext.m_remote = params->remote;
			ext.m_userCallbackName = _PNT("RequestExclusiveLoadDataByFieldNameAndValue");
			params->m_server->m_delegate->OnException(ext);
		}

		return 0; // 리턴값 무의미
	}

	bool CDbCacheServer2Impl::RequestExclusiveLoadDataByFieldNameAndValue_Core(
		Proud::HostID remote,
		Proud::RmiContext& /*rmiContext*/,
		const Proud::String &rootTableName,
		const Proud::String &fieldName,
		const Proud::CVariant &cmpValue,
		const int64_t& tag,
		const Proud::ByteArray &message
		)
	{
		String queryString;
		queryString.Format(_PNT("%s = %s"), fieldName, VariantToString(cmpValue));
		CLoadRequestPtr request(new CLoadRequest(true, remote, tag, message, GetPreciseCurrentTimeMs()));

		CriticalSectionLock mainLock(m_cs, true);
		LoadDataCore(mainLock, remote, rootTableName, queryString, request);
		return true;
	}

	//#RequestExclusiveLoadDataByGuid 2 서버에서 요청을 받음
	DEFRMI_DB2C2S_RequestExclusiveLoadDataByGuid(CDbCacheServer2Impl)
	{
		// #RequestExclusiveLoadDataByGuid 3 별도 스레드에서 처리하게 작업 큐에 저장
		// 여기서 바로 처리하지 않는다. DB 처리를 전담하는 스레드에서 처리할 수 있도록 작업 큐에 쌓는다.		
		RequestExclusiveLoadDataByGuid_Param* params = new RequestExclusiveLoadDataByGuid_Param(
			this,
			remote,
			rmiContext,
			rootTableName,
			rootUUID,
			tag,
			message
			);

		m_dbJobQueue->PushJob(CDbCacheJobQueue::JobType_ExclusiveLoadDataByGuid, params);

		return true;
	}

	DWORD WINAPI RequestExclusiveLoadDataByGuid_Core_Static(void* context)
	{

		CHeldPtr<RequestExclusiveLoadDataByGuid_Param> params((RequestExclusiveLoadDataByGuid_Param*)context);

		try
		{
			params->m_server->RequestExclusiveLoadDataByGuid_Core(
				params->remote,
				params->rmiContext,
				params->rootTableName,
				params->rootUUID,
				params->tag,
				params->message
				);
		}
		catch (std::bad_alloc &e)
		{
			Exception ex(e);
			ex.m_remote = params->remote;
			ex.m_userCallbackName = _PNT("RequestExclusiveLoadDataByGuid");

			// void CNetCoreImpl::FreePreventOutOfMemory()를 호출해야 하는데...
			params->m_server->m_delegate->OnException(ex);
			params->m_server->m_netServer->Stop();
		}
		catch (Proud::Exception&e)
		{
			e.m_remote = params->remote;
			e.m_userCallbackName = _PNT("RequestExclusiveLoadDataByGuid");
			params->m_server->m_delegate->OnException(e);
		}
		catch (std::exception &e)
		{
			Exception ex(e);
			ex.m_remote = params->remote;
			ex.m_userCallbackName = _PNT("RequestExclusiveLoadDataByGuid");
			params->m_server->m_delegate->OnException(ex);
		}
		catch (_com_error &e)
		{
			Exception ext; Exception_UpdateFromComError(ext, e);
			ext.m_remote = params->remote;
			ext.m_userCallbackName = _PNT("RequestExclusiveLoadDataByGuid");
			params->m_server->m_delegate->OnException(ext);
		}

		return 0; // 리턴값 무의미
	}

	// RequestExclusiveLoadDataByGuid의 핵심 처리
	bool CDbCacheServer2Impl::RequestExclusiveLoadDataByGuid_Core(
		Proud::HostID remote,
		Proud::RmiContext& /*rmiContext*/,
		const Proud::String &rootTableName,
		const Proud::Guid &rootUUID,
		const int64_t& tag,
		const Proud::ByteArray &message
		)
	{
		//#RequestExclusiveLoadDataByGuid 4 작업 큐에서 꺼내서 처리
		
		CLoadRequestPtr request(new CLoadRequest(true, remote, tag, message, GetPreciseCurrentTimeMs()));
		LoadedDataList unloadRequestList;
		CriticalSectionLock mainLock(m_cs, true);

		// 유효하지 않은 Guid인지 검사.
		if (rootUUID == Guid())
		{
			CFailInfo_S info;
			info.m_reason = ErrorType_UserRequested;
			info.m_comment = _PNT("RootUUID is not valid");
			request->m_failList.AddTail(info);
			NotifyLoadDataCompleteOnlyIfUnloadWaiterIsNone(request);
			return true;
		}

		// DataTree가 이미 메모리에 로드되어있는지 확인.
		CLoadedData2Ptr_S loadedData = CheckLoadedDataAndProcessRequest(rootUUID, request, unloadRequestList);
		if (loadedData == NULL)
		{
			// 로드되어있지 않다면 DB로부터 로드한다.
			LoadDataCore(mainLock, remote, rootTableName, GetUuidQueryString(rootUUID), request);
			return true;
		}

		// 이제, 메모리에 로드는 되었다. 

		// Unload요청이 필요하면 요청하고 즉시 응답 가능하면 응답한다.
		LoadDataCore_Finish(mainLock, request, &unloadRequestList);
		return true;
	}

	DEFRMI_DB2C2S_RequestExclusiveLoadDataByQuery(CDbCacheServer2Impl)
	{
		RequestExclusiveLoadDataByQuery_Param* params = new RequestExclusiveLoadDataByQuery_Param(
			this,
			remote,
			rmiContext,
			rootTableName,
			queryString,
			tag,
			message
			);

		m_dbJobQueue->PushJob(CDbCacheJobQueue::JobType_ExclusiveLoadDataByQuery, params);

		return true;
	}

	DWORD WINAPI RequestExclusiveLoadDataByQuery_Core_Static(void* context)
	{
		CHeldPtr<RequestExclusiveLoadDataByQuery_Param> params((RequestExclusiveLoadDataByQuery_Param*)context);

		try
		{
			params->m_server->RequestExclusiveLoadDataByQuery_Core(
				params->remote,
				params->rmiContext,
				params->rootTableName,
				params->queryString,
				params->tag,
				params->message
				);
		}
		catch (std::bad_alloc &e)
		{
			Exception ex(e);
			ex.m_remote = params->remote;
			ex.m_userCallbackName = _PNT("RequestExclusiveLoadDataByQuery");

			// void CNetCoreImpl::FreePreventOutOfMemory()를 호출해야 하는데...
			params->m_server->m_delegate->OnException(ex);
			params->m_server->m_netServer->Stop();
		}
		catch (Proud::Exception&e)
		{
			e.m_remote = params->remote;
			e.m_userCallbackName = _PNT("RequestExclusiveLoadDataByQuery");
			params->m_server->m_delegate->OnException(e);
		}
		catch (std::exception &e)
		{
			Exception ex(e);
			ex.m_remote = params->remote;
			ex.m_userCallbackName = _PNT("RequestExclusiveLoadDataByQuery");
			params->m_server->m_delegate->OnException(ex);
		}
		catch (_com_error &e)
		{
			Exception ext; Exception_UpdateFromComError(ext, e);
			ext.m_remote = params->remote;
			ext.m_userCallbackName = _PNT("RequestExclusiveLoadDataByQuery");
			params->m_server->m_delegate->OnException(ext);
		}

		return 0; // 리턴값 무의미
	}

	bool CDbCacheServer2Impl::RequestExclusiveLoadDataByQuery_Core(
		Proud::HostID remote,
		Proud::RmiContext& /*rmiContext*/,
		const Proud::String &rootTableName,
		const Proud::String &queryString,
		const int64_t& tag,
		const Proud::ByteArray &message
		)
	{
		CLoadRequestPtr request(new CLoadRequest(true, remote, tag, message, GetPreciseCurrentTimeMs()));

		CriticalSectionLock mainLock(m_cs, true);
		LoadDataCore(mainLock, remote, rootTableName, queryString, request);
		return true;
	}

	DEFRMI_DB2C2S_RequestExclusiveLoadNewData(CDbCacheServer2Impl)
	{
		RequestExclusiveLoadNewData_Param* params = new RequestExclusiveLoadNewData_Param(
			this,
			remote,
			rmiContext,
			rootTableName,
			addDataBlock,
			tag,
			transaction
			);

		m_dbJobQueue->PushJob(CDbCacheJobQueue::JobType_ExclusiveLoadData_New, params);

		return true;
	}

	DWORD WINAPI RequestExclusiveLoadNewData_Core_Static(void* context)
	{
		CHeldPtr<RequestExclusiveLoadNewData_Param> params((RequestExclusiveLoadNewData_Param*)context);

		try
		{
			params->m_server->RequestExclusiveLoadNewData_Core(
				params->remote,
				params->rmiContext,
				params->rootTableName,
				params->addDataBlock,
				params->tag,
				params->transaction
				);
		}
		catch (std::bad_alloc &e)
		{
			Exception ex(e);
			ex.m_remote = params->remote;
			ex.m_userCallbackName = _PNT("RequestExclusiveLoadNewData");

			// void CNetCoreImpl::FreePreventOutOfMemory()를 호출해야 하는데...
			params->m_server->m_delegate->OnException(ex);
			params->m_server->m_netServer->Stop();
		}
		catch (Proud::Exception&e)
		{
			e.m_remote = params->remote;
			e.m_userCallbackName = _PNT("RequestExclusiveLoadNewData");
			params->m_server->m_delegate->OnException(e);
		}
		catch (std::exception &e)
		{
			Exception ex(e);
			ex.m_remote = params->remote;
			ex.m_userCallbackName = _PNT("RequestExclusiveLoadNewData");
			params->m_server->m_delegate->OnException(ex);
		}
		catch (_com_error &e)
		{
			Exception ext; Exception_UpdateFromComError(ext, e);
			ext.m_remote = params->remote;
			ext.m_userCallbackName = _PNT("RequestExclusiveLoadNewData");
			params->m_server->m_delegate->OnException(ext);
		}

		return 0; // 리턴값 무의미
	}

	bool CDbCacheServer2Impl::RequestExclusiveLoadNewData_Core(
		Proud::HostID remote,
		Proud::RmiContext& /*rmiContext*/,
		const Proud::String &rootTableName,
		const Proud::ByteArray &addDataBlock,
		const int64_t& tag,
		const bool transaction
		)
	{
		ExclusiveLoadData_NewRoot(remote, rootTableName, addDataBlock, tag, transaction);
		return true;
	}

	DEFRMI_DB2C2S_RequestUnloadDataBySessionGuid(CDbCacheServer2Impl)
	{
		_pn_unused(rmiContext);

		CriticalSectionLock lock(m_cs, true);

		CDbRemoteClientPtr_S rc = GetAuthedRemoteClientByHostID(remote);
		if (rc == NULL)
		{
			return true;
		}

		CLoadedData2Ptr_S data = GetLoadedDataBySessionGuid(sessionGuid);
		if (data != NULL)
		{
			m_loadedDataListBySession.Remove(sessionGuid);
			data->MarkUnload();
			HandoverExclusiveOwnership(data, messageToNextLoader);
		}
		else
		{
			//modify by rekfkno1 - data가 없는경우 클라에게 알린다.
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_LoadedDataNotFound, ErrorType_LoadedDataNotFound, _PNT("DB.UNLD"));
		}
		return true;
	}

	// 기존 데이터 점유하고 있는 디비캐시클라가, 다른 디비 캐시클라가 같은 데이터를 로딩 및 점유하려는 것을 거부한 경우
	DEFRMI_DB2C2S_DenyUnloadData(CDbCacheServer2Impl)
	{
		_pn_unused(rmiContext);

		CriticalSectionLock lock(m_cs, true);

		CDbRemoteClientPtr_S rc = GetAuthedRemoteClientByHostID(remote);
		if (rc == NULL)
		{
			return true;
		}

		CLoadedData2Ptr_S data = GetLoadedDataBySessionGuid(sessionGuid);

		if (data == NULL)
		{
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_LoadedDataNotFound, ErrorType_LoadedDataNotFound, _PNT("DB.UNLD"));
			return true;
		}

		if (data->GetLoaderClientHostID() != remote)
		{
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_PermissionDenied, ErrorType_PermissionDenied, _PNT("DB.UNLD"));
			return true;
		}

		//이게 온다면 모두 실패.
		//onclientleave에서 처리하고 있으므로, requester을 검사하는 로직은 필요 없다.
		// 		while ( data->m_unloadRequests.GetCount() > 0 )
		// 		{
		// 			LoadDataUnloadRequestPair& req = data->m_unloadRequests.RemoveHead();
		// 			data->m_unloadRequestsMap.Remove(req.m_requester);
		// 			m_s2cProxy.NotifyExclusiveLoadDataFailed(req.m_requester.m_hostID, g_ReliableSendForPN, req.m_requester.m_sessionGuid, ErrorType_PermissionDenied, String(), req.m_value.m_tag, messageToRequester);
		// 		}

		CFailInfo_S info;
		info.m_uuid = data->m_data.GetRootUUID();
		info.m_reason = ErrorType_PermissionDenied;
		info.m_message = messageToRequester;
		info.m_comment = _PNT("Current loaded data owner as DB cache client denied the handover.");

		DenyUnloadDataProc(data, info, false); 
		return true;
	}

	DEFRMI_DB2C2S_RequestForceCompleteUnload(CDbCacheServer2Impl)
	{
		_pn_unused(rmiContext);

		CriticalSectionLock lock(m_cs, true);

		CDbRemoteClientPtr_S rc = GetAuthedRemoteClientByHostID(remote);
		if (!rc)
		{
			return true;
		}

		CLoadedData2Ptr_S loadData = GetLoadedDataByRootGuid(rootUUID);
		if (loadData && !loadData->IsLoaded())
		{
			loadData->m_hibernateStartTimeMs = GetPreciseCurrentTimeMs() - loadData->m_unloadedHibernateDuration - OneSecond;
		}
		return true;
	}

	// root UUID가 가리키는 LD를 메모리에서 언로딩한다.
	// unloadAllClient=true이면 이양 처리 없이 전부다 권리 포기. 가령, caller가 data isolate를 하는 경우.
	void CDbCacheServer2Impl::UnloadLoadeeByRootUUID(HostID clientID, Guid rootUUID, bool unloadAllClient)
	{
		CriticalSectionLock lock(m_cs, true);

		CDbRemoteClientPtr_S rc = GetAuthedRemoteClientByHostID(clientID);
		if (rc == NULL)
		{
			return;
		}

		CLoadedData2Ptr_S data = GetLoadedDataByRootGuid(rootUUID);
		if (data != NULL)
		{
			m_loadedDataListBySession.Remove(data->m_data.sessionGuid);
			data->MarkUnload();

			//독점권 이양
			if (unloadAllClient)
			{
				// Isolate 처리 시 이쪽으로 빠진다.
				CFailInfo_S info;
				info.m_uuid = data->m_data.GetRootUUID();
				info.m_reason = ErrorType_UserRequested;
				info.m_comment = _PNT("is isolated.");
				DenyUnloadDataProc(data, info, true);
			}
			else
			{
				HandoverExclusiveOwnership(data);
			}
		}
		else
		{
			// 로딩할 데이터가 없네? 경고 노티.
			m_s2cProxy.NotifyWarning(clientID, g_ReliableSendForPN, ErrorType_LoadedDataNotFound, ErrorType_LoadedDataNotFound, _PNT("DB.UNLD"));
		}
	}

	Proud::String CDbCacheServer2Impl::GetUuidQueryString(const Guid& uuid)
	{
		// MySQL에서는 UUID에 중괄호가 붙기 때문에 이를 구분해줘야 합니다.
		String ret;
		if (m_dbmsType == MsSql)
		{
			ret.Format(_PNT(" UUID='%s'"), uuid.ToString());
		}
		else
		{
			ret.Format(_PNT(" UUID='%s'"), uuid.ToBracketString());
		}
		return ret;
	}

	// data를 독점 로딩하려고 요청한 디비캐시 클라가 거부당했을 때의 처리
	void CDbCacheServer2Impl::DenyUnloadDataProc(CLoadedData2Ptr_S data, CFailInfo_S& info, bool denyAll)
	{
		// denyAll=true인 경우, 모든 requester에 대해 실패 처리를 한다.
		// false인 경우 첫 requester에 대해 실패 처리를 한다. 
		// 또 다른 requester가 있는 경우 그 requester에게 unload request를 보낸다.
		// 그러면 거기서 deny or unload를 하겠고.
		while (data->m_unloadRequests.GetCount() > 0)
		{
			CLoadRequestPtr req = data->m_unloadRequests.RemoveHead();
			ProcessLoadDataFail(req, info);

			// Isolate 처리 시에는 루프를 돌면서 모두 Deny한다.
			if (!denyAll)
			{
				//그 뒤의 클라가 있다면 바로 독점권이양을 종용하고 함수를 끝낸다.
				if (data->m_unloadRequests.GetCount() > 0)
				{
					CFastArray<Guid> session;
					CFastArray<ByteArray> message;
					session.Add(data->m_data.GetSessionGuid());
					message.Add(data->m_unloadRequests.GetHead()->m_message); // 다음 요청자의 메시지를 전달한다.
					m_s2cProxy.NotifyDataUnloadRequested(
						data->GetLoaderClientHostID(), // 독점권이 이양되지 않았으므로 기존 로더에게 보낸다.
						g_ReliableSendForPN,
						session,
						message,
						false,
						(int64_t)m_unloadRequestTimeoutTimeMs
						);
				}
				break;
			}
		}
	}

	// LD를 unload request를 했던 다음 대기자(db cache client)가 새 독점권을 가진다.
	// 만약 또다른 대기자가 더 있으면 막 지금 소유권을 가진 대기자에게 독점권을 놔줄건지 묻는 메시징을 한다.
	void CDbCacheServer2Impl::HandoverExclusiveOwnership(
		CLoadedData2Ptr_S data,
		const ByteArray& messageToNextLoader/*=ByteArray()*/)
	{
		//독점권 이양
		if (data->m_unloadRequests.GetCount() <= 0)
			return;

		// 독점권 이양 요청이 있는 경우
		// 요청 큐에서 꺼내 처리한다.
		CLoadRequestPtr req = data->m_unloadRequests.RemoveHead();

		// 새로운 세션을 만들고 로드
		RegisterLoadedData(data, req->m_requester, NewSessionGuid());

		// 성공 목록에 추가 
		CSuccessInfo_S info;
		info.m_data = data;
		info.m_message = messageToNextLoader;

		// ProcessLoadDataSuccess 내에서 요청에 대한 응답과 독점권 이양요청을 수행한다.
		ProcessLoadDataSuccess(req, info);
	}

	bool CDbCacheServer2Impl::HasNoChildTable(const String& rootTableName)
	{
		// Child가 전혀 없으면 true 리턴 
		for (CFastArray<CCachedTableName>::iterator it = m_tableNames.begin(); it != m_tableNames.end(); ++it)
		{
			if ((*it).m_rootTableName == rootTableName)
				return (*it).m_childTableNames.GetCount() == 0;
		}
		return true;
	}

	bool CDbCacheServer2Impl::ProcessLoadDataFail(CLoadRequestPtr& request, CFailInfo_S& info)
	{
		// 실패 정보를 남긴다.
		request->m_failList.AddTail(info);
		if (request->m_isExclusiveRequest)
		{
			--request->m_waitForUnloadCount;
		}

		assert(request->m_waitForUnloadCount >= 0);
		return NotifyLoadDataCompleteOnlyIfUnloadWaiterIsNone(request);
	}

	bool CDbCacheServer2Impl::ProcessLoadDataSuccess(CLoadRequestPtr& request, CSuccessInfo_S& info)
	{
		AssertIsLockedByCurrentThread(m_cs);

		request->m_successList.AddTail(info);
		if (request->m_isExclusiveRequest)
		{
			--request->m_waitForUnloadCount;
		}

		assert(request->m_waitForUnloadCount >= 0);
		return NotifyLoadDataCompleteOnlyIfUnloadWaiterIsNone(request);
	}

	// request에 든 requester 즉 db cache client에게 NotifyDataUnloadRequested를 보낸다. 보낼 수 있다면.
	bool CDbCacheServer2Impl::NotifyLoadDataCompleteOnlyIfUnloadWaiterIsNone(CLoadRequestPtr& request)
	{
		AssertIsLockedByCurrentThread(m_cs);

		// 아직 Unload 여부를 기다려야 하는 데이터가 있으면 실패 
		if (request->m_waitForUnloadCount != 0)
		{
			return false;
		}

		// 릭 체크.
		// LoadDataCore_Finish에서 OnWarning 콜백 중
		// EnquePendedDbmsWorksForActiveLoadees가 실행되면 
		// 최대 2가 될 수 있다.
		assert(request.GetRefCount() <= 2);

		// 리팩토링이 필요할듯? => 이정도도 충분.
		intptr_t success = request->m_successList.GetCount();
		intptr_t fail = request->m_failList.GetCount();
		CFastArray<ByteArray>	successList_loadedData;
		CFastArray<Guid>		successList_sessionGuid;
		CFastArray<ByteArray>	successList_message;
		CFastArray<Guid>		failList_uuid;
		CFastArray<ErrorType>	failList_reason;
		CFastArray<String>		failList_comment;
		CFastArray<int32_t>		failList_hresult;
		CFastArray<ByteArray>	failList_message;

		// 조금이라도 나을까 싶어서..
		successList_loadedData.SetCapacity(success);
		successList_sessionGuid.SetCapacity(success);
		successList_message.SetCapacity(success);
		failList_uuid.SetCapacity(fail);
		failList_reason.SetCapacity(fail);
		failList_comment.SetCapacity(fail);
		failList_hresult.SetCapacity(fail);
		failList_message.SetCapacity(fail);

		// 성공 목록	
		bool isExclusiveRequest = request->m_isExclusiveRequest;
		for (SuccessInfoList_S::iterator it = request->m_successList.begin(); it != request->m_successList.end(); ++it)
		{
			CSuccessInfo_S& info = *it;
			ByteArray loadDataBlock;

			// 독점 완료 표시
			if (isExclusiveRequest)
				info.m_data->m_state = CLoadedData2_S::LoadState_Exclusive;

			info.m_data->m_data.ToByteArray(loadDataBlock);
			successList_loadedData.Add(loadDataBlock);
			successList_sessionGuid.Add(info.m_data->m_data.sessionGuid);
			successList_message.Add(info.m_message);
		}

		// 실패 목록
		for (FailInfoList_S::iterator it = request->m_failList.begin(); it != request->m_failList.end(); ++it)
		{
			CFailInfo_S& info = *it;
			assert(info.m_reason != ErrorType_Ok);

			failList_uuid.Add(info.m_uuid);
			failList_reason.Add(info.m_reason);
			failList_comment.Add(info.m_comment);
			failList_hresult.Add(info.m_hResult);
			failList_message.Add(info.m_message);
		}

		// #RequestExclusiveLoadDataByGuid 7 사용자의 로딩 요청에 대한 결과를 클라에게 노티
		m_s2cProxy.NotifyLoadDataComplete(request->m_requester, g_ReliableSendForPN,
			request->m_isExclusiveRequest,
			request->m_tag,
			successList_loadedData,
			successList_sessionGuid,
			successList_message,
			failList_uuid,
			failList_reason,
			failList_comment,
			failList_hresult,
			failList_message
			);

		// 뒤에 대기하고 있는 독점권 이양 요청들을 처리해준다.
		CFastArray<Guid> sessionList;;
		CFastArray<ByteArray> messageList;
		for (SuccessInfoList_S::iterator it = request->m_successList.begin(); it != request->m_successList.end(); ++it)
		{
			CLoadedData2Ptr_S& data = (*it).m_data;
			if (data->m_unloadRequests.GetCount() > 0)
			{
				sessionList.Add(data->m_data.GetSessionGuid());
				messageList.Add(data->m_unloadRequests.GetHead()->m_message);
			}
		}
		if (sessionList.GetCount() > 0)
		{
			m_s2cProxy.NotifyDataUnloadRequested(
				request->m_requester, 
				g_ReliableSendForPN, 
				sessionList, 
				messageList, 
				false,
				(int64_t)m_unloadRequestTimeoutTimeMs);
		}

		return true;
	}

	// 새로운 세션 발급
	Guid CDbCacheServer2Impl::NewSessionGuid()
	{
		Guid newSession;
		do
		{
			newSession = Win32Guid::NewGuid();
		} while (GetLoadedDataBySessionGuid(newSession) != NULL);

		return newSession;
	}

	// 로딩된 데이터를 loaded LD list에 넣어두되 비독점 상태.
	void CDbCacheServer2Impl::RegisterLoadedData(CLoadedData2Ptr_S& loadedData)
	{
		// 메모리에 로드만 하고 독점은 하지 않는다.
		// 비독점 요청에서 사용 
		loadedData->MarkUnload();
		m_loadedDataList.Add(loadedData->m_data.GetRootUUID(), loadedData);
	}

	// 로딩된 데이터를 loaded LD list에 넣는다. 특정 놈에 의해 독점 상태이다.
	void CDbCacheServer2Impl::RegisterLoadedData(
		CLoadedData2Ptr_S& loadedData,
		HostID requester,
		const Guid& sessionGuid)
	{
		// 메모리에 로드하고 독점시키거나 독점요청 진행중 상태로 바꿀 때 사용.
		loadedData->MarkLoad(requester, sessionGuid, CLoadedData2_S::LoadState_Requested);
		m_loadedDataListBySession.Add(sessionGuid, loadedData);
		m_loadedDataList.Add(loadedData->m_data.GetRootUUID(), loadedData);
	}

	// rootUUID에 해당하는 데이터가 메모리에 없으면 NULL을 반환한다.
	// 메모리에 이미 로드되어 있으면 해당 데이터의 독점 상태에 따라 요청을 처리해주고 그 데이터를 반환한다.
	CLoadedData2Ptr_S CDbCacheServer2Impl::CheckLoadedDataAndProcessRequest(
		const Guid&			rootUUID,
		CLoadRequestPtr&	request,
		LoadedDataList&		unloadRequestList)
	{
		//#RequestExclusiveLoadDataByGuid 5 이미 로딩된 상태인지 체크

		// 메모리에 로드된 데이터가 있는가
		CLoadedData2Ptr_S loadedData = GetLoadedDataByRootGuid(rootUUID);
		if (loadedData == NULL)
			return loadedData; // 없으면 NULL 

		// 로드된 데이터가 존재함.
		// 성공 처리가 가능하면 처리한다.
		assert(request != NULL);
		if (!request->m_isExclusiveRequest)
		{
			// 비독점 요청은 무조건 성공.
			// hibernateStartTime을 갱신하지는 않는다.
			CSuccessInfo_S successInfo;
			successInfo.m_data = loadedData;
			request->m_successList.AddTail(successInfo);
		}
		else // 사용자가 독점으로 로딩하려 한다.
		{
			// 아무도 독점하고 있지 않은 데이터면
			if (loadedData->m_state == CLoadedData2_S::LoadState_None)
			{
				// 바로 성공 처리 가능하다.
				// 이 데이터는 성공하더라도 다른 데이터의 독점권 이양을 기다려야 할 수 있으므로
				// LoadState_Exclusive가 아닌 LoadState_Requested으로 설정해둔다.
				RegisterLoadedData(loadedData, request->m_requester, NewSessionGuid());

				CSuccessInfo_S successInfo;
				successInfo.m_data = loadedData;
				request->m_successList.AddTail(successInfo);
			}
			else
			{
				// 해당 데이터가 이미 독점 중 혹은 독점요청이 진행 중인 경우이다.
				// Unload요청 목록에 넣어놓고 나중에 한번에 처리한다.
				unloadRequestList.AddTail(loadedData);
			}
		}

		return loadedData;
	}

	// 새로운 데이터를 생성하여 DB에 저장하고 캐싱한다.
	// 만약 기존에 존재하는 데이터를 추가하려 한다면 실패로 간주한다.
	void CDbCacheServer2Impl::ExclusiveLoadData_NewRoot(
		const HostID&		remote,
		const String&		rootTableName,
		const ByteArray&	addDataBlock,
		const int64_t&		tag,
		const bool			transaction
		)
	{
		NTTNTRACE("CDbCacheServer2Impl::ExclusiveLoadData_NewRoot [transaction]: %d\n", transaction);

		CLoadRequestPtr request(new CLoadRequest(true, remote, tag, ByteArray(), GetPreciseCurrentTimeMs()));
		CFailInfo_S failInfo;

		// 파라미터 유효성 검사 
		CriticalSectionLock mainLock(m_cs, true);
		if (GetAuthedRemoteClientByHostID(remote) == NULL)
		{
			return;
		}

		if (IsValidRootTable(rootTableName) == false)
		{
			failInfo.m_reason = ErrorType_UserRequested;
			failInfo.m_comment = _PNT("Invalid Root Table Name.");

			request->m_failList.AddTail(failInfo);
			NotifyLoadDataCompleteOnlyIfUnloadWaiterIsNone(request);
			return;
		}

		// DB에 접근해야 하므로 잠시 락을 푼다.
		mainLock.Unlock();

		CPropNodePtr addData(new CPropNode(rootTableName));
		addData->FromByteArray(addDataBlock);
		try
		{
			CAdoConnection conn;
			conn.Open(m_dbmsConnectionString, m_dbmsType);
			SetAdditionalEncodingSettings(conn);

			// 이 while문은 정상적인 경우 1회만 수행한다.
			while (true)
			{
				// 새로운 UUID를 발급한다.
				addData->m_UUID = Win32Guid::NewGuid();
				if (addData->UUID == Guid())
				{
					// 잘못된 UUID가 있으면 다시 발급 시도한다.
					continue;
				}

				// 새로 발급한 UUID가 DB에 존재하는지 확인.
				if (transaction)
					conn.BeginTrans();

				CAdoRecordset rsNewData;
				rsNewData.Open(conn, OpenForAppend, String::NewFormat(_PNT("select * from %s where %s"),
					gds(rootTableName.GetString()),
					GetUuidQueryString(addData->GetUUID())
					));

				if (rsNewData.IsEOF() == false)
				{
					// 이미 존재하는 UUID가 발급되었으면 다시 발급 시도한다.
					if (transaction)
						conn.RollbackTrans();
					continue;
				}

				// 발급 성공.
				// 정책 상 루트 노드는 UUID, OwnerUUID, RootUUID가 모두 동일하다.
				addData->m_ownerUUID = addData->m_UUID;
				addData->m_RootUUID = addData->m_UUID;

				// DB에 저장.
				rsNewData.AddNew();
				CLoadedData2_S::DbmsSave(rsNewData, *addData, false);
				rsNewData.Update();

				if (transaction)
					conn.CommitTrans();

				break;
			}
		}
		catch (_com_error &e)
		{
			Exception ext; Exception_UpdateFromComError(ext, e);
			ext.m_remote = remote;
			m_delegate->OnException(ext);
			failInfo.m_reason = ErrorType_DatabaseAccessFailed;
			failInfo.m_comment = (const PNTCHAR*)e.Description();
			failInfo.m_hResult = e.Error();

			mainLock.Lock();
			request->m_failList.AddTail(failInfo);
			NotifyLoadDataCompleteOnlyIfUnloadWaiterIsNone(request);
			return;
		}
		catch (Exception &e)
		{
			e.m_remote = remote;
			m_delegate->OnException(e);
			failInfo.m_reason = ErrorType_DatabaseAccessFailed;
			failInfo.m_comment = StringA2T(e.what());

			mainLock.Lock();
			request->m_failList.AddTail(failInfo);
			NotifyLoadDataCompleteOnlyIfUnloadWaiterIsNone(request);
			return;
		}
		mainLock.Lock();

		// 락을 푼 동안 데이터가 로드되었을 수 있으므로.
		CLoadedData2Ptr_S loadedData = GetLoadedDataByRootGuid(addData->UUID);
		if (loadedData != NULL)
		{
			// 그 사이 해당 데이터가 로드되었다면 실패처리  
			failInfo.m_reason = ErrorType_AlreadyExists;
			failInfo.m_comment = _PNT("이미 존재하는 데이터를 로드할때는 ExclusiveLoadNewData를 사용 할 수 없습니다.");

			request->m_failList.AddTail(failInfo);
			NotifyLoadDataCompleteOnlyIfUnloadWaiterIsNone(request);
			return;
		}

		// 새로운 데이터를 생성.
		loadedData = CLoadedData2Ptr_S(new CLoadedData2_S(this));
		loadedData->m_data.InsertChild(CPropNodePtr(), addData);
		loadedData->m_data._INTERNAL_ClearChangeNode();

		// 독점 로드 
		RegisterLoadedData(loadedData, remote, NewSessionGuid());

		// 성공 처리
		CSuccessInfo_S successInfo;
		successInfo.m_data = loadedData;
		request->m_successList.AddTail(successInfo);
		NotifyLoadDataCompleteOnlyIfUnloadWaiterIsNone(request);
	}

	// loaded data가 아직 메모리에 없어, DB로부터 로딩한 후 메모리에 넣는 과정을 수행하는 시간 긴 함수
	void CDbCacheServer2Impl::LoadDataCore(
		CriticalSectionLock&	mainLock,
		const HostID&			remote,			// 요청자
		const String&			rootTableName,	// 테이블명 
		const String&			queryString,	// 검색 조건 
		CLoadRequestPtr&		request			// 요청 객체 
		)
	{
		//#RequestExclusiveLoadDataByGuid 6 DB에 없으면 로딩

		assert(request != NULL);
		AssertIsLockedByCurrentThread(*mainLock.GetCriticalSection());

		// 유효성 검사.
		if (GetAuthedRemoteClientByHostID(remote) == NULL)
		{
			// 잘못된 HostID 
			return;
		}
		if (!m_allowNonExclusiveAccess && !request->m_isExclusiveRequest)
		{
			// 비독점 접근을 허용하지 않도록 설정되어있다면
			// 비독점 요청은 실패로 처리한다. 
			CFailInfo_S failInfo;
			failInfo.m_reason = ErrorType_PermissionDenied;
			failInfo.m_comment = _PNT("Non-exclusive data access is denied.");

			request->m_failList.AddTail(failInfo);
			NotifyLoadDataCompleteOnlyIfUnloadWaiterIsNone(request);
			return;
		}
		if (IsValidRootTable(rootTableName) == false)
		{
			// 잘못된 루트 테이블명 
			CFailInfo_S failInfo;
			failInfo.m_reason = ErrorType_UserRequested;
			failInfo.m_comment = _PNT("Invalid root table name.");

			request->m_failList.AddTail(failInfo);
			NotifyLoadDataCompleteOnlyIfUnloadWaiterIsNone(request);
			return;
		}

		// DB에서 가져온 루트 노드들(로딩 후보). 하위노드들을 로드하기 전 잠시 담아놓기 위함.
		LoadedDataList candidateList;

		// 이미 독점로딩된 데이터들을 모아뒀다가 한번에 요청하기 위함.
		LoadedDataList unloadRequestList;

		// 루트 로드 
		LoadDataCore_LoadRoot(mainLock, request, rootTableName, queryString, candidateList, unloadRequestList);
	}

	// LoadDataCore함수가 너무 길어져서 분할
	void CDbCacheServer2Impl::LoadDataCore_LoadRoot(
		CriticalSectionLock&	mainLock,
		CLoadRequestPtr&		request,
		const String&			rootTableName,
		const String&			queryString,
		LoadedDataList&			candidateList,
		LoadedDataList&			unloadRequestList
		)
	{

		// DB에 접근하여 Root 목록을 가져온다.
		AssertIsLockedByCurrentThread(*mainLock.GetCriticalSection());
		mainLock.Unlock();

		// main lock이 걸린 상태에서 DB access를 하면 처리 병목이 발생하므로 assert 검사를 수행.
		AssertIsNotLockedByCurrentThread(*mainLock.GetCriticalSection());

		CAdoConnection conn;
		CAdoRecordset rs;
		try
		{
			conn.Open(m_dbmsConnectionString, m_dbmsType);
SetAdditionalEncodingSettings(conn);
			rs.Open(conn, OpenForRead, String::NewFormat(_PNT("select * from %s where %s"), rootTableName.GetString(), queryString.GetString()));
		}
		catch (_com_error &e)
		{
			Exception ext; Exception_UpdateFromComError(ext, e);
			m_delegate->OnException(ext);

			CFailInfo_S failInfo;
			failInfo.m_reason = ErrorType_LoadedDataNotFound;
			failInfo.m_comment.Format(_PNT("RootUUID not found. %s"), (const PNTCHAR*)e.Description());
			failInfo.m_hResult = e.Error();

			mainLock.Lock();
			request->m_failList.AddTail(failInfo);
			NotifyLoadDataCompleteOnlyIfUnloadWaiterIsNone(request);
			return;
		}
		catch (Exception &e)
		{
			m_delegate->OnException(e);

			CFailInfo_S failInfo;
			failInfo.m_reason = ErrorType_LoadedDataNotFound;
			failInfo.m_comment = _PNT("RootUUID not found.");

			mainLock.Lock();
			request->m_failList.AddTail(failInfo);
			NotifyLoadDataCompleteOnlyIfUnloadWaiterIsNone(request);
			return;
		}
		mainLock.Lock();

		// 조회된 Root목록을 상태에 따라 분류한다.
		try
		{
			for (; rs.IsEOF() == false; rs.MoveNext())
			{
				Guid rootUUID;
				try
				{
					// 새로 추가될 데이터 
					rootUUID = rs.GetFieldValue(_PNT("UUID"));

					// Isolate 여부 검사. 
					// Isolate 상태인 데이터는 비독점 조회도 불허한다.
					if (CheckIsolateDataList(rootUUID))
					{
						CFailInfo_S info;
						info.m_reason = ErrorType_PermissionDenied;
						info.m_comment.Format(_PNT("Data (root=%s) is already isolated."), rootUUID.ToString().GetString());
						info.m_uuid = rootUUID;
						request->m_failList.AddTail(info);

						continue;
					}

					// 이미 로드된 데이터인지
					CLoadedData2Ptr_S loadedData = CheckLoadedDataAndProcessRequest(rootUUID, request, unloadRequestList);
					if (loadedData == NULL)
					{
						// 전혀 로드되지 않은 데이터인 경우 
						// 후보 목록에 추가한다.

						// 데이터 생성  
						CPropNodePtr rootNode = CPropNodePtr(new CPropNode(rootTableName));
						loadedData = CLoadedData2Ptr_S(new CLoadedData2_S(this));
						CLoadedData2_S::DbmsLoad(rs, *rootNode);

						// UUID에 대한 유효성 검사
						if (rootNode->UUID == Guid() || rootNode->OwnerUUID == Guid() || rootNode->RootUUID == Guid())
							throw Exception("UserDefine Record has NULL GUID!");

						// Root 노드는 모든 UUID가 같아야한다.
						// 그렇지 않은 레코드는 Root 노드가 아닌 하위 노드이므로 여기선 무시한다.
						if ( (rootNode->UUID == rootNode->OwnerUUID && rootNode->OwnerUUID == rootNode->RootUUID) == false )
						{
							continue;
						}

						// 루트 완성 
						loadedData->m_data.InsertChild(CPropNodePtr(), rootNode);
						loadedData->m_data._INTERNAL_ClearChangeNode();
						candidateList.AddTail(loadedData);
					}
				}
				catch (_com_error& e)
				{
					CFailInfo_S info;
					info.m_reason = ErrorType_Unexpected;
					info.m_hResult = e.Error();
					info.m_comment = (const PNTCHAR*)e.Description();
					info.m_uuid = rootUUID;
					request->m_failList.AddTail(info);
					continue;
				}
				catch (Exception& e)
				{
					// 잘못된 UUID인 경우 
					// 실패 목록에 추가 
					CFailInfo_S info;
					info.m_reason = ErrorType_Unexpected;
					info.m_comment = StringA2T(e.what());
					info.m_uuid = rootUUID;
					request->m_failList.AddTail(info);
					continue;
				}
			}
			rs.Close();
			conn.Close();
		}
		catch (_com_error& e)
		{
			// 전체 실패로 간주한다.
			SuccessInfoList_S::iterator it;
			for (it = request->m_successList.begin(); it != request->m_successList.end(); ++it)
			{
				CLoadedData2Ptr_S& data = (*it).m_data;
				if (data->m_unloadRequests.GetCount() == 0)
				{
					m_loadedDataListBySession.Remove(data->m_data.sessionGuid);
					data->MarkUnload();
				}
				else
				{
					HandoverExclusiveOwnership(data);
				}
			}
			request->m_successList.Clear();
			request->m_failList.Clear();

			CFailInfo_S info;
			info.m_reason = ErrorType_Unexpected;
			info.m_hResult = e.Error();
			info.m_comment = (const PNTCHAR*)e.Description();
			request->m_failList.AddTail(info);
			NotifyLoadDataCompleteOnlyIfUnloadWaiterIsNone(request);
			return;
		}
		catch (Exception& e)
		{
			// 전체 실패로 간주한다.
			SuccessInfoList_S::iterator it;
			for (it = request->m_successList.begin(); it != request->m_successList.end(); ++it)
			{
				CLoadedData2Ptr_S& data = (*it).m_data;
				if (data->m_unloadRequests.GetCount() == 0)
				{
					m_loadedDataListBySession.Remove(data->m_data.sessionGuid);
					data->MarkUnload();
				}
				else
				{
					HandoverExclusiveOwnership(data);
				}
			}
			request->m_successList.Clear();
			request->m_failList.Clear();

			CFailInfo_S info;
			info.m_reason = ErrorType_Unexpected;
			info.m_comment = StringA2T(e.what());
			request->m_failList.AddTail(info);
			NotifyLoadDataCompleteOnlyIfUnloadWaiterIsNone(request);
			return;
		}

		// 하위노드들 로드
		LoadDataCore_LoadChildren(mainLock, request, rootTableName, candidateList, unloadRequestList);
	}

	// LoadDataCore함수가 너무 길어져서 분할
	void CDbCacheServer2Impl::LoadDataCore_LoadChildren(
		CriticalSectionLock&	mainLock,
		CLoadRequestPtr&		request,
		const String&			rootTableName,
		LoadedDataList&			candidateList,
		LoadedDataList&			unloadRequestList
		)
	{
		// 후보들의 하위노드를 DBMS로부터 로드한다.
		AssertIsLockedByCurrentThread(*mainLock.GetCriticalSection());

		for (LoadedDataList::iterator it = candidateList.begin(); it != candidateList.end(); ++it)
		{
			CLoadedData2Ptr_S& candidateData = *it;
			CLoadedData2Ptr_S loadedData(NULL);

			// 하위 테이블이 존재하는가 
			if (HasNoChildTable(rootTableName) == false)
			{
				// 모든 하위노드들을 DB에서 로딩한다.
				mainLock.Unlock();

				// main lock이 걸린 상태에서 DB access를 하면 처리 병목이 발생하므로 assert 검사를 수행.
				AssertIsNotLockedByCurrentThread(*mainLock.GetCriticalSection());

				bool loadSuccess;
				CFailInfo_S info;
				try
				{
					CAdoConnection conn;
					conn.Open(m_dbmsConnectionString, m_dbmsType);
					SetAdditionalEncodingSettings(conn);
					loadSuccess = LoadDataForChildTablesFromDbms(
						conn, 
						rootTableName, 
						candidateData->m_data.GetRootUUID(), 
						candidateData->GetData()
						);
				}
				catch (_com_error &e)
				{
					Exception ext; Exception_UpdateFromComError(ext, e);
					info.m_comment = (const PNTCHAR*)e.Description();
					info.m_hResult = e.Error();
					m_delegate->OnException(ext);
					loadSuccess = false;
				}
				catch (Exception &e)
				{
					info.m_comment = StringA2T(e.what());
					m_delegate->OnException(e);
					loadSuccess = false;
				}
				mainLock.Lock();

				if (loadSuccess == false)
				{
					// DB로부터의 로드에 실패한경우
					// 실패 목록에 추가
					info.m_reason = ErrorType_Unexpected;
					info.m_comment.Format(_PNT("Loading data failure. %s"), info.m_comment);
					info.m_uuid = candidateData->m_data.RootUUID;
					request->m_failList.AddTail(info);
					continue;
				}

				// DBMS에서 데이터를 가져오는 중 다른 쓰레드에서 로드가 완료된 경우가 있을 수 있으므로
				loadedData = CheckLoadedDataAndProcessRequest(candidateData->m_data.RootUUID, request, unloadRequestList);
			}

			if (loadedData == NULL)
			{
				// 다른 쓰레드에서 로드가 일어나지 않은 경우 
				// 해당 후보를 로드시키고 성공 목록에 추가
				if (request->m_isExclusiveRequest)
				{
					RegisterLoadedData(candidateData, request->m_requester, NewSessionGuid());
				}
				else
				{
					RegisterLoadedData(candidateData);
				}

				CSuccessInfo_S successInfo;
				successInfo.m_data = candidateData;
				request->m_successList.AddTail(successInfo);
			}
		}

		assert(request->m_waitForUnloadCount >= 0);
		LoadDataCore_Finish(mainLock, request, &unloadRequestList);
	}

	// 데이터 로드 요청을 마무리하는 함수.
	// Unload요청이 필요하면 요청하고 즉시 응답 가능하면 응답한다.
	void CDbCacheServer2Impl::LoadDataCore_Finish(
		CriticalSectionLock&	mainLock,
		CLoadRequestPtr&		request,
		LoadedDataList*			unloadRequestList
		)
	{
		AssertIsLockedByCurrentThread(*mainLock.GetCriticalSection());

		RequestUnload(request, unloadRequestList);
		NotifyLoadDataCompleteOnlyIfUnloadWaiterIsNone(request);

		// 로드 과정이 너무 오래 걸리는 경우 경고를 발생시킨다.
		// 주로 DB접근이 오래걸리는 경우이다.
		if ( GetPreciseCurrentTimeMs() - request->m_requestTimeMs >= m_unloadRequestTimeoutTimeMs )
		{
			mainLock.Unlock();
			AssertIsNotLockedByCurrentThread(*mainLock.GetCriticalSection());

			m_delegate->OnWarning(ErrorInfo::From(
				ErrorType_TimeOut,
				request->m_requester,
				_PNT("데이터를 로드하는데 오랜 시간이 걸리고 있습니다.")
				));

			// 유지보수 중 실수 방지를 위해 다시 잠궈준다.
			mainLock.Lock();
		}
	}

	// 독점권 이양을 기다려야 하는 경우
	// 각 독점자들에게 독점권 이양을 요청한다.
	void CDbCacheServer2Impl::RequestUnload(CLoadRequestPtr& request, LoadedDataList* unloadRequestList/*=NULL*/)
	{
		AssertIsLockedByCurrentThread(m_cs);

		// 비독점 요청은 언로드 요청이 필요 없다.
		if ( request->m_isExclusiveRequest == false )
			return;

		assert(unloadRequestList != NULL);

		// 독점권 이양을 요청할 데이터 목록을 순회한다.
		CFastMap<HostID, CFastArray<Guid>> temp;
		for (LoadedDataList::iterator it = unloadRequestList->begin(); it != unloadRequestList->end(); ++it)
		{
			CLoadedData2Ptr_S& data = *it;

			// 지금 당장 요청 할 수 있는 세션들을 추려낸다.
			if (data->m_state == CLoadedData2_S::LoadState_Exclusive
				&& data->m_unloadRequests.GetCount() == 0)
			{
				// 당장 요청 할 수 있는 세션
				temp[data->GetLoaderClientHostID()].Add(data->m_data.GetSessionGuid());
			}

			// 기다려야할 요청이 늘었다.
			++request->m_waitForUnloadCount;

			// 요청큐에 큐잉
			data->m_unloadRequests.AddTail(request);
		}

		// 지금 당장 요청 가능한 세션이 있는지
		CFastArray<ByteArray> commonMessage;
		commonMessage.Add(request->m_message);
		for (CFastMap<HostID, CFastArray<Guid>>::iterator it = temp.begin(); it != temp.end(); ++it)
		{
			HostID toHost = it->GetFirst();
			CFastArray<Guid>& sessionList = it->GetSecond();
			if (sessionList.GetCount() > 0)
			{
				// 있는 경우 요청 
				m_s2cProxy.NotifyDataUnloadRequested(
					toHost,
					g_ReliableSendForPN,
					sessionList,
					commonMessage,
					true,
					(int64_t)m_unloadRequestTimeoutTimeMs);
			}
		}
	}

	// hostID가 소유자인 loaded data를 모두 unload한다.
	void CDbCacheServer2Impl::UnloadLoadeeByLoaderHostID(HostID clientID)
	{
		for (LoadedDataList2_S::iterator i = m_loadedDataList.begin(); i != m_loadedDataList.end(); i++)
		{
			// 이 데이터에 대한 모든 Unload Request에서 해당 HostID에 대한 요청을 제거한다. 
			// 나가는 db cache client이므로.
			CLoadedData2Ptr_S& loadedData = i->GetSecond();
			CFastList2<CLoadRequestPtr, int>::iterator unloadReqIter = loadedData->m_unloadRequests.begin();
			while (unloadReqIter != loadedData->m_unloadRequests.end())
			{
				if ((*unloadReqIter)->m_requester == clientID)
				{
					unloadReqIter = loadedData->m_unloadRequests.erase(unloadReqIter);
					// Requester가 종료중이므로 응답을 할 필요는 없다.
				}
				else
				{
					++unloadReqIter;
				}
			}

			// 종료 중인 클라이언트가 소유자인 경우 
			if (loadedData->GetLoaderClientHostID() == clientID)
			{
				//독점권을 다른 클라에게 이양한다.
				if (loadedData->m_unloadRequests.GetCount() > 0)
				{
					// 접속종료하는 클라이언트가 가진 모든 독점권을 다른 클라이언트에게 이양한다.
					HandoverExclusiveOwnership(loadedData);
				}
				else
				{
					// 해당 데이터는 어떤 클라이언트에서도 사용되지 않게 되므로
					// Unload 시켜준다.
					m_loadedDataListBySession.Remove(loadedData->m_data.sessionGuid);
					loadedData->MarkUnload();
				}
			}
		}
	}
}

