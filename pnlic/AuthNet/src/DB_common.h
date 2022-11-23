#pragma once

namespace DB2C2S {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_RequestDbCacheClientLogon = (::Proud::RmiID)(400+1);
               
    static const ::Proud::RmiID Rmi_RequestExclusiveLoadDataByFieldNameAndValue = (::Proud::RmiID)(400+2);
               
    static const ::Proud::RmiID Rmi_RequestExclusiveLoadDataByGuid = (::Proud::RmiID)(400+3);
               
    static const ::Proud::RmiID Rmi_RequestExclusiveLoadDataByQuery = (::Proud::RmiID)(400+4);
               
    static const ::Proud::RmiID Rmi_RequestExclusiveLoadNewData = (::Proud::RmiID)(400+5);
               
    static const ::Proud::RmiID Rmi_RequestUnloadDataBySessionGuid = (::Proud::RmiID)(400+6);
               
    static const ::Proud::RmiID Rmi_DenyUnloadData = (::Proud::RmiID)(400+7);
               
    static const ::Proud::RmiID Rmi_RequestForceCompleteUnload = (::Proud::RmiID)(400+8);
               
    static const ::Proud::RmiID Rmi_RequestAddData = (::Proud::RmiID)(400+9);
               
    static const ::Proud::RmiID Rmi_RequestUpdateData = (::Proud::RmiID)(400+10);
               
    static const ::Proud::RmiID Rmi_RequestRemoveData = (::Proud::RmiID)(400+11);
               
    static const ::Proud::RmiID Rmi_RequestUpdateDataList = (::Proud::RmiID)(400+12);
               
    static const ::Proud::RmiID Rmi_AddData = (::Proud::RmiID)(400+13);
               
    static const ::Proud::RmiID Rmi_UpdateData = (::Proud::RmiID)(400+14);
               
    static const ::Proud::RmiID Rmi_RemoveData = (::Proud::RmiID)(400+15);
               
    static const ::Proud::RmiID Rmi_UpdateDataList = (::Proud::RmiID)(400+16);
               
    static const ::Proud::RmiID Rmi_MoveData = (::Proud::RmiID)(400+17);
               
    static const ::Proud::RmiID Rmi_RequestNonExclusiveSnapshotDataByFieldNameAndValue = (::Proud::RmiID)(400+18);
               
    static const ::Proud::RmiID Rmi_RequestNonExclusiveSnapshotDataByGuid = (::Proud::RmiID)(400+19);
               
    static const ::Proud::RmiID Rmi_RequestNonExclusiveSnapshotDataByQuery = (::Proud::RmiID)(400+20);
               
    static const ::Proud::RmiID Rmi_RequestNonExclusiveAddData = (::Proud::RmiID)(400+21);
               
    static const ::Proud::RmiID Rmi_RequestNonExclusiveRemoveData = (::Proud::RmiID)(400+22);
               
    static const ::Proud::RmiID Rmi_RequestNonExclusiveSetValueIf = (::Proud::RmiID)(400+23);
               
    static const ::Proud::RmiID Rmi_RequestNonExclusiveModifyValue = (::Proud::RmiID)(400+24);
               
    static const ::Proud::RmiID Rmi_RequestIsolateData = (::Proud::RmiID)(400+25);
               
    static const ::Proud::RmiID Rmi_RequestDeisolateData = (::Proud::RmiID)(400+26);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


namespace DB2S2C {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_NotifyDbCacheClientLogonFailed = (::Proud::RmiID)(500+1);
               
    static const ::Proud::RmiID Rmi_NotifyDbCacheClientLogonSuccess = (::Proud::RmiID)(500+2);
               
    static const ::Proud::RmiID Rmi_NotifyLoadDataComplete = (::Proud::RmiID)(500+3);
               
    static const ::Proud::RmiID Rmi_NotifyDataUnloadRequested = (::Proud::RmiID)(500+4);
               
    static const ::Proud::RmiID Rmi_NotifyUnloadRequestTimeoutTimeMs = (::Proud::RmiID)(500+5);
               
    static const ::Proud::RmiID Rmi_NotifyAddDataFailed = (::Proud::RmiID)(500+6);
               
    static const ::Proud::RmiID Rmi_NotifyAddDataSuccess = (::Proud::RmiID)(500+7);
               
    static const ::Proud::RmiID Rmi_NotifyUpdateDataFailed = (::Proud::RmiID)(500+8);
               
    static const ::Proud::RmiID Rmi_NotifyUpdateDataSuccess = (::Proud::RmiID)(500+9);
               
    static const ::Proud::RmiID Rmi_NotifyUpdateDataListFailed = (::Proud::RmiID)(500+10);
               
    static const ::Proud::RmiID Rmi_NotifyUpdateDataListSuccess = (::Proud::RmiID)(500+11);
               
    static const ::Proud::RmiID Rmi_NotifyRemoveDataFailed = (::Proud::RmiID)(500+12);
               
    static const ::Proud::RmiID Rmi_NotifyRemoveDataSuccess = (::Proud::RmiID)(500+13);
               
    static const ::Proud::RmiID Rmi_NotifyNonExclusiveAddDataFailed = (::Proud::RmiID)(500+14);
               
    static const ::Proud::RmiID Rmi_NotifyNonExclusiveAddDataSuccess = (::Proud::RmiID)(500+15);
               
    static const ::Proud::RmiID Rmi_NotifySomeoneAddData = (::Proud::RmiID)(500+16);
               
    static const ::Proud::RmiID Rmi_NotifyNonExclusiveRemoveDataFailed = (::Proud::RmiID)(500+17);
               
    static const ::Proud::RmiID Rmi_NotifyNonExclusiveRemoveDataSuccess = (::Proud::RmiID)(500+18);
               
    static const ::Proud::RmiID Rmi_NotifySomeoneRemoveData = (::Proud::RmiID)(500+19);
               
    static const ::Proud::RmiID Rmi_NotifyNonExclusiveSetValueFailed = (::Proud::RmiID)(500+20);
               
    static const ::Proud::RmiID Rmi_NotifyNonExclusiveSetValueSuccess = (::Proud::RmiID)(500+21);
               
    static const ::Proud::RmiID Rmi_NotifySomeoneSetValue = (::Proud::RmiID)(500+22);
               
    static const ::Proud::RmiID Rmi_NotifyNonExclusiveModifyValueFailed = (::Proud::RmiID)(500+23);
               
    static const ::Proud::RmiID Rmi_NotifyNonExclusiveModifyValueSuccess = (::Proud::RmiID)(500+24);
               
    static const ::Proud::RmiID Rmi_NotifySomeoneModifyValue = (::Proud::RmiID)(500+25);
               
    static const ::Proud::RmiID Rmi_NotifyWarning = (::Proud::RmiID)(500+26);
               
    static const ::Proud::RmiID Rmi_NotifyDbmsWriteDone = (::Proud::RmiID)(500+27);
               
    static const ::Proud::RmiID Rmi_NotifyDbmsAccessError = (::Proud::RmiID)(500+28);
               
    static const ::Proud::RmiID Rmi_NotifyIsolateDataSuccess = (::Proud::RmiID)(500+29);
               
    static const ::Proud::RmiID Rmi_NotifyIsolateDataFailed = (::Proud::RmiID)(500+30);
               
    static const ::Proud::RmiID Rmi_NotifyDeisolateDataSuccess = (::Proud::RmiID)(500+31);
               
    static const ::Proud::RmiID Rmi_NotifyDeisolateDataFailed = (::Proud::RmiID)(500+32);
               
    static const ::Proud::RmiID Rmi_NotifyDataForceUnloaded = (::Proud::RmiID)(500+33);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


 
