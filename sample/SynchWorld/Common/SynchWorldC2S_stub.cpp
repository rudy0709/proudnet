﻿  





// Generated by PIDL compiler.
// Do not modify this file, but modify the source .pidl file.

  
#include "SynchWorldC2S_stub.h"


const unsigned char sz_SynchWorldC2S_stub_hRmi[] = 
{ 0x00, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xff, 0x00, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xff,
0xaa, 0xbb, 0xcc, 0xdd, 0xff,0x99, 0xaa, 0xbb, 0xcc, 0x99, 0xaa, 0xdd, 0xff, 0x00 };   


namespace SynchWorldC2S {


	bool Stub::ProcessReceivedMessage(::Proud::CReceivedMessage &pa, void* hostTag) 
	{
#ifndef __FreeBSD__ 
		{
			// unusable but required. you may ignore it, because it does not occur any worthless load.
			unsigned char x = sz_SynchWorldC2S_stub_hRmi[0]; 
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
			case Rmi_RequestLocalHeroSpawn:
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
			
					String heroName; __msg >> heroName;
					Vector3 position; __msg >> position;
					float direction; __msg >> direction;
					m_core->PostCheckReadMessage(__msg,RmiName_RequestLocalHeroSpawn);
					
			
					if(m_enableNotifyCallFromStub && !m_internalUse)
					{
						::Proud::String parameterString;
						
						::Proud::AppendTextOut(parameterString,heroName);	
										
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,position);	
										
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,direction);	
						
						NotifyCallFromStub(remote, (::Proud::RmiID)Rmi_RequestLocalHeroSpawn, 
							RmiName_RequestLocalHeroSpawn,parameterString);
			
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_RequestLocalHeroSpawn, 
							RmiName_RequestLocalHeroSpawn, parameterString);
			#endif
					}
					else if(!m_internalUse)
					{
			#ifdef VIZAGENT
						m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_RequestLocalHeroSpawn, 
							RmiName_RequestLocalHeroSpawn, _PNT(""));
			#endif
					}
						
					int64_t __t0 = 0;
					if(!m_internalUse && m_enableStubProfiling)
					{
						::Proud::BeforeRmiSummary summary;
						summary.m_rmiID = (::Proud::RmiID)Rmi_RequestLocalHeroSpawn;
						summary.m_rmiName = RmiName_RequestLocalHeroSpawn;
						summary.m_hostID = remote;
						summary.m_hostTag = hostTag;
						BeforeRmiInvocation(summary);
			
						__t0 = ::Proud::GetPreciseCurrentTimeMs();
					}
						
					// Call this method.
					bool __ret = RequestLocalHeroSpawn (remote,ctx , heroName, position, direction );
						
					if(__ret==false)
					{
						// Error: RMI function that a user did not create has been called. 
						m_core->ShowNotImplementedRmiWarning(RmiName_RequestLocalHeroSpawn);
					}
						
					if(!m_internalUse && m_enableStubProfiling)
					{
						::Proud::AfterRmiSummary summary;
						summary.m_rmiID = (::Proud::RmiID)Rmi_RequestLocalHeroSpawn;
						summary.m_rmiName = RmiName_RequestLocalHeroSpawn;
						summary.m_hostID = remote;
						summary.m_hostTag = hostTag;
						int64_t __t1;
			
						__t1 = ::Proud::GetPreciseCurrentTimeMs();
			
						summary.m_elapsedTime = (uint32_t)(__t1 - __t0);
						AfterRmiInvocation(summary);
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
	const PNTCHAR* Stub::RmiName_RequestLocalHeroSpawn =_PNT("RequestLocalHeroSpawn");
	#else
	const PNTCHAR* Stub::RmiName_RequestLocalHeroSpawn =_PNT("");
	#endif
	const PNTCHAR* Stub::RmiName_First = RmiName_RequestLocalHeroSpawn;

}



