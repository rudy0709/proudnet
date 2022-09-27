﻿




// Generated by PIDL compiler.
// Do not modify this file, but modify the source .pidl file.


#include "EmergencyC2S_stub.h"


const unsigned char sz_EmergencyC2S_stub_hRmi[] =
{ 0x00, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xff, 0x00, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xff,
0xaa, 0xbb, 0xcc, 0xdd, 0xff,0x99, 0xaa, 0xbb, 0xcc, 0x99, 0xaa, 0xdd, 0xff, 0x00 };


namespace EmergencyC2S {


    bool Stub::ProcessReceivedMessage(::Proud::CReceivedMessage &pa, void* hostTag)
    {
#ifndef __FreeBSD__
        {
            // unusable but required. you may ignore it, because it does not occur any worthless load.
            unsigned char x = sz_EmergencyC2S_stub_hRmi[0];
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
			case Rmi_EmergencyLogData_Begin:
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
			
			        timespec loggingTime; __msg >> loggingTime;
					int32_t connectCount; __msg >> connectCount;
					uint32_t remotePeerCount; __msg >> remotePeerCount;
					uint32_t directP2PEnablePeerCount; __msg >> directP2PEnablePeerCount;
					Proud::String natDeviceName; __msg >> natDeviceName;
					Proud::HostID peerID; __msg >> peerID;
					int iopendingcount; __msg >> iopendingcount;
					uint64_t totalTcpIssueSendBytes; __msg >> totalTcpIssueSendBytes;
					m_core->PostCheckReadMessage(__msg,RmiName_EmergencyLogData_Begin);
			
			
			        if(m_enableNotifyCallFromStub && !m_internalUse)
			        {
			            ::Proud::String parameterString;
			
			            ::Proud::AppendTextOut(parameterString,loggingTime);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,connectCount);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,remotePeerCount);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,directP2PEnablePeerCount);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,natDeviceName);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,peerID);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,iopendingcount);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,totalTcpIssueSendBytes);
			
			            NotifyCallFromStub(remote, (::Proud::RmiID)Rmi_EmergencyLogData_Begin,
			                RmiName_EmergencyLogData_Begin,parameterString);
			
			#ifdef VIZAGENT
			            m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_EmergencyLogData_Begin,
			                RmiName_EmergencyLogData_Begin, parameterString);
			#endif
			        }
			        else if(!m_internalUse)
			        {
			#ifdef VIZAGENT
			            m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_EmergencyLogData_Begin,
			                RmiName_EmergencyLogData_Begin, _PNT(""));
			#endif
			        }
			
			        int64_t __t0 = 0;
			        if(!m_internalUse && m_enableStubProfiling)
			        {
			            ::Proud::BeforeRmiSummary summary;
			            summary.m_rmiID = (::Proud::RmiID)Rmi_EmergencyLogData_Begin;
			            summary.m_rmiName = RmiName_EmergencyLogData_Begin;
			            summary.m_hostID = remote;
			            summary.m_hostTag = hostTag;
			            BeforeRmiInvocation(summary);
			
			            __t0 = ::Proud::GetPreciseCurrentTimeMs();
			        }
			
			        // Call this method.
			        bool __ret = EmergencyLogData_Begin (remote,ctx , loggingTime, connectCount, remotePeerCount, directP2PEnablePeerCount, natDeviceName, peerID, iopendingcount, totalTcpIssueSendBytes );
			
			        if(__ret==false)
			        {
			            // Error: RMI function that a user did not create has been called.
			            m_core->ShowNotImplementedRmiWarning(RmiName_EmergencyLogData_Begin);
			        }
			
			        if(!m_internalUse && m_enableStubProfiling)
			        {
			            ::Proud::AfterRmiSummary summary;
			            summary.m_rmiID = (::Proud::RmiID)Rmi_EmergencyLogData_Begin;
			            summary.m_rmiName = RmiName_EmergencyLogData_Begin;
			            summary.m_hostID = remote;
			            summary.m_hostTag = hostTag;
			            int64_t __t1;
			
			            __t1 = ::Proud::GetPreciseCurrentTimeMs();
			
			            summary.m_elapsedTime = (uint32_t)(__t1 - __t0);
			            AfterRmiInvocation(summary);
			        }
			    }
			    break;
			case Rmi_EmergencyLogData_Error:
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
			
			        int32_t msgSizeErrorCount; __msg >> msgSizeErrorCount;
					int32_t netResetErrorCount; __msg >> netResetErrorCount;
					int32_t intrErrorCount; __msg >> intrErrorCount;
					int32_t connResetErrorCount; __msg >> connResetErrorCount;
					uint32_t lastErrorCompletionLength; __msg >> lastErrorCompletionLength;
					m_core->PostCheckReadMessage(__msg,RmiName_EmergencyLogData_Error);
			
			
			        if(m_enableNotifyCallFromStub && !m_internalUse)
			        {
			            ::Proud::String parameterString;
			
			            ::Proud::AppendTextOut(parameterString,msgSizeErrorCount);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,netResetErrorCount);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,intrErrorCount);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,connResetErrorCount);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,lastErrorCompletionLength);
			
			            NotifyCallFromStub(remote, (::Proud::RmiID)Rmi_EmergencyLogData_Error,
			                RmiName_EmergencyLogData_Error,parameterString);
			
			#ifdef VIZAGENT
			            m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_EmergencyLogData_Error,
			                RmiName_EmergencyLogData_Error, parameterString);
			#endif
			        }
			        else if(!m_internalUse)
			        {
			#ifdef VIZAGENT
			            m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_EmergencyLogData_Error,
			                RmiName_EmergencyLogData_Error, _PNT(""));
			#endif
			        }
			
			        int64_t __t0 = 0;
			        if(!m_internalUse && m_enableStubProfiling)
			        {
			            ::Proud::BeforeRmiSummary summary;
			            summary.m_rmiID = (::Proud::RmiID)Rmi_EmergencyLogData_Error;
			            summary.m_rmiName = RmiName_EmergencyLogData_Error;
			            summary.m_hostID = remote;
			            summary.m_hostTag = hostTag;
			            BeforeRmiInvocation(summary);
			
			            __t0 = ::Proud::GetPreciseCurrentTimeMs();
			        }
			
			        // Call this method.
			        bool __ret = EmergencyLogData_Error (remote,ctx , msgSizeErrorCount, netResetErrorCount, intrErrorCount, connResetErrorCount, lastErrorCompletionLength );
			
			        if(__ret==false)
			        {
			            // Error: RMI function that a user did not create has been called.
			            m_core->ShowNotImplementedRmiWarning(RmiName_EmergencyLogData_Error);
			        }
			
			        if(!m_internalUse && m_enableStubProfiling)
			        {
			            ::Proud::AfterRmiSummary summary;
			            summary.m_rmiID = (::Proud::RmiID)Rmi_EmergencyLogData_Error;
			            summary.m_rmiName = RmiName_EmergencyLogData_Error;
			            summary.m_hostID = remote;
			            summary.m_hostTag = hostTag;
			            int64_t __t1;
			
			            __t1 = ::Proud::GetPreciseCurrentTimeMs();
			
			            summary.m_elapsedTime = (uint32_t)(__t1 - __t0);
			            AfterRmiInvocation(summary);
			        }
			    }
			    break;
			case Rmi_EmergencyLogData_Stats:
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
			
			        uint64_t totalTcpReceiveBytes; __msg >> totalTcpReceiveBytes;
					uint64_t totalTcpSendBytes; __msg >> totalTcpSendBytes;
					uint64_t totalUdpSendCount; __msg >> totalUdpSendCount;
					uint64_t totalUdpSendBytes; __msg >> totalUdpSendBytes;
					uint64_t totalUdpReceiveCount; __msg >> totalUdpReceiveCount;
					uint64_t totalUdpReceiveBytes; __msg >> totalUdpReceiveBytes;
					m_core->PostCheckReadMessage(__msg,RmiName_EmergencyLogData_Stats);
			
			
			        if(m_enableNotifyCallFromStub && !m_internalUse)
			        {
			            ::Proud::String parameterString;
			
			            ::Proud::AppendTextOut(parameterString,totalTcpReceiveBytes);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,totalTcpSendBytes);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,totalUdpSendCount);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,totalUdpSendBytes);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,totalUdpReceiveCount);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,totalUdpReceiveBytes);
			
			            NotifyCallFromStub(remote, (::Proud::RmiID)Rmi_EmergencyLogData_Stats,
			                RmiName_EmergencyLogData_Stats,parameterString);
			
			#ifdef VIZAGENT
			            m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_EmergencyLogData_Stats,
			                RmiName_EmergencyLogData_Stats, parameterString);
			#endif
			        }
			        else if(!m_internalUse)
			        {
			#ifdef VIZAGENT
			            m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_EmergencyLogData_Stats,
			                RmiName_EmergencyLogData_Stats, _PNT(""));
			#endif
			        }
			
			        int64_t __t0 = 0;
			        if(!m_internalUse && m_enableStubProfiling)
			        {
			            ::Proud::BeforeRmiSummary summary;
			            summary.m_rmiID = (::Proud::RmiID)Rmi_EmergencyLogData_Stats;
			            summary.m_rmiName = RmiName_EmergencyLogData_Stats;
			            summary.m_hostID = remote;
			            summary.m_hostTag = hostTag;
			            BeforeRmiInvocation(summary);
			
			            __t0 = ::Proud::GetPreciseCurrentTimeMs();
			        }
			
			        // Call this method.
			        bool __ret = EmergencyLogData_Stats (remote,ctx , totalTcpReceiveBytes, totalTcpSendBytes, totalUdpSendCount, totalUdpSendBytes, totalUdpReceiveCount, totalUdpReceiveBytes );
			
			        if(__ret==false)
			        {
			            // Error: RMI function that a user did not create has been called.
			            m_core->ShowNotImplementedRmiWarning(RmiName_EmergencyLogData_Stats);
			        }
			
			        if(!m_internalUse && m_enableStubProfiling)
			        {
			            ::Proud::AfterRmiSummary summary;
			            summary.m_rmiID = (::Proud::RmiID)Rmi_EmergencyLogData_Stats;
			            summary.m_rmiName = RmiName_EmergencyLogData_Stats;
			            summary.m_hostID = remote;
			            summary.m_hostTag = hostTag;
			            int64_t __t1;
			
			            __t1 = ::Proud::GetPreciseCurrentTimeMs();
			
			            summary.m_elapsedTime = (uint32_t)(__t1 - __t0);
			            AfterRmiInvocation(summary);
			        }
			    }
			    break;
			case Rmi_EmergencyLogData_OSVersion:
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
			
			        uint32_t osMajorVersion; __msg >> osMajorVersion;
					uint32_t osMinorVersion; __msg >> osMinorVersion;
					uint8_t productType; __msg >> productType;
					uint16_t processorArchitecture; __msg >> processorArchitecture;
					m_core->PostCheckReadMessage(__msg,RmiName_EmergencyLogData_OSVersion);
			
			
			        if(m_enableNotifyCallFromStub && !m_internalUse)
			        {
			            ::Proud::String parameterString;
			
			            ::Proud::AppendTextOut(parameterString,osMajorVersion);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,osMinorVersion);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,productType);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,processorArchitecture);
			
			            NotifyCallFromStub(remote, (::Proud::RmiID)Rmi_EmergencyLogData_OSVersion,
			                RmiName_EmergencyLogData_OSVersion,parameterString);
			
			#ifdef VIZAGENT
			            m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_EmergencyLogData_OSVersion,
			                RmiName_EmergencyLogData_OSVersion, parameterString);
			#endif
			        }
			        else if(!m_internalUse)
			        {
			#ifdef VIZAGENT
			            m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_EmergencyLogData_OSVersion,
			                RmiName_EmergencyLogData_OSVersion, _PNT(""));
			#endif
			        }
			
			        int64_t __t0 = 0;
			        if(!m_internalUse && m_enableStubProfiling)
			        {
			            ::Proud::BeforeRmiSummary summary;
			            summary.m_rmiID = (::Proud::RmiID)Rmi_EmergencyLogData_OSVersion;
			            summary.m_rmiName = RmiName_EmergencyLogData_OSVersion;
			            summary.m_hostID = remote;
			            summary.m_hostTag = hostTag;
			            BeforeRmiInvocation(summary);
			
			            __t0 = ::Proud::GetPreciseCurrentTimeMs();
			        }
			
			        // Call this method.
			        bool __ret = EmergencyLogData_OSVersion (remote,ctx , osMajorVersion, osMinorVersion, productType, processorArchitecture );
			
			        if(__ret==false)
			        {
			            // Error: RMI function that a user did not create has been called.
			            m_core->ShowNotImplementedRmiWarning(RmiName_EmergencyLogData_OSVersion);
			        }
			
			        if(!m_internalUse && m_enableStubProfiling)
			        {
			            ::Proud::AfterRmiSummary summary;
			            summary.m_rmiID = (::Proud::RmiID)Rmi_EmergencyLogData_OSVersion;
			            summary.m_rmiName = RmiName_EmergencyLogData_OSVersion;
			            summary.m_hostID = remote;
			            summary.m_hostTag = hostTag;
			            int64_t __t1;
			
			            __t1 = ::Proud::GetPreciseCurrentTimeMs();
			
			            summary.m_elapsedTime = (uint32_t)(__t1 - __t0);
			            AfterRmiInvocation(summary);
			        }
			    }
			    break;
			case Rmi_EmergencyLogData_LogEvent:
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
			
			        int logLevel; __msg >> logLevel;
					Proud::LogCategory logCategory; __msg >> logCategory;
					Proud::HostID logHostID; __msg >> logHostID;
					Proud::String logMessage; __msg >> logMessage;
					Proud::String logFunction; __msg >> logFunction;
					int logLine; __msg >> logLine;
					m_core->PostCheckReadMessage(__msg,RmiName_EmergencyLogData_LogEvent);
			
			
			        if(m_enableNotifyCallFromStub && !m_internalUse)
			        {
			            ::Proud::String parameterString;
			
			            ::Proud::AppendTextOut(parameterString,logLevel);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,logCategory);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,logHostID);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,logMessage);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,logFunction);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,logLine);
			
			            NotifyCallFromStub(remote, (::Proud::RmiID)Rmi_EmergencyLogData_LogEvent,
			                RmiName_EmergencyLogData_LogEvent,parameterString);
			
			#ifdef VIZAGENT
			            m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_EmergencyLogData_LogEvent,
			                RmiName_EmergencyLogData_LogEvent, parameterString);
			#endif
			        }
			        else if(!m_internalUse)
			        {
			#ifdef VIZAGENT
			            m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_EmergencyLogData_LogEvent,
			                RmiName_EmergencyLogData_LogEvent, _PNT(""));
			#endif
			        }
			
			        int64_t __t0 = 0;
			        if(!m_internalUse && m_enableStubProfiling)
			        {
			            ::Proud::BeforeRmiSummary summary;
			            summary.m_rmiID = (::Proud::RmiID)Rmi_EmergencyLogData_LogEvent;
			            summary.m_rmiName = RmiName_EmergencyLogData_LogEvent;
			            summary.m_hostID = remote;
			            summary.m_hostTag = hostTag;
			            BeforeRmiInvocation(summary);
			
			            __t0 = ::Proud::GetPreciseCurrentTimeMs();
			        }
			
			        // Call this method.
			        bool __ret = EmergencyLogData_LogEvent (remote,ctx , logLevel, logCategory, logHostID, logMessage, logFunction, logLine );
			
			        if(__ret==false)
			        {
			            // Error: RMI function that a user did not create has been called.
			            m_core->ShowNotImplementedRmiWarning(RmiName_EmergencyLogData_LogEvent);
			        }
			
			        if(!m_internalUse && m_enableStubProfiling)
			        {
			            ::Proud::AfterRmiSummary summary;
			            summary.m_rmiID = (::Proud::RmiID)Rmi_EmergencyLogData_LogEvent;
			            summary.m_rmiName = RmiName_EmergencyLogData_LogEvent;
			            summary.m_hostID = remote;
			            summary.m_hostTag = hostTag;
			            int64_t __t1;
			
			            __t1 = ::Proud::GetPreciseCurrentTimeMs();
			
			            summary.m_elapsedTime = (uint32_t)(__t1 - __t0);
			            AfterRmiInvocation(summary);
			        }
			    }
			    break;
			case Rmi_EmergencyLogData_End:
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
			
			        uint32_t serverUdpAddrCount; __msg >> serverUdpAddrCount;
					uint32_t remoteUdpAddrCount; __msg >> remoteUdpAddrCount;
					m_core->PostCheckReadMessage(__msg,RmiName_EmergencyLogData_End);
			
			
			        if(m_enableNotifyCallFromStub && !m_internalUse)
			        {
			            ::Proud::String parameterString;
			
			            ::Proud::AppendTextOut(parameterString,serverUdpAddrCount);
						parameterString += _PNT(", ");
						::Proud::AppendTextOut(parameterString,remoteUdpAddrCount);
			
			            NotifyCallFromStub(remote, (::Proud::RmiID)Rmi_EmergencyLogData_End,
			                RmiName_EmergencyLogData_End,parameterString);
			
			#ifdef VIZAGENT
			            m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_EmergencyLogData_End,
			                RmiName_EmergencyLogData_End, parameterString);
			#endif
			        }
			        else if(!m_internalUse)
			        {
			#ifdef VIZAGENT
			            m_core->Viz_NotifyRecvToStub(remote, (::Proud::RmiID)Rmi_EmergencyLogData_End,
			                RmiName_EmergencyLogData_End, _PNT(""));
			#endif
			        }
			
			        int64_t __t0 = 0;
			        if(!m_internalUse && m_enableStubProfiling)
			        {
			            ::Proud::BeforeRmiSummary summary;
			            summary.m_rmiID = (::Proud::RmiID)Rmi_EmergencyLogData_End;
			            summary.m_rmiName = RmiName_EmergencyLogData_End;
			            summary.m_hostID = remote;
			            summary.m_hostTag = hostTag;
			            BeforeRmiInvocation(summary);
			
			            __t0 = ::Proud::GetPreciseCurrentTimeMs();
			        }
			
			        // Call this method.
			        bool __ret = EmergencyLogData_End (remote,ctx , serverUdpAddrCount, remoteUdpAddrCount );
			
			        if(__ret==false)
			        {
			            // Error: RMI function that a user did not create has been called.
			            m_core->ShowNotImplementedRmiWarning(RmiName_EmergencyLogData_End);
			        }
			
			        if(!m_internalUse && m_enableStubProfiling)
			        {
			            ::Proud::AfterRmiSummary summary;
			            summary.m_rmiID = (::Proud::RmiID)Rmi_EmergencyLogData_End;
			            summary.m_rmiName = RmiName_EmergencyLogData_End;
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
	const PNTCHAR* Stub::RmiName_EmergencyLogData_Begin =_PNT("EmergencyLogData_Begin");
	#else
	const PNTCHAR* Stub::RmiName_EmergencyLogData_Begin =_PNT("");
	#endif
	#ifdef USE_RMI_NAME_STRING
	const PNTCHAR* Stub::RmiName_EmergencyLogData_Error =_PNT("EmergencyLogData_Error");
	#else
	const PNTCHAR* Stub::RmiName_EmergencyLogData_Error =_PNT("");
	#endif
	#ifdef USE_RMI_NAME_STRING
	const PNTCHAR* Stub::RmiName_EmergencyLogData_Stats =_PNT("EmergencyLogData_Stats");
	#else
	const PNTCHAR* Stub::RmiName_EmergencyLogData_Stats =_PNT("");
	#endif
	#ifdef USE_RMI_NAME_STRING
	const PNTCHAR* Stub::RmiName_EmergencyLogData_OSVersion =_PNT("EmergencyLogData_OSVersion");
	#else
	const PNTCHAR* Stub::RmiName_EmergencyLogData_OSVersion =_PNT("");
	#endif
	#ifdef USE_RMI_NAME_STRING
	const PNTCHAR* Stub::RmiName_EmergencyLogData_LogEvent =_PNT("EmergencyLogData_LogEvent");
	#else
	const PNTCHAR* Stub::RmiName_EmergencyLogData_LogEvent =_PNT("");
	#endif
	#ifdef USE_RMI_NAME_STRING
	const PNTCHAR* Stub::RmiName_EmergencyLogData_End =_PNT("EmergencyLogData_End");
	#else
	const PNTCHAR* Stub::RmiName_EmergencyLogData_End =_PNT("");
	#endif
	const PNTCHAR* Stub::RmiName_First = RmiName_EmergencyLogData_Begin;

}



