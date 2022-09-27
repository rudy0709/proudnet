#pragma once

namespace AgentC2S {


    //Message ID that replies to each RMI method.
    static const ::Proud::RmiID Rmi_RequestCredential = (::Proud::RmiID)(100+1);
    static const ::Proud::RmiID Rmi_ReportStatusBegin = (::Proud::RmiID)(100+2);
    static const ::Proud::RmiID Rmi_ReportStatusValue = (::Proud::RmiID)(100+3);
    static const ::Proud::RmiID Rmi_ReportStatusEnd = (::Proud::RmiID)(100+4);
    static const ::Proud::RmiID Rmi_ReportServerAppState = (::Proud::RmiID)(100+5);
    static const ::Proud::RmiID Rmi_EventLog = (::Proud::RmiID)(100+6);

    // List that has RMI ID.
    extern ::Proud::RmiID g_RmiIDList[];
    // RmiID List Count
    extern int g_RmiIDListCount;

}



// Forward declarations


// Declarations


