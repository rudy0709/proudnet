﻿  






// Generated by PIDL compiler.
// Do not modify this file, but modify the source .pidl file.
   
#pragma once


#include "ChatC2S_common.h"

     
namespace ChatC2S {


	class Stub : public ::Proud::IRmiStub
	{
	public:
               
		virtual bool RequestLogon ( ::Proud::HostID, ::Proud::RmiContext& , const Proud::String & )		{ 
			return false;
		} 

#define DECRMI_ChatC2S_RequestLogon bool RequestLogon ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & userName) PN_OVERRIDE

#define DEFRMI_ChatC2S_RequestLogon(DerivedClass) bool DerivedClass::RequestLogon ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & userName)
#define CALL_ChatC2S_RequestLogon RequestLogon ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & userName)
#define PARAM_ChatC2S_RequestLogon ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & userName)
               
		virtual bool Chat ( ::Proud::HostID, ::Proud::RmiContext& , const Proud::String & )		{ 
			return false;
		} 

#define DECRMI_ChatC2S_Chat bool Chat ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & text) PN_OVERRIDE

#define DEFRMI_ChatC2S_Chat(DerivedClass) bool DerivedClass::Chat ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & text)
#define CALL_ChatC2S_Chat Chat ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & text)
#define PARAM_ChatC2S_Chat ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & text)
               
		virtual bool RequestP2PGroup ( ::Proud::HostID, ::Proud::RmiContext& , const Proud::HostIDArray & )		{ 
			return false;
		} 

#define DECRMI_ChatC2S_RequestP2PGroup bool RequestP2PGroup ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::HostIDArray & groupMemberList) PN_OVERRIDE

#define DEFRMI_ChatC2S_RequestP2PGroup(DerivedClass) bool DerivedClass::RequestP2PGroup ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::HostIDArray & groupMemberList)
#define CALL_ChatC2S_RequestP2PGroup RequestP2PGroup ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::HostIDArray & groupMemberList)
#define PARAM_ChatC2S_RequestP2PGroup ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::HostIDArray & groupMemberList)
               
		virtual bool RequestLeaveP2PGroup ( ::Proud::HostID, ::Proud::RmiContext& , const Proud::HostID & )		{ 
			return false;
		} 

#define DECRMI_ChatC2S_RequestLeaveP2PGroup bool RequestLeaveP2PGroup ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::HostID & groupID) PN_OVERRIDE

#define DEFRMI_ChatC2S_RequestLeaveP2PGroup(DerivedClass) bool DerivedClass::RequestLeaveP2PGroup ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::HostID & groupID)
#define CALL_ChatC2S_RequestLeaveP2PGroup RequestLeaveP2PGroup ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::HostID & groupID)
#define PARAM_ChatC2S_RequestLeaveP2PGroup ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::HostID & groupID)
 
		virtual bool ProcessReceivedMessage(::Proud::CReceivedMessage &pa, void* hostTag) PN_OVERRIDE;
		static const PNTCHAR* RmiName_RequestLogon;
		static const PNTCHAR* RmiName_Chat;
		static const PNTCHAR* RmiName_RequestP2PGroup;
		static const PNTCHAR* RmiName_RequestLeaveP2PGroup;
		static const PNTCHAR* RmiName_First;
		virtual ::Proud::RmiID* GetRmiIDList() PN_OVERRIDE { return g_RmiIDList; }
		virtual int GetRmiIDListCount() PN_OVERRIDE { return g_RmiIDListCount; }
	};

#ifdef SUPPORTS_CPP11 
	
	class StubFunctional : public Stub 
	{
	public:
               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const Proud::String & ) > RequestLogon_Function;
		virtual bool RequestLogon ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & userName) 
		{ 
			if (RequestLogon_Function==nullptr) 
				return true; 
			return RequestLogon_Function(remote,rmiContext, userName); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const Proud::String & ) > Chat_Function;
		virtual bool Chat ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & text) 
		{ 
			if (Chat_Function==nullptr) 
				return true; 
			return Chat_Function(remote,rmiContext, text); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const Proud::HostIDArray & ) > RequestP2PGroup_Function;
		virtual bool RequestP2PGroup ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::HostIDArray & groupMemberList) 
		{ 
			if (RequestP2PGroup_Function==nullptr) 
				return true; 
			return RequestP2PGroup_Function(remote,rmiContext, groupMemberList); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const Proud::HostID & ) > RequestLeaveP2PGroup_Function;
		virtual bool RequestLeaveP2PGroup ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::HostID & groupID) 
		{ 
			if (RequestLeaveP2PGroup_Function==nullptr) 
				return true; 
			return RequestLeaveP2PGroup_Function(remote,rmiContext, groupID); 
		}

	};
#endif

}


