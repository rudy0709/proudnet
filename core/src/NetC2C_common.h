#pragma once

#include "CompactFieldMap.h"
namespace ProudC2C {


	//Message ID that replies to each RMI method. 
               
    static const ::Proud::RmiID Rmi_HolsterP2PHolepunchTrial = (::Proud::RmiID)(65000+1);
               
    static const ::Proud::RmiID Rmi_ReportUdpMessageCount = (::Proud::RmiID)(65000+2);
               
    static const ::Proud::RmiID Rmi_RoundTripLatencyPing = (::Proud::RmiID)(65000+3);
               
    static const ::Proud::RmiID Rmi_RoundTripLatencyPong = (::Proud::RmiID)(65000+4);

	// List that has RMI ID.
	extern ::Proud::RmiID g_RmiIDList[];
	// RmiID List Count
	extern int g_RmiIDListCount;

}


 

// Forward declarations


// Declarations



