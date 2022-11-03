﻿  





// Generated by PIDL compiler.
// Do not modify this file, but modify the source .pidl file.

  
#include "Viz_stub.h"



const unsigned char sz_Viz_stub_hRmi[] = 
{ 0x00, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xff, 0x00, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xff,
0xaa, 0xbb, 0xcc, 0xdd, 0xff,0x99, 0xaa, 0xbb, 0xcc, 0x99, 0xaa, 0xdd, 0xff, 0x00 };   


namespace VizC2S {


	bool Stub::ProcessReceivedMessage(::Proud::CReceivedMessage &pa, void* hostTag) 
	{
#ifndef __FreeBSD__ 
		{
			// unusable but required. you may ignore it, because it does not occur any worthless load.
			unsigned char x = sz_Viz_stub_hRmi[0]; 
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
			case Rmi_RequestLogin:
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
			
					Proud::String loginKey; __msg >> loginKey;
					Proud::HostID vizOwnerHostID; __msg >> vizOwnerHostID;
					m_core->PostCheckReadMessage(__msg,RmiName_RequestLogin);
					
			
					if(m_enableNotifyCallFromStub && !m_internalUse)
					{
						::Proud::String parameterString;
						
						::Proud::AppendTextOut(parameterString,loginKey);	
										
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,vizOwnerHostID);	
						
						NotifyCallFromStub(remote, (::Proud::RmiID)Rmi_RequestLogin, 
							RmiName_RequestLogin,parameterString);
			
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_RequestLogin, 
							RmiName_RequestLogin, parameterString);
			#endif
					}
					else if(!m_internalUse)
					{
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_RequestLogin, 
							RmiName_RequestLogin, _PNT(""));
			#endif
					}
						
					int64_t __t0 = 0;
					if(!m_internalUse)
					{
						if(m_enableStubProfiling)
						{
							::Proud::BeforeRmiSummary __summary;
							__summary.m_rmiID = (::Proud::RmiID)Rmi_RequestLogin;
							__summary.m_rmiName = RmiName_RequestLogin;
							__summary.m_hostID = remote;
							__summary.m_hostTag = hostTag;
							BeforeRmiInvocation(__summary);
							__t0 = ::Proud::GetPreciseCurrentTimeMs();
						}
					}
						
					// Call this method.
					bool __ret = RequestLogin (remote,ctx , loginKey, vizOwnerHostID );
						
					if(__ret==false)
					{
						// Error: RMI function that a user did not create has been called. 
						m_core->ShowNotImplementedRmiWarning(RmiName_RequestLogin);
					}
			
					int64_t __t1;
			
					if(!m_internalUse && m_enableStubProfiling)
					{
						__t1 = ::Proud::GetPreciseCurrentTimeMs() - __t0;
			
						::Proud::AfterRmiSummary __summary;
						__summary.m_rmiID = (::Proud::RmiID)Rmi_RequestLogin;
						__summary.m_rmiName = RmiName_RequestLogin;
						__summary.m_hostID = remote;
						__summary.m_hostTag = hostTag;
			
						__summary.m_elapsedTime = (uint32_t)__t1;
			
						AfterRmiInvocation(__summary);
					}
				}
				break;
			case Rmi_NotifyCommon_SendRmi:
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
			
					Proud::CFastArray<Proud::HostID> sendTo; __msg >> sendTo;
					Proud::VizMessageSummary summary; __msg >> summary;
					m_core->PostCheckReadMessage(__msg,RmiName_NotifyCommon_SendRmi);
					
			
					if(m_enableNotifyCallFromStub && !m_internalUse)
					{
						::Proud::String parameterString;
						
						::Proud::AppendTextOut(parameterString,sendTo);	
										
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,summary);	
						
						NotifyCallFromStub(remote, (::Proud::RmiID)Rmi_NotifyCommon_SendRmi, 
							RmiName_NotifyCommon_SendRmi,parameterString);
			
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifyCommon_SendRmi, 
							RmiName_NotifyCommon_SendRmi, parameterString);
			#endif
					}
					else if(!m_internalUse)
					{
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifyCommon_SendRmi, 
							RmiName_NotifyCommon_SendRmi, _PNT(""));
			#endif
					}
						
					int64_t __t0 = 0;
					if(!m_internalUse)
					{
						if(m_enableStubProfiling)
						{
							::Proud::BeforeRmiSummary __summary;
							__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifyCommon_SendRmi;
							__summary.m_rmiName = RmiName_NotifyCommon_SendRmi;
							__summary.m_hostID = remote;
							__summary.m_hostTag = hostTag;
							BeforeRmiInvocation(__summary);
							__t0 = ::Proud::GetPreciseCurrentTimeMs();
						}
					}
						
					// Call this method.
					bool __ret = NotifyCommon_SendRmi (remote,ctx , sendTo, summary );
						
					if(__ret==false)
					{
						// Error: RMI function that a user did not create has been called. 
						m_core->ShowNotImplementedRmiWarning(RmiName_NotifyCommon_SendRmi);
					}
			
					int64_t __t1;
			
					if(!m_internalUse && m_enableStubProfiling)
					{
						__t1 = ::Proud::GetPreciseCurrentTimeMs() - __t0;
			
						::Proud::AfterRmiSummary __summary;
						__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifyCommon_SendRmi;
						__summary.m_rmiName = RmiName_NotifyCommon_SendRmi;
						__summary.m_hostID = remote;
						__summary.m_hostTag = hostTag;
			
						__summary.m_elapsedTime = (uint32_t)__t1;
			
						AfterRmiInvocation(__summary);
					}
				}
				break;
			case Rmi_NotifyCommon_ReceiveRmi:
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
			
					Proud::HostID recvFrom; __msg >> recvFrom;
					Proud::String rmiName; __msg >> rmiName;
					int rmiID; __msg >> rmiID;
					m_core->PostCheckReadMessage(__msg,RmiName_NotifyCommon_ReceiveRmi);
					
			
					if(m_enableNotifyCallFromStub && !m_internalUse)
					{
						::Proud::String parameterString;
						
						::Proud::AppendTextOut(parameterString,recvFrom);	
										
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,rmiName);	
										
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,rmiID);	
						
						NotifyCallFromStub(remote, (::Proud::RmiID)Rmi_NotifyCommon_ReceiveRmi, 
							RmiName_NotifyCommon_ReceiveRmi,parameterString);
			
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifyCommon_ReceiveRmi, 
							RmiName_NotifyCommon_ReceiveRmi, parameterString);
			#endif
					}
					else if(!m_internalUse)
					{
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifyCommon_ReceiveRmi, 
							RmiName_NotifyCommon_ReceiveRmi, _PNT(""));
			#endif
					}
						
					int64_t __t0 = 0;
					if(!m_internalUse)
					{
						if(m_enableStubProfiling)
						{
							::Proud::BeforeRmiSummary __summary;
							__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifyCommon_ReceiveRmi;
							__summary.m_rmiName = RmiName_NotifyCommon_ReceiveRmi;
							__summary.m_hostID = remote;
							__summary.m_hostTag = hostTag;
							BeforeRmiInvocation(__summary);
							__t0 = ::Proud::GetPreciseCurrentTimeMs();
						}
					}
						
					// Call this method.
					bool __ret = NotifyCommon_ReceiveRmi (remote,ctx , recvFrom, rmiName, rmiID );
						
					if(__ret==false)
					{
						// Error: RMI function that a user did not create has been called. 
						m_core->ShowNotImplementedRmiWarning(RmiName_NotifyCommon_ReceiveRmi);
					}
			
					int64_t __t1;
			
					if(!m_internalUse && m_enableStubProfiling)
					{
						__t1 = ::Proud::GetPreciseCurrentTimeMs() - __t0;
			
						::Proud::AfterRmiSummary __summary;
						__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifyCommon_ReceiveRmi;
						__summary.m_rmiName = RmiName_NotifyCommon_ReceiveRmi;
						__summary.m_hostID = remote;
						__summary.m_hostTag = hostTag;
			
						__summary.m_elapsedTime = (uint32_t)__t1;
			
						AfterRmiInvocation(__summary);
					}
				}
				break;
			case Rmi_NotifyCli_ConnectionState:
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
			
					Proud::ConnectionState connectionState; __msg >> connectionState;
					m_core->PostCheckReadMessage(__msg,RmiName_NotifyCli_ConnectionState);
					
			
					if(m_enableNotifyCallFromStub && !m_internalUse)
					{
						::Proud::String parameterString;
						
						::Proud::AppendTextOut(parameterString,connectionState);	
						
						NotifyCallFromStub(remote, (::Proud::RmiID)Rmi_NotifyCli_ConnectionState, 
							RmiName_NotifyCli_ConnectionState,parameterString);
			
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifyCli_ConnectionState, 
							RmiName_NotifyCli_ConnectionState, parameterString);
			#endif
					}
					else if(!m_internalUse)
					{
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifyCli_ConnectionState, 
							RmiName_NotifyCli_ConnectionState, _PNT(""));
			#endif
					}
						
					int64_t __t0 = 0;
					if(!m_internalUse)
					{
						if(m_enableStubProfiling)
						{
							::Proud::BeforeRmiSummary __summary;
							__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifyCli_ConnectionState;
							__summary.m_rmiName = RmiName_NotifyCli_ConnectionState;
							__summary.m_hostID = remote;
							__summary.m_hostTag = hostTag;
							BeforeRmiInvocation(__summary);
							__t0 = ::Proud::GetPreciseCurrentTimeMs();
						}
					}
						
					// Call this method.
					bool __ret = NotifyCli_ConnectionState (remote,ctx , connectionState );
						
					if(__ret==false)
					{
						// Error: RMI function that a user did not create has been called. 
						m_core->ShowNotImplementedRmiWarning(RmiName_NotifyCli_ConnectionState);
					}
			
					int64_t __t1;
			
					if(!m_internalUse && m_enableStubProfiling)
					{
						__t1 = ::Proud::GetPreciseCurrentTimeMs() - __t0;
			
						::Proud::AfterRmiSummary __summary;
						__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifyCli_ConnectionState;
						__summary.m_rmiName = RmiName_NotifyCli_ConnectionState;
						__summary.m_hostID = remote;
						__summary.m_hostTag = hostTag;
			
						__summary.m_elapsedTime = (uint32_t)__t1;
			
						AfterRmiInvocation(__summary);
					}
				}
				break;
			case Rmi_NotifyCli_Peers_Clear:
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
			
					m_core->PostCheckReadMessage(__msg,RmiName_NotifyCli_Peers_Clear);
					
			
					if(m_enableNotifyCallFromStub && !m_internalUse)
					{
						::Proud::String parameterString;
						
									
						NotifyCallFromStub(remote, (::Proud::RmiID)Rmi_NotifyCli_Peers_Clear, 
							RmiName_NotifyCli_Peers_Clear,parameterString);
			
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifyCli_Peers_Clear, 
							RmiName_NotifyCli_Peers_Clear, parameterString);
			#endif
					}
					else if(!m_internalUse)
					{
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifyCli_Peers_Clear, 
							RmiName_NotifyCli_Peers_Clear, _PNT(""));
			#endif
					}
						
					int64_t __t0 = 0;
					if(!m_internalUse)
					{
						if(m_enableStubProfiling)
						{
							::Proud::BeforeRmiSummary __summary;
							__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifyCli_Peers_Clear;
							__summary.m_rmiName = RmiName_NotifyCli_Peers_Clear;
							__summary.m_hostID = remote;
							__summary.m_hostTag = hostTag;
							BeforeRmiInvocation(__summary);
							__t0 = ::Proud::GetPreciseCurrentTimeMs();
						}
					}
						
					// Call this method.
					bool __ret = NotifyCli_Peers_Clear (remote,ctx  );
						
					if(__ret==false)
					{
						// Error: RMI function that a user did not create has been called. 
						m_core->ShowNotImplementedRmiWarning(RmiName_NotifyCli_Peers_Clear);
					}
			
					int64_t __t1;
			
					if(!m_internalUse && m_enableStubProfiling)
					{
						__t1 = ::Proud::GetPreciseCurrentTimeMs() - __t0;
			
						::Proud::AfterRmiSummary __summary;
						__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifyCli_Peers_Clear;
						__summary.m_rmiName = RmiName_NotifyCli_Peers_Clear;
						__summary.m_hostID = remote;
						__summary.m_hostTag = hostTag;
			
						__summary.m_elapsedTime = (uint32_t)__t1;
			
						AfterRmiInvocation(__summary);
					}
				}
				break;
			case Rmi_NotifyCli_Peers_AddOrEdit:
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
			
					Proud::HostID remotePeerID; __msg >> remotePeerID;
					m_core->PostCheckReadMessage(__msg,RmiName_NotifyCli_Peers_AddOrEdit);
					
			
					if(m_enableNotifyCallFromStub && !m_internalUse)
					{
						::Proud::String parameterString;
						
						::Proud::AppendTextOut(parameterString,remotePeerID);	
						
						NotifyCallFromStub(remote, (::Proud::RmiID)Rmi_NotifyCli_Peers_AddOrEdit, 
							RmiName_NotifyCli_Peers_AddOrEdit,parameterString);
			
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifyCli_Peers_AddOrEdit, 
							RmiName_NotifyCli_Peers_AddOrEdit, parameterString);
			#endif
					}
					else if(!m_internalUse)
					{
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifyCli_Peers_AddOrEdit, 
							RmiName_NotifyCli_Peers_AddOrEdit, _PNT(""));
			#endif
					}
						
					int64_t __t0 = 0;
					if(!m_internalUse)
					{
						if(m_enableStubProfiling)
						{
							::Proud::BeforeRmiSummary __summary;
							__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifyCli_Peers_AddOrEdit;
							__summary.m_rmiName = RmiName_NotifyCli_Peers_AddOrEdit;
							__summary.m_hostID = remote;
							__summary.m_hostTag = hostTag;
							BeforeRmiInvocation(__summary);
							__t0 = ::Proud::GetPreciseCurrentTimeMs();
						}
					}
						
					// Call this method.
					bool __ret = NotifyCli_Peers_AddOrEdit (remote,ctx , remotePeerID );
						
					if(__ret==false)
					{
						// Error: RMI function that a user did not create has been called. 
						m_core->ShowNotImplementedRmiWarning(RmiName_NotifyCli_Peers_AddOrEdit);
					}
			
					int64_t __t1;
			
					if(!m_internalUse && m_enableStubProfiling)
					{
						__t1 = ::Proud::GetPreciseCurrentTimeMs() - __t0;
			
						::Proud::AfterRmiSummary __summary;
						__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifyCli_Peers_AddOrEdit;
						__summary.m_rmiName = RmiName_NotifyCli_Peers_AddOrEdit;
						__summary.m_hostID = remote;
						__summary.m_hostTag = hostTag;
			
						__summary.m_elapsedTime = (uint32_t)__t1;
			
						AfterRmiInvocation(__summary);
					}
				}
				break;
			case Rmi_NotifySrv_ClientEmpty:
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
			
					m_core->PostCheckReadMessage(__msg,RmiName_NotifySrv_ClientEmpty);
					
			
					if(m_enableNotifyCallFromStub && !m_internalUse)
					{
						::Proud::String parameterString;
						
									
						NotifyCallFromStub(remote, (::Proud::RmiID)Rmi_NotifySrv_ClientEmpty, 
							RmiName_NotifySrv_ClientEmpty,parameterString);
			
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifySrv_ClientEmpty, 
							RmiName_NotifySrv_ClientEmpty, parameterString);
			#endif
					}
					else if(!m_internalUse)
					{
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifySrv_ClientEmpty, 
							RmiName_NotifySrv_ClientEmpty, _PNT(""));
			#endif
					}
						
					int64_t __t0 = 0;
					if(!m_internalUse)
					{
						if(m_enableStubProfiling)
						{
							::Proud::BeforeRmiSummary __summary;
							__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifySrv_ClientEmpty;
							__summary.m_rmiName = RmiName_NotifySrv_ClientEmpty;
							__summary.m_hostID = remote;
							__summary.m_hostTag = hostTag;
							BeforeRmiInvocation(__summary);
							__t0 = ::Proud::GetPreciseCurrentTimeMs();
						}
					}
						
					// Call this method.
					bool __ret = NotifySrv_ClientEmpty (remote,ctx  );
						
					if(__ret==false)
					{
						// Error: RMI function that a user did not create has been called. 
						m_core->ShowNotImplementedRmiWarning(RmiName_NotifySrv_ClientEmpty);
					}
			
					int64_t __t1;
			
					if(!m_internalUse && m_enableStubProfiling)
					{
						__t1 = ::Proud::GetPreciseCurrentTimeMs() - __t0;
			
						::Proud::AfterRmiSummary __summary;
						__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifySrv_ClientEmpty;
						__summary.m_rmiName = RmiName_NotifySrv_ClientEmpty;
						__summary.m_hostID = remote;
						__summary.m_hostTag = hostTag;
			
						__summary.m_elapsedTime = (uint32_t)__t1;
			
						AfterRmiInvocation(__summary);
					}
				}
				break;
			case Rmi_NotifySrv_Clients_AddOrEdit:
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
			
					Proud::HostID clientID; __msg >> clientID;
					m_core->PostCheckReadMessage(__msg,RmiName_NotifySrv_Clients_AddOrEdit);
					
			
					if(m_enableNotifyCallFromStub && !m_internalUse)
					{
						::Proud::String parameterString;
						
						::Proud::AppendTextOut(parameterString,clientID);	
						
						NotifyCallFromStub(remote, (::Proud::RmiID)Rmi_NotifySrv_Clients_AddOrEdit, 
							RmiName_NotifySrv_Clients_AddOrEdit,parameterString);
			
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifySrv_Clients_AddOrEdit, 
							RmiName_NotifySrv_Clients_AddOrEdit, parameterString);
			#endif
					}
					else if(!m_internalUse)
					{
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifySrv_Clients_AddOrEdit, 
							RmiName_NotifySrv_Clients_AddOrEdit, _PNT(""));
			#endif
					}
						
					int64_t __t0 = 0;
					if(!m_internalUse)
					{
						if(m_enableStubProfiling)
						{
							::Proud::BeforeRmiSummary __summary;
							__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifySrv_Clients_AddOrEdit;
							__summary.m_rmiName = RmiName_NotifySrv_Clients_AddOrEdit;
							__summary.m_hostID = remote;
							__summary.m_hostTag = hostTag;
							BeforeRmiInvocation(__summary);
							__t0 = ::Proud::GetPreciseCurrentTimeMs();
						}
					}
						
					// Call this method.
					bool __ret = NotifySrv_Clients_AddOrEdit (remote,ctx , clientID );
						
					if(__ret==false)
					{
						// Error: RMI function that a user did not create has been called. 
						m_core->ShowNotImplementedRmiWarning(RmiName_NotifySrv_Clients_AddOrEdit);
					}
			
					int64_t __t1;
			
					if(!m_internalUse && m_enableStubProfiling)
					{
						__t1 = ::Proud::GetPreciseCurrentTimeMs() - __t0;
			
						::Proud::AfterRmiSummary __summary;
						__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifySrv_Clients_AddOrEdit;
						__summary.m_rmiName = RmiName_NotifySrv_Clients_AddOrEdit;
						__summary.m_hostID = remote;
						__summary.m_hostTag = hostTag;
			
						__summary.m_elapsedTime = (uint32_t)__t1;
			
						AfterRmiInvocation(__summary);
					}
				}
				break;
			case Rmi_NotifySrv_Clients_Remove:
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
			
					Proud::HostID clientID; __msg >> clientID;
					m_core->PostCheckReadMessage(__msg,RmiName_NotifySrv_Clients_Remove);
					
			
					if(m_enableNotifyCallFromStub && !m_internalUse)
					{
						::Proud::String parameterString;
						
						::Proud::AppendTextOut(parameterString,clientID);	
						
						NotifyCallFromStub(remote, (::Proud::RmiID)Rmi_NotifySrv_Clients_Remove, 
							RmiName_NotifySrv_Clients_Remove,parameterString);
			
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifySrv_Clients_Remove, 
							RmiName_NotifySrv_Clients_Remove, parameterString);
			#endif
					}
					else if(!m_internalUse)
					{
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifySrv_Clients_Remove, 
							RmiName_NotifySrv_Clients_Remove, _PNT(""));
			#endif
					}
						
					int64_t __t0 = 0;
					if(!m_internalUse)
					{
						if(m_enableStubProfiling)
						{
							::Proud::BeforeRmiSummary __summary;
							__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifySrv_Clients_Remove;
							__summary.m_rmiName = RmiName_NotifySrv_Clients_Remove;
							__summary.m_hostID = remote;
							__summary.m_hostTag = hostTag;
							BeforeRmiInvocation(__summary);
							__t0 = ::Proud::GetPreciseCurrentTimeMs();
						}
					}
						
					// Call this method.
					bool __ret = NotifySrv_Clients_Remove (remote,ctx , clientID );
						
					if(__ret==false)
					{
						// Error: RMI function that a user did not create has been called. 
						m_core->ShowNotImplementedRmiWarning(RmiName_NotifySrv_Clients_Remove);
					}
			
					int64_t __t1;
			
					if(!m_internalUse && m_enableStubProfiling)
					{
						__t1 = ::Proud::GetPreciseCurrentTimeMs() - __t0;
			
						::Proud::AfterRmiSummary __summary;
						__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifySrv_Clients_Remove;
						__summary.m_rmiName = RmiName_NotifySrv_Clients_Remove;
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
	const PNTCHAR* Stub::RmiName_RequestLogin =_PNT("RequestLogin");
	#else
	const PNTCHAR* Stub::RmiName_RequestLogin =_PNT("");
	#endif
	#ifdef USE_RMI_NAME_STRING
	const PNTCHAR* Stub::RmiName_NotifyCommon_SendRmi =_PNT("NotifyCommon_SendRmi");
	#else
	const PNTCHAR* Stub::RmiName_NotifyCommon_SendRmi =_PNT("");
	#endif
	#ifdef USE_RMI_NAME_STRING
	const PNTCHAR* Stub::RmiName_NotifyCommon_ReceiveRmi =_PNT("NotifyCommon_ReceiveRmi");
	#else
	const PNTCHAR* Stub::RmiName_NotifyCommon_ReceiveRmi =_PNT("");
	#endif
	#ifdef USE_RMI_NAME_STRING
	const PNTCHAR* Stub::RmiName_NotifyCli_ConnectionState =_PNT("NotifyCli_ConnectionState");
	#else
	const PNTCHAR* Stub::RmiName_NotifyCli_ConnectionState =_PNT("");
	#endif
	#ifdef USE_RMI_NAME_STRING
	const PNTCHAR* Stub::RmiName_NotifyCli_Peers_Clear =_PNT("NotifyCli_Peers_Clear");
	#else
	const PNTCHAR* Stub::RmiName_NotifyCli_Peers_Clear =_PNT("");
	#endif
	#ifdef USE_RMI_NAME_STRING
	const PNTCHAR* Stub::RmiName_NotifyCli_Peers_AddOrEdit =_PNT("NotifyCli_Peers_AddOrEdit");
	#else
	const PNTCHAR* Stub::RmiName_NotifyCli_Peers_AddOrEdit =_PNT("");
	#endif
	#ifdef USE_RMI_NAME_STRING
	const PNTCHAR* Stub::RmiName_NotifySrv_ClientEmpty =_PNT("NotifySrv_ClientEmpty");
	#else
	const PNTCHAR* Stub::RmiName_NotifySrv_ClientEmpty =_PNT("");
	#endif
	#ifdef USE_RMI_NAME_STRING
	const PNTCHAR* Stub::RmiName_NotifySrv_Clients_AddOrEdit =_PNT("NotifySrv_Clients_AddOrEdit");
	#else
	const PNTCHAR* Stub::RmiName_NotifySrv_Clients_AddOrEdit =_PNT("");
	#endif
	#ifdef USE_RMI_NAME_STRING
	const PNTCHAR* Stub::RmiName_NotifySrv_Clients_Remove =_PNT("NotifySrv_Clients_Remove");
	#else
	const PNTCHAR* Stub::RmiName_NotifySrv_Clients_Remove =_PNT("");
	#endif
	const PNTCHAR* Stub::RmiName_First = RmiName_RequestLogin;

}



namespace VizS2C {


	bool Stub::ProcessReceivedMessage(::Proud::CReceivedMessage &pa, void* hostTag) 
	{
#ifndef __FreeBSD__ 
		{
			// unusable but required. you may ignore it, because it does not occur any worthless load.
			unsigned char x = sz_Viz_stub_hRmi[0]; 
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
			case Rmi_NotifyLoginOk:
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
			
					m_core->PostCheckReadMessage(__msg,RmiName_NotifyLoginOk);
					
			
					if(m_enableNotifyCallFromStub && !m_internalUse)
					{
						::Proud::String parameterString;
						
									
						NotifyCallFromStub(remote, (::Proud::RmiID)Rmi_NotifyLoginOk, 
							RmiName_NotifyLoginOk,parameterString);
			
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifyLoginOk, 
							RmiName_NotifyLoginOk, parameterString);
			#endif
					}
					else if(!m_internalUse)
					{
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifyLoginOk, 
							RmiName_NotifyLoginOk, _PNT(""));
			#endif
					}
						
					int64_t __t0 = 0;
					if(!m_internalUse)
					{
						if(m_enableStubProfiling)
						{
							::Proud::BeforeRmiSummary __summary;
							__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifyLoginOk;
							__summary.m_rmiName = RmiName_NotifyLoginOk;
							__summary.m_hostID = remote;
							__summary.m_hostTag = hostTag;
							BeforeRmiInvocation(__summary);
							__t0 = ::Proud::GetPreciseCurrentTimeMs();
						}
					}
						
					// Call this method.
					bool __ret = NotifyLoginOk (remote,ctx  );
						
					if(__ret==false)
					{
						// Error: RMI function that a user did not create has been called. 
						m_core->ShowNotImplementedRmiWarning(RmiName_NotifyLoginOk);
					}
			
					int64_t __t1;
			
					if(!m_internalUse && m_enableStubProfiling)
					{
						__t1 = ::Proud::GetPreciseCurrentTimeMs() - __t0;
			
						::Proud::AfterRmiSummary __summary;
						__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifyLoginOk;
						__summary.m_rmiName = RmiName_NotifyLoginOk;
						__summary.m_hostID = remote;
						__summary.m_hostTag = hostTag;
			
						__summary.m_elapsedTime = (uint32_t)__t1;
			
						AfterRmiInvocation(__summary);
					}
				}
				break;
			case Rmi_NotifyLoginFailed:
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
			
					Proud::ErrorType reason; __msg >> reason;
					m_core->PostCheckReadMessage(__msg,RmiName_NotifyLoginFailed);
					
			
					if(m_enableNotifyCallFromStub && !m_internalUse)
					{
						::Proud::String parameterString;
						
						::Proud::AppendTextOut(parameterString,reason);	
						
						NotifyCallFromStub(remote, (::Proud::RmiID)Rmi_NotifyLoginFailed, 
							RmiName_NotifyLoginFailed,parameterString);
			
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifyLoginFailed, 
							RmiName_NotifyLoginFailed, parameterString);
			#endif
					}
					else if(!m_internalUse)
					{
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_NotifyLoginFailed, 
							RmiName_NotifyLoginFailed, _PNT(""));
			#endif
					}
						
					int64_t __t0 = 0;
					if(!m_internalUse)
					{
						if(m_enableStubProfiling)
						{
							::Proud::BeforeRmiSummary __summary;
							__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifyLoginFailed;
							__summary.m_rmiName = RmiName_NotifyLoginFailed;
							__summary.m_hostID = remote;
							__summary.m_hostTag = hostTag;
							BeforeRmiInvocation(__summary);
							__t0 = ::Proud::GetPreciseCurrentTimeMs();
						}
					}
						
					// Call this method.
					bool __ret = NotifyLoginFailed (remote,ctx , reason );
						
					if(__ret==false)
					{
						// Error: RMI function that a user did not create has been called. 
						m_core->ShowNotImplementedRmiWarning(RmiName_NotifyLoginFailed);
					}
			
					int64_t __t1;
			
					if(!m_internalUse && m_enableStubProfiling)
					{
						__t1 = ::Proud::GetPreciseCurrentTimeMs() - __t0;
			
						::Proud::AfterRmiSummary __summary;
						__summary.m_rmiID = (::Proud::RmiID)Rmi_NotifyLoginFailed;
						__summary.m_rmiName = RmiName_NotifyLoginFailed;
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
	const PNTCHAR* Stub::RmiName_NotifyLoginOk =_PNT("NotifyLoginOk");
	#else
	const PNTCHAR* Stub::RmiName_NotifyLoginOk =_PNT("");
	#endif
	#ifdef USE_RMI_NAME_STRING
	const PNTCHAR* Stub::RmiName_NotifyLoginFailed =_PNT("NotifyLoginFailed");
	#else
	const PNTCHAR* Stub::RmiName_NotifyLoginFailed =_PNT("");
	#endif
	const PNTCHAR* Stub::RmiName_First = RmiName_NotifyLoginOk;

}



