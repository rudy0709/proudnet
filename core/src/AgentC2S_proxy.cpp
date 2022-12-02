﻿




// Generated by PIDL compiler.
// Do not modify this file, but modify the source .pidl file.


#include "AgentC2S_proxy.h"

namespace AgentC2S {


        
	bool Proxy::RequestCredential ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int & cookie)	{
		::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_RequestCredential;
__msg.Write(__msgid); 
	
__msg << cookie;
		
		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_RequestCredential, (::Proud::RmiID)Rmi_RequestCredential);
	}

	bool Proxy::RequestCredential ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int & cookie)  	{
		::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_RequestCredential;
__msg.Write(__msgid); 
	
__msg << cookie;
		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_RequestCredential, (::Proud::RmiID)Rmi_RequestCredential);
	}
        
	bool Proxy::ReportStatusBegin ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const byte & type, const Proud::String & statusText)	{
		::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_ReportStatusBegin;
__msg.Write(__msgid); 
	
__msg << type;
__msg << statusText;
		
		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_ReportStatusBegin, (::Proud::RmiID)Rmi_ReportStatusBegin);
	}

	bool Proxy::ReportStatusBegin ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const byte & type, const Proud::String & statusText)  	{
		::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_ReportStatusBegin;
__msg.Write(__msgid); 
	
__msg << type;
__msg << statusText;
		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_ReportStatusBegin, (::Proud::RmiID)Rmi_ReportStatusBegin);
	}
        
	bool Proxy::ReportStatusValue ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & key, const Proud::String & value)	{
		::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_ReportStatusValue;
__msg.Write(__msgid); 
	
__msg << key;
__msg << value;
		
		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_ReportStatusValue, (::Proud::RmiID)Rmi_ReportStatusValue);
	}

	bool Proxy::ReportStatusValue ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::String & key, const Proud::String & value)  	{
		::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_ReportStatusValue;
__msg.Write(__msgid); 
	
__msg << key;
__msg << value;
		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_ReportStatusValue, (::Proud::RmiID)Rmi_ReportStatusValue);
	}
        
	bool Proxy::ReportStatusEnd ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext )	{
		::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_ReportStatusEnd;
__msg.Write(__msgid); 
	
		
		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_ReportStatusEnd, (::Proud::RmiID)Rmi_ReportStatusEnd);
	}

	bool Proxy::ReportStatusEnd ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext)  	{
		::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_ReportStatusEnd;
__msg.Write(__msgid); 
	
		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_ReportStatusEnd, (::Proud::RmiID)Rmi_ReportStatusEnd);
	}
        
	bool Proxy::ReportServerAppState ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const float & cpuUserTime, const float & cpuKerenlTime, const uint32_t & memorySize)	{
		::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_ReportServerAppState;
__msg.Write(__msgid); 
	
__msg << cpuUserTime;
__msg << cpuKerenlTime;
__msg << memorySize;
		
		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_ReportServerAppState, (::Proud::RmiID)Rmi_ReportServerAppState);
	}

	bool Proxy::ReportServerAppState ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const float & cpuUserTime, const float & cpuKerenlTime, const uint32_t & memorySize)  	{
		::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_ReportServerAppState;
__msg.Write(__msgid); 
	
__msg << cpuUserTime;
__msg << cpuKerenlTime;
__msg << memorySize;
		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_ReportServerAppState, (::Proud::RmiID)Rmi_ReportServerAppState);
	}
        
	bool Proxy::EventLog ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int & logType, const Proud::String & txt)	{
		::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_EventLog;
__msg.Write(__msgid); 
	
__msg << logType;
__msg << txt;
		
		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_EventLog, (::Proud::RmiID)Rmi_EventLog);
	}

	bool Proxy::EventLog ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const int & logType, const Proud::String & txt)  	{
		::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_EventLog;
__msg.Write(__msgid); 
	
__msg << logType;
__msg << txt;
		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_EventLog, (::Proud::RmiID)Rmi_EventLog);
	}
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_RequestCredential =_PNT("RequestCredential");
#else
const PNTCHAR* Proxy::RmiName_RequestCredential =_PNT("");
#endif
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_ReportStatusBegin =_PNT("ReportStatusBegin");
#else
const PNTCHAR* Proxy::RmiName_ReportStatusBegin =_PNT("");
#endif
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_ReportStatusValue =_PNT("ReportStatusValue");
#else
const PNTCHAR* Proxy::RmiName_ReportStatusValue =_PNT("");
#endif
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_ReportStatusEnd =_PNT("ReportStatusEnd");
#else
const PNTCHAR* Proxy::RmiName_ReportStatusEnd =_PNT("");
#endif
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_ReportServerAppState =_PNT("ReportServerAppState");
#else
const PNTCHAR* Proxy::RmiName_ReportServerAppState =_PNT("");
#endif
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_EventLog =_PNT("EventLog");
#else
const PNTCHAR* Proxy::RmiName_EventLog =_PNT("");
#endif
const PNTCHAR* Proxy::RmiName_First = RmiName_RequestCredential;

}



