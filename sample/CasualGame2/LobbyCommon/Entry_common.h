#pragma once

namespace EntryC2S {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_RequestReturnToEntry = (::Proud::RmiID)(1500+1);
               
    static const ::Proud::RmiID Rmi_RequestCreateNewGamer = (::Proud::RmiID)(1500+2);
               
    static const ::Proud::RmiID Rmi_RequestFirstLogon = (::Proud::RmiID)(1500+3);
               
    static const ::Proud::RmiID Rmi_RequestHeroSlots = (::Proud::RmiID)(1500+4);
               
    static const ::Proud::RmiID Rmi_RequestSelectHero = (::Proud::RmiID)(1500+5);
               
    static const ::Proud::RmiID Rmi_RequestAddHero = (::Proud::RmiID)(1500+6);
               
    static const ::Proud::RmiID Rmi_RequestRemoveHero = (::Proud::RmiID)(1500+7);
               
    static const ::Proud::RmiID Rmi_RequestLobbyList = (::Proud::RmiID)(1500+8);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


namespace EntryS2C {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_NotifyCreateNewGamerSuccess = (::Proud::RmiID)(2000+1);
               
    static const ::Proud::RmiID Rmi_NotifyCreateNewGamerFailed = (::Proud::RmiID)(2000+2);
               
    static const ::Proud::RmiID Rmi_NotifyUnauthedAccess = (::Proud::RmiID)(2000+3);
               
    static const ::Proud::RmiID Rmi_ShowError = (::Proud::RmiID)(2000+4);
               
    static const ::Proud::RmiID Rmi_NotifyFirstLogonFailed = (::Proud::RmiID)(2000+5);
               
    static const ::Proud::RmiID Rmi_NotifyReturnToEntryFailed = (::Proud::RmiID)(2000+6);
               
    static const ::Proud::RmiID Rmi_NotifyFirstLogonSuccess = (::Proud::RmiID)(2000+7);
               
    static const ::Proud::RmiID Rmi_NotifySelectHeroFailed = (::Proud::RmiID)(2000+8);
               
    static const ::Proud::RmiID Rmi_NotifySelectHeroSuccess = (::Proud::RmiID)(2000+9);
               
    static const ::Proud::RmiID Rmi_HeroList_Begin = (::Proud::RmiID)(2000+10);
               
    static const ::Proud::RmiID Rmi_HeroList_Add = (::Proud::RmiID)(2000+11);
               
    static const ::Proud::RmiID Rmi_HeroList_End = (::Proud::RmiID)(2000+12);
               
    static const ::Proud::RmiID Rmi_RemovedHeroList_Begin = (::Proud::RmiID)(2000+13);
               
    static const ::Proud::RmiID Rmi_RemovedHeroList_Add = (::Proud::RmiID)(2000+14);
               
    static const ::Proud::RmiID Rmi_RemovedHeroList_End = (::Proud::RmiID)(2000+15);
               
    static const ::Proud::RmiID Rmi_NotifySelectedHero = (::Proud::RmiID)(2000+16);
               
    static const ::Proud::RmiID Rmi_NotifyAddHeroSuccess = (::Proud::RmiID)(2000+17);
               
    static const ::Proud::RmiID Rmi_NotifyAddHeroFailed = (::Proud::RmiID)(2000+18);
               
    static const ::Proud::RmiID Rmi_NotifyRemoveHeroSuccess = (::Proud::RmiID)(2000+19);
               
    static const ::Proud::RmiID Rmi_LobbyList_Begin = (::Proud::RmiID)(2000+20);
               
    static const ::Proud::RmiID Rmi_LobbyList_Add = (::Proud::RmiID)(2000+21);
               
    static const ::Proud::RmiID Rmi_LobbyList_End = (::Proud::RmiID)(2000+22);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


 

// Forward declarations


// Declarations



