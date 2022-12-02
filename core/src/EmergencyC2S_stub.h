﻿  






// Generated by PIDL compiler.
// Do not modify this file, but modify the source .pidl file.
   
#pragma once


#include "EmergencyC2S_common.h"

     
namespace EmergencyC2S {


	class Stub : public ::Proud::IRmiStub
	{
	public:
               
		virtual bool EmergencyLogData_Begin ( ::Proud::HostID, ::Proud::RmiContext& , const timespec & , const int32_t & , const uint32_t & , const uint32_t & , const Proud::String & , const Proud::HostID & , const int & , const uint64_t & )		{ 
			return false;
		} 

#define DECRMI_EmergencyC2S_EmergencyLogData_Begin bool EmergencyLogData_Begin ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const timespec & loggingTime, const int32_t & connectCount, const uint32_t & remotePeerCount, const uint32_t & directP2PEnablePeerCount, const Proud::String & natDeviceName, const Proud::HostID & peerID, const int & iopendingcount, const uint64_t & totalTcpIssueSendBytes) PN_OVERRIDE

#define DEFRMI_EmergencyC2S_EmergencyLogData_Begin(DerivedClass) bool DerivedClass::EmergencyLogData_Begin ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const timespec & loggingTime, const int32_t & connectCount, const uint32_t & remotePeerCount, const uint32_t & directP2PEnablePeerCount, const Proud::String & natDeviceName, const Proud::HostID & peerID, const int & iopendingcount, const uint64_t & totalTcpIssueSendBytes)
#define CALL_EmergencyC2S_EmergencyLogData_Begin EmergencyLogData_Begin ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const timespec & loggingTime, const int32_t & connectCount, const uint32_t & remotePeerCount, const uint32_t & directP2PEnablePeerCount, const Proud::String & natDeviceName, const Proud::HostID & peerID, const int & iopendingcount, const uint64_t & totalTcpIssueSendBytes)
#define PARAM_EmergencyC2S_EmergencyLogData_Begin ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const timespec & loggingTime, const int32_t & connectCount, const uint32_t & remotePeerCount, const uint32_t & directP2PEnablePeerCount, const Proud::String & natDeviceName, const Proud::HostID & peerID, const int & iopendingcount, const uint64_t & totalTcpIssueSendBytes)
               
		virtual bool EmergencyLogData_Error ( ::Proud::HostID, ::Proud::RmiContext& , const int32_t & , const int32_t & , const int32_t & , const int32_t & , const uint32_t & )		{ 
			return false;
		} 

#define DECRMI_EmergencyC2S_EmergencyLogData_Error bool EmergencyLogData_Error ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int32_t & msgSizeErrorCount, const int32_t & netResetErrorCount, const int32_t & intrErrorCount, const int32_t & connResetErrorCount, const uint32_t & lastErrorCompletionLength) PN_OVERRIDE

#define DEFRMI_EmergencyC2S_EmergencyLogData_Error(DerivedClass) bool DerivedClass::EmergencyLogData_Error ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int32_t & msgSizeErrorCount, const int32_t & netResetErrorCount, const int32_t & intrErrorCount, const int32_t & connResetErrorCount, const uint32_t & lastErrorCompletionLength)
#define CALL_EmergencyC2S_EmergencyLogData_Error EmergencyLogData_Error ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int32_t & msgSizeErrorCount, const int32_t & netResetErrorCount, const int32_t & intrErrorCount, const int32_t & connResetErrorCount, const uint32_t & lastErrorCompletionLength)
#define PARAM_EmergencyC2S_EmergencyLogData_Error ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int32_t & msgSizeErrorCount, const int32_t & netResetErrorCount, const int32_t & intrErrorCount, const int32_t & connResetErrorCount, const uint32_t & lastErrorCompletionLength)
               
		virtual bool EmergencyLogData_Stats ( ::Proud::HostID, ::Proud::RmiContext& , const uint64_t & , const uint64_t & , const uint64_t & , const uint64_t & , const uint64_t & , const uint64_t & )		{ 
			return false;
		} 

#define DECRMI_EmergencyC2S_EmergencyLogData_Stats bool EmergencyLogData_Stats ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const uint64_t & totalTcpReceiveBytes, const uint64_t & totalTcpSendBytes, const uint64_t & totalUdpSendCount, const uint64_t & totalUdpSendBytes, const uint64_t & totalUdpReceiveCount, const uint64_t & totalUdpReceiveBytes) PN_OVERRIDE

#define DEFRMI_EmergencyC2S_EmergencyLogData_Stats(DerivedClass) bool DerivedClass::EmergencyLogData_Stats ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const uint64_t & totalTcpReceiveBytes, const uint64_t & totalTcpSendBytes, const uint64_t & totalUdpSendCount, const uint64_t & totalUdpSendBytes, const uint64_t & totalUdpReceiveCount, const uint64_t & totalUdpReceiveBytes)
#define CALL_EmergencyC2S_EmergencyLogData_Stats EmergencyLogData_Stats ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const uint64_t & totalTcpReceiveBytes, const uint64_t & totalTcpSendBytes, const uint64_t & totalUdpSendCount, const uint64_t & totalUdpSendBytes, const uint64_t & totalUdpReceiveCount, const uint64_t & totalUdpReceiveBytes)
#define PARAM_EmergencyC2S_EmergencyLogData_Stats ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const uint64_t & totalTcpReceiveBytes, const uint64_t & totalTcpSendBytes, const uint64_t & totalUdpSendCount, const uint64_t & totalUdpSendBytes, const uint64_t & totalUdpReceiveCount, const uint64_t & totalUdpReceiveBytes)
               
		virtual bool EmergencyLogData_OSVersion ( ::Proud::HostID, ::Proud::RmiContext& , const uint32_t & , const uint32_t & , const uint8_t & , const uint16_t & )		{ 
			return false;
		} 

#define DECRMI_EmergencyC2S_EmergencyLogData_OSVersion bool EmergencyLogData_OSVersion ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const uint32_t & osMajorVersion, const uint32_t & osMinorVersion, const uint8_t & productType, const uint16_t & processorArchitecture) PN_OVERRIDE

#define DEFRMI_EmergencyC2S_EmergencyLogData_OSVersion(DerivedClass) bool DerivedClass::EmergencyLogData_OSVersion ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const uint32_t & osMajorVersion, const uint32_t & osMinorVersion, const uint8_t & productType, const uint16_t & processorArchitecture)
#define CALL_EmergencyC2S_EmergencyLogData_OSVersion EmergencyLogData_OSVersion ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const uint32_t & osMajorVersion, const uint32_t & osMinorVersion, const uint8_t & productType, const uint16_t & processorArchitecture)
#define PARAM_EmergencyC2S_EmergencyLogData_OSVersion ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const uint32_t & osMajorVersion, const uint32_t & osMinorVersion, const uint8_t & productType, const uint16_t & processorArchitecture)
               
		virtual bool EmergencyLogData_LogEvent ( ::Proud::HostID, ::Proud::RmiContext& , const int & , const Proud::LogCategory & , const Proud::HostID & , const Proud::String & , const Proud::String & , const int & )		{ 
			return false;
		} 

#define DECRMI_EmergencyC2S_EmergencyLogData_LogEvent bool EmergencyLogData_LogEvent ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int & logLevel, const Proud::LogCategory & logCategory, const Proud::HostID & logHostID, const Proud::String & logMessage, const Proud::String & logFunction, const int & logLine) PN_OVERRIDE

#define DEFRMI_EmergencyC2S_EmergencyLogData_LogEvent(DerivedClass) bool DerivedClass::EmergencyLogData_LogEvent ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int & logLevel, const Proud::LogCategory & logCategory, const Proud::HostID & logHostID, const Proud::String & logMessage, const Proud::String & logFunction, const int & logLine)
#define CALL_EmergencyC2S_EmergencyLogData_LogEvent EmergencyLogData_LogEvent ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int & logLevel, const Proud::LogCategory & logCategory, const Proud::HostID & logHostID, const Proud::String & logMessage, const Proud::String & logFunction, const int & logLine)
#define PARAM_EmergencyC2S_EmergencyLogData_LogEvent ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int & logLevel, const Proud::LogCategory & logCategory, const Proud::HostID & logHostID, const Proud::String & logMessage, const Proud::String & logFunction, const int & logLine)
               
		virtual bool EmergencyLogData_End ( ::Proud::HostID, ::Proud::RmiContext& , const uint32_t & , const uint32_t & )		{ 
			return false;
		} 

#define DECRMI_EmergencyC2S_EmergencyLogData_End bool EmergencyLogData_End ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const uint32_t & serverUdpAddrCount, const uint32_t & remoteUdpAddrCount) PN_OVERRIDE

#define DEFRMI_EmergencyC2S_EmergencyLogData_End(DerivedClass) bool DerivedClass::EmergencyLogData_End ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const uint32_t & serverUdpAddrCount, const uint32_t & remoteUdpAddrCount)
#define CALL_EmergencyC2S_EmergencyLogData_End EmergencyLogData_End ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const uint32_t & serverUdpAddrCount, const uint32_t & remoteUdpAddrCount)
#define PARAM_EmergencyC2S_EmergencyLogData_End ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const uint32_t & serverUdpAddrCount, const uint32_t & remoteUdpAddrCount)
 
		virtual bool ProcessReceivedMessage(::Proud::CReceivedMessage &pa, void* hostTag) PN_OVERRIDE;
		static const PNTCHAR* RmiName_EmergencyLogData_Begin;
		static const PNTCHAR* RmiName_EmergencyLogData_Error;
		static const PNTCHAR* RmiName_EmergencyLogData_Stats;
		static const PNTCHAR* RmiName_EmergencyLogData_OSVersion;
		static const PNTCHAR* RmiName_EmergencyLogData_LogEvent;
		static const PNTCHAR* RmiName_EmergencyLogData_End;
		static const PNTCHAR* RmiName_First;
		virtual ::Proud::RmiID* GetRmiIDList() PN_OVERRIDE { return g_RmiIDList; }
		virtual int GetRmiIDListCount() PN_OVERRIDE { return g_RmiIDListCount; }
	};

#ifdef SUPPORTS_CPP11 
	
	class StubFunctional : public Stub 
	{
	public:
               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const timespec & , const int32_t & , const uint32_t & , const uint32_t & , const Proud::String & , const Proud::HostID & , const int & , const uint64_t & ) > EmergencyLogData_Begin_Function;
		virtual bool EmergencyLogData_Begin ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const timespec & loggingTime, const int32_t & connectCount, const uint32_t & remotePeerCount, const uint32_t & directP2PEnablePeerCount, const Proud::String & natDeviceName, const Proud::HostID & peerID, const int & iopendingcount, const uint64_t & totalTcpIssueSendBytes) 
		{ 
			if (EmergencyLogData_Begin_Function==nullptr) 
				return true; 
			return EmergencyLogData_Begin_Function(remote,rmiContext, loggingTime, connectCount, remotePeerCount, directP2PEnablePeerCount, natDeviceName, peerID, iopendingcount, totalTcpIssueSendBytes); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const int32_t & , const int32_t & , const int32_t & , const int32_t & , const uint32_t & ) > EmergencyLogData_Error_Function;
		virtual bool EmergencyLogData_Error ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int32_t & msgSizeErrorCount, const int32_t & netResetErrorCount, const int32_t & intrErrorCount, const int32_t & connResetErrorCount, const uint32_t & lastErrorCompletionLength) 
		{ 
			if (EmergencyLogData_Error_Function==nullptr) 
				return true; 
			return EmergencyLogData_Error_Function(remote,rmiContext, msgSizeErrorCount, netResetErrorCount, intrErrorCount, connResetErrorCount, lastErrorCompletionLength); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const uint64_t & , const uint64_t & , const uint64_t & , const uint64_t & , const uint64_t & , const uint64_t & ) > EmergencyLogData_Stats_Function;
		virtual bool EmergencyLogData_Stats ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const uint64_t & totalTcpReceiveBytes, const uint64_t & totalTcpSendBytes, const uint64_t & totalUdpSendCount, const uint64_t & totalUdpSendBytes, const uint64_t & totalUdpReceiveCount, const uint64_t & totalUdpReceiveBytes) 
		{ 
			if (EmergencyLogData_Stats_Function==nullptr) 
				return true; 
			return EmergencyLogData_Stats_Function(remote,rmiContext, totalTcpReceiveBytes, totalTcpSendBytes, totalUdpSendCount, totalUdpSendBytes, totalUdpReceiveCount, totalUdpReceiveBytes); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const uint32_t & , const uint32_t & , const uint8_t & , const uint16_t & ) > EmergencyLogData_OSVersion_Function;
		virtual bool EmergencyLogData_OSVersion ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const uint32_t & osMajorVersion, const uint32_t & osMinorVersion, const uint8_t & productType, const uint16_t & processorArchitecture) 
		{ 
			if (EmergencyLogData_OSVersion_Function==nullptr) 
				return true; 
			return EmergencyLogData_OSVersion_Function(remote,rmiContext, osMajorVersion, osMinorVersion, productType, processorArchitecture); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const int & , const Proud::LogCategory & , const Proud::HostID & , const Proud::String & , const Proud::String & , const int & ) > EmergencyLogData_LogEvent_Function;
		virtual bool EmergencyLogData_LogEvent ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int & logLevel, const Proud::LogCategory & logCategory, const Proud::HostID & logHostID, const Proud::String & logMessage, const Proud::String & logFunction, const int & logLine) 
		{ 
			if (EmergencyLogData_LogEvent_Function==nullptr) 
				return true; 
			return EmergencyLogData_LogEvent_Function(remote,rmiContext, logLevel, logCategory, logHostID, logMessage, logFunction, logLine); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const uint32_t & , const uint32_t & ) > EmergencyLogData_End_Function;
		virtual bool EmergencyLogData_End ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const uint32_t & serverUdpAddrCount, const uint32_t & remoteUdpAddrCount) 
		{ 
			if (EmergencyLogData_End_Function==nullptr) 
				return true; 
			return EmergencyLogData_End_Function(remote,rmiContext, serverUdpAddrCount, remoteUdpAddrCount); 
		}

	};
#endif

}


