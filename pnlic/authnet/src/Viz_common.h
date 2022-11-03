#pragma once

namespace VizC2S {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_RequestLogin = (::Proud::RmiID)(1200+1);
               
    static const ::Proud::RmiID Rmi_NotifyCommon_SendRmi = (::Proud::RmiID)(1200+2);
               
    static const ::Proud::RmiID Rmi_NotifyCommon_ReceiveRmi = (::Proud::RmiID)(1200+3);
               
    static const ::Proud::RmiID Rmi_NotifyCli_ConnectionState = (::Proud::RmiID)(1200+4);
               
    static const ::Proud::RmiID Rmi_NotifyCli_Peers_Clear = (::Proud::RmiID)(1200+5);
               
    static const ::Proud::RmiID Rmi_NotifyCli_Peers_AddOrEdit = (::Proud::RmiID)(1200+6);
               
    static const ::Proud::RmiID Rmi_NotifySrv_ClientEmpty = (::Proud::RmiID)(1200+7);
               
    static const ::Proud::RmiID Rmi_NotifySrv_Clients_AddOrEdit = (::Proud::RmiID)(1200+8);
               
    static const ::Proud::RmiID Rmi_NotifySrv_Clients_Remove = (::Proud::RmiID)(1200+9);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


namespace VizS2C {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_NotifyLoginOk = (::Proud::RmiID)(1250+1);
               
    static const ::Proud::RmiID Rmi_NotifyLoginFailed = (::Proud::RmiID)(1250+2);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


 
