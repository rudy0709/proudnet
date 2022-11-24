#pragma once

#if defined(_WIN32)

#include "../include/NetServerInterface.h"
#include "../include/INetServerEvent.h"
#include "../include/fastlist.h"
#include "../include/dbcacheserver2.h"
#include "../include/DbLoadedData2.h"
#include "../include/ThreadUtil.h"
#include "../include/MilisecTimer.h"
#include "../include/TimerThread.h"
#include "../include/Singleton.h"
#include "dbloadeddata2_s.h"

#include "DB_stub.h"
#include "DB_proxy.h"

#include "FastList2.h"
#include "AsyncWork.h"

#include "NumUtil.h"


namespace Proud
{
	class CDbCacheServer2Impl;
	struct SaveToDbmsContext;
	

	class CDBCacheWorker;
	class CDbCacheJobQueue;

	class CDbCacheServer2Impl :public CDbCacheServer2, public DB2C2S::Stub, public INetServerEvent
	{
	public:
		DB2S2C::Proxy m_s2cProxy;
		CHeldPtr<CNetServer> m_netServer;
		DbRemoteClients m_remoteClients;
		CriticalSection m_cs;

		// 비록 사용하지는 않지만 ADO 객체의 잦은 로딩을 막기 위해 연결만 하나 해두도록 한다.
		CAdoConnection m_initialDbmsConnection;
		String m_dbmsConnectionString;
		String m_dbmsPoolString;	//!< DB 연결을 할 경우 ADO 에 min, max pool 정보를 설정하기 위한.
		DbmsType m_dbmsType;
		String m_authenticationKey;
		IDbCacheServerDelegate2* m_delegate;

		// 로딩된 객체들. root UUID가 key다.
		LoadedDataList2_S m_loadedDataList;
		// 로딩된 객체들. 단, 로딩을 할때 사용자에게 발부되었던 session guid가 key다.
		LoadedDataList2_S m_loadedDataListBySession;

		// isolate 된 data tree 목록
		IsolateDataList2_S m_isolatedDataList;

		CFastArray<CCachedTableName> m_tableNames;

		int64_t m_saveToDbmsInterval, m_unloadedHibernateDuration;

		// 비독점 API를 허가/금지
		bool m_allowNonExclusiveAccess;

		// 격리 요청한 클라이언트가 종료 할 때 격리를 해제 할 것인지 여부 (기본 true)
		bool m_autoDeisolateData; 

		//PROUDNET_VOLATILE_ALIGN  int64_t m_cachedServerTime;

		PROUDNET_VOLATILE_ALIGN  LONG m_saveToDbmsWorkingCount;

		CHeldPtr<CAsyncWorksOwner> m_asyncLoadWorks;

		CriticalSection m_startOrStopCS;

		//CHeldPtr<CTimerQueueTimer> m_frameMoveWorker;

		// lanclient의 ontick로 대체하려 했으나...
		// lanclient의 stop를 호출하게 되면 ontick가 멈추기 때문에 pendlist가 마무리되지 않고 끝나는 상황이
		// 될수 있으므로 이걸 무조건 쓰자.
		CTimerThread *m_frameMoveWorker;

		CDbCacheJobQueue *m_dbJobQueue;

		int m_commandTimeoutTime;
	protected:
		//!< SaveToDbmsContext, ExclusiveLoadData, ExclusiveLoadData_New 의 처리를 하는 스레드 워커의 집합이다.
		//!< 서버 start 시에 설정받은 값으로 작동한다.
		CDBCacheWorker* m_pdbCacheWorker;

		// 이슈 2226 MySQL 데드락 발생 문제로 인해 추가된 기능. (RecursiveUpdate가 원인으로 추정)
		// 기본값은 false.
		// 이 값이 true이면 RecursiveUpdate 작업과 관련하여 DB접근할때만 m_recursiveUpdateLock를 잡아 직렬화.
		// 별도의 쓰레드를 두지 못한 이유는 m_dbJobQueue에 있는 요청들의 처리 순서를 지키기 위함.
		bool m_serializeRecursiveUpdateWork;

		// m_serializeRecursiveUpdateWork가 true일때만 사용됩니다.
		CriticalSection m_recursiveUpdateLock;

		// 독점요청 타임아웃 제한 시간.
		// 기본값은 CDbConfig::DefaultUnloadRequestTimeoutTimeMs
		volatile int64_t m_unloadRequestTimeoutTimeMs;

	public:
		CDbCacheServer2Impl(void);
		~CDbCacheServer2Impl(void);

		void WaitForPendedWorkFinished();
		virtual void Start(CDbCacheServer2StartParameter &params) PN_OVERRIDE;
		void Stop();

		intptr_t GetCachingDataTreeCount();

	private:
		// table name들을 검사하고 각 table에 필요한 procedure를 생성한다.
		void CheckTableNames(CFastArray< CCachedTableName >& tableNames);
		void CreateProcedure(const String& tableName, CFastArray<String>* childTableNames = NULL);
		bool IsValidRootTable(String strRoot);
		void NonExclusiveSnapshotData(CriticalSectionLock& lock, const HostID remote, const String& rootTableName, const Guid& rootUUID, const int64_t tag);

		bool NotifyLoadDataCompleteOnlyIfUnloadWaiterIsNone(CLoadRequestPtr& request);
		bool ProcessLoadDataFail(CLoadRequestPtr& request, CFailInfo_S& info);
		bool ProcessLoadDataSuccess(CLoadRequestPtr& request, CSuccessInfo_S& info);
		bool HasNoChildTable(const String& rootTableName);
		void HandoverExclusiveOwnership(CLoadedData2Ptr_S data, const ByteArray& messageToNextLoader=ByteArray());
		void DenyUnloadDataProc(CLoadedData2Ptr_S data, CFailInfo_S& info, bool denyAll);
		void RequestUnload(CLoadRequestPtr& request, LoadedDataList* unloadRequestList=NULL);
		Guid NewSessionGuid();
		String GetUuidQueryString(const Guid& uuid);

	public:
		void SetDbmsWriteIntervalMs(int64_t val);
		void SetUnloadedDataHibernateDurationMs(int64_t val);

		virtual void OnClientJoin(CNetClientInfo *clientInfo) PN_OVERRIDE;
		virtual void OnClientLeave(CNetClientInfo *clientInfo, ErrorInfo *errorInfo, const ByteArray& comment) PN_OVERRIDE;
		virtual bool OnConnectionRequest(AddrPort /*clientAddr*/, ByteArray &/*userDataFromClient*/, ByteArray &/*reply*/) PN_OVERRIDE{ return true; }
		virtual void OnError(ErrorInfo *errorInfo) PN_OVERRIDE;
		virtual void OnP2PGroupJoinMemberAckComplete(HostID /*groupHostID*/, HostID /*memberHostID*/, ErrorType /*result*/) PN_OVERRIDE{};;
		virtual void OnUserWorkerThreadBegin() PN_OVERRIDE{};
		virtual void OnUserWorkerThreadEnd() PN_OVERRIDE{};
		virtual void OnClientHackSuspected(HostID /*clientID*/, HackType /*hackType*/) PN_OVERRIDE{};
		virtual void OnWarning(Proud::ErrorInfo*) PN_OVERRIDE;
		virtual void OnInformation(Proud::ErrorInfo*)  PN_OVERRIDE{};
		virtual void OnException(Exception &e)  PN_OVERRIDE;
		virtual void OnNoRmiProcessed(Proud::RmiID /*rmiID*/)  PN_OVERRIDE{};

		// DB2C2S

		void ExclusiveLoadData_NewRoot(
			const HostID&		remote,
			const String&		rootTableName,
			const ByteArray&	addDataBlock,
			const int64_t&		tag,
			const bool			transaction
			);

		void LoadDataCore(
			CriticalSectionLock&	mainLock,
			const HostID&			remote,
			const String&			rootTableName,
			const String&			queryString,
			CLoadRequestPtr&		request
			);

		void LoadDataCore_LoadRoot(
			CriticalSectionLock&	mainLock,
			CLoadRequestPtr&		request,
			const String&			rootTableName,
			const String&			queryString,
			LoadedDataList&			candidateList,
			LoadedDataList&			unloadRequestList
			);

		void LoadDataCore_LoadChildren(
			CriticalSectionLock&	mainLock,
			CLoadRequestPtr&		request,
			const String&			rootTableName,
			LoadedDataList&			candidateList,
			LoadedDataList&			unloadRequestList
			);

		void LoadDataCore_Finish(
			CriticalSectionLock&	mainLock,
			CLoadRequestPtr&		request,
			LoadedDataList*			unloadRequestList
			);

		DECRMI_DB2C2S_RequestDbCacheClientLogon;
		
		DECRMI_DB2C2S_RequestExclusiveLoadDataByFieldNameAndValue;
		bool RequestExclusiveLoadDataByFieldNameAndValue_Core(
			Proud::HostID remote, 
			Proud::RmiContext &rmiContext, 
			const Proud::String &rootTableName, 
			const Proud::String &fieldName, 
			const Proud::CVariant &cmpValue, 
			const int64_t& tag, 
			const Proud::ByteArray &message
			);

		DECRMI_DB2C2S_RequestExclusiveLoadDataByGuid;
		bool RequestExclusiveLoadDataByGuid_Core(
			Proud::HostID remote, 
			Proud::RmiContext &rmiContext, 
			const Proud::String &rootTableName, 
			const Proud::Guid &rootUUID, 
			const int64_t& tag, 
			const Proud::ByteArray &message
			);

		DECRMI_DB2C2S_RequestExclusiveLoadDataByQuery;
		bool RequestExclusiveLoadDataByQuery_Core(
			Proud::HostID remote, 
			Proud::RmiContext &rmiContext, 
			const Proud::String &rootTableName, 
			const Proud::String &queryStirng, 
			const int64_t& tag, 
			const Proud::ByteArray &message
			);

		DECRMI_DB2C2S_RequestExclusiveLoadNewData;
		bool RequestExclusiveLoadNewData_Core(
			Proud::HostID remote,
			Proud::RmiContext &rmiContext,
			const Proud::String &rootTableName,
			const Proud::ByteArray &addDataBlock,
			const int64_t& tag,
			const bool transaction
			);

		DECRMI_DB2C2S_RequestUnloadDataBySessionGuid;
		DECRMI_DB2C2S_DenyUnloadData;
		DECRMI_DB2C2S_RequestForceCompleteUnload;
		DECRMI_DB2C2S_RequestAddData;
		DECRMI_DB2C2S_RequestUpdateData;
		DECRMI_DB2C2S_RequestUpdateDataList;
		DECRMI_DB2C2S_RequestRemoveData;
		DECRMI_DB2C2S_AddData;
		DECRMI_DB2C2S_UpdateData;
		DECRMI_DB2C2S_UpdateDataList;
		DECRMI_DB2C2S_RemoveData;
		DECRMI_DB2C2S_MoveData;
		DECRMI_DB2C2S_RequestNonExclusiveSnapshotDataByFieldNameAndValue;
		DECRMI_DB2C2S_RequestNonExclusiveSnapshotDataByGuid;
		DECRMI_DB2C2S_RequestNonExclusiveSnapshotDataByQuery;
		DECRMI_DB2C2S_RequestNonExclusiveAddData;
		DECRMI_DB2C2S_RequestNonExclusiveRemoveData;
		DECRMI_DB2C2S_RequestNonExclusiveSetValueIf;
		DECRMI_DB2C2S_RequestNonExclusiveModifyValue;
		DECRMI_DB2C2S_RequestIsolateData;
		DECRMI_DB2C2S_RequestDeisolateData;

		CDbRemoteClientPtr_S GetRemoteClientByHostID(HostID clientID);
		CDbRemoteClientPtr_S GetAuthedRemoteClientByHostID(HostID clientID);

		 void RegisterLoadedData(CLoadedData2Ptr_S& loadedData);
		 void RegisterLoadedData(
			CLoadedData2Ptr_S& loadedData, 
			HostID requester,
			Guid& sessionGuid
		);

		CLoadedData2Ptr_S CheckLoadedDataAndProcessRequest(
			const Guid&			rootUUID,
			CLoadRequestPtr&	request,
			LoadedDataList&		unloadRequestList
		);

		bool LoadDataFromDbms(String roottableName, String searchString, CLoadedData2Ptr_S &outData, String& comment);
		bool LoadDataFromDbms(String roottableName, Guid rootUUID, CLoadedData2Ptr_S &outData);
		bool LoadDataForChildTablesFromDbms(CAdoConnection &conn, String rootTableName, Guid rootguid, CLoadedData2* data);

		CLoadedData2Ptr_S GetLoadedDataByRootGuid(const Guid &guid);
		CLoadedData2Ptr_S GetLoadedDataBySessionGuid(const Guid &guid);
		String VariantToString(const CVariant& var);

		void UnloadLoadeeByLoaderHostID(HostID clientID);

		void UnloadLoadeeByRootUUID(HostID clientID, Guid rootUUID, bool unloadAllClient = false);

		void Heartbeat();

		void UpdateLocalDataList(HostID remote, CLoadedData2Ptr_S loadedData, int64_t tag, DbmsWritePropNodeListPendArray& modifyDataList, bool hardwork = true);
	private:
		void EnquePendedDbmsWorksForActiveLoadees();
		//void EnquePendedDbmsWorksForDeletees();

		void ExtractUpdateDataFromArray(HostID remote,
			CLoadedData2 &loadData,
			const ByteArray& inArray,
			DbmsWritePropNodeListPendArray& outArray);

		//노드의 RootUUID를 바꾼다.
		void ChangeDBRootUUID(CAdoConnection& conn, DbmsWritePropNodeListPendArray *pendList, Guid &DestRootUUID);

		bool CheckIsolateDataList(Guid rootUUID);

	public:
		static DWORD WINAPI StaticSaveToDbms(void* context);
		void SaveToDbms(SaveToDbmsContext* ctx);
		void SaveToDbms_Internal(DbmsWritePropNodeListPendArray& saveArray, bool transaction);

		AddrPort GetTcpListenerLocalAddr()
		{
			CFastArray<AddrPort> output;
			m_netServer->GetTcpListenerLocalAddrs(output);
			assert(output.GetCount() == 1);
			return output[0];
		}

		virtual void SetDefaultTimeoutTimeMs(int newValInMs);
		virtual void SetDefaultTimeoutTimeSec(double newValInSec);
		virtual void SetUnloadRequestTimeoutTimeMs(int64_t timeoutMs) override;
		virtual int64_t GetUnloadRequestTimeoutTimeMs() const override;

		virtual bool GetAllRemoteClientAddrPort(CFastArray<AddrPort> &ret) PN_OVERRIDE;

		void RefreshLastModifiedTimeMs(const bool & writeDbmsImmediately, CLoadedData2Ptr_S loadedData);

	};

	void StaticFrameMoveFunction2(void* context);

	// per-LD 단위로, 디비에 저장할 작업 정보
	struct SaveToDbmsContext
	{
		// LD의 root guid
		Guid m_exclusiveLoadeeGuid;
		CLoadedData2Ptr_S m_deleteeData;
		CDbCacheServer2Impl* m_server;
	};

	//     struct AutoObliterateParam
	//     {
	//         String m_dbmsConnectionString;
	//         CPnTimeSpan m_autoObliterateDuration;
	//         CDbCacheServer2Impl* m_server;
	//     };

	DWORD WINAPI RequestExclusiveLoadDataByFieldNameAndValue_Core_Static(void* context);
	class RequestExclusiveLoadDataByFieldNameAndValue_Param : public CAsyncWork
	{
	public:
		CDbCacheServer2Impl* m_server;
		Proud::HostID remote;
		Proud::RmiContext rmiContext;

		Proud::String rootTableName;
		Proud::String fieldName;
		Proud::CVariant cmpValue;
		int64_t tag;
		Proud::ByteArray message;

		RequestExclusiveLoadDataByFieldNameAndValue_Param(
			CDbCacheServer2Impl* server, 
			Proud::HostID remote, 
			Proud::RmiContext &rmiContext, 
			const Proud::String &rootTableName, 
			const Proud::String &fieldName, 
			const Proud::CVariant &cmpValue, 
			const int64_t& tag, 
			const Proud::ByteArray &message
			)
			:CAsyncWork(server->m_asyncLoadWorks)
		{
			this->m_server = server;
			this->remote = remote;
			this->rmiContext = rmiContext;

			this->rootTableName = rootTableName;
			this->fieldName = fieldName;
			this->cmpValue = cmpValue;
			this->tag = tag;
			this->message = message;
		}
	};

	DWORD WINAPI RequestExclusiveLoadDataByGuid_Core_Static(void* context);
	class RequestExclusiveLoadDataByGuid_Param : public CAsyncWork
	{
	public:
		CDbCacheServer2Impl* m_server;
		Proud::HostID remote;
		Proud::RmiContext rmiContext;

		Proud::String rootTableName;
		Proud::Guid rootUUID;
		int64_t tag;
		Proud::ByteArray message;

		RequestExclusiveLoadDataByGuid_Param(
			CDbCacheServer2Impl* server, 
			Proud::HostID remote, 
			Proud::RmiContext &rmiContext, 
			const Proud::String &rootTableName, 
			const Proud::Guid &rootUUID, 
			const int64_t& tag, 
			const Proud::ByteArray &message
			)
			:CAsyncWork(server->m_asyncLoadWorks)
		{
			this->m_server = server;
			this->remote = remote;
			this->rmiContext = rmiContext;

			this->rootTableName = rootTableName;
			this->rootUUID = rootUUID;
			this->tag = tag;
			this->message = message;
		}
	};

	DWORD WINAPI RequestExclusiveLoadDataByQuery_Core_Static(void* context);
	class RequestExclusiveLoadDataByQuery_Param : public CAsyncWork
	{
	public:
		CDbCacheServer2Impl* m_server;
		Proud::HostID remote;
		Proud::RmiContext rmiContext;

		Proud::String rootTableName;
		Proud::String queryString;
		Proud::Guid sessionGuid;
		int64_t tag;
		Proud::ByteArray message;

		RequestExclusiveLoadDataByQuery_Param(
			CDbCacheServer2Impl* server, 
			Proud::HostID remote, 
			Proud::RmiContext &rmiContext, 
			const Proud::String &rootTableName, 
			const Proud::String &queryString, 
			const int64_t& tag, 
			const Proud::ByteArray &message
			)
			:CAsyncWork(server->m_asyncLoadWorks)
		{
			this->m_server = server;
			this->remote = remote;
			this->rmiContext = rmiContext;

			this->rootTableName = rootTableName;
			this->queryString = queryString;
			this->tag = tag;
			this->message = message;
		}
	};

	DWORD WINAPI RequestExclusiveLoadNewData_Core_Static(void* context);
	class RequestExclusiveLoadNewData_Param :public CAsyncWork
	{
	public:
		CDbCacheServer2Impl* m_server;
		Proud::HostID remote;
		Proud::RmiContext rmiContext;

		Proud::String rootTableName;
		Proud::ByteArray addDataBlock;
		int64_t tag;
		bool transaction;

		RequestExclusiveLoadNewData_Param(
			CDbCacheServer2Impl* server, 
			Proud::HostID remote, 
			Proud::RmiContext &rmiContext, 
			const Proud::String &rootTableName, 
			const Proud::ByteArray &addDataBlock, 
			const int64_t& tag,
			const bool transaction
			)
			:CAsyncWork(server->m_asyncLoadWorks)
		{
			this->m_server = server;
			this->remote = remote;
			this->rmiContext = rmiContext;

			this->rootTableName = rootTableName;
			this->addDataBlock = addDataBlock;
			this->tag = tag;
			this->transaction = transaction;
		}
	};

	class CPendNodeCompare
	{
		DbmsType m_dbmsType;
	public:
		CPendNodeCompare(DbmsType dbmsType) : m_dbmsType(dbmsType){}

		inline int operator()(const DbmsWritePropNodeListPend &lhs, const DbmsWritePropNodeListPend &rhs) const
		{
			int ret;
			ret = lhs.m_node->GetStringTypeName().Compare(rhs.m_node->GetStringTypeName());
			if ( ret == 0 )
			{
				if ( m_dbmsType == MySql )
				{
					return memcmp(&lhs.m_node->UUID, &rhs.m_node->UUID, sizeof(Guid));
				}
				else
				{
					int ret = memcmp(lhs.m_node->UUID.Data4 + 2, rhs.m_node->UUID.Data4 + 2, sizeof(unsigned char) * 6);
					if ( ret == 0 )
					{
						ret = memcmp(&lhs.m_node->UUID, &rhs.m_node->UUID, sizeof(UUID) - sizeof(unsigned char) * 6);

					}

					return ret;
				}
			}
			else
			{
				return ret;
			}
		}
	};

}

#endif // _WIN32
