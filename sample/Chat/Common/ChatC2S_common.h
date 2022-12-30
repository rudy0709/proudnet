#pragma once

namespace ChatC2S {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_RequestLogon = (::Proud::RmiID)(4000+1);
               
    static const ::Proud::RmiID Rmi_Chat = (::Proud::RmiID)(4000+2);
               
    static const ::Proud::RmiID Rmi_RequestP2PGroup = (::Proud::RmiID)(4000+3);
               
    static const ::Proud::RmiID Rmi_RequestLeaveP2PGroup = (::Proud::RmiID)(4000+4);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


 

// Forward declarations


// Declarations



