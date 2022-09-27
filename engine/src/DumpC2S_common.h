#pragma once

namespace DumpC2S {


    //Message ID that replies to each RMI method.
    static const ::Proud::RmiID Rmi_Dump_Start = (::Proud::RmiID)(600+1);
    static const ::Proud::RmiID Rmi_Dump_Chunk = (::Proud::RmiID)(600+2);
    static const ::Proud::RmiID Rmi_Dump_End = (::Proud::RmiID)(600+3);

    // List that has RMI ID.
    extern ::Proud::RmiID g_RmiIDList[];
    // RmiID List Count
    extern int g_RmiIDListCount;

}



// Forward declarations


// Declarations


