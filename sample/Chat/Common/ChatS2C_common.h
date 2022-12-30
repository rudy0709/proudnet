#pragma once

namespace ChatS2C {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_ShowChat = (::Proud::RmiID)(2000+1);
               
    static const ::Proud::RmiID Rmi_UserList_Add = (::Proud::RmiID)(2000+2);
               
    static const ::Proud::RmiID Rmi_UserList_Remove = (::Proud::RmiID)(2000+3);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


 

// Forward declarations


// Declarations



