﻿
// Generated by PIDL compiler.
// Do not modify this file, but modify the source .pidl file.


#include "Viz_common.h"
namespace VizC2S {


	::Proud::RmiID g_RmiIDList[] = {
               
		Rmi_RequestLogin,
               
		Rmi_NotifyCommon_SendRmi,
               
		Rmi_NotifyCommon_ReceiveRmi,
               
		Rmi_NotifyCli_ConnectionState,
               
		Rmi_NotifyCli_Peers_Clear,
               
		Rmi_NotifyCli_Peers_AddOrEdit,
               
		Rmi_NotifySrv_ClientEmpty,
               
		Rmi_NotifySrv_Clients_AddOrEdit,
               
		Rmi_NotifySrv_Clients_Remove,
	};

	int g_RmiIDListCount = 9;

}


namespace VizS2C {


	::Proud::RmiID g_RmiIDList[] = {
               
		Rmi_NotifyLoginOk,
               
		Rmi_NotifyLoginFailed,
	};

	int g_RmiIDListCount = 2;

}


