﻿
// Generated by PIDL compiler.
// Do not modify this file, but modify the source .pidl file.


#include "DB_common.h"
namespace DB2C2S {


	::Proud::RmiID g_RmiIDList[] = {
               
		Rmi_RequestDbCacheClientLogon,
               
		Rmi_RequestExclusiveLoadDataByFieldNameAndValue,
               
		Rmi_RequestExclusiveLoadDataByGuid,
               
		Rmi_RequestExclusiveLoadDataByQuery,
               
		Rmi_RequestExclusiveLoadNewData,
               
		Rmi_RequestUnloadDataBySessionGuid,
               
		Rmi_DenyUnloadData,
               
		Rmi_RequestForceCompleteUnload,
               
		Rmi_RequestAddData,
               
		Rmi_RequestUpdateData,
               
		Rmi_RequestRemoveData,
               
		Rmi_RequestUpdateDataList,
               
		Rmi_AddData,
               
		Rmi_UpdateData,
               
		Rmi_RemoveData,
               
		Rmi_UpdateDataList,
               
		Rmi_MoveData,
               
		Rmi_RequestNonExclusiveSnapshotDataByFieldNameAndValue,
               
		Rmi_RequestNonExclusiveSnapshotDataByGuid,
               
		Rmi_RequestNonExclusiveSnapshotDataByQuery,
               
		Rmi_RequestNonExclusiveAddData,
               
		Rmi_RequestNonExclusiveRemoveData,
               
		Rmi_RequestNonExclusiveSetValueIf,
               
		Rmi_RequestNonExclusiveModifyValue,
               
		Rmi_RequestIsolateData,
               
		Rmi_RequestDeisolateData,
	};

	int g_RmiIDListCount = 26;

}


namespace DB2S2C {


	::Proud::RmiID g_RmiIDList[] = {
               
		Rmi_NotifyDbCacheClientLogonFailed,
               
		Rmi_NotifyDbCacheClientLogonSuccess,
               
		Rmi_NotifyLoadDataComplete,
               
		Rmi_NotifyDataUnloadRequested,
               
		Rmi_NotifyUnloadRequestTimeoutTimeMs,
               
		Rmi_NotifyAddDataFailed,
               
		Rmi_NotifyAddDataSuccess,
               
		Rmi_NotifyUpdateDataFailed,
               
		Rmi_NotifyUpdateDataSuccess,
               
		Rmi_NotifyUpdateDataListFailed,
               
		Rmi_NotifyUpdateDataListSuccess,
               
		Rmi_NotifyRemoveDataFailed,
               
		Rmi_NotifyRemoveDataSuccess,
               
		Rmi_NotifyNonExclusiveAddDataFailed,
               
		Rmi_NotifyNonExclusiveAddDataSuccess,
               
		Rmi_NotifySomeoneAddData,
               
		Rmi_NotifyNonExclusiveRemoveDataFailed,
               
		Rmi_NotifyNonExclusiveRemoveDataSuccess,
               
		Rmi_NotifySomeoneRemoveData,
               
		Rmi_NotifyNonExclusiveSetValueFailed,
               
		Rmi_NotifyNonExclusiveSetValueSuccess,
               
		Rmi_NotifySomeoneSetValue,
               
		Rmi_NotifyNonExclusiveModifyValueFailed,
               
		Rmi_NotifyNonExclusiveModifyValueSuccess,
               
		Rmi_NotifySomeoneModifyValue,
               
		Rmi_NotifyWarning,
               
		Rmi_NotifyDbmsWriteDone,
               
		Rmi_NotifyDbmsAccessError,
               
		Rmi_NotifyIsolateDataSuccess,
               
		Rmi_NotifyIsolateDataFailed,
               
		Rmi_NotifyDeisolateDataSuccess,
               
		Rmi_NotifyDeisolateDataFailed,
               
		Rmi_NotifyDataForceUnloaded,
	};

	int g_RmiIDListCount = 33;

}





