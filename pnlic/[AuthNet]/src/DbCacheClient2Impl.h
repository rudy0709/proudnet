#pragma once

#include "../include/DbCacheClient2.h"
#include "../include/FakeClr.h"
#include "../include/NetClientInterface.h"
#include "../include/INetClientEvent.h"
#include "../include/DbLoadedData.h"
#include "../include/Event.h"
#include "FreeList.h"
#include "../include/IRMIProxy.h"
#include "../include/IRMIStub.h"
#include "../include/sysutil.h"
#include "FastMap2.h"
#include "FastList2.h"
#include "DB_stub.h"
#include "DB_proxy.h"
#include "dbcacheclient2blocked.h"


namespace Proud
{
	typedef CFastMap2<Guid, CLoadedData2Ptr, int> LoadedData2List;
	typedef CObjectPool<Proud::Event> EventPool;

	typedef CFastMap2<intptr_t, BlockedEventPtr, int> EventMap;
	//	typedef CFastMap2<Guid,intptr_t,int> CallbackMap;

	class CDbCacheClient2Impl : public CDbCacheClient2, public INetClientEvent, public DB2S2C::Stub
	{
		CriticalSection m_cs;

		CHeldPtr<CNetClient> m_netClient;
		DB2C2S::Proxy m_c2sProxy;
		String m_authenticationKey;
		IDbCacheClientDelegate2* m_delegate;

		LoadedData2List m_loadedDataList;
		LoadedData2List m_loadedDataListBySession;

		EventPool	m_eventPool;

		// BlockedXXX를 호출했을 때 스레드를 깨우기 위한 용도
		EventMap	m_activeEvents;

		bool m_loggedOn;

		// DB cache 전용 RMI를 실행할 스레드 풀.
		// 내장된 NC가 사용한다.
		// 사용자가 '외부 스레드풀 쓰겠음'을 선언 안하는 경우 따로 생성되어 이것이 사용된다.
		CThreadPool* m_userWorkerThreadPool;

	public:

		// 언로드 요청을 받은 시간들을 저장하는 큐
		typedef CFastList2<int64_t, int> RequestedTimeQueue;
		// 언로드 요청을 각 세션별로 관리하기 위한 맵 (key = Session)
		typedef CFastMap2<Guid, RequestedTimeQueue, int> UnloadRequests;
	private:
		// 서버로부터 Unload요청을 받았을 때 
		// 사용자가 그에 대한 처리를 했는지 여부를 확인하기 위해 추가된 정보.
		//  - 메인락으로 보호한다.
		//  - 서버로부터 Unload요청을 받을 때 삽입 (NotifyDataUnloadRequested)
		//  - 사용자가 Unload 또는 Deny를 하면 제거 (UnloadDataBySessionGuid or DenyUnloadData)
		//  - 요청이 너무 오래됐을 때 제거
		UnloadRequests m_unloadRequests;

		// 독점요청 타임아웃 제한 시간.
		// 기본값은 CDbConfig::DefaultUnloadRequestTimeoutTimeMs
		// 서버로부터 NotifyDataUnloadRequested를 받을 때마다 갱신한다.
		volatile int64_t m_unloadRequestTimeoutTimeMs;

		void ProcessUnloadRequestTimeout1(UnloadRequests& outWarnings);
		void ProcessUnloadRequestTimeout2(const UnloadRequests& warnings) const;
	private:
		intptr_t GetCallbackArg_Lock(const Guid& waitingTicket);
		intptr_t GetCallbackArg_NoLock(const Guid& waitingTicket);

		virtual void OnTick(void* context);

	public:
		CDbCacheClient2Impl(void);
		~CDbCacheClient2Impl(void);

		// 유저 API
		bool Connect(CDbCacheClient2ConnectParameter& param, ErrorInfoPtr& outError = ErrorInfoPtr());
		bool Connect(CDbCacheClient2ConnectParameter& param);
		void Disconnect();

		// 내부 이벤트 콜백
		virtual void OnJoinServerComplete(ErrorInfo *info, const ByteArray &replyFromServer) override;
		virtual void OnLeaveServer(ErrorInfo *errorInfo) override;
		virtual void OnP2PMemberJoin(HostID memberHostID, HostID groupHostID, int memberCount, const ByteArray &customField) override{}
		virtual void OnP2PMemberLeave(HostID memberHostID, HostID groupHostID, int memberCount) override{}
		virtual void OnError(ErrorInfo* errorInfo) override;
		virtual void OnWarning(ErrorInfo *errorInfo) override;
		virtual void OnInformation(ErrorInfo *errorInfo) override{}
		virtual void OnException(Exception &e) override;
		virtual void OnNoRmiProcessed(Proud::RmiID rmiID) override{}

		virtual void OnChangeP2PRelayState(HostID remoteHostID, ErrorType reason) {}
		virtual void OnSynchronizeServerTime() override{}

		void AssureCSLock() const;
		CLoadedData2Ptr GetOrgLoadedDataBySessionGuid(const Guid& sessionGuid) const;
		CLoadedData2* GetOrgLoadedDataByRootUUID(const Guid& rootUUID) const;

		//독점 로딩 요청 메서드.
		virtual void RequestExclusiveLoadData(String rootTableName, String fieldName, CVariant cmpValue, intptr_t tag, ByteArray& message = ByteArray()) PN_OVERRIDE;

		virtual void RequestExclusiveLoadDataByGuid(String rootTableName, Guid rootUUID, intptr_t tag, ByteArray &message = ByteArray()) PN_OVERRIDE;

		virtual void RequestExclusiveLoadDataByQuery(String rootTableName, String queryString, intptr_t tag, ByteArray &message = ByteArray()) PN_OVERRIDE;

		virtual void RequestExclusiveLoadNewData(String rootTableName, CPropNodePtr addData, intptr_t tag, bool transaction) PN_OVERRIDE;

		//요청 응답형 메서드.
		virtual bool RequestAddData(Guid rootUUID, Guid ownerUUID, CPropNodePtr addData, intptr_t tag) PN_OVERRIDE;

		virtual bool RequestUpdateData(CPropNodePtr updateData, intptr_t tag) PN_OVERRIDE;

		virtual bool RequestRemoveData(Guid rootUUID, Guid removeUUID, intptr_t tag) PN_OVERRIDE;

		virtual bool RequestRecursiveUpdateData(CLoadedData2Ptr loadedData, intptr_t tag, bool transactional = false) PN_OVERRIDE;

		// 블록 메서드.
		virtual bool BlockedAddData(Guid rootUUID, Guid ownerUUID, CPropNodePtr addData, uint32_t timeOutTime = 30000, ErrorInfoPtr outError = ErrorInfoPtr()) PN_OVERRIDE;

		virtual bool BlockedUpdateData(CPropNodePtr updateData, uint32_t timeOutTime = 30000, ErrorInfoPtr outError = ErrorInfoPtr()) PN_OVERRIDE;

		virtual bool BlockedRemoveData(Guid rootUUID, Guid removeUUID, uint32_t timeOutTime = 30000, ErrorInfoPtr outError = ErrorInfoPtr()) PN_OVERRIDE;

		virtual bool BlockedRecursiveUpdateData(CLoadedData2Ptr loadedData, bool transactional = false, uint32_t timeOutTime = 30000, ErrorInfoPtr outError = ErrorInfoPtr()) PN_OVERRIDE;

		//일방적 메서드.
		virtual bool UnilateralMoveData(String rootTableName, Guid rootUUID, Guid nodeUUID, Guid destRootUUID, Guid destNodeUUID, bool writeDbmsImmediately = true) PN_OVERRIDE;

		virtual bool UnilateralAddData(Guid rootUUID, Guid ownerUUID, CPropNodePtr addData, bool writeDbmsImmediately = true) PN_OVERRIDE;

		virtual bool UnilateralUpdateData(CPropNodePtr updateData, bool writeDbmsImmediately = true) PN_OVERRIDE;

		virtual bool UnilateralRemoveData(Guid rootUUID, Guid removeUUID, bool writeDbmsImmediately = true) PN_OVERRIDE;

		virtual bool UnilateralRecursiveUpdateData(CLoadedData2Ptr loadedData, bool transactional = false, bool writeDbmsImmediately = true) PN_OVERRIDE;

		// Unload 메서드 DB Cache Client에서 Load를 해제 시킨다.
		virtual bool UnloadDataBySessionGuid(Guid sessionGuid, ByteArray& messageToNextLoader = ByteArray()) PN_OVERRIDE;

		virtual void DenyUnloadData(Guid sessionGuid, ByteArray& messageToRequester = ByteArray()) PN_OVERRIDE;

		/** 이 메서드를 호출하면 unload된 데이터를 완전히 unload를 시행한다. 즉 DB cache server2의 메모리에서도 제거해버린다. */
		virtual void ForceCompleteUnload(Guid rootUUID) PN_OVERRIDE;

		// 비독점 메서드.
		virtual void RequestNonExclusiveSnapshotData(String rootTableName, String fieldName, CVariant cmpValue, intptr_t tag) PN_OVERRIDE;

		virtual void RequestNonExclusiveSnapshotDataByGuid(String rootTableName, Guid rootUUID, intptr_t tag) PN_OVERRIDE;

		virtual void RequestNonExclusiveSnapshotDataByQuery(String rootTableName, String searchString, intptr_t tag) PN_OVERRIDE;

		virtual void RequestNonExclusiveAddData(String rootTableName, Guid rootUUID, Guid ownerUUID, CPropNodePtr addData, intptr_t tag, ByteArray &messageToLoader = ByteArray()) PN_OVERRIDE;

		virtual void RequestNonExclusiveRemoveData(String rootTableName, Guid rootUUID, Guid removeUUID, intptr_t tag, ByteArray &messageToLoader = ByteArray()) PN_OVERRIDE;

		virtual void RequestNonExclusiveSetValueIf(String rootTableName, Guid rootUUID, Guid nodeUUID, String propertyName, CVariant newValue, ValueCompareType compareType, CVariant compareValue, intptr_t tag, ByteArray &messageToLoader = ByteArray()) PN_OVERRIDE;

		virtual void RequestNonExclusiveModifyValue(String rootTableName, Guid rootUUID, Guid nodeUUID, String propertyName, ValueOperType operType, CVariant value, intptr_t tag, ByteArray &messageToLoader = ByteArray()) PN_OVERRIDE;

		virtual bool RequestIsolateData(Guid rootUUID, String rootTableName, Guid &outSessionGuid) PN_OVERRIDE;

		virtual bool RequestDeisolateData(Guid rootUUID, String filterText, Guid &outSessionGuid) PN_OVERRIDE;

		// Get 메서드.
		virtual CLoadedData2Ptr GetClonedLoadedDataBySessionGuid(Guid sessionGuid);

		virtual CLoadedData2Ptr GetClonedLoadedDataByUUID(Guid rootUUID);

		/** 이 클라이언트의 HostID를 얻는다. */
		virtual HostID GetLocalHostID() PN_OVERRIDE;

		/** 이 DB cache client2가 DB cache server2에 로그온(인증)이 완료된 상태인지 여부를 구한다. */
		virtual bool IsLoggedOn();

		//////////////////////////////////////////////////////////////////////////

		//rmi stub함수들
		DECRMI_DB2S2C_NotifyDbCacheClientLogonFailed;
		DECRMI_DB2S2C_NotifyDbCacheClientLogonSuccess;

		DECRMI_DB2S2C_NotifyLoadDataComplete;

		DECRMI_DB2S2C_NotifyDataUnloadRequested;
		// 		DECRMI_DB2S2C_NotifyDataUnloadRequestedList;
		// 		DECRMI_DB2S2C_NotifyDataUnloadRequestedListCommonMessage;

		DECRMI_DB2S2C_NotifyAddDataFailed;
		DECRMI_DB2S2C_NotifyAddDataSuccess;

		DECRMI_DB2S2C_NotifyUpdateDataFailed;
		DECRMI_DB2S2C_NotifyUpdateDataSuccess;

		DECRMI_DB2S2C_NotifyUpdateDataListFailed;
		DECRMI_DB2S2C_NotifyUpdateDataListSuccess;

		DECRMI_DB2S2C_NotifyRemoveDataFailed;
		DECRMI_DB2S2C_NotifyRemoveDataSuccess;

		DECRMI_DB2S2C_NotifyNonExclusiveAddDataFailed;
		DECRMI_DB2S2C_NotifyNonExclusiveAddDataSuccess;

		DECRMI_DB2S2C_NotifySomeoneAddData;

		DECRMI_DB2S2C_NotifyNonExclusiveRemoveDataFailed;
		DECRMI_DB2S2C_NotifyNonExclusiveRemoveDataSuccess;

		DECRMI_DB2S2C_NotifySomeoneRemoveData;

		DECRMI_DB2S2C_NotifyNonExclusiveSetValueFailed;
		DECRMI_DB2S2C_NotifyNonExclusiveSetValueSuccess;

		DECRMI_DB2S2C_NotifySomeoneSetValue;

		DECRMI_DB2S2C_NotifyNonExclusiveModifyValueFailed;
		DECRMI_DB2S2C_NotifyNonExclusiveModifyValueSuccess;

		DECRMI_DB2S2C_NotifyDeisolateDataSuccess;
		DECRMI_DB2S2C_NotifyDeisolateDataFailed;
		DECRMI_DB2S2C_NotifyIsolateDataSuccess;
		DECRMI_DB2S2C_NotifyIsolateDataFailed;

		DECRMI_DB2S2C_NotifySomeoneModifyValue;

		DECRMI_DB2S2C_NotifyDbmsWriteDone;
		DECRMI_DB2S2C_NotifyDbmsAccessError;

		DECRMI_DB2S2C_NotifyWarning;
		DECRMI_DB2S2C_NotifyDataForceUnloaded;

	private:
		bool IsCurrentThreadUserWorker();

	};
}
