#pragma once

namespace ProudC2C {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_HolsterP2PHolepunchTrial = (::Proud::RmiID)(65000+1);
               
    static const ::Proud::RmiID Rmi_ReportUdpMessageCount = (::Proud::RmiID)(65000+2);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


 
