﻿




// Generated by PIDL compiler.
// Do not modify this file, but modify the source .pidl file.


#include "EmergencyC2S_proxy.h"

namespace EmergencyC2S {


        
	bool Proxy::EmergencyLogData_Begin ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int64_t & loggingTime, const int32_t & connectCount, const uint32_t & remotePeerCount, const uint32_t & directP2PEnablePeerCount, const Proud::String & natDeviceName, const Proud::HostID & peerID, const int & iopendingcount, const uint64_t & totalTcpIssueSendBytes)	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_EmergencyLogData_Begin;
__msg.Write(__msgid);
	
__msg << loggingTime;

__msg << connectCount;

__msg << remotePeerCount;

__msg << directP2PEnablePeerCount;

__msg << natDeviceName;

__msg << peerID;

__msg << iopendingcount;

__msg << totalTcpIssueSendBytes;


		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_EmergencyLogData_Begin, (::Proud::RmiID)Rmi_EmergencyLogData_Begin);
	}

	bool Proxy::EmergencyLogData_Begin ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int64_t & loggingTime, const int32_t & connectCount, const uint32_t & remotePeerCount, const uint32_t & directP2PEnablePeerCount, const Proud::String & natDeviceName, const Proud::HostID & peerID, const int & iopendingcount, const uint64_t & totalTcpIssueSendBytes)  	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_EmergencyLogData_Begin;
__msg.Write(__msgid);
	
__msg << loggingTime;

__msg << connectCount;

__msg << remotePeerCount;

__msg << directP2PEnablePeerCount;

__msg << natDeviceName;

__msg << peerID;

__msg << iopendingcount;

__msg << totalTcpIssueSendBytes;

		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_EmergencyLogData_Begin, (::Proud::RmiID)Rmi_EmergencyLogData_Begin);
	}
        
	bool Proxy::EmergencyLogData_Error ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int32_t & msgSizeErrorCount, const int32_t & netResetErrorCount, const int32_t & intrErrorCount, const int32_t & connResetErrorCount, const uint32_t & lastErrorCompletionLength)	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_EmergencyLogData_Error;
__msg.Write(__msgid);
	
__msg << msgSizeErrorCount;

__msg << netResetErrorCount;

__msg << intrErrorCount;

__msg << connResetErrorCount;

__msg << lastErrorCompletionLength;


		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_EmergencyLogData_Error, (::Proud::RmiID)Rmi_EmergencyLogData_Error);
	}

	bool Proxy::EmergencyLogData_Error ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int32_t & msgSizeErrorCount, const int32_t & netResetErrorCount, const int32_t & intrErrorCount, const int32_t & connResetErrorCount, const uint32_t & lastErrorCompletionLength)  	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_EmergencyLogData_Error;
__msg.Write(__msgid);
	
__msg << msgSizeErrorCount;

__msg << netResetErrorCount;

__msg << intrErrorCount;

__msg << connResetErrorCount;

__msg << lastErrorCompletionLength;

		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_EmergencyLogData_Error, (::Proud::RmiID)Rmi_EmergencyLogData_Error);
	}
        
	bool Proxy::EmergencyLogData_Stats ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const uint64_t & totalTcpReceiveBytes, const uint64_t & totalTcpSendBytes, const uint64_t & totalUdpSendCount, const uint64_t & totalUdpSendBytes, const uint64_t & totalUdpReceiveCount, const uint64_t & totalUdpReceiveBytes)	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_EmergencyLogData_Stats;
__msg.Write(__msgid);
	
__msg << totalTcpReceiveBytes;

__msg << totalTcpSendBytes;

__msg << totalUdpSendCount;

__msg << totalUdpSendBytes;

__msg << totalUdpReceiveCount;

__msg << totalUdpReceiveBytes;


		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_EmergencyLogData_Stats, (::Proud::RmiID)Rmi_EmergencyLogData_Stats);
	}

	bool Proxy::EmergencyLogData_Stats ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const uint64_t & totalTcpReceiveBytes, const uint64_t & totalTcpSendBytes, const uint64_t & totalUdpSendCount, const uint64_t & totalUdpSendBytes, const uint64_t & totalUdpReceiveCount, const uint64_t & totalUdpReceiveBytes)  	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_EmergencyLogData_Stats;
__msg.Write(__msgid);
	
__msg << totalTcpReceiveBytes;

__msg << totalTcpSendBytes;

__msg << totalUdpSendCount;

__msg << totalUdpSendBytes;

__msg << totalUdpReceiveCount;

__msg << totalUdpReceiveBytes;

		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_EmergencyLogData_Stats, (::Proud::RmiID)Rmi_EmergencyLogData_Stats);
	}
        
	bool Proxy::EmergencyLogData_OSVersion ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const uint32_t & osMajorVersion, const uint32_t & osMinorVersion, const uint8_t & productType, const uint16_t & processorArchitecture)	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_EmergencyLogData_OSVersion;
__msg.Write(__msgid);
	
__msg << osMajorVersion;

__msg << osMinorVersion;

__msg << productType;

__msg << processorArchitecture;


		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_EmergencyLogData_OSVersion, (::Proud::RmiID)Rmi_EmergencyLogData_OSVersion);
	}

	bool Proxy::EmergencyLogData_OSVersion ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const uint32_t & osMajorVersion, const uint32_t & osMinorVersion, const uint8_t & productType, const uint16_t & processorArchitecture)  	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_EmergencyLogData_OSVersion;
__msg.Write(__msgid);
	
__msg << osMajorVersion;

__msg << osMinorVersion;

__msg << productType;

__msg << processorArchitecture;

		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_EmergencyLogData_OSVersion, (::Proud::RmiID)Rmi_EmergencyLogData_OSVersion);
	}
        
	bool Proxy::EmergencyLogData_LogEvent ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int & logLevel, const Proud::LogCategory & logCategory, const Proud::HostID & logHostID, const Proud::String & logMessage, const Proud::String & logFunction, const int & logLine)	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_EmergencyLogData_LogEvent;
__msg.Write(__msgid);
	
__msg << logLevel;

__msg << logCategory;

__msg << logHostID;

__msg << logMessage;

__msg << logFunction;

__msg << logLine;


		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_EmergencyLogData_LogEvent, (::Proud::RmiID)Rmi_EmergencyLogData_LogEvent);
	}

	bool Proxy::EmergencyLogData_LogEvent ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int & logLevel, const Proud::LogCategory & logCategory, const Proud::HostID & logHostID, const Proud::String & logMessage, const Proud::String & logFunction, const int & logLine)  	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_EmergencyLogData_LogEvent;
__msg.Write(__msgid);
	
__msg << logLevel;

__msg << logCategory;

__msg << logHostID;

__msg << logMessage;

__msg << logFunction;

__msg << logLine;

		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_EmergencyLogData_LogEvent, (::Proud::RmiID)Rmi_EmergencyLogData_LogEvent);
	}
        
	bool Proxy::EmergencyLogData_End ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const uint32_t & serverUdpAddrCount, const uint32_t & remoteUdpAddrCount)	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_EmergencyLogData_End;
__msg.Write(__msgid);
	
__msg << serverUdpAddrCount;

__msg << remoteUdpAddrCount;


		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_EmergencyLogData_End, (::Proud::RmiID)Rmi_EmergencyLogData_End);
	}

	bool Proxy::EmergencyLogData_End ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const uint32_t & serverUdpAddrCount, const uint32_t & remoteUdpAddrCount)  	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_EmergencyLogData_End;
__msg.Write(__msgid);
	
__msg << serverUdpAddrCount;

__msg << remoteUdpAddrCount;

		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_EmergencyLogData_End, (::Proud::RmiID)Rmi_EmergencyLogData_End);
	}
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_EmergencyLogData_Begin =_PNT("EmergencyLogData_Begin");
#else
const PNTCHAR* Proxy::RmiName_EmergencyLogData_Begin =_PNT("");
#endif
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_EmergencyLogData_Error =_PNT("EmergencyLogData_Error");
#else
const PNTCHAR* Proxy::RmiName_EmergencyLogData_Error =_PNT("");
#endif
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_EmergencyLogData_Stats =_PNT("EmergencyLogData_Stats");
#else
const PNTCHAR* Proxy::RmiName_EmergencyLogData_Stats =_PNT("");
#endif
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_EmergencyLogData_OSVersion =_PNT("EmergencyLogData_OSVersion");
#else
const PNTCHAR* Proxy::RmiName_EmergencyLogData_OSVersion =_PNT("");
#endif
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_EmergencyLogData_LogEvent =_PNT("EmergencyLogData_LogEvent");
#else
const PNTCHAR* Proxy::RmiName_EmergencyLogData_LogEvent =_PNT("");
#endif
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_EmergencyLogData_End =_PNT("EmergencyLogData_End");
#else
const PNTCHAR* Proxy::RmiName_EmergencyLogData_End =_PNT("");
#endif
const PNTCHAR* Proxy::RmiName_First = RmiName_EmergencyLogData_Begin;

}



