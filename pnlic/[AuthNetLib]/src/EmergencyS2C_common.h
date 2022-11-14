#pragma once

namespace EmergencyS2C {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_EmergencyLogData_AckComplete = (::Proud::RmiID)(750+1);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


 
