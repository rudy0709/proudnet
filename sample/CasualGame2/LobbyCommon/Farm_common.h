#pragma once

namespace FarmC2S {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_RequestFarmLogon = (::Proud::RmiID)(10000+1);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


namespace FarmS2C {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_NotifyFarmLogonFailed = (::Proud::RmiID)(11000+1);
               
    static const ::Proud::RmiID Rmi_NotifyFarmLogonSuccess = (::Proud::RmiID)(11000+2);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


namespace FarmC2C {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_NotifyFarmClientInfo = (::Proud::RmiID)(12000+1);
               
    static const ::Proud::RmiID Rmi_RequestCreateGameRoom = (::Proud::RmiID)(12000+2);
               
    static const ::Proud::RmiID Rmi_NotifyCreateGameRoomResult = (::Proud::RmiID)(12000+3);
               
    static const ::Proud::RmiID Rmi_GameRoom_Appear = (::Proud::RmiID)(12000+4);
               
    static const ::Proud::RmiID Rmi_GameRoom_ShowState = (::Proud::RmiID)(12000+5);
               
    static const ::Proud::RmiID Rmi_GameRoom_Disappear = (::Proud::RmiID)(12000+6);
               
    static const ::Proud::RmiID Rmi_RequestJoinGameRoom = (::Proud::RmiID)(12000+7);
               
    static const ::Proud::RmiID Rmi_NotifyJoinGameRoomResult = (::Proud::RmiID)(12000+8);
               
    static const ::Proud::RmiID Rmi_NotifyStatusServer = (::Proud::RmiID)(12000+9);
               
    static const ::Proud::RmiID Rmi_RequestCreateCredential = (::Proud::RmiID)(12000+10);
               
    static const ::Proud::RmiID Rmi_NotifyCreatedCredential = (::Proud::RmiID)(12000+11);
               
    static const ::Proud::RmiID Rmi_RequestCheckCredential = (::Proud::RmiID)(12000+12);
               
    static const ::Proud::RmiID Rmi_NotifyCheckCredentialSuccess = (::Proud::RmiID)(12000+13);
               
    static const ::Proud::RmiID Rmi_NotifyCheckCredentialFail = (::Proud::RmiID)(12000+14);
               
    static const ::Proud::RmiID Rmi_UserLogOut = (::Proud::RmiID)(12000+15);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


 

// Forward declarations


// Declarations



