#pragma once

namespace Simple {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_Chat = (::Proud::RmiID)(2000+1);
               
    static const ::Proud::RmiID Rmi_ShowChat = (::Proud::RmiID)(2000+2);
               
    static const ::Proud::RmiID Rmi_SystemChat = (::Proud::RmiID)(2000+3);
               
    static const ::Proud::RmiID Rmi_P2PChat = (::Proud::RmiID)(2000+4);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


 

// Forward declarations


// Declarations



