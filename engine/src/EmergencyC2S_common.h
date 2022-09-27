#pragma once

namespace EmergencyC2S {


    //Message ID that replies to each RMI method.
    static const ::Proud::RmiID Rmi_EmergencyLogData_Begin = (::Proud::RmiID)(700+1);
    static const ::Proud::RmiID Rmi_EmergencyLogData_Error = (::Proud::RmiID)(700+2);
    static const ::Proud::RmiID Rmi_EmergencyLogData_Stats = (::Proud::RmiID)(700+3);
    static const ::Proud::RmiID Rmi_EmergencyLogData_OSVersion = (::Proud::RmiID)(700+4);
    static const ::Proud::RmiID Rmi_EmergencyLogData_LogEvent = (::Proud::RmiID)(700+5);
    static const ::Proud::RmiID Rmi_EmergencyLogData_End = (::Proud::RmiID)(700+6);

    // List that has RMI ID.
    extern ::Proud::RmiID g_RmiIDList[];
    // RmiID List Count
    extern int g_RmiIDListCount;

}



// Forward declarations


// Declarations


