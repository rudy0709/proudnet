﻿



  
// Generated by PIDL compiler.
// Do not modify this file, but modify the source .pidl file.

#pragma once


#include "DB_common.h"

namespace DB2C2S {


	class Proxy : public ::Proud::IRmiProxy
	{
	public:
	virtual bool RequestDbCacheClientLogon ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & authenticationKey) PN_SEALED; 
	virtual bool RequestDbCacheClientLogon ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::String & authenticationKey)   PN_SEALED;  
	virtual bool RequestExclusiveLoadDataByFieldNameAndValue ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & rootTableName, const Proud::String & fieldName, const Proud::CVariant & cmpValue, const int64_t & tag, const Proud::ByteArray & message) PN_SEALED; 
	virtual bool RequestExclusiveLoadDataByFieldNameAndValue ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::String & rootTableName, const Proud::String & fieldName, const Proud::CVariant & cmpValue, const int64_t & tag, const Proud::ByteArray & message)   PN_SEALED;  
	virtual bool RequestExclusiveLoadDataByGuid ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & rootTableName, const Proud::Guid & rootUUID, const int64_t & tag, const Proud::ByteArray & message) PN_SEALED; 
	virtual bool RequestExclusiveLoadDataByGuid ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::String & rootTableName, const Proud::Guid & rootUUID, const int64_t & tag, const Proud::ByteArray & message)   PN_SEALED;  
	virtual bool RequestExclusiveLoadDataByQuery ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & rootTableName, const Proud::String & queryString, const int64_t & tag, const Proud::ByteArray & message) PN_SEALED; 
	virtual bool RequestExclusiveLoadDataByQuery ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::String & rootTableName, const Proud::String & queryString, const int64_t & tag, const Proud::ByteArray & message)   PN_SEALED;  
	virtual bool RequestExclusiveLoadNewData ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & rootTableName, const Proud::ByteArray & addDataBlock, const int64_t & tag, const bool & transaction) PN_SEALED; 
	virtual bool RequestExclusiveLoadNewData ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::String & rootTableName, const Proud::ByteArray & addDataBlock, const int64_t & tag, const bool & transaction)   PN_SEALED;  
	virtual bool RequestUnloadDataBySessionGuid ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & sessionGuid, const Proud::ByteArray & messageToNextLoader) PN_SEALED; 
	virtual bool RequestUnloadDataBySessionGuid ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & sessionGuid, const Proud::ByteArray & messageToNextLoader)   PN_SEALED;  
	virtual bool DenyUnloadData ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & sessionGuid, const Proud::ByteArray & messageToRequester) PN_SEALED; 
	virtual bool DenyUnloadData ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & sessionGuid, const Proud::ByteArray & messageToRequester)   PN_SEALED;  
	virtual bool RequestForceCompleteUnload ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & rootUUID) PN_SEALED; 
	virtual bool RequestForceCompleteUnload ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & rootUUID)   PN_SEALED;  
	virtual bool RequestAddData ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & rootUUID, const Proud::Guid & ownerUUID, const Proud::ByteArray & addDataBlock, const int64_t & tag, const bool & blocked) PN_SEALED; 
	virtual bool RequestAddData ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & rootUUID, const Proud::Guid & ownerUUID, const Proud::ByteArray & addDataBlock, const int64_t & tag, const bool & blocked)   PN_SEALED;  
	virtual bool RequestUpdateData ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & rootUUID, const Proud::ByteArray & updateDataBlock, const int64_t & tag, const bool & blocked) PN_SEALED; 
	virtual bool RequestUpdateData ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & rootUUID, const Proud::ByteArray & updateDataBlock, const int64_t & tag, const bool & blocked)   PN_SEALED;  
	virtual bool RequestRemoveData ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & rootUUID, const Proud::Guid & removeUUID, const int64_t & tag, const bool & blocked) PN_SEALED; 
	virtual bool RequestRemoveData ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & rootUUID, const Proud::Guid & removeUUID, const int64_t & tag, const bool & blocked)   PN_SEALED;  
	virtual bool RequestUpdateDataList ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & rootUUID, const Proud::ByteArray & changeBlock, const int64_t & tag, const bool & transaction, const bool & blocked) PN_SEALED; 
	virtual bool RequestUpdateDataList ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & rootUUID, const Proud::ByteArray & changeBlock, const int64_t & tag, const bool & transaction, const bool & blocked)   PN_SEALED;  
	virtual bool AddData ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & rootUUID, const Proud::Guid & ownerUUID, const Proud::ByteArray & addDataBlock, const bool & writeDbmsImmediately) PN_SEALED; 
	virtual bool AddData ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & rootUUID, const Proud::Guid & ownerUUID, const Proud::ByteArray & addDataBlock, const bool & writeDbmsImmediately)   PN_SEALED;  
	virtual bool UpdateData ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & rootUUID, const Proud::ByteArray & updateDataBlock, const bool & writeDbmsImmediately) PN_SEALED; 
	virtual bool UpdateData ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & rootUUID, const Proud::ByteArray & updateDataBlock, const bool & writeDbmsImmediately)   PN_SEALED;  
	virtual bool RemoveData ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & rootUUID, const Proud::Guid & removeUUID, const bool & writeDbmsImmediately) PN_SEALED; 
	virtual bool RemoveData ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & rootUUID, const Proud::Guid & removeUUID, const bool & writeDbmsImmediately)   PN_SEALED;  
	virtual bool UpdateDataList ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & rootUUID, const Proud::ByteArray & updateDataBlock, const bool & transactional, const bool & writeDbmsImmediately) PN_SEALED; 
	virtual bool UpdateDataList ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & rootUUID, const Proud::ByteArray & updateDataBlock, const bool & transactional, const bool & writeDbmsImmediately)   PN_SEALED;  
	virtual bool MoveData ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & rootTableName, const Proud::Guid & rootUUID, const Proud::Guid & nodeUUID, const Proud::Guid & destRootUUID, const Proud::Guid & destNodeUUID, const bool & writeDbmsImmediately) PN_SEALED; 
	virtual bool MoveData ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::String & rootTableName, const Proud::Guid & rootUUID, const Proud::Guid & nodeUUID, const Proud::Guid & destRootUUID, const Proud::Guid & destNodeUUID, const bool & writeDbmsImmediately)   PN_SEALED;  
	virtual bool RequestNonExclusiveSnapshotDataByFieldNameAndValue ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & rootTableName, const Proud::String & fieldName, const Proud::CVariant & cmpValue, const int64_t & tag) PN_SEALED; 
	virtual bool RequestNonExclusiveSnapshotDataByFieldNameAndValue ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::String & rootTableName, const Proud::String & fieldName, const Proud::CVariant & cmpValue, const int64_t & tag)   PN_SEALED;  
	virtual bool RequestNonExclusiveSnapshotDataByGuid ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & rootTableName, const Proud::Guid & rootUUID, const int64_t & tag) PN_SEALED; 
	virtual bool RequestNonExclusiveSnapshotDataByGuid ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::String & rootTableName, const Proud::Guid & rootUUID, const int64_t & tag)   PN_SEALED;  
	virtual bool RequestNonExclusiveSnapshotDataByQuery ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & rootTableName, const Proud::String & queryString, const int64_t & tag) PN_SEALED; 
	virtual bool RequestNonExclusiveSnapshotDataByQuery ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::String & rootTableName, const Proud::String & queryString, const int64_t & tag)   PN_SEALED;  
	virtual bool RequestNonExclusiveAddData ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & rootTableName, const Proud::Guid & rootUUID, const Proud::Guid & ownerUUID, const Proud::ByteArray & addDataBlock, const int64_t & tag, const Proud::ByteArray & messageToLoader) PN_SEALED; 
	virtual bool RequestNonExclusiveAddData ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::String & rootTableName, const Proud::Guid & rootUUID, const Proud::Guid & ownerUUID, const Proud::ByteArray & addDataBlock, const int64_t & tag, const Proud::ByteArray & messageToLoader)   PN_SEALED;  
	virtual bool RequestNonExclusiveRemoveData ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & rootTableName, const Proud::Guid & rootUUID, const Proud::Guid & removeUUID, const int64_t & tag, const Proud::ByteArray & messageToLoader) PN_SEALED; 
	virtual bool RequestNonExclusiveRemoveData ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::String & rootTableName, const Proud::Guid & rootUUID, const Proud::Guid & removeUUID, const int64_t & tag, const Proud::ByteArray & messageToLoader)   PN_SEALED;  
	virtual bool RequestNonExclusiveSetValueIf ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & rootTableName, const Proud::Guid & rootUUID, const Proud::Guid & nodeUUID, const Proud::String & propertyName, const Proud::CVariant & newValue, const int & compareType, const Proud::CVariant & compareValue, const int64_t & tag, const Proud::ByteArray & messageToLoader) PN_SEALED; 
	virtual bool RequestNonExclusiveSetValueIf ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::String & rootTableName, const Proud::Guid & rootUUID, const Proud::Guid & nodeUUID, const Proud::String & propertyName, const Proud::CVariant & newValue, const int & compareType, const Proud::CVariant & compareValue, const int64_t & tag, const Proud::ByteArray & messageToLoader)   PN_SEALED;  
	virtual bool RequestNonExclusiveModifyValue ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & rootTableName, const Proud::Guid & rootUUID, const Proud::Guid & nodeUUID, const Proud::String & propertyName, const int & operType, const Proud::CVariant & value, const int64_t & tag, const Proud::ByteArray & messageToLoader) PN_SEALED; 
	virtual bool RequestNonExclusiveModifyValue ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::String & rootTableName, const Proud::Guid & rootUUID, const Proud::Guid & nodeUUID, const Proud::String & propertyName, const int & operType, const Proud::CVariant & value, const int64_t & tag, const Proud::ByteArray & messageToLoader)   PN_SEALED;  
	virtual bool RequestIsolateData ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & rootUUID, const Proud::String & rootTableName, const Proud::Guid & sessionGuid) PN_SEALED; 
	virtual bool RequestIsolateData ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & rootUUID, const Proud::String & rootTableName, const Proud::Guid & sessionGuid)   PN_SEALED;  
	virtual bool RequestDeisolateData ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & rootUUID, const Proud::String & filterText, const Proud::Guid & sessionGuid) PN_SEALED; 
	virtual bool RequestDeisolateData ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & rootUUID, const Proud::String & filterText, const Proud::Guid & sessionGuid)   PN_SEALED;  
static const PNTCHAR* RmiName_RequestDbCacheClientLogon;
static const PNTCHAR* RmiName_RequestExclusiveLoadDataByFieldNameAndValue;
static const PNTCHAR* RmiName_RequestExclusiveLoadDataByGuid;
static const PNTCHAR* RmiName_RequestExclusiveLoadDataByQuery;
static const PNTCHAR* RmiName_RequestExclusiveLoadNewData;
static const PNTCHAR* RmiName_RequestUnloadDataBySessionGuid;
static const PNTCHAR* RmiName_DenyUnloadData;
static const PNTCHAR* RmiName_RequestForceCompleteUnload;
static const PNTCHAR* RmiName_RequestAddData;
static const PNTCHAR* RmiName_RequestUpdateData;
static const PNTCHAR* RmiName_RequestRemoveData;
static const PNTCHAR* RmiName_RequestUpdateDataList;
static const PNTCHAR* RmiName_AddData;
static const PNTCHAR* RmiName_UpdateData;
static const PNTCHAR* RmiName_RemoveData;
static const PNTCHAR* RmiName_UpdateDataList;
static const PNTCHAR* RmiName_MoveData;
static const PNTCHAR* RmiName_RequestNonExclusiveSnapshotDataByFieldNameAndValue;
static const PNTCHAR* RmiName_RequestNonExclusiveSnapshotDataByGuid;
static const PNTCHAR* RmiName_RequestNonExclusiveSnapshotDataByQuery;
static const PNTCHAR* RmiName_RequestNonExclusiveAddData;
static const PNTCHAR* RmiName_RequestNonExclusiveRemoveData;
static const PNTCHAR* RmiName_RequestNonExclusiveSetValueIf;
static const PNTCHAR* RmiName_RequestNonExclusiveModifyValue;
static const PNTCHAR* RmiName_RequestIsolateData;
static const PNTCHAR* RmiName_RequestDeisolateData;
static const PNTCHAR* RmiName_First;
		Proxy()
		{
			if(m_signature != 1)
				::Proud::ShowUserMisuseError(::Proud::ProxyBadSignatureErrorText);
		}

		virtual ::Proud::RmiID* GetRmiIDList() PN_OVERRIDE { return g_RmiIDList; } 
		virtual int GetRmiIDListCount() PN_OVERRIDE { return g_RmiIDListCount; }
	};

}


namespace DB2S2C {


	class Proxy : public ::Proud::IRmiProxy
	{
	public:
	virtual bool NotifyDbCacheClientLogonFailed ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::ErrorType & reason) PN_SEALED; 
	virtual bool NotifyDbCacheClientLogonFailed ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::ErrorType & reason)   PN_SEALED;  
	virtual bool NotifyDbCacheClientLogonSuccess ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext ) PN_SEALED; 
	virtual bool NotifyDbCacheClientLogonSuccess ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext)   PN_SEALED;  
	virtual bool NotifyLoadDataComplete ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const bool & isExclusive, const int64_t & tag, const Proud::CFastArray<Proud::ByteArray> & successList_loadedData, const Proud::CFastArray<Proud::Guid> & successList_sessionGuid, const Proud::CFastArray<Proud::ByteArray> & successList_message, const Proud::CFastArray<Proud::Guid> & failList_uuid, const Proud::CFastArray<Proud::ErrorType> & failList_reason, const Proud::CFastArray<Proud::String> & failList_comment, const Proud::CFastArray<int32_t> & failList_hresult, const Proud::CFastArray<Proud::ByteArray> & failList_message) PN_SEALED; 
	virtual bool NotifyLoadDataComplete ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const bool & isExclusive, const int64_t & tag, const Proud::CFastArray<Proud::ByteArray> & successList_loadedData, const Proud::CFastArray<Proud::Guid> & successList_sessionGuid, const Proud::CFastArray<Proud::ByteArray> & successList_message, const Proud::CFastArray<Proud::Guid> & failList_uuid, const Proud::CFastArray<Proud::ErrorType> & failList_reason, const Proud::CFastArray<Proud::String> & failList_comment, const Proud::CFastArray<int32_t> & failList_hresult, const Proud::CFastArray<Proud::ByteArray> & failList_message)   PN_SEALED;  
	virtual bool NotifyDataUnloadRequested ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::CFastArray<Proud::Guid> & sessionList, const Proud::CFastArray<Proud::ByteArray> & messageList, const bool & commonMessage, const int64_t & unloadRequestTimeoutTimeMs) PN_SEALED; 
	virtual bool NotifyDataUnloadRequested ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::CFastArray<Proud::Guid> & sessionList, const Proud::CFastArray<Proud::ByteArray> & messageList, const bool & commonMessage, const int64_t & unloadRequestTimeoutTimeMs)   PN_SEALED;  
	virtual bool NotifyUnloadRequestTimeoutTimeMs ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int64_t & unloadRequestTimeoutTimeMs) PN_SEALED; 
	virtual bool NotifyUnloadRequestTimeoutTimeMs ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int64_t & unloadRequestTimeoutTimeMs)   PN_SEALED;  
	virtual bool NotifyAddDataFailed ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int64_t & tag, const Proud::ErrorType & reason, const Proud::String & comment, const int32_t & hresult, const Proud::String & source, const bool & blocked) PN_SEALED; 
	virtual bool NotifyAddDataFailed ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int64_t & tag, const Proud::ErrorType & reason, const Proud::String & comment, const int32_t & hresult, const Proud::String & source, const bool & blocked)   PN_SEALED;  
	virtual bool NotifyAddDataSuccess ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int64_t & tag, const Proud::Guid & rootUUID, const Proud::ByteArray & addDataBlock, const bool & blocked) PN_SEALED; 
	virtual bool NotifyAddDataSuccess ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int64_t & tag, const Proud::Guid & rootUUID, const Proud::ByteArray & addDataBlock, const bool & blocked)   PN_SEALED;  
	virtual bool NotifyUpdateDataFailed ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int64_t & tag, const Proud::ErrorType & reason, const Proud::String & comment, const int32_t & hresult, const Proud::String & source, const bool & blocked) PN_SEALED; 
	virtual bool NotifyUpdateDataFailed ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int64_t & tag, const Proud::ErrorType & reason, const Proud::String & comment, const int32_t & hresult, const Proud::String & source, const bool & blocked)   PN_SEALED;  
	virtual bool NotifyUpdateDataSuccess ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int64_t & tag, const Proud::Guid & rootUUID, const Proud::ByteArray & updataDataBlock, const bool & blocked) PN_SEALED; 
	virtual bool NotifyUpdateDataSuccess ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int64_t & tag, const Proud::Guid & rootUUID, const Proud::ByteArray & updataDataBlock, const bool & blocked)   PN_SEALED;  
	virtual bool NotifyUpdateDataListFailed ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int64_t & tag, const Proud::ErrorType & reason, const Proud::String & comment, const int32_t & hresult, const Proud::String & source, const bool & blocked) PN_SEALED; 
	virtual bool NotifyUpdateDataListFailed ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int64_t & tag, const Proud::ErrorType & reason, const Proud::String & comment, const int32_t & hresult, const Proud::String & source, const bool & blocked)   PN_SEALED;  
	virtual bool NotifyUpdateDataListSuccess ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int64_t & tag, const Proud::Guid & rootUUID, const Proud::ByteArray & updateDataBlock, const bool & blocked) PN_SEALED; 
	virtual bool NotifyUpdateDataListSuccess ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int64_t & tag, const Proud::Guid & rootUUID, const Proud::ByteArray & updateDataBlock, const bool & blocked)   PN_SEALED;  
	virtual bool NotifyRemoveDataFailed ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int64_t & tag, const Proud::ErrorType & reason, const Proud::String & comment, const int32_t & hresult, const Proud::String & source, const bool & blocked) PN_SEALED; 
	virtual bool NotifyRemoveDataFailed ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int64_t & tag, const Proud::ErrorType & reason, const Proud::String & comment, const int32_t & hresult, const Proud::String & source, const bool & blocked)   PN_SEALED;  
	virtual bool NotifyRemoveDataSuccess ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int64_t & tag, const Proud::Guid & rootUUID, const Proud::Guid & removedUUID, const bool & blocked) PN_SEALED; 
	virtual bool NotifyRemoveDataSuccess ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int64_t & tag, const Proud::Guid & rootUUID, const Proud::Guid & removedUUID, const bool & blocked)   PN_SEALED;  
	virtual bool NotifyNonExclusiveAddDataFailed ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int64_t & tag, const Proud::ErrorType & reason, const Proud::String & comment, const int32_t & hresult, const Proud::String & source) PN_SEALED; 
	virtual bool NotifyNonExclusiveAddDataFailed ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int64_t & tag, const Proud::ErrorType & reason, const Proud::String & comment, const int32_t & hresult, const Proud::String & source)   PN_SEALED;  
	virtual bool NotifyNonExclusiveAddDataSuccess ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int64_t & tag, const Proud::Guid & rootUUID, const Proud::ByteArray & addDataBlock) PN_SEALED; 
	virtual bool NotifyNonExclusiveAddDataSuccess ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int64_t & tag, const Proud::Guid & rootUUID, const Proud::ByteArray & addDataBlock)   PN_SEALED;  
	virtual bool NotifySomeoneAddData ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & rootUUID, const Proud::ByteArray & addDataBlock, const Proud::ByteArray & message, const Proud::Guid & destParentUUID) PN_SEALED; 
	virtual bool NotifySomeoneAddData ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & rootUUID, const Proud::ByteArray & addDataBlock, const Proud::ByteArray & message, const Proud::Guid & destParentUUID)   PN_SEALED;  
	virtual bool NotifyNonExclusiveRemoveDataFailed ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int64_t & tag, const Proud::ErrorType & reason, const Proud::String & comment, const int32_t & hresult, const Proud::String & source) PN_SEALED; 
	virtual bool NotifyNonExclusiveRemoveDataFailed ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int64_t & tag, const Proud::ErrorType & reason, const Proud::String & comment, const int32_t & hresult, const Proud::String & source)   PN_SEALED;  
	virtual bool NotifyNonExclusiveRemoveDataSuccess ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int64_t & tag, const Proud::Guid & rootUUID, const Proud::Guid & removeUUID) PN_SEALED; 
	virtual bool NotifyNonExclusiveRemoveDataSuccess ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int64_t & tag, const Proud::Guid & rootUUID, const Proud::Guid & removeUUID)   PN_SEALED;  
	virtual bool NotifySomeoneRemoveData ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & rootUUID, const Proud::Guid & removeUUID, const Proud::ByteArray & message) PN_SEALED; 
	virtual bool NotifySomeoneRemoveData ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & rootUUID, const Proud::Guid & removeUUID, const Proud::ByteArray & message)   PN_SEALED;  
	virtual bool NotifyNonExclusiveSetValueFailed ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int64_t & tag, const Proud::ErrorType & reason, const Proud::String & comment, const int32_t & hresult, const Proud::String & source) PN_SEALED; 
	virtual bool NotifyNonExclusiveSetValueFailed ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int64_t & tag, const Proud::ErrorType & reason, const Proud::String & comment, const int32_t & hresult, const Proud::String & source)   PN_SEALED;  
	virtual bool NotifyNonExclusiveSetValueSuccess ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int64_t & tag, const Proud::Guid & rootUUID, const Proud::ByteArray & updateDataBlock) PN_SEALED; 
	virtual bool NotifyNonExclusiveSetValueSuccess ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int64_t & tag, const Proud::Guid & rootUUID, const Proud::ByteArray & updateDataBlock)   PN_SEALED;  
	virtual bool NotifySomeoneSetValue ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & rootUUID, const Proud::ByteArray & updateDataBlock, const Proud::ByteArray & message) PN_SEALED; 
	virtual bool NotifySomeoneSetValue ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & rootUUID, const Proud::ByteArray & updateDataBlock, const Proud::ByteArray & message)   PN_SEALED;  
	virtual bool NotifyNonExclusiveModifyValueFailed ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int64_t & tag, const Proud::ErrorType & reason, const Proud::String & comment, const int32_t & hresult, const Proud::String & source) PN_SEALED; 
	virtual bool NotifyNonExclusiveModifyValueFailed ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int64_t & tag, const Proud::ErrorType & reason, const Proud::String & comment, const int32_t & hresult, const Proud::String & source)   PN_SEALED;  
	virtual bool NotifyNonExclusiveModifyValueSuccess ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int64_t & tag, const Proud::Guid & rootUUID, const Proud::ByteArray & updateDataBlock) PN_SEALED; 
	virtual bool NotifyNonExclusiveModifyValueSuccess ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int64_t & tag, const Proud::Guid & rootUUID, const Proud::ByteArray & updateDataBlock)   PN_SEALED;  
	virtual bool NotifySomeoneModifyValue ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & rootUUID, const Proud::ByteArray & updateDataBlock, const Proud::ByteArray & message) PN_SEALED; 
	virtual bool NotifySomeoneModifyValue ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & rootUUID, const Proud::ByteArray & updateDataBlock, const Proud::ByteArray & message)   PN_SEALED;  
	virtual bool NotifyWarning ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::ErrorType & errorType, const Proud::ErrorType & detailtype, const Proud::String & comment) PN_SEALED; 
	virtual bool NotifyWarning ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::ErrorType & errorType, const Proud::ErrorType & detailtype, const Proud::String & comment)   PN_SEALED;  
	virtual bool NotifyDbmsWriteDone ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::DbmsWritePropNodePendType & type, const Proud::Guid & loadeeGuid) PN_SEALED; 
	virtual bool NotifyDbmsWriteDone ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::DbmsWritePropNodePendType & type, const Proud::Guid & loadeeGuid)   PN_SEALED;  
	virtual bool NotifyDbmsAccessError ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int64_t & tag, const Proud::ByteArray & message, const Proud::String & comment) PN_SEALED; 
	virtual bool NotifyDbmsAccessError ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int64_t & tag, const Proud::ByteArray & message, const Proud::String & comment)   PN_SEALED;  
	virtual bool NotifyIsolateDataSuccess ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & sessionGuid, const Proud::Guid & rootUUID, const Proud::ByteArray & dataBlock) PN_SEALED; 
	virtual bool NotifyIsolateDataSuccess ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & sessionGuid, const Proud::Guid & rootUUID, const Proud::ByteArray & dataBlock)   PN_SEALED;  
	virtual bool NotifyIsolateDataFailed ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & sessionGuid, const Proud::ErrorType & reason, const Proud::String & comment) PN_SEALED; 
	virtual bool NotifyIsolateDataFailed ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & sessionGuid, const Proud::ErrorType & reason, const Proud::String & comment)   PN_SEALED;  
	virtual bool NotifyDeisolateDataSuccess ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & sessionGuid, const Proud::Guid & rootUUID, const Proud::ByteArray & dataBlock) PN_SEALED; 
	virtual bool NotifyDeisolateDataSuccess ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & sessionGuid, const Proud::Guid & rootUUID, const Proud::ByteArray & dataBlock)   PN_SEALED;  
	virtual bool NotifyDeisolateDataFailed ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & sessionGuid, const Proud::ErrorType & reason, const Proud::String & comment) PN_SEALED; 
	virtual bool NotifyDeisolateDataFailed ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & sessionGuid, const Proud::ErrorType & reason, const Proud::String & comment)   PN_SEALED;  
	virtual bool NotifyDataForceUnloaded ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::Guid & sessionGuid, const Proud::Guid & rootGuid) PN_SEALED; 
	virtual bool NotifyDataForceUnloaded ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::Guid & sessionGuid, const Proud::Guid & rootGuid)   PN_SEALED;  
static const PNTCHAR* RmiName_NotifyDbCacheClientLogonFailed;
static const PNTCHAR* RmiName_NotifyDbCacheClientLogonSuccess;
static const PNTCHAR* RmiName_NotifyLoadDataComplete;
static const PNTCHAR* RmiName_NotifyDataUnloadRequested;
static const PNTCHAR* RmiName_NotifyUnloadRequestTimeoutTimeMs;
static const PNTCHAR* RmiName_NotifyAddDataFailed;
static const PNTCHAR* RmiName_NotifyAddDataSuccess;
static const PNTCHAR* RmiName_NotifyUpdateDataFailed;
static const PNTCHAR* RmiName_NotifyUpdateDataSuccess;
static const PNTCHAR* RmiName_NotifyUpdateDataListFailed;
static const PNTCHAR* RmiName_NotifyUpdateDataListSuccess;
static const PNTCHAR* RmiName_NotifyRemoveDataFailed;
static const PNTCHAR* RmiName_NotifyRemoveDataSuccess;
static const PNTCHAR* RmiName_NotifyNonExclusiveAddDataFailed;
static const PNTCHAR* RmiName_NotifyNonExclusiveAddDataSuccess;
static const PNTCHAR* RmiName_NotifySomeoneAddData;
static const PNTCHAR* RmiName_NotifyNonExclusiveRemoveDataFailed;
static const PNTCHAR* RmiName_NotifyNonExclusiveRemoveDataSuccess;
static const PNTCHAR* RmiName_NotifySomeoneRemoveData;
static const PNTCHAR* RmiName_NotifyNonExclusiveSetValueFailed;
static const PNTCHAR* RmiName_NotifyNonExclusiveSetValueSuccess;
static const PNTCHAR* RmiName_NotifySomeoneSetValue;
static const PNTCHAR* RmiName_NotifyNonExclusiveModifyValueFailed;
static const PNTCHAR* RmiName_NotifyNonExclusiveModifyValueSuccess;
static const PNTCHAR* RmiName_NotifySomeoneModifyValue;
static const PNTCHAR* RmiName_NotifyWarning;
static const PNTCHAR* RmiName_NotifyDbmsWriteDone;
static const PNTCHAR* RmiName_NotifyDbmsAccessError;
static const PNTCHAR* RmiName_NotifyIsolateDataSuccess;
static const PNTCHAR* RmiName_NotifyIsolateDataFailed;
static const PNTCHAR* RmiName_NotifyDeisolateDataSuccess;
static const PNTCHAR* RmiName_NotifyDeisolateDataFailed;
static const PNTCHAR* RmiName_NotifyDataForceUnloaded;
static const PNTCHAR* RmiName_First;
		Proxy()
		{
			if(m_signature != 1)
				::Proud::ShowUserMisuseError(::Proud::ProxyBadSignatureErrorText);
		}

		virtual ::Proud::RmiID* GetRmiIDList() PN_OVERRIDE { return g_RmiIDList; } 
		virtual int GetRmiIDListCount() PN_OVERRIDE { return g_RmiIDListCount; }
	};

}



