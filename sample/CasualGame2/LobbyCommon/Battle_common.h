#pragma once

namespace BattleC2S {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_RequestNextLogon = (::Proud::RmiID)(5000+1);
               
    static const ::Proud::RmiID Rmi_RequestToggleBattleReady = (::Proud::RmiID)(5000+2);
               
    static const ::Proud::RmiID Rmi_RequestStartPlayMode = (::Proud::RmiID)(5000+3);
               
    static const ::Proud::RmiID Rmi_NotifyLoadFinished = (::Proud::RmiID)(5000+4);
               
    static const ::Proud::RmiID Rmi_LeaveBattleRoom = (::Proud::RmiID)(5000+5);
               
    static const ::Proud::RmiID Rmi_RequestLocalHeroSpawn = (::Proud::RmiID)(5000+6);
               
    static const ::Proud::RmiID Rmi_RequestBulletSpawn = (::Proud::RmiID)(5000+7);
               
    static const ::Proud::RmiID Rmi_LocalHero_Move = (::Proud::RmiID)(5000+8);
               
    static const ::Proud::RmiID Rmi_NotifyGotoLobbyServer = (::Proud::RmiID)(5000+9);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


namespace BattleS2C {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_NotifyUnauthedAccess = (::Proud::RmiID)(6000+1);
               
    static const ::Proud::RmiID Rmi_ShowError = (::Proud::RmiID)(6000+2);
               
    static const ::Proud::RmiID Rmi_NotifyNextLogonFailed = (::Proud::RmiID)(6000+3);
               
    static const ::Proud::RmiID Rmi_NotifyNextLogonSuccess = (::Proud::RmiID)(6000+4);
               
    static const ::Proud::RmiID Rmi_HeroSlot_Appear = (::Proud::RmiID)(6000+5);
               
    static const ::Proud::RmiID Rmi_HeroSlot_Disappear = (::Proud::RmiID)(6000+6);
               
    static const ::Proud::RmiID Rmi_HeroSlot_ShowState = (::Proud::RmiID)(6000+7);
               
    static const ::Proud::RmiID Rmi_NotifyGameRoomInfo = (::Proud::RmiID)(6000+8);
               
    static const ::Proud::RmiID Rmi_NotifyStartPlayModeFailed = (::Proud::RmiID)(6000+9);
               
    static const ::Proud::RmiID Rmi_GotoWaitingMode = (::Proud::RmiID)(6000+10);
               
    static const ::Proud::RmiID Rmi_GotoLoadingMode = (::Proud::RmiID)(6000+11);
               
    static const ::Proud::RmiID Rmi_GotoPlayMode = (::Proud::RmiID)(6000+12);
               
    static const ::Proud::RmiID Rmi_NotifyLocalHeroViewersGroupID = (::Proud::RmiID)(6000+13);
               
    static const ::Proud::RmiID Rmi_RemoteHero_Appear = (::Proud::RmiID)(6000+14);
               
    static const ::Proud::RmiID Rmi_RemoteHero_Disappear = (::Proud::RmiID)(6000+15);
               
    static const ::Proud::RmiID Rmi_NotifyBullet_Create = (::Proud::RmiID)(6000+16);
               
    static const ::Proud::RmiID Rmi_NotifyBullet_Move = (::Proud::RmiID)(6000+17);
               
    static const ::Proud::RmiID Rmi_NotifyBullet_Delete = (::Proud::RmiID)(6000+18);
               
    static const ::Proud::RmiID Rmi_NotifyHeroScore = (::Proud::RmiID)(6000+19);
               
    static const ::Proud::RmiID Rmi_NotyfyGotoLobby = (::Proud::RmiID)(6000+20);
               
    static const ::Proud::RmiID Rmi_NotifyItem_Create = (::Proud::RmiID)(6000+21);
               
    static const ::Proud::RmiID Rmi_NotifyItem_Delete = (::Proud::RmiID)(6000+22);
               
    static const ::Proud::RmiID Rmi_NotifyBullet_Clear = (::Proud::RmiID)(6000+23);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


namespace BattleC2C {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_P2P_Chat = (::Proud::RmiID)(7000+1);
               
    static const ::Proud::RmiID Rmi_P2P_LocalHero_Move = (::Proud::RmiID)(7000+2);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


 

// Forward declarations


// Declarations



