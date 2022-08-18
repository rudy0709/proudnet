/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "../include/variant-marshaler.h"
#include "DbCacheClient2Impl.h"
#include "dbconfig.h"
#include "RmiContextImpl.h"
#include "pidl/DB_common.cpp"
#include "pidl/DB_stub.cpp"
#include "pidl/DB_proxy.cpp"
#include "CriticalSectImpl.h"


namespace Proud
{
	CDbCacheClient2ConnectParameter::CDbCacheClient2ConnectParameter()
	{
		m_userWorkerThreadCount = 0;
		m_externalNetWorkerThreadPool = NULL;
		m_externalUserWorkerThreadPool = NULL;
		m_allowExceptionEvent = true;
	}

	CDbCacheClient2* CDbCacheClient2::New()
	{
		return new CDbCacheClient2Impl;
	}

	bool CDbCacheClient2::Connect(CDbCacheClient2ConnectParameter& param)
	{
		ErrorInfoPtr blackHole;
		return Connect(param, blackHole);
	}

	CDbCacheClient2Impl::CDbCacheClient2Impl(void)
	{
		m_unloadRequestTimeoutTimeMs = CDbConfig::DefaultUnloadRequestTimeoutTimeMs;
		m_userWorkerThreadPool = NULL;
	}

	CDbCacheClient2Impl::~CDbCacheClient2Impl(void)
	{
		Disconnect();

		m_netClient.Free();
		if ( m_userWorkerThreadPool != NULL )
			delete m_userWorkerThreadPool;
	}

	bool CDbCacheClient2Impl::Connect(CDbCacheClient2ConnectParameter& param)
	{
		ErrorInfoPtr blackHole;
		return CDbCacheClient2Impl::Connect(param, blackHole);
	}

	bool CDbCacheClient2Impl::Connect(CDbCacheClient2ConnectParameter& param, ErrorInfoPtr& outError)
	{
		CriticalSectionLock lock(m_cs, true);

		//modify by rekfkno1 - 이제 이것은 필요 없다...disconnect에서 free하지 않기 때문이다.
		//if(m_lanClient)
		//	ThrowInvalidArgumentException();

		if ( m_netClient == NULL )
		{
			m_netClient.Attach(CNetClient::Create());

			m_netClient->AttachProxy(&m_c2sProxy);
			m_netClient->AttachStub(this);
			m_netClient->SetEventSink(this);
		}

		m_authenticationKey = param.m_authenticationKey;
		m_delegate = param.m_delegate;
		m_loggedOn=false;

		// 서버와의 연결을 시도한다.
		CNetConnectionParam p1;
		p1.m_protocolVersion = CDbConfig::ProtocolVersion;
		p1.m_serverIP=param.m_serverAddr;
		p1.m_serverPort=param.m_serverPort;
		p1.m_timerCallbackIntervalMs = 1300;
		p1.m_externalNetWorkerThreadPool = param.m_externalNetWorkerThreadPool;
		p1.m_externalUserWorkerThreadPool = param.m_externalUserWorkerThreadPool;
		p1.m_allowExceptionEvent = param.m_allowExceptionEvent;

		if ( p1.m_externalNetWorkerThreadPool != NULL )
			p1.m_netWorkerThreadModel = ThreadModel_UseExternalThreadPool;
		else
			p1.m_netWorkerThreadModel = ThreadModel_MultiThreaded;

		p1.m_userWorkerThreadModel = ThreadModel_UseExternalThreadPool;
		if ( p1.m_externalUserWorkerThreadPool == NULL )
		{
			int threadCount = param.m_userWorkerThreadCount;
			// DB cache client의 RMI 콜백은 모두 CPU time이다. 따라서 CPU 갯수만.
			if (threadCount == 0)
				threadCount = GetNoofProcessors();

			m_userWorkerThreadPool = CThreadPool::Create(NULL, threadCount);
			p1.m_externalUserWorkerThreadPool = m_userWorkerThreadPool;
		}

		bool ret = m_netClient->Connect(p1, outError);

		return ret;

	}
	
	void CDbCacheClient2Impl::Disconnect()
	{
		{
			CriticalSectionLock lock(m_cs, true);

			if ( m_netClient == NULL )
				return;
			else if ( m_netClient->HasServerConnection() )
			{
				UnloadRequests warnings;
				ProcessUnloadRequestTimeout1(warnings);
				lock.Unlock();
				ProcessUnloadRequestTimeout2(warnings);
			}
		}

		// m_cs를 잠그지 않은 채로 사용중인 user worker thread가 모두 파괴되어야 한다. 따라서...
		m_netClient->Disconnect();

		{
			CriticalSectionLock lock(m_cs, true);

			m_loadedDataList.Clear();
			m_loadedDataListBySession.Clear();
			m_loggedOn = false;
		}
	}


	//////////////////////////////////////////////////////////////////////////
	//event function
	void CDbCacheClient2Impl::OnJoinServerComplete(ErrorInfo *info, const ByteArray& /*replyFromServer*/)
	{
		if ( info->m_errorType==ErrorType_Ok )
		{
			// 서버에 인증 키를 보낸다.
			m_c2sProxy.RequestDbCacheClientLogon(HostID_Server, g_SecureReliableSendForPN, m_authenticationKey);
		}
		else
		{
			// 유저에게 결과 노티를 보낸다.
			m_delegate->OnJoinDbCacheServerComplete(info);
		}
	}

	void CDbCacheClient2Impl::OnLeaveServer(ErrorInfo *errorInfo)
	{
		// 유저에게 결과 노티를 보낸다.
		m_delegate->OnLeaveDbCacheServer(errorInfo->m_errorType);

		// 모든 데이터의 언로드 처리를 시행한다.
		m_loadedDataList.Clear();
		m_loadedDataListBySession.Clear();
		m_unloadRequests.Clear();
	}

	void CDbCacheClient2Impl::OnWarning(Proud::ErrorInfo *errorInfo)
	{
		NTTNTRACE("DB cache client에서 경고 발생: %s\n", StringT2A(errorInfo->ToString()).GetString());

		m_delegate->OnWarning(errorInfo);
	}

	void CDbCacheClient2Impl::OnError(ErrorInfo *errorInfo)
	{
		NTTNTRACE("DB cache client에서 오류 발생: %s\n", StringT2A(errorInfo->ToString()).GetString());

		m_delegate->OnError(errorInfo);
	}

	void CDbCacheClient2Impl::OnException(const Exception &e)
	{
		NTTNTRACE("DB cache client에서 Exception 발생: %s\n", e.what());

		m_delegate->OnException(e);
	}

	//////////////////////////////////////////////////////////////////////////
	// internal function
	Proud::CLoadedData2Ptr CDbCacheClient2Impl::GetOrgLoadedDataBySessionGuid(const Guid& sessionGuid) const
	{
		AssureCSLock();

		LoadedData2List::iterator iter = m_loadedDataListBySession.find(sessionGuid);
		if ( iter != m_loadedDataListBySession.end() )
			return iter->GetSecond();

		return CLoadedData2Ptr();
	}

	// root UUID를 근거로 loaded data 객체를 찾는다.
	CLoadedData2* CDbCacheClient2Impl::GetOrgLoadedDataByRootUUID(const Guid& rootUUID) const
	{
		AssureCSLock();

		if ( rootUUID == Guid() )
			return CLoadedData2Ptr();


		CLoadedData2Ptr output;
		if ( m_loadedDataList.Lookup(rootUUID, output) )
			return output;

		return CLoadedData2Ptr();
	}

	void CDbCacheClient2Impl::AssureCSLock() const 
	{
		if ( !IsCriticalSectionLockedByCurrentThread(m_cs) )
		{
			stringstream ss;
			ss << __FUNCTION__ << ": critical section is not locked!";
			throw Exception(ss.str().c_str());
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// interface to CDbCacheClient

	bool CDbCacheClient2Impl::RequestAddData(Guid rootUUID, Guid ownerUUID, CPropNodePtr addData, intptr_t tag)
	{
		CriticalSectionLock lock(m_cs, true);

		// RootUUID가 해당 DB Cache Client에 Load된 RootUUID인지 확인한다.
		CLoadedData2* loadedData = GetOrgLoadedDataByRootUUID(rootUUID);
		if ( loadedData == NULL )
			return false;

		// 해당 트리에서 OwnerUUID가 존재하는지 확인한다.
		CPropNode* parentNode = loadedData->GetNode(ownerUUID);
		if ( parentNode == NULL )
			return false;

		// 새로운 노드의 TableName이 없으면 안된다.
		String addData_(addData->GetTypeName());
		if ( addData_.CompareNoCase(_PNT("")) == 0 )
		//if ( !Tstrcmp(addData->GetTypeName(), _PNT("")) )
			return false;

		addData->m_UUID = Win32Guid::NewGuid();
		addData->m_RootUUID = rootUUID;
		addData->m_ownerUUID = ownerUUID;

		ByteArray addDataBlock;
		addData->ToByteArray(addDataBlock);

		// 서버에 요청한다.
		m_c2sProxy.RequestAddData(HostID_Server, g_ReliableSendForPN, rootUUID, ownerUUID, addDataBlock, (int64_t)tag, false);
		return true;
	}

	bool CDbCacheClient2Impl::RequestUpdateData(CPropNodePtr updateData, intptr_t tag)
	{
		CriticalSectionLock lock(m_cs, true);

		// Update를 하는 노드의 Root Node가 로드되어 있는지 확인
		CLoadedData2* oldOwnerData = GetOrgLoadedDataByRootUUID(updateData->RootUUID);
		if ( oldOwnerData == NULL )
			return false;

		// 로드된 데이터에서 Update를 실행하는 노드가 있는지 확인
		CPropNode* oldData=oldOwnerData->GetNode(updateData->UUID);
		if ( oldData == NULL )
			return false;

		ByteArray block;
		updateData->ToByteArray(block);

		// 서버에 요청한다.
		m_c2sProxy.RequestUpdateData(HostID_Server, g_ReliableSendForPN, oldOwnerData->RootUUID, block, (int64_t)tag, false);

		return true;
	}

	bool CDbCacheClient2Impl::RequestRecursiveUpdateData(CLoadedData2Ptr loadedData, intptr_t tag, bool transactional /*= true*/)
	{

		if ( loadedData == NULL )
			return false;

		CriticalSectionLock lock(m_cs, true);

		// Update를 하는 Root Node가 로드되어 있는지 확인
		CLoadedData2* oldOwnerData = GetOrgLoadedDataByRootUUID(loadedData->RootUUID);
		if ( oldOwnerData == NULL )
			return false;

		// 서버에 요청한다.
		ByteArray userblock;

		//olddata를 호출하도록 변경 이유: 외부에서 들어온데이터는 신뢰 할수 없기때문이다.
		loadedData->_INTERNAL_ChangeToByteArray(userblock);

		//보낸후에 dirtyflag와 삭제리스트를 삭제한다.
		loadedData->_INTERNAL_ClearChangeNode();

		m_c2sProxy.RequestUpdateDataList(HostID_Server, g_ReliableSendForPN, oldOwnerData->RootUUID, userblock, (int64_t)tag, transactional, false);
		return true;
	}

	//하위를 모두 지울것이다...
	//만일 root를 지운다면 warning를 띄우고, unload를 호출해야 하겠다.
	bool CDbCacheClient2Impl::RequestRemoveData(Guid rootUUID, Guid removeUUID, intptr_t tag)
	{
		CriticalSectionLock lock(m_cs, true);

		CLoadedData2* data = GetOrgLoadedDataByRootUUID(rootUUID);
		if ( data == NULL )
			return false;

		CPropNode* removeData = data->GetNode(removeUUID);
		if ( removeData == NULL )
			return false;

		// 서버에 전송
		m_c2sProxy.RequestRemoveData(HostID_Server, g_ReliableSendForPN, rootUUID, removeUUID, (int64_t)tag, false);
		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// one side method
	bool CDbCacheClient2Impl::UnilateralAddData(Guid rootUUID, Guid ownerUUID, CPropNodePtr addData, bool writeDbmsImmediately /*= true*/)
	{
		CriticalSectionLock lock(m_cs, true);

		CLoadedData2* loadedData = GetOrgLoadedDataByRootUUID(rootUUID);
		if ( loadedData == NULL )
			return false;

		CPropNodePtr parentNode = loadedData->GetNode(ownerUUID);
		if ( parentNode == NULL )
			return false;

		addData->m_UUID = Win32Guid::NewGuid();
		addData->m_RootUUID = rootUUID;
		addData->m_ownerUUID = ownerUUID;

		//local에 먼저 변화를 준다.
		CPropNodePtr newAddData = addData->CloneNoChildren();

		if ( ErrorType_Ok != loadedData->InsertChild(parentNode, newAddData) )
			return false;

		loadedData->_INTERNAL_ClearChangeNode();

		ByteArray addDataBlock;
		newAddData->ToByteArray(addDataBlock);

		loadedData->_INTERNAL_ClearChangeNode();

		m_c2sProxy.AddData(HostID_Server, g_ReliableSendForPN, rootUUID, ownerUUID, addDataBlock, writeDbmsImmediately);

		return true;
	}

	bool CDbCacheClient2Impl::UnilateralUpdateData(CPropNodePtr updateData, bool writeDbmsImmediately /*= true*/)
	{
		CriticalSectionLock lock(m_cs, true);

		CLoadedData2* oldOwnerData= GetOrgLoadedDataByRootUUID(updateData->RootUUID);
		if ( oldOwnerData == NULL )
			return false;

		CPropNode* oldData=oldOwnerData->GetNode(updateData->UUID);
		if ( oldData == NULL )
			return false;

		*oldData = (CProperty)*updateData;

		// 서버에 요청한다.
		ByteArray block;
		oldData->ToByteArray(block);

		m_c2sProxy.UpdateData(HostID_Server, g_ReliableSendForPN, oldOwnerData->RootUUID, block, writeDbmsImmediately);

		return true;
	}

	bool CDbCacheClient2Impl::UnilateralRecursiveUpdateData(CLoadedData2Ptr loadedData, bool transactional /*= true*/, bool writeDbmsImmediately /*= true*/)
	{
		if ( !loadedData )
			return false;

		CriticalSectionLock lock(m_cs, true);

		CLoadedData2* oldOwnerData= GetOrgLoadedDataByRootUUID(loadedData->RootUUID);

		if ( oldOwnerData == NULL )
			return false;

		// 로컬 데이터 갱신
		loadedData->_INTERNAL_NOACCESSMODE_CopyTo_Diff(*oldOwnerData);

		// 서버에 요청한다.
		ByteArray userblock;

		//olddata를 호출하도록 변경 이유: 외부에서 들어온데이터는 신뢰 할수 없기때문이다.
		oldOwnerData->_INTERNAL_ChangeToByteArray(userblock);

		//보낸후에 dirtyflag와 삭제리스트를 삭제한다.
		oldOwnerData->_INTERNAL_ClearChangeNode();
		loadedData->_INTERNAL_ClearChangeNode();

		m_c2sProxy.UpdateDataList(HostID_Server, g_ReliableSendForPN, oldOwnerData->RootUUID, userblock, transactional, writeDbmsImmediately);
		return true;
	}

	bool CDbCacheClient2Impl::UnilateralMoveData(String rootTableName, Guid rootUUID, Guid moveNodeUUID, Guid destRootUUID, Guid destNodeUUID, bool writeDbmsImmediately)
	{
		//루트 노드를 이동시키려 한다면 Error를 낸다. 
		if ( moveNodeUUID == rootUUID )
			return false;

		// Root Table Name이 없으면 Destination Node가 Load되어 있지 않을 시 Load할 수 없다.
		if ( rootTableName == String() )
			return false;

		CriticalSectionLock lock(m_cs, true);

		CLoadedData2* srcData = GetOrgLoadedDataByRootUUID(rootUUID);
		if ( srcData == NULL )
			return false;

		CPropNodePtr srcNode = srcData->GetNode(moveNodeUUID);
		if ( srcNode == NULL )
			return false;

		CLoadedData2* destData = GetOrgLoadedDataByRootUUID(destRootUUID);

		CPropNodePtr destNode = CPropNodePtr();

		if ( destData != NULL )
		{
			// DestRootUUID가 현재의 DB Cache Client에 이미 Load된 상태이다.
			destNode = destData->GetNode(destNodeUUID);

			// DestRootUUID를 찾았지만 Node를 찾을 수 없다니~!!
			if ( destNode == NULL )
			{
				// 사용자에게 실패를 노티
				return false;
			}
		}

		// Self일 때만 Cache Client에서 먼저 로컬 메모리의 MoveNode를 실행한다.
		// 주의: Local to Local에서 MoveNode를 실행하면 나중에 NotifySomeoneAddData를 받아야 되는데
		// 이미 Local에서 작업이 끝난 내용이기 때문에 작업이 무시된다.
		// 따라서 Local to Local에서는 srcNode의 데이터만 삭제하고 서버를 통해서 데이터를 받는다.
		if ( destNode != NULL && rootUUID == destRootUUID )
		{
			ErrorType ret;

			ret = srcData->MovePropNode(*destData, srcNode, destNode);

			if ( ret != ErrorType_Ok )
			{
				ErrorInfo info;
				info.m_comment = _PNT("Error : UnilateralMoveData MovePropNode Fail");
				info.m_errorType = ret;

				lock.Unlock();

				m_delegate->OnWarning(&info);

				return false;
			}

		}
		else
		{
			// 서버에 요청했으므로 이제 local 에서 삭제. 하지만 서버에서 처리하므로 removelist에는 추가하지 않아도 됩니다.
			ErrorType ret;
			ret = srcData->RemoveNode(srcNode, false);
			if ( ret != ErrorType_Ok )
			{
				ErrorInfo info;
				info.m_comment = _PNT("Error : UnilateralMoveData RemoveNode Fail");
				info.m_errorType = ret;

				lock.Unlock();

				m_delegate->OnWarning(&info);

				return false;
			}

		}

		// 서버에 요청합니다.
		m_c2sProxy.MoveData(HostID_Server, g_ReliableSendForPN, rootTableName, rootUUID, moveNodeUUID, destRootUUID, destNodeUUID, writeDbmsImmediately);

		srcData->_INTERNAL_ClearChangeNode();

		return true;
	}

	bool CDbCacheClient2Impl::UnilateralRemoveData(Guid rootUUID, Guid removeUUID, bool writeDbmsImmediately /*= true*/)
	{
		CriticalSectionLock lock(m_cs, true);

		CLoadedData2* oldOwnerData = GetOrgLoadedDataByRootUUID(rootUUID);
		if ( oldOwnerData == NULL )
			return false;

		CPropNodePtr oldData=oldOwnerData->GetNode(removeUUID);
		if ( oldData == NULL )
			return false;

		// 서버에 요청한다.	
		m_c2sProxy.RemoveData(HostID_Server, g_ReliableSendForPN, oldOwnerData->RootUUID, removeUUID, writeDbmsImmediately);

		// 서버에 요청했으므로 이제 local 에서 삭제. 하지만 서버에서 처리하므로 removelist에는 추가하지 않아도 된다.
		ErrorType ret;
		ret = oldOwnerData->RemoveNode(oldData, false);
		if ( ret != ErrorType_Ok )
		{
			ErrorInfo info;
			info.m_comment = _PNT("Error : UnilateralRemoveData failed to remove node.");
			info.m_errorType = ret;

			lock.Unlock();

			m_delegate->OnWarning(&info);

			return false;
		}

		//change된것을 clear한다.
		oldOwnerData->_INTERNAL_ClearChangeNode();

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// nonexclusive method

	void CDbCacheClient2Impl::RequestNonExclusiveSnapshotData(String rootTableName, String fieldName, CVariant cmpValue, intptr_t tag)
	{
		rootTableName.MakeUpper();

		// 서버에 요청한다.
		m_c2sProxy.RequestNonExclusiveSnapshotDataByFieldNameAndValue(HostID_Server, g_ReliableSendForPN, rootTableName, fieldName, cmpValue, (int64_t)tag);
	}

	void CDbCacheClient2Impl::RequestNonExclusiveSnapshotDataByGuid(String rootTableName, Guid rootUUID, intptr_t tag)
	{
		rootTableName.MakeUpper();

		// 서버에 요청한다.
		m_c2sProxy.RequestNonExclusiveSnapshotDataByGuid(HostID_Server, g_ReliableSendForPN, rootTableName, rootUUID, (int64_t)tag);
	}

	void CDbCacheClient2Impl::RequestNonExclusiveSnapshotDataByQuery(String rootTableName, String searchString, intptr_t tag)
	{
		rootTableName.MakeUpper();

		// 서버에 요청한다.
		m_c2sProxy.RequestNonExclusiveSnapshotDataByQuery(HostID_Server, g_ReliableSendForPN, rootTableName, searchString, (int64_t)tag);
	}

	void CDbCacheClient2Impl::RequestNonExclusiveAddData(String rootTableName, Guid rootUUID, Guid ownerUUID, CPropNodePtr addData, intptr_t tag, const ByteArray& message)
	{
		CriticalSectionLock lock(m_cs, true);

		if ( rootUUID==Guid() )
		{
			ThrowInvalidArgumentException();
		}

		if ( ownerUUID == Guid() )
		{
			ThrowInvalidArgumentException();
		}

		String addData_(addData->GetTypeName());
		if (addData_.CompareNoCase(_PNT("")) == 0 )
		//if ( !Tstrcmp(addData->GetTypeName(), _PNT("")) )
		{
			ThrowInvalidArgumentException();
		}

		rootTableName.MakeUpper();

		addData->m_RootUUID = rootUUID;
		addData->m_ownerUUID = ownerUUID;
		addData->m_UUID = Win32Guid::NewGuid();

		ByteArray dataBlock;
		addData->ToByteArray(dataBlock);

		// 서버에 전송
		m_c2sProxy.RequestNonExclusiveAddData(HostID_Server, g_ReliableSendForPN, rootTableName, rootUUID, ownerUUID, dataBlock, (int64_t)tag, message);
	}

	void CDbCacheClient2Impl::RequestNonExclusiveRemoveData(String rootTableName, Guid rootUUID, Guid removeUUID, intptr_t tag, const ByteArray& message)
	{
		rootTableName.MakeUpper();

		// 서버에 전송
		m_c2sProxy.RequestNonExclusiveRemoveData(HostID_Server, g_ReliableSendForPN, rootTableName, rootUUID, removeUUID, (int64_t)tag, message);
	}

	void CDbCacheClient2Impl::RequestNonExclusiveSetValueIf(String rootTableName, Guid rootUUID, Guid nodeUUID, String propertyName, CVariant newValue, ValueCompareType compareType, CVariant compareValue, intptr_t tag, const Proud::ByteArray& message)
	{
		rootTableName.MakeUpper();

		m_c2sProxy.RequestNonExclusiveSetValueIf(HostID_Server, g_ReliableSendForPN, rootTableName, rootUUID, nodeUUID, propertyName, newValue, compareType, compareValue, (int64_t)tag, message);
	}

	void CDbCacheClient2Impl::RequestNonExclusiveModifyValue(String rootTableName, Guid rootUUID, Guid nodeUUID, String propertyName, ValueOperType operType, CVariant value, intptr_t tag, const Proud::ByteArray& message)
	{
		rootTableName.MakeUpper();

		m_c2sProxy.RequestNonExclusiveModifyValue(HostID_Server, g_ReliableSendForPN, rootTableName, rootUUID, nodeUUID, propertyName, operType, value, (int64_t)tag, message);
	}

	//////////////////////////////////////////////////////////////////////////
	//rmi stub
	DEFRMI_DB2S2C_NotifyDbCacheClientLogonFailed(CDbCacheClient2Impl)
	{
		_pn_unused(rmiContext);

		// 유저에게 결과 노티를 보낸다.
		ErrorInfo info;
		info.m_errorType = reason;
		info.m_remote = remote;
		info.m_comment = _PNT("Certification Failed to DB Cache");

		m_delegate->OnJoinDbCacheServerComplete(&info);
		return true;
	}

	DEFRMI_DB2S2C_NotifyDbCacheClientLogonSuccess(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		{
			CriticalSectionLock lock(m_cs, true);
			m_loggedOn=true;
		}

		// 유저에게 결과 노티를 보낸다.
		ErrorInfo info;

		m_delegate->OnJoinDbCacheServerComplete(&info);
		return true;
	}

	DEFRMI_DB2S2C_NotifyAddDataFailed(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		CriticalSectionLock lock(m_cs, true);

		intptr_t castTag = (intptr_t)tag;

		// Blocked 계열 함수에 대한 응답인지 체크 
		if ( blocked )
		{
			// Blocked의 경우 해당 이벤트를 찾아서 알림
			BlockedEventPtr blockEvent;
			if ( m_activeEvents.TryGetValue(castTag, blockEvent) )
			{
				blockEvent->m_success = false;
				blockEvent->m_errorType = reason;
				blockEvent->m_comment = comment;
				blockEvent->m_hResult = (HRESULT)hresult;
				blockEvent->m_source = source;
				
				blockEvent->m_event->Set();
				lock.Unlock();
			}
			else
			{
				ErrorInfo info;
				info.m_errorType = ErrorType_Unexpected;
				info.m_comment = _PNT("blocked method but no active event");

				lock.Unlock();

				m_delegate->OnWarning(&info);
			}
		}
		else
		{
			// Blocked가 아닌 경우 콜백을 통해 관련 정보를 그대로 전달
			IDbCacheClientDelegate2::CCallbackArgs args;
			args.m_items.SetCount(1);

			args.m_tag = castTag;
			args.m_items[0].m_reason = reason;
			args.m_items[0].m_comment = comment;
			args.m_items[0].m_hResult = (HRESULT)hresult;
			args.m_items[0].m_source = source;

			lock.Unlock();

			m_delegate->OnAddDataFailed(args);
		}
		return true;
	}

	DEFRMI_DB2S2C_NotifyAddDataSuccess(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		CriticalSectionLock lock(m_cs, true);

		// 받은 add 데이터를 로컬 메모리에 저장한다.		
		CPropNodePtr addData(new CPropNode(_PNT("")));
		addData->FromByteArray(addDataBlock);

		CLoadedData2* loadData = GetOrgLoadedDataByRootUUID(rootUUID);
		if ( loadData == NULL )
			return true;	// 이미 unload된 상태에서 뒷북으로 온 메시지이므로 그냥 버린다.

		if ( addData->UUID==Guid() || addData->OwnerUUID==Guid() || addData->TypeName==_PNT("") )
		{
			stringstream ss;
			ss << __FUNCTION__ << ": tree or node GUID or table name is empty!";
			throw Exception(ss.str().c_str());
		}

		CPropNodePtr ownerData = loadData->GetNode(addData->OwnerUUID);

		if ( ownerData == NULL )
		{
			ErrorInfo info;
			info.m_errorType = ErrorType_Unexpected;
			info.m_comment = _PNT("Error : NotifyAddDataSuccess: OwnerNode not found.");

			lock.Unlock();

			m_delegate->OnWarning(&info);
			return true;
		}

		ErrorType ret = loadData->InsertChild(ownerData, addData);
		if ( ret != ErrorType_Ok )
		{
			ErrorInfo info;
			info.m_errorType = ErrorType_Unexpected;
			info.m_comment = _PNT("Error : NotifyAddDataSuccess: InsertChild failed.");

			lock.Unlock();

			m_delegate->OnWarning(&info);
			return true;
		}

		loadData->_INTERNAL_ClearChangeNode();

		intptr_t castTag = (intptr_t)tag;

		// Blocked 계열 함수에 대한 응답인지 체크 
		if ( blocked )
		{
			// Blocked의 경우 해당 이벤트를 찾아서 알림
			BlockedEventPtr blockEvent;
			if ( m_activeEvents.TryGetValue(castTag, blockEvent) )
			{
				blockEvent->m_success = true;

				blockEvent->m_event->Set();
				lock.Unlock();
			}
			else
			{
				ErrorInfo info;
				info.m_errorType = ErrorType_Unexpected;
				info.m_comment = _PNT("blocked method but no active event");

				lock.Unlock();

				m_delegate->OnWarning(&info);
			}
		}
		else
		{
			// Blocked가 아닌 경우 콜백을 통해 관련 정보를 그대로 전달
			CLoadedData2Ptr clonedData=loadData->Clone();
			IDbCacheClientDelegate2::CCallbackArgs args;
			args.m_items.SetCount(1);

			args.m_tag = castTag;
			args.m_items[0].m_loadedData = clonedData;
			args.m_items[0].m_data = clonedData->GetNode(addData->UUID);
			args.m_items[0].m_sessionGuid = clonedData->sessionGuid;
			args.m_items[0].m_rootUUID = clonedData->RootUUID;

			lock.Unlock();

			m_delegate->OnAddDataSuccess(args);
		}

		return true;
	}

	DEFRMI_DB2S2C_NotifyUpdateDataFailed(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		CriticalSectionLock lock(m_cs, true);

		intptr_t castTag = (intptr_t)tag;

		// Blocked 계열 함수에 대한 응답인지 체크 
		if ( blocked )
		{
			// Blocked의 경우 해당 이벤트를 찾아서 알림
			BlockedEventPtr blockEvent;
			if ( m_activeEvents.TryGetValue(castTag, blockEvent) )
			{
				blockEvent->m_success = false;
				blockEvent->m_errorType = reason;
				blockEvent->m_comment = comment;
				blockEvent->m_hResult = (HRESULT)hresult;
				blockEvent->m_source = source;

				blockEvent->m_event->Set();
				lock.Unlock();
			}
			else
			{
				ErrorInfo info;
				info.m_errorType = ErrorType_Unexpected;
				info.m_comment = _PNT("blocked method but no active event");

				lock.Unlock();

				m_delegate->OnWarning(&info);
			}
		}
		else
		{
			// Blocked가 아닌 경우 콜백을 통해 관련 정보를 그대로 전달
			IDbCacheClientDelegate2::CCallbackArgs args;
			args.m_items.SetCount(1);

			args.m_tag = castTag;
			args.m_items[0].m_reason = reason;
			args.m_items[0].m_comment = comment;
			args.m_items[0].m_hResult = (HRESULT)hresult;
			args.m_items[0].m_source = source;
			lock.Unlock();

			m_delegate->OnUpdateDataFailed(args);
		}
		return true;
	}

	DEFRMI_DB2S2C_NotifyUpdateDataSuccess(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		CriticalSectionLock lock(m_cs, true);

		// 받은 update 데이터를 로컬 메모리에 저장한다.		
		CPropNode updateData(_PNT(""));
		updateData.FromByteArray(updataDataBlock);

		CLoadedData2* loadData = GetOrgLoadedDataByRootUUID(rootUUID);
		if ( loadData == NULL )
			return true;	// 이미 unload된 상태에서 뒷북으로 온 메시지이므로 그냥 버린다.

		if ( updateData.UUID==Guid() || updateData.OwnerUUID==Guid() || updateData.TypeName==_PNT("") )
		{
			stringstream ss;
			ss << __FUNCTION__ << ": Incorrect guid or tablename!";
			throw Exception(ss.str().c_str());
		}

		//local update.
		CPropNodePtr orgData = loadData->GetNode(updateData.UUID);

		*orgData = (CProperty)updateData;

		intptr_t castTag = (intptr_t)tag;

		// Blocked 계열 함수에 대한 응답인지 체크 
		if ( blocked )
		{
			// Blocked의 경우 해당 이벤트를 찾아서 알림
			BlockedEventPtr blockEvent;
			if ( m_activeEvents.TryGetValue(castTag, blockEvent) )
			{
				blockEvent->m_success = true;
				
				blockEvent->m_event->Set();
				lock.Unlock();
			}
			else
			{
				ErrorInfo info;
				info.m_errorType = ErrorType_Unexpected;
				info.m_comment = _PNT("blocked method but no active event");

				lock.Unlock();

				m_delegate->OnWarning(&info);
			}
		}
		else
		{
			// Blocked가 아닌 경우 콜백을 통해 관련 정보를 그대로 전달
			CLoadedData2Ptr clonedData=loadData->Clone();
			IDbCacheClientDelegate2::CCallbackArgs args;
			args.m_items.SetCount(1);

			args.m_tag = castTag;
			args.m_items[0].m_loadedData = clonedData;
			args.m_items[0].m_data = clonedData->GetNode(updateData.UUID);
			args.m_items[0].m_rootUUID = clonedData->RootUUID;
			args.m_items[0].m_sessionGuid = clonedData->sessionGuid;

			lock.Unlock();
			m_delegate->OnUpdateDataSuccess(args);
		}

		return true;
	}

	DEFRMI_DB2S2C_NotifyUpdateDataListFailed(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		CriticalSectionLock lock(m_cs, true);

		intptr_t castTag = (intptr_t)tag;

		// Blocked 계열 함수에 대한 응답인지 체크 
		if ( blocked )
		{
			// Blocked의 경우 해당 이벤트를 찾아서 알림
			BlockedEventPtr blockEvent;
			if ( m_activeEvents.TryGetValue(castTag, blockEvent) )
			{
				blockEvent->m_success = false;
				blockEvent->m_errorType = reason;
				blockEvent->m_comment = comment;
				blockEvent->m_hResult = (HRESULT)hresult;
				blockEvent->m_source = source;

				blockEvent->m_event->Set();
				lock.Unlock();
			}
			else
			{
				ErrorInfo info;
				info.m_errorType = ErrorType_Unexpected;
				info.m_comment = _PNT("blocked method but no active event");

				lock.Unlock();

				m_delegate->OnWarning(&info);
			}
		}
		else
		{
			// Blocked가 아닌 경우 콜백을 통해 관련 정보를 그대로 전달
			IDbCacheClientDelegate2::CCallbackArgs args;
			args.m_items.SetCount(1);

			args.m_tag = castTag;
			args.m_items[0].m_reason = reason;
			args.m_items[0].m_comment = comment;
			args.m_items[0].m_hResult = (HRESULT)hresult;
			args.m_items[0].m_source = source;
			lock.Unlock();

			m_delegate->OnUpdateDataFailed(args);
		}
		return true;
	}

	DEFRMI_DB2S2C_NotifyUpdateDataListSuccess(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		CriticalSectionLock lock(m_cs, true);

		CLoadedData2* loadData = GetOrgLoadedDataByRootUUID(rootUUID);
		if ( !loadData )
			return true;

		// 로컬 데이터 갱신
		loadData->_INTERNAL_FromByteArrayToChangeList(updateDataBlock);
		loadData->_INTERNAL_ClearChangeNode();

		intptr_t castTag = (intptr_t)tag;

		// Blocked 계열 함수에 대한 응답인지 체크 
		if ( blocked )
		{
			// Blocked의 경우 해당 이벤트를 찾아서 알림
			BlockedEventPtr blockEvent;
			if ( m_activeEvents.TryGetValue(castTag, blockEvent) )
			{
				blockEvent->m_success = true;

				blockEvent->m_event->Set();
				lock.Unlock();
			}
			else
			{
				ErrorInfo info;
				info.m_errorType = ErrorType_Unexpected;
				info.m_comment = _PNT("blocked method but no active event");

				lock.Unlock();

				m_delegate->OnWarning(&info);
			}
		}
		else
		{
			// Blocked가 아닌 경우 콜백을 통해 관련 정보를 그대로 전달
			CLoadedData2Ptr clonedData=loadData->Clone();
			IDbCacheClientDelegate2::CCallbackArgs args;
			args.m_items.SetCount(1);

			args.m_tag = castTag;
			args.m_items[0].m_loadedData = clonedData;
			args.m_items[0].m_rootUUID = clonedData->RootUUID;
			args.m_items[0].m_sessionGuid = clonedData->sessionGuid;
			lock.Unlock();

			m_delegate->OnUpdateDataSuccess(args);
		}

		return true;
	}

	DEFRMI_DB2S2C_NotifyRemoveDataFailed(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		CriticalSectionLock lock(m_cs, true);

		intptr_t castTag = (intptr_t)tag;

		// Blocked 계열 함수에 대한 응답인지 체크 
		if ( blocked )
		{
			// Blocked의 경우 해당 이벤트를 찾아서 알림
			BlockedEventPtr blockEvent;
			if ( m_activeEvents.TryGetValue(castTag, blockEvent) )
			{
				blockEvent->m_success = false;
				blockEvent->m_errorType = reason;
				blockEvent->m_comment = comment;
				blockEvent->m_hResult = (HRESULT)hresult;
				blockEvent->m_source = source;

				blockEvent->m_event->Set();
				lock.Unlock();
			}
			else
			{
				ErrorInfo info;
				info.m_errorType = ErrorType_Unexpected;
				info.m_comment = _PNT("blocked method but no active event");

				lock.Unlock();

				m_delegate->OnWarning(&info);
			}
		}
		else
		{
			// Blocked가 아닌 경우 콜백을 통해 관련 정보를 그대로 전달
			IDbCacheClientDelegate2::CCallbackArgs args;
			args.m_items.SetCount(1);

			args.m_tag = castTag;
			args.m_items[0].m_reason = reason;
			args.m_items[0].m_comment = comment;
			args.m_items[0].m_hResult = (HRESULT)hresult;
			args.m_items[0].m_source = source;

			lock.Unlock();

			m_delegate->OnRemoveDataFailed(args);
		}
		return true;
	}

	DEFRMI_DB2S2C_NotifyRemoveDataSuccess(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		CriticalSectionLock lock(m_cs, true);

		CLoadedData2* loadData = GetOrgLoadedDataByRootUUID(rootUUID);
		if ( loadData == NULL )
			return true;	// 이미 unload된 상태에서 뒷북으로 온 메시지이므로 그냥 버린다.

		CPropNodePtr removeNode = loadData->GetNode(removedUUID);

		if ( removeNode == NULL )
		{
			stringstream ss;
			ss << __FUNCTION__ << ": Incorrect guid!";
			throw Exception(ss.str().c_str());
		}

		//local update.
		loadData->RemoveNode(removeNode);

		//change된것을 clear한다.
		loadData->_INTERNAL_ClearChangeNode();

		intptr_t castTag = (intptr_t)tag;

		// Blocked 계열 함수에 대한 응답인지 체크 
		if ( blocked )
		{
			// Blocked의 경우 해당 이벤트를 찾아서 알림
			BlockedEventPtr blockEvent;
			if ( m_activeEvents.TryGetValue(castTag, blockEvent) )
			{
				blockEvent->m_success = true;

				blockEvent->m_event->Set();
				lock.Unlock();
			}
			else
			{
				ErrorInfo info;
				info.m_errorType = ErrorType_Unexpected;
				info.m_comment = _PNT("blocked method but no active event");

				lock.Unlock();

				m_delegate->OnWarning(&info);
			}
		}
		else
		{
			// Blocked가 아닌 경우 콜백을 통해 관련 정보를 그대로 전달
			CLoadedData2Ptr clonedData=loadData->Clone();
			IDbCacheClientDelegate2::CCallbackArgs args;
			args.m_items.SetCount(1);
						
			args.m_tag = castTag;
			args.m_items[0].m_data = removeNode;
			args.m_items[0].m_loadedData = clonedData;
			args.m_items[0].m_sessionGuid = clonedData->sessionGuid;
			args.m_items[0].m_rootUUID = clonedData->RootUUID;

			lock.Unlock();

			m_delegate->OnRemoveDataSuccess(args);
		}

		return true;
	}

	DEFRMI_DB2S2C_NotifyNonExclusiveAddDataFailed(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		IDbCacheClientDelegate2::CCallbackArgs args;
		args.m_items.SetCount(1);

		args.m_tag = (intptr_t)tag;
		args.m_items[0].m_reason = reason;
		args.m_items[0].m_comment = comment;
		args.m_items[0].m_hResult = (HRESULT)hresult;
		args.m_items[0].m_source = source;

		m_delegate->OnNonExclusiveAddDataAck(args);
		return true;
	}
	DEFRMI_DB2S2C_NotifyNonExclusiveAddDataSuccess(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		CPropNodePtr addData = CPropNodePtr(new CPropNode(_PNT("")));
		addData->FromByteArray(addDataBlock);

		IDbCacheClientDelegate2::CCallbackArgs args;
		args.m_items.SetCount(1);

		args.m_tag = (intptr_t)tag;
		args.m_items[0].m_rootUUID = rootUUID;
		args.m_items[0].m_data = addData;

		m_delegate->OnNonExclusiveAddDataAck(args);
		return true;
	}

	DEFRMI_DB2S2C_NotifySomeoneAddData(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);
		_pn_unused(destParentUUID);

		CriticalSectionLock lock(m_cs, true);

		CLoadedData2* loadData = GetOrgLoadedDataByRootUUID(rootUUID);

		if ( loadData == NULL )
			return true;	// 이미 unload된 상태에서 뒷북으로 온 메시지이므로 그냥 버린다.

		CMessage msg;
		msg.UseExternalBuffer((uint8_t *)addDataBlock.GetData(), addDataBlock.GetCount());
		msg.SetLength(addDataBlock.GetCount());

		int nodeCount = 0;
		msg >> nodeCount;
		Guid addDataGuid;

		for ( int i = 0; i < nodeCount; i++ )
		{
			CPropNodePtr ptr = CPropNodePtr(new CPropNode(_PNT("")));
			msg >> *ptr;

			if ( ptr->UUID == Guid() || ptr->OwnerUUID == Guid() || ptr->TypeName == _PNT("") )
			{
				stringstream ss;
				ss << __FUNCTION__ << ": Incorrect guid or tablename!";
				throw Exception(ss.str().c_str());
			}

			CPropNodePtr ownerData;

			ownerData = loadData->GetNode(ptr->OwnerUUID);

			if ( ownerData == NULL )
				throw Exception("OwnerData not found");

			loadData->InsertChild(ownerData, ptr);

			if ( i == 0 )
				addDataGuid = ptr->UUID;
		}

		loadData->_INTERNAL_ClearChangeNode();

		CLoadedData2Ptr clonedData=loadData->Clone();

		IDbCacheClientDelegate2::CCallbackArgs args;
		args.m_items.SetCount(1);

		args.m_items[0].m_loadedData = clonedData;
		args.m_items[0].m_data = clonedData->GetNode(addDataGuid);
		args.m_items[0].m_message = message;
		args.m_items[0].m_sessionGuid = clonedData->sessionGuid;
		args.m_items[0].m_rootUUID = clonedData->RootUUID;

		lock.Unlock();

		m_delegate->OnSomeoneAddData(args);
		return true;
	}

	DEFRMI_DB2S2C_NotifyNonExclusiveRemoveDataFailed(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		IDbCacheClientDelegate2::CCallbackArgs args;
		args.m_items.SetCount(1);

		args.m_tag = (intptr_t)tag;
		args.m_items[0].m_reason = reason;
		args.m_items[0].m_comment = comment;
		args.m_items[0].m_hResult = (HRESULT) hresult;
		args.m_items[0].m_source = source;

		m_delegate->OnNonExclusiveRemoveDataAck(args);
		return true;
	}

	DEFRMI_DB2S2C_NotifyNonExclusiveRemoveDataSuccess(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		IDbCacheClientDelegate2::CCallbackArgs args;
		args.m_items.SetCount(1);

		args.m_tag = (intptr_t)tag;
		args.m_items[0].m_rootUUID = rootUUID;
		args.m_items[0].m_UUID = removeUUID;


		m_delegate->OnNonExclusiveRemoveDataAck(args);
		return true;
	}

	DEFRMI_DB2S2C_NotifySomeoneRemoveData(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		CriticalSectionLock lock(m_cs, true);

		CLoadedData2* loadData= GetOrgLoadedDataByRootUUID(rootUUID);
		if ( loadData == NULL )
			return true;	// 이미 unload된 상태에서 뒷북으로 온 메시지이므로 그냥 버린다.

		CPropNodePtr removeData = loadData->GetNode(removeUUID);

		if ( removeData == NULL )
		{
			stringstream ss;
			ss << __FUNCTION__ << ": Incorrect guid!";
			throw Exception(ss.str().c_str());
		}

		loadData->RemoveNode(removeData);

		//change된것을 clear한다.
		loadData->_INTERNAL_ClearChangeNode();

		CLoadedData2Ptr clonedData=loadData->Clone();

		IDbCacheClientDelegate2::CCallbackArgs args;
		args.m_items.SetCount(1);

		args.m_items[0].m_loadedData = clonedData;
		args.m_items[0].m_data = removeData;
		args.m_items[0].m_message = message;
		args.m_items[0].m_loadedData = clonedData;
		args.m_items[0].m_sessionGuid = clonedData->sessionGuid;
		args.m_items[0].m_rootUUID = clonedData->RootUUID;
		
		lock.Unlock();

		m_delegate->OnSomeoneRemoveData(args);
		return true;
	}

	DEFRMI_DB2S2C_NotifyNonExclusiveSetValueFailed(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		IDbCacheClientDelegate2::CCallbackArgs args;
		args.m_items.SetCount(1);

		args.m_tag = (intptr_t)tag;
		args.m_items[0].m_reason = reason;
		args.m_items[0].m_comment = comment;
		args.m_items[0].m_hResult = (HRESULT) hresult;
		args.m_items[0].m_source = source;

		m_delegate->OnNonExclusiveSetValueIfFailed(args);
		return true;
	}
	DEFRMI_DB2S2C_NotifyNonExclusiveSetValueSuccess(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		CPropNodePtr updateData = CPropNodePtr(new CPropNode(_PNT("")));
		updateData->FromByteArray(updateDataBlock);

		IDbCacheClientDelegate2::CCallbackArgs args;
		args.m_items.SetCount(1);
		
		args.m_tag = (intptr_t)tag;
		args.m_items[0].m_rootUUID = rootUUID;
		args.m_items[0].m_data = updateData;

		m_delegate->OnNonExclusiveSetValueIfSuccess(args);
		return true;
	}

	DEFRMI_DB2S2C_NotifySomeoneSetValue(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		CriticalSectionLock lock(m_cs, true);

		CPropNode updateData(_PNT(""));
		updateData.FromByteArray(updateDataBlock);

		CLoadedData2* loadData= GetOrgLoadedDataByRootUUID(rootUUID);
		if ( loadData == NULL )
			return true;	// 이미 unload된 상태에서 뒷북으로 온 메시지이므로 그냥 버린다.


		if ( updateData.UUID==Guid() || updateData.OwnerUUID==Guid() || updateData.TypeName==_PNT("") )
		{
			stringstream ss;
			ss << __FUNCTION__ << ": Incorrect guid or table name!";
			throw Exception(ss.str().c_str());
		}

		CPropNodePtr orgData = loadData->GetNode(updateData.UUID);

		if ( !orgData )
		{
			stringstream ss;
			ss << __FUNCTION__ << ": Incorrect guid!";
			throw Exception(ss.str().c_str());
		}

		*orgData = (CProperty)updateData;

		CLoadedData2Ptr clonedData=loadData->Clone();

		IDbCacheClientDelegate2::CCallbackArgs args;
		args.m_items.SetCount(1);

		args.m_items[0].m_loadedData = clonedData;
		args.m_items[0].m_data = clonedData->GetNode(updateData.UUID);
		args.m_items[0].m_message = message;
		args.m_items[0].m_sessionGuid = clonedData->sessionGuid;
		args.m_items[0].m_rootUUID = clonedData->RootUUID;

		lock.Unlock();

		m_delegate->OnSomeoneSetValue(args);

		return true;
	}

	DEFRMI_DB2S2C_NotifyNonExclusiveModifyValueFailed(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		IDbCacheClientDelegate2::CCallbackArgs args; 
		args.m_items.SetCount(1);
		
		args.m_tag = (intptr_t)tag;
		args.m_items[0].m_reason = reason;
		args.m_items[0].m_comment = comment;
		args.m_items[0].m_hResult = (HRESULT)hresult;
		args.m_items[0].m_source = source;

		m_delegate->OnNonExclusiveModifyValueFailed(args);
		return true;
	}
	DEFRMI_DB2S2C_NotifyNonExclusiveModifyValueSuccess(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		CPropNodePtr updateData = CPropNodePtr(new CPropNode(_PNT("")));
		updateData->FromByteArray(updateDataBlock);

		IDbCacheClientDelegate2::CCallbackArgs args; 
		args.m_items.SetCount(1);

		args.m_tag = (intptr_t)tag;
		args.m_items[0].m_rootUUID = rootUUID;
		args.m_items[0].m_data = updateData;

		m_delegate->OnNonExclusiveModifyValueSuccess(args);
		return true;
	}

	DEFRMI_DB2S2C_NotifySomeoneModifyValue(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		CriticalSectionLock lock(m_cs, true);

		CPropNode updateData(_PNT(""));
		updateData.FromByteArray(updateDataBlock);

		CLoadedData2* loadData= GetOrgLoadedDataByRootUUID(rootUUID);
		if ( loadData == NULL )
			return true;	// 이미 unload된 상태에서 뒷북으로 온 메시지이므로 그냥 버린다.

		if ( updateData.UUID==Guid() || updateData.OwnerUUID==Guid() || updateData.TypeName==_PNT("") )
		{
			stringstream ss;
			ss << __FUNCTION__ << ": Incorrect guid or table name!";
			throw Exception(ss.str().c_str());
		}

		CPropNodePtr orgData = loadData->GetNode(updateData.UUID);

		if ( !orgData )
		{
			stringstream ss;
			ss << __FUNCTION__ << ": Incorrect guid!";
			throw Exception(ss.str().c_str());
		}

		*orgData = (CProperty)updateData;

		CLoadedData2Ptr clonedData=loadData->Clone();

		IDbCacheClientDelegate2::CCallbackArgs args;
		args.m_items.SetCount(1);

		args.m_items[0].m_loadedData = clonedData;
		args.m_items[0].m_data = clonedData->GetNode(updateData.UUID);
		args.m_items[0].m_message = message;
		args.m_items[0].m_rootUUID = clonedData->RootUUID;
		args.m_items[0].m_sessionGuid = clonedData->sessionGuid;

		lock.Unlock();

		m_delegate->OnSomeoneModifyValue(args);

		return true;
	}

	DEFRMI_DB2S2C_NotifyDbmsWriteDone(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		m_delegate->OnDbmsWriteDone(type, loadeeGuid);
		return true;
	}
	DEFRMI_DB2S2C_NotifyDbmsAccessError(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		IDbCacheClientDelegate2::CCallbackArgs args;
		args.m_tag = (intptr_t)tag;
		
		args.m_items.SetCount(1);
		args.m_items[0].m_message = message;
		args.m_items[0].m_comment = comment;

		m_delegate->OnAccessError(args);
		return true;
	}

	DEFRMI_DB2S2C_NotifyWarning(CDbCacheClient2Impl)
	{
		_pn_unused(remote);
		_pn_unused(rmiContext);

		ErrorInfo info;
		info.m_errorType = errorType;
		info.m_detailType = detailtype;
		info.m_comment = comment;
		NTTNTRACE("DB cache server에서 경고를 받음 : %s\n", info.ToString().GetString());

		m_delegate->OnWarning(&info);
		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// etc....
	Proud::CLoadedData2Ptr CDbCacheClient2Impl::GetClonedLoadedDataBySessionGuid(Guid sessionGuid)
	{
		CriticalSectionLock lock(m_cs, true);
		CLoadedData2* org = GetOrgLoadedDataBySessionGuid(sessionGuid);

		if ( !org )
			return CLoadedData2Ptr();

		return org->Clone();
	}


	Proud::CLoadedData2Ptr CDbCacheClient2Impl::GetClonedLoadedDataByUUID(Guid rootUUID)
	{
		CriticalSectionLock lock(m_cs, true);
		CLoadedData2* org = GetOrgLoadedDataByRootUUID(rootUUID);

		if ( !org )
			return CLoadedData2Ptr();

		return org->Clone();
	}


	Proud::HostID CDbCacheClient2Impl::GetLocalHostID()
	{
		return m_netClient->GetLocalHostID();
	}

	bool CDbCacheClient2Impl::IsLoggedOn()
	{
		return m_loggedOn;
	}

	bool CDbCacheClient2Impl::IsCurrentThreadUserWorker()
	{
		AssureCSLock();

		if ( !m_netClient )
			return false;

		CFastArray<CThreadInfo> infos;
		m_netClient->GetUserWorkerThreadInfo(infos);

		if ( infos.GetCount() == 0 )
			return false;

		uint64_t tid = Proud::GetCurrentThreadID();

		for ( int i = 0; i < (int)infos.GetCount(); i++ )
		{
			if ( infos[i].m_threadID == tid )
				return true;
		}

		return false;
	}

	void CDbCacheClient2Impl::OnTick(void* /*context*/)
	{
		CriticalSectionLock lock(m_cs, true);

		m_eventPool.ShrinkOnNeed();

		UnloadRequests warnings;
		ProcessUnloadRequestTimeout1(warnings);
		lock.Unlock();
		ProcessUnloadRequestTimeout2(warnings);
	}

	// 서버로부터 받은 Unload 요청들 중 
	// 오랫동안 처리되지 않은 세션들을 outWarnings로 옮긴다.
	// 타임아웃의 기준을 서버의 설정값으로 처리하므로 
	// 여기서 타임아웃 처리된 세션은 서버에서도 타임아웃 되었을 가능성이 높다.
	void CDbCacheClient2Impl::ProcessUnloadRequestTimeout1(UnloadRequests& outWarnings)
	{
		CriticalSectionLock lock(m_cs, true);

		int64_t nowTime = GetPreciseCurrentTimeMs();
		for (UnloadRequests::iterator it = m_unloadRequests.begin();
			it != m_unloadRequests.end();)
		{
			const Guid& session = it.GetFirst();
			RequestedTimeQueue& reqs = it.GetSecond();

			assert(GetOrgLoadedDataBySessionGuid(session) != NULL);
			assert(reqs.IsEmpty() == false);

			do
			{
				// m_unloadRequests에선 요청을 받은 시각을 저장했지만
				// outWarnings에는 요청시간과 현재시간의 차(경과시간)를 저장한다.
				int64_t elapseTime = nowTime - reqs.GetHead();
				if (elapseTime > m_unloadRequestTimeoutTimeMs)
				{
					outWarnings[session].AddTail(elapseTime);
					reqs.RemoveHead();
				}
				else
				{
					break;
				}
			} while (reqs.IsEmpty() == false);

			if (reqs.IsEmpty())
				it = m_unloadRequests.erase(it);
			else
				++it;
		}
	}

	// 인자로 받은 warnings들에 대해 경고를 콜백해준다.
	// 이 함수는 반드시 메인락이 걸리지 않은 상태에서만 호출되어야 한다.
	void CDbCacheClient2Impl::ProcessUnloadRequestTimeout2(const UnloadRequests& warnings) const
	{
		AssertIsNotLockedByCurrentThread(m_cs);

		for ( UnloadRequests::iterator it = warnings.begin();
			 it != warnings.end();
			 ++it )
		{
			const Guid& session = it.GetFirst();
			RequestedTimeQueue& reqs = it.GetSecond();
			assert( reqs.IsEmpty() == false );
			while ( reqs.IsEmpty() == false )
			{
				String msg = Proud::String::NewFormat(_PNT("Session {%s}에 대한 Unload 요청이 %lldms동안 처리되지 않았습니다."),
													  session.ToString().GetString(),
													  reqs.RemoveHead());

				m_delegate->OnWarning(ErrorInfo::From(ErrorType_TimeOut, HostID_None, msg));
			}
		}
	}

	IDbCacheClientDelegate2::CCallbackArgs::CCallbackArgs()
	{
		m_tag = NULL;
// 		m_sessionGuid = Guid();
// 		m_rootUUID = Guid();
// 		m_loadedData = CLoadedData2Ptr();
// 		m_data = CPropNodePtr();
// 		m_reason = ErrorType_Ok;
// 		m_hResult = S_OK;
	}

	IDbCacheClientDelegate2::CCallbackArgs::CItem::CItem()
	{
		m_reason = ErrorType_Ok;
		m_hResult = 0;
	}

}
