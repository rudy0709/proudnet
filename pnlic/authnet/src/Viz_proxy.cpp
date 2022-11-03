﻿




// Generated by PIDL compiler.
// Do not modify this file, but modify the source .pidl file.


#include "Viz_proxy.h"

namespace VizC2S {


        
	bool Proxy::RequestLogin ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::String & loginKey, const Proud::HostID & vizOwnerHostID)	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_RequestLogin;
__msg.Write(__msgid);
	
__msg << loginKey;

__msg << vizOwnerHostID;


		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_RequestLogin, (::Proud::RmiID)Rmi_RequestLogin);
	}

	bool Proxy::RequestLogin ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::String & loginKey, const Proud::HostID & vizOwnerHostID)  	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_RequestLogin;
__msg.Write(__msgid);
	
__msg << loginKey;

__msg << vizOwnerHostID;

		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_RequestLogin, (::Proud::RmiID)Rmi_RequestLogin);
	}
        
	bool Proxy::NotifyCommon_SendRmi ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::CFastArray<Proud::HostID> & sendTo, const Proud::VizMessageSummary & summary)	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_NotifyCommon_SendRmi;
__msg.Write(__msgid);
	
__msg << sendTo;

__msg << summary;


		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_NotifyCommon_SendRmi, (::Proud::RmiID)Rmi_NotifyCommon_SendRmi);
	}

	bool Proxy::NotifyCommon_SendRmi ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::CFastArray<Proud::HostID> & sendTo, const Proud::VizMessageSummary & summary)  	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_NotifyCommon_SendRmi;
__msg.Write(__msgid);
	
__msg << sendTo;

__msg << summary;

		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_NotifyCommon_SendRmi, (::Proud::RmiID)Rmi_NotifyCommon_SendRmi);
	}
        
	bool Proxy::NotifyCommon_ReceiveRmi ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::HostID & recvFrom, const Proud::String & rmiName, const int & rmiID)	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_NotifyCommon_ReceiveRmi;
__msg.Write(__msgid);
	
__msg << recvFrom;

__msg << rmiName;

__msg << rmiID;


		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_NotifyCommon_ReceiveRmi, (::Proud::RmiID)Rmi_NotifyCommon_ReceiveRmi);
	}

	bool Proxy::NotifyCommon_ReceiveRmi ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::HostID & recvFrom, const Proud::String & rmiName, const int & rmiID)  	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_NotifyCommon_ReceiveRmi;
__msg.Write(__msgid);
	
__msg << recvFrom;

__msg << rmiName;

__msg << rmiID;

		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_NotifyCommon_ReceiveRmi, (::Proud::RmiID)Rmi_NotifyCommon_ReceiveRmi);
	}
        
	bool Proxy::NotifyCli_ConnectionState ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::ConnectionState & connectionState)	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_NotifyCli_ConnectionState;
__msg.Write(__msgid);
	
__msg << connectionState;


		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_NotifyCli_ConnectionState, (::Proud::RmiID)Rmi_NotifyCli_ConnectionState);
	}

	bool Proxy::NotifyCli_ConnectionState ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::ConnectionState & connectionState)  	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_NotifyCli_ConnectionState;
__msg.Write(__msgid);
	
__msg << connectionState;

		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_NotifyCli_ConnectionState, (::Proud::RmiID)Rmi_NotifyCli_ConnectionState);
	}
        
	bool Proxy::NotifyCli_Peers_Clear ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext )	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_NotifyCli_Peers_Clear;
__msg.Write(__msgid);
	

		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_NotifyCli_Peers_Clear, (::Proud::RmiID)Rmi_NotifyCli_Peers_Clear);
	}

	bool Proxy::NotifyCli_Peers_Clear ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext)  	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_NotifyCli_Peers_Clear;
__msg.Write(__msgid);
	
		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_NotifyCli_Peers_Clear, (::Proud::RmiID)Rmi_NotifyCli_Peers_Clear);
	}
        
	bool Proxy::NotifyCli_Peers_AddOrEdit ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::HostID & remotePeerID)	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_NotifyCli_Peers_AddOrEdit;
__msg.Write(__msgid);
	
__msg << remotePeerID;


		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_NotifyCli_Peers_AddOrEdit, (::Proud::RmiID)Rmi_NotifyCli_Peers_AddOrEdit);
	}

	bool Proxy::NotifyCli_Peers_AddOrEdit ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::HostID & remotePeerID)  	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_NotifyCli_Peers_AddOrEdit;
__msg.Write(__msgid);
	
__msg << remotePeerID;

		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_NotifyCli_Peers_AddOrEdit, (::Proud::RmiID)Rmi_NotifyCli_Peers_AddOrEdit);
	}
        
	bool Proxy::NotifySrv_ClientEmpty ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext )	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_NotifySrv_ClientEmpty;
__msg.Write(__msgid);
	

		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_NotifySrv_ClientEmpty, (::Proud::RmiID)Rmi_NotifySrv_ClientEmpty);
	}

	bool Proxy::NotifySrv_ClientEmpty ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext)  	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_NotifySrv_ClientEmpty;
__msg.Write(__msgid);
	
		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_NotifySrv_ClientEmpty, (::Proud::RmiID)Rmi_NotifySrv_ClientEmpty);
	}
        
	bool Proxy::NotifySrv_Clients_AddOrEdit ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::HostID & clientID)	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_NotifySrv_Clients_AddOrEdit;
__msg.Write(__msgid);
	
__msg << clientID;


		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_NotifySrv_Clients_AddOrEdit, (::Proud::RmiID)Rmi_NotifySrv_Clients_AddOrEdit);
	}

	bool Proxy::NotifySrv_Clients_AddOrEdit ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::HostID & clientID)  	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_NotifySrv_Clients_AddOrEdit;
__msg.Write(__msgid);
	
__msg << clientID;

		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_NotifySrv_Clients_AddOrEdit, (::Proud::RmiID)Rmi_NotifySrv_Clients_AddOrEdit);
	}
        
	bool Proxy::NotifySrv_Clients_Remove ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::HostID & clientID)	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_NotifySrv_Clients_Remove;
__msg.Write(__msgid);
	
__msg << clientID;


		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_NotifySrv_Clients_Remove, (::Proud::RmiID)Rmi_NotifySrv_Clients_Remove);
	}

	bool Proxy::NotifySrv_Clients_Remove ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::HostID & clientID)  	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_NotifySrv_Clients_Remove;
__msg.Write(__msgid);
	
__msg << clientID;

		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_NotifySrv_Clients_Remove, (::Proud::RmiID)Rmi_NotifySrv_Clients_Remove);
	}
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_RequestLogin =_PNT("RequestLogin");
#else
const PNTCHAR* Proxy::RmiName_RequestLogin =_PNT("");
#endif
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_NotifyCommon_SendRmi =_PNT("NotifyCommon_SendRmi");
#else
const PNTCHAR* Proxy::RmiName_NotifyCommon_SendRmi =_PNT("");
#endif
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_NotifyCommon_ReceiveRmi =_PNT("NotifyCommon_ReceiveRmi");
#else
const PNTCHAR* Proxy::RmiName_NotifyCommon_ReceiveRmi =_PNT("");
#endif
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_NotifyCli_ConnectionState =_PNT("NotifyCli_ConnectionState");
#else
const PNTCHAR* Proxy::RmiName_NotifyCli_ConnectionState =_PNT("");
#endif
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_NotifyCli_Peers_Clear =_PNT("NotifyCli_Peers_Clear");
#else
const PNTCHAR* Proxy::RmiName_NotifyCli_Peers_Clear =_PNT("");
#endif
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_NotifyCli_Peers_AddOrEdit =_PNT("NotifyCli_Peers_AddOrEdit");
#else
const PNTCHAR* Proxy::RmiName_NotifyCli_Peers_AddOrEdit =_PNT("");
#endif
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_NotifySrv_ClientEmpty =_PNT("NotifySrv_ClientEmpty");
#else
const PNTCHAR* Proxy::RmiName_NotifySrv_ClientEmpty =_PNT("");
#endif
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_NotifySrv_Clients_AddOrEdit =_PNT("NotifySrv_Clients_AddOrEdit");
#else
const PNTCHAR* Proxy::RmiName_NotifySrv_Clients_AddOrEdit =_PNT("");
#endif
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_NotifySrv_Clients_Remove =_PNT("NotifySrv_Clients_Remove");
#else
const PNTCHAR* Proxy::RmiName_NotifySrv_Clients_Remove =_PNT("");
#endif
const PNTCHAR* Proxy::RmiName_First = RmiName_RequestLogin;

}


namespace VizS2C {


        
	bool Proxy::NotifyLoginOk ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext )	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_NotifyLoginOk;
__msg.Write(__msgid);
	

		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_NotifyLoginOk, (::Proud::RmiID)Rmi_NotifyLoginOk);
	}

	bool Proxy::NotifyLoginOk ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext)  	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_NotifyLoginOk;
__msg.Write(__msgid);
	
		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_NotifyLoginOk, (::Proud::RmiID)Rmi_NotifyLoginOk);
	}
        
	bool Proxy::NotifyLoginFailed ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const Proud::ErrorType & reason)	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_NotifyLoginFailed;
__msg.Write(__msgid);
	
__msg << reason;


		return RmiSend(&remote,1,rmiContext,__msg,
			RmiName_NotifyLoginFailed, (::Proud::RmiID)Rmi_NotifyLoginFailed);
	}

	bool Proxy::NotifyLoginFailed ( ::Proud::HostID *remotes, int remoteCount, ::Proud::RmiContext &rmiContext, const Proud::ErrorType & reason)  	{
		
::Proud::CMessage __msg;
__msg.UseInternalBuffer();
__msg.SetSimplePacketMode(m_core->IsSimplePacketMode());

::Proud::RmiID __msgid=(::Proud::RmiID)Rmi_NotifyLoginFailed;
__msg.Write(__msgid);
	
__msg << reason;

		
		return RmiSend(remotes,remoteCount,rmiContext,__msg,
			RmiName_NotifyLoginFailed, (::Proud::RmiID)Rmi_NotifyLoginFailed);
	}
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_NotifyLoginOk =_PNT("NotifyLoginOk");
#else
const PNTCHAR* Proxy::RmiName_NotifyLoginOk =_PNT("");
#endif
#ifdef USE_RMI_NAME_STRING
const PNTCHAR* Proxy::RmiName_NotifyLoginFailed =_PNT("NotifyLoginFailed");
#else
const PNTCHAR* Proxy::RmiName_NotifyLoginFailed =_PNT("");
#endif
const PNTCHAR* Proxy::RmiName_First = RmiName_NotifyLoginOk;

}



