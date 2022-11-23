#pragma once

namespace ProudC2S {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_ReliablePing = (::Proud::RmiID)(64000+1);
               
    static const ::Proud::RmiID Rmi_P2P_NotifyDirectP2PDisconnected = (::Proud::RmiID)(64000+2);
               
    static const ::Proud::RmiID Rmi_NotifyUdpToTcpFallbackByClient = (::Proud::RmiID)(64000+3);
               
    static const ::Proud::RmiID Rmi_P2PGroup_MemberJoin_Ack = (::Proud::RmiID)(64000+4);
               
    static const ::Proud::RmiID Rmi_NotifyP2PHolepunchSuccess = (::Proud::RmiID)(64000+5);
               
    static const ::Proud::RmiID Rmi_ShutdownTcp = (::Proud::RmiID)(64000+6);
               
    static const ::Proud::RmiID Rmi_NotifyLog = (::Proud::RmiID)(64000+7);
               
    static const ::Proud::RmiID Rmi_NotifyLogHolepunchFreqFail = (::Proud::RmiID)(64000+8);
               
    static const ::Proud::RmiID Rmi_NotifyNatDeviceName = (::Proud::RmiID)(64000+9);
               
    static const ::Proud::RmiID Rmi_NotifyJitDirectP2PTriggered = (::Proud::RmiID)(64000+10);
               
    static const ::Proud::RmiID Rmi_NotifyNatDeviceNameDetected = (::Proud::RmiID)(64000+11);
               
    static const ::Proud::RmiID Rmi_NotifySendSpeed = (::Proud::RmiID)(64000+12);
               
    static const ::Proud::RmiID Rmi_ReportP2PPeerPing = (::Proud::RmiID)(64000+13);
               
    static const ::Proud::RmiID Rmi_C2S_RequestCreateUdpSocket = (::Proud::RmiID)(64000+14);
               
    static const ::Proud::RmiID Rmi_C2S_CreateUdpSocketAck = (::Proud::RmiID)(64000+15);
               
    static const ::Proud::RmiID Rmi_ReportC2CUdpMessageCount = (::Proud::RmiID)(64000+16);
               
    static const ::Proud::RmiID Rmi_ReportC2SUdpMessageTrialCount = (::Proud::RmiID)(64000+17);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


 
