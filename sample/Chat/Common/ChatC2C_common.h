#pragma once

namespace ChatC2C {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_P2P_Chat = (::Proud::RmiID)(3000+1);
               
    static const ::Proud::RmiID Rmi_P2P_BigData = (::Proud::RmiID)(3000+2);
               
    static const ::Proud::RmiID Rmi_P2P_BigDataAck = (::Proud::RmiID)(3000+3);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


 

// Forward declarations


// Declarations



