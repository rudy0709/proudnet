#pragma once

namespace ProudS2C {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_P2PGroup_MemberJoin = (::Proud::RmiID)(63000+1);
               
    static const ::Proud::RmiID Rmi_RequestP2PHolepunch = (::Proud::RmiID)(63000+2);
               
    static const ::Proud::RmiID Rmi_P2P_NotifyDirectP2PDisconnected2 = (::Proud::RmiID)(63000+3);
               
    static const ::Proud::RmiID Rmi_P2P_NotifyP2PMemberOffline = (::Proud::RmiID)(63000+4);
               
    static const ::Proud::RmiID Rmi_P2P_NotifyP2PMemberOnline = (::Proud::RmiID)(63000+5);
               
    static const ::Proud::RmiID Rmi_P2PGroup_MemberLeave = (::Proud::RmiID)(63000+6);
               
    static const ::Proud::RmiID Rmi_NotifyDirectP2PEstablish = (::Proud::RmiID)(63000+7);
               
    static const ::Proud::RmiID Rmi_ReliablePong = (::Proud::RmiID)(63000+8);
               
    static const ::Proud::RmiID Rmi_EnableLog = (::Proud::RmiID)(63000+9);
               
    static const ::Proud::RmiID Rmi_DisableLog = (::Proud::RmiID)(63000+10);
               
    static const ::Proud::RmiID Rmi_NotifyUdpToTcpFallbackByServer = (::Proud::RmiID)(63000+11);
               
    static const ::Proud::RmiID Rmi_NotifySpeedHackDetectorEnabled = (::Proud::RmiID)(63000+12);
               
    static const ::Proud::RmiID Rmi_ShutdownTcpAck = (::Proud::RmiID)(63000+13);
               
    static const ::Proud::RmiID Rmi_RequestAutoPrune = (::Proud::RmiID)(63000+14);
               
    static const ::Proud::RmiID Rmi_NewDirectP2PConnection = (::Proud::RmiID)(63000+15);
               
    static const ::Proud::RmiID Rmi_RequestMeasureSendSpeed = (::Proud::RmiID)(63000+16);
               
    static const ::Proud::RmiID Rmi_S2C_RequestCreateUdpSocket = (::Proud::RmiID)(63000+17);
               
    static const ::Proud::RmiID Rmi_S2C_CreateUdpSocketAck = (::Proud::RmiID)(63000+18);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


 
