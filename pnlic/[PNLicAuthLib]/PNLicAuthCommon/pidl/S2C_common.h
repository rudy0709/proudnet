#pragma once

namespace LicenseS2C {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_NotifyAuthentication = (::Proud::RmiID)(550+1);
               
    static const ::Proud::RmiID Rmi_NotifyAuthenticationComment = (::Proud::RmiID)(550+2);
               
    static const ::Proud::RmiID Rmi_NotifyRecordServerInfo = (::Proud::RmiID)(550+3);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


 
