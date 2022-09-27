#pragma once

namespace AgentS2C {


    //Message ID that replies to each RMI method.
    static const ::Proud::RmiID Rmi_NotifyCredential = (::Proud::RmiID)(150+1);
    static const ::Proud::RmiID Rmi_RequestServerAppStop = (::Proud::RmiID)(150+2);

    // List that has RMI ID.
    extern ::Proud::RmiID g_RmiIDList[];
    // RmiID List Count
    extern int g_RmiIDListCount;

}



// Forward declarations


// Declarations


