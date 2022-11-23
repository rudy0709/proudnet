#pragma once

namespace LicenseC2S {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_RequestLicenseKey = (::Proud::RmiID)(550+1);
               
    static const ::Proud::RmiID Rmi_RequestAuthentication = (::Proud::RmiID)(550+2);
               
    static const ::Proud::RmiID Rmi_RequestRecordServerInfo = (::Proud::RmiID)(550+3);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


 
