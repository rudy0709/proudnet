﻿  






// Generated by PIDL compiler.
// Do not modify this file, but modify the source .pidl file.
   
#pragma once


#include "S2C_common.h"

     
namespace LicenseS2C {


	class Stub : public ::Proud::IRmiStub
	{
	public:
               
		virtual bool NotifyAuthentication ( ::Proud::HostID, ::Proud::RmiContext& , const BYTE & )		{ 
			return false;
		} 

#define DECRMI_LicenseS2C_NotifyAuthentication bool NotifyAuthentication ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const BYTE & result) PN_OVERRIDE

#define DEFRMI_LicenseS2C_NotifyAuthentication(DerivedClass) bool DerivedClass::NotifyAuthentication ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const BYTE & result)
#define CALL_LicenseS2C_NotifyAuthentication NotifyAuthentication ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const BYTE & result)
#define PARAM_LicenseS2C_NotifyAuthentication ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const BYTE & result)
               
		virtual bool NotifyAuthenticationComment ( ::Proud::HostID, ::Proud::RmiContext& , const BYTE & , const Proud::String & )		{ 
			return false;
		} 

#define DECRMI_LicenseS2C_NotifyAuthenticationComment bool NotifyAuthenticationComment ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const BYTE & result, const Proud::String & comment) PN_OVERRIDE

#define DEFRMI_LicenseS2C_NotifyAuthenticationComment(DerivedClass) bool DerivedClass::NotifyAuthenticationComment ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const BYTE & result, const Proud::String & comment)
#define CALL_LicenseS2C_NotifyAuthenticationComment NotifyAuthenticationComment ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const BYTE & result, const Proud::String & comment)
#define PARAM_LicenseS2C_NotifyAuthenticationComment ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const BYTE & result, const Proud::String & comment)
               
		virtual bool NotifyRecordServerInfo ( ::Proud::HostID, ::Proud::RmiContext& , const BYTE & )		{ 
			return false;
		} 

#define DECRMI_LicenseS2C_NotifyRecordServerInfo bool NotifyRecordServerInfo ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const BYTE & result) PN_OVERRIDE

#define DEFRMI_LicenseS2C_NotifyRecordServerInfo(DerivedClass) bool DerivedClass::NotifyRecordServerInfo ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const BYTE & result)
#define CALL_LicenseS2C_NotifyRecordServerInfo NotifyRecordServerInfo ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const BYTE & result)
#define PARAM_LicenseS2C_NotifyRecordServerInfo ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const BYTE & result)
 
		virtual bool ProcessReceivedMessage(::Proud::CReceivedMessage &pa, void* hostTag) PN_OVERRIDE;
		static const PNTCHAR* RmiName_NotifyAuthentication;
		static const PNTCHAR* RmiName_NotifyAuthenticationComment;
		static const PNTCHAR* RmiName_NotifyRecordServerInfo;
		static const PNTCHAR* RmiName_First;
		virtual ::Proud::RmiID* GetRmiIDList() PN_OVERRIDE { return g_RmiIDList; }
		virtual int GetRmiIDListCount() PN_OVERRIDE { return g_RmiIDListCount; }
	};

#ifdef SUPPORTS_CPP11 
	
	class StubFunctional : public Stub 
	{
	public:
               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const BYTE & ) > NotifyAuthentication_Function;
		virtual bool NotifyAuthentication ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const BYTE & result) 
		{ 
			if (NotifyAuthentication_Function==nullptr) 
				return true; 
			return NotifyAuthentication_Function(remote,rmiContext, result); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const BYTE & , const Proud::String & ) > NotifyAuthenticationComment_Function;
		virtual bool NotifyAuthenticationComment ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const BYTE & result, const Proud::String & comment) 
		{ 
			if (NotifyAuthenticationComment_Function==nullptr) 
				return true; 
			return NotifyAuthenticationComment_Function(remote,rmiContext, result, comment); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const BYTE & ) > NotifyRecordServerInfo_Function;
		virtual bool NotifyRecordServerInfo ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const BYTE & result) 
		{ 
			if (NotifyRecordServerInfo_Function==nullptr) 
				return true; 
			return NotifyRecordServerInfo_Function(remote,rmiContext, result); 
		}

	};
#endif

}


