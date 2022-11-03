﻿  





// Generated by PIDL compiler.
// Do not modify this file, but modify the source .pidl file.

  
#include "AgentS2C_stub.h"



const unsigned char sz_AgentS2C_stub_hRmi[] = 
{ 0x00, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xff, 0x00, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xff,
0xaa, 0xbb, 0xcc, 0xdd, 0xff,0x99, 0xaa, 0xbb, 0xcc, 0x99, 0xaa, 0xdd, 0xff, 0x00 };   


namespace AgentS2C {


	bool Stub::ProcessReceivedMessage(::Proud::CReceivedMessage &pa, void* hostTag) 
	{
#ifndef __FreeBSD__ 
		{
			// unusable but required. you may ignore it, because it does not occur any worthless load.
			unsigned char x = sz_AgentS2C_stub_hRmi[0]; 
			x++; 
		}
#endif
		::Proud::HostID remote=pa.GetRemoteHostID();
		if(remote==::Proud::HostID_None)
		{
			ShowUnknownHostIDWarning(remote);
		}

		::Proud::CMessage &__msg=pa.GetReadOnlyMessage();
		int orgReadOffset = __msg.GetReadOffset();

		::Proud::RmiID __rmiID;
		if(!__msg.Read(__rmiID))
			goto __fail;
					
		switch((int)__rmiID) // case is to prevent from clang compile error
		{
			case Rmi_NotifyCredential:
				{
					::Proud::RmiContext ctx;
					ctx.m_rmiID = __rmiID;
					ctx.m_sentFrom=pa.GetRemoteHostID();
					ctx.m_relayed=pa.IsRelayed();
					ctx.m_hostTag = hostTag;
					ctx.m_encryptMode = pa.GetEncryptMode();
					ctx.m_compressMode = pa.GetCompressMode();
			
			        if(BeforeDeserialize(remote, ctx, __msg) == false)
			        {
			            // The user don't want to call the RMI function. 
						// So, We fake that it has been already called.
						__msg.SetReadOffset(__msg.GetLength());
			            return true;
			        }
			
					bool authentication; __msg >> authentication;
					m_core->PostCheckReadMessage(__msg,RmiName_NotifyCredential);
					
			
					if(m_enableNotifyCallFromStub && !m_internalUse)
					{
						::Proud::String parameterString;
						
						::Proud::AppendTextOut(parameterString,authentication);	
						
						NotifyCallFromStub(remote, (::Proud::RmiID)Rmi_NotifyCredential, 
							RmiName_NotifyCredential,parameterString);
			
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifyCredential, 
							RmiName_NotifyCredential, parameterString);
			#endif
					}
					else if(!m_internalUse)
					{
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifyCredential, 
							RmiName_NotifyCredential, _PNT(""));
			#endif
					}
						
					int64_t __t0 = 0;
					if(!m_internalUse)
					{
						if(m_enableStubProfiling)
						{
							::Proud::BeforeRmiSummary __summary;
							__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifyCredential;
							__summary.m_rmiName = RmiName_NotifyCredential;
							__summary.m_hostID = remote;
							__summary.m_hostTag = hostTag;
							BeforeRmiInvocation(__summary);
							__t0 = ::Proud::GetPreciseCurrentTimeMs();
						}
					}
						
					// Call this method.
					bool __ret = NotifyCredential (remote,ctx , authentication );
						
					if(__ret==false)
					{
						// Error: RMI function that a user did not create has been called. 
						m_core->ShowNotImplementedRmiWarning(RmiName_NotifyCredential);
					}
			
					int64_t __t1;
			
					if(!m_internalUse && m_enableStubProfiling)
					{
						__t1 = ::Proud::GetPreciseCurrentTimeMs() - __t0;
			
						::Proud::AfterRmiSummary __summary;
						__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifyCredential;
						__summary.m_rmiName = RmiName_NotifyCredential;
						__summary.m_hostID = remote;
						__summary.m_hostTag = hostTag;
			
						__summary.m_elapsedTime = (uint32_t)__t1;
			
						AfterRmiInvocation(__summary);
					}
				}
				break;
			case Rmi_RequestServerAppStop:
				{
					::Proud::RmiContext ctx;
					ctx.m_rmiID = __rmiID;
					ctx.m_sentFrom=pa.GetRemoteHostID();
					ctx.m_relayed=pa.IsRelayed();
					ctx.m_hostTag = hostTag;
					ctx.m_encryptMode = pa.GetEncryptMode();
					ctx.m_compressMode = pa.GetCompressMode();
			
			        if(BeforeDeserialize(remote, ctx, __msg) == false)
			        {
			            // The user don't want to call the RMI function. 
						// So, We fake that it has been already called.
						__msg.SetReadOffset(__msg.GetLength());
			            return true;
			        }
			
					m_core->PostCheckReadMessage(__msg,RmiName_RequestServerAppStop);
					
			
					if(m_enableNotifyCallFromStub && !m_internalUse)
					{
						::Proud::String parameterString;
						
									
						NotifyCallFromStub(remote, (::Proud::RmiID)Rmi_RequestServerAppStop, 
							RmiName_RequestServerAppStop,parameterString);
			
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_RequestServerAppStop, 
							RmiName_RequestServerAppStop, parameterString);
			#endif
					}
					else if(!m_internalUse)
					{
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_RequestServerAppStop, 
							RmiName_RequestServerAppStop, _PNT(""));
			#endif
					}
						
					int64_t __t0 = 0;
					if(!m_internalUse)
					{
						if(m_enableStubProfiling)
						{
							::Proud::BeforeRmiSummary __summary;
							__summary.m_rmiID = (::Proud::RmiID)Rmi_RequestServerAppStop;
							__summary.m_rmiName = RmiName_RequestServerAppStop;
							__summary.m_hostID = remote;
							__summary.m_hostTag = hostTag;
							BeforeRmiInvocation(__summary);
							__t0 = ::Proud::GetPreciseCurrentTimeMs();
						}
					}
						
					// Call this method.
					bool __ret = RequestServerAppStop (remote,ctx  );
						
					if(__ret==false)
					{
						// Error: RMI function that a user did not create has been called. 
						m_core->ShowNotImplementedRmiWarning(RmiName_RequestServerAppStop);
					}
			
					int64_t __t1;
			
					if(!m_internalUse && m_enableStubProfiling)
					{
						__t1 = ::Proud::GetPreciseCurrentTimeMs() - __t0;
			
						::Proud::AfterRmiSummary __summary;
						__summary.m_rmiID = (::Proud::RmiID)Rmi_RequestServerAppStop;
						__summary.m_rmiName = RmiName_RequestServerAppStop;
						__summary.m_hostID = remote;
						__summary.m_hostTag = hostTag;
			
						__summary.m_elapsedTime = (uint32_t)__t1;
			
						AfterRmiInvocation(__summary);
					}
				}
				break;
		default:
			goto __fail;
		}		
		return true;
__fail:
		{
			__msg.SetReadOffset(orgReadOffset);
			return false;
		}
	}
	#ifdef USE_RMI_NAME_STRING
	const PNTCHAR* Stub::RmiName_NotifyCredential =_PNT("NotifyCredential");
	#else
	const PNTCHAR* Stub::RmiName_NotifyCredential =_PNT("");
	#endif
	#ifdef USE_RMI_NAME_STRING
	const PNTCHAR* Stub::RmiName_RequestServerAppStop =_PNT("RequestServerAppStop");
	#else
	const PNTCHAR* Stub::RmiName_RequestServerAppStop =_PNT("");
	#endif
	const PNTCHAR* Stub::RmiName_First = RmiName_NotifyCredential;

}



