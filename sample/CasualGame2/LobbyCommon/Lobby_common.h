#pragma once

namespace LobbyC2S {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_RequestNextLogon = (::Proud::RmiID)(3000+1);
               
    static const ::Proud::RmiID Rmi_NotifyChannelFormReady = (::Proud::RmiID)(3000+2);
               
    static const ::Proud::RmiID Rmi_Chat = (::Proud::RmiID)(3000+3);
               
    static const ::Proud::RmiID Rmi_RequestCreateGameRoom = (::Proud::RmiID)(3000+4);
               
    static const ::Proud::RmiID Rmi_RequestJoinGameRoom = (::Proud::RmiID)(3000+5);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


namespace LobbyS2C {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_NotifyUnauthedAccess = (::Proud::RmiID)(4000+1);
               
    static const ::Proud::RmiID Rmi_ShowError = (::Proud::RmiID)(4000+2);
               
    static const ::Proud::RmiID Rmi_NotifyNextLogonFailed = (::Proud::RmiID)(4000+3);
               
    static const ::Proud::RmiID Rmi_NotifyNextLogonSuccess = (::Proud::RmiID)(4000+4);
               
    static const ::Proud::RmiID Rmi_HeroSlot_Appear = (::Proud::RmiID)(4000+5);
               
    static const ::Proud::RmiID Rmi_HeroSlot_Disappear = (::Proud::RmiID)(4000+6);
               
    static const ::Proud::RmiID Rmi_GameRoom_Appear = (::Proud::RmiID)(4000+7);
               
    static const ::Proud::RmiID Rmi_GameRoom_ShowState = (::Proud::RmiID)(4000+8);
               
    static const ::Proud::RmiID Rmi_GameRoom_Disappear = (::Proud::RmiID)(4000+9);
               
    static const ::Proud::RmiID Rmi_LocalHeroSlot_Appear = (::Proud::RmiID)(4000+10);
               
    static const ::Proud::RmiID Rmi_ShowChat = (::Proud::RmiID)(4000+11);
               
    static const ::Proud::RmiID Rmi_NotifyCreateRoomFailed = (::Proud::RmiID)(4000+12);
               
    static const ::Proud::RmiID Rmi_NotifyCreateRoomSuccess = (::Proud::RmiID)(4000+13);
               
    static const ::Proud::RmiID Rmi_NotifyJoinRoomFailed = (::Proud::RmiID)(4000+14);
               
    static const ::Proud::RmiID Rmi_NotifyJoinRoomSuccess = (::Proud::RmiID)(4000+15);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


 

// Forward declarations


// Declarations



