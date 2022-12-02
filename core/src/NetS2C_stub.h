﻿  






// Generated by PIDL compiler.
// Do not modify this file, but modify the source .pidl file.
   
#pragma once

#include "CompactFieldMap.h"

#include "NetS2C_common.h"

     
namespace ProudS2C {


	class Stub : public ::Proud::IRmiStub
	{
	public:
               
		virtual bool P2PGroup_MemberJoin ( ::Proud::HostID, ::Proud::RmiContext& , const HostID & , const HostID & , const ByteArray & , const int & , const ByteArray & , const ByteArray & , const int & , const Proud::Guid & , const bool & , const bool & , const int & , const int & , const CompactFieldMap & )		{ 
			return false;
		} 

#define DECRMI_ProudS2C_P2PGroup_MemberJoin bool P2PGroup_MemberJoin ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & groupHostID, const HostID & memberHostID, const ByteArray & message, const int & eventID, const ByteArray & p2pAESSessionKey, const ByteArray & p2pFastSessionKey, const int & p2pFirstFrameNumber, const Proud::Guid & connectionMagicNumber, const bool & allowDirectP2P, const bool & pairRecycled, const int & reliableRTT, const int & unreliableRTT, const CompactFieldMap & fieldMap) PN_OVERRIDE

#define DEFRMI_ProudS2C_P2PGroup_MemberJoin(DerivedClass) bool DerivedClass::P2PGroup_MemberJoin ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & groupHostID, const HostID & memberHostID, const ByteArray & message, const int & eventID, const ByteArray & p2pAESSessionKey, const ByteArray & p2pFastSessionKey, const int & p2pFirstFrameNumber, const Proud::Guid & connectionMagicNumber, const bool & allowDirectP2P, const bool & pairRecycled, const int & reliableRTT, const int & unreliableRTT, const CompactFieldMap & fieldMap)
#define CALL_ProudS2C_P2PGroup_MemberJoin P2PGroup_MemberJoin ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & groupHostID, const HostID & memberHostID, const ByteArray & message, const int & eventID, const ByteArray & p2pAESSessionKey, const ByteArray & p2pFastSessionKey, const int & p2pFirstFrameNumber, const Proud::Guid & connectionMagicNumber, const bool & allowDirectP2P, const bool & pairRecycled, const int & reliableRTT, const int & unreliableRTT, const CompactFieldMap & fieldMap)
#define PARAM_ProudS2C_P2PGroup_MemberJoin ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & groupHostID, const HostID & memberHostID, const ByteArray & message, const int & eventID, const ByteArray & p2pAESSessionKey, const ByteArray & p2pFastSessionKey, const int & p2pFirstFrameNumber, const Proud::Guid & connectionMagicNumber, const bool & allowDirectP2P, const bool & pairRecycled, const int & reliableRTT, const int & unreliableRTT, const CompactFieldMap & fieldMap)
               
		virtual bool RequestP2PHolepunch ( ::Proud::HostID, ::Proud::RmiContext& , const HostID & , const AddrPort & , const AddrPort & , const CompactFieldMap & )		{ 
			return false;
		} 

#define DECRMI_ProudS2C_RequestP2PHolepunch bool RequestP2PHolepunch ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerID, const AddrPort & internalAddr, const AddrPort & externalAddr, const CompactFieldMap & fieldMap) PN_OVERRIDE

#define DEFRMI_ProudS2C_RequestP2PHolepunch(DerivedClass) bool DerivedClass::RequestP2PHolepunch ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerID, const AddrPort & internalAddr, const AddrPort & externalAddr, const CompactFieldMap & fieldMap)
#define CALL_ProudS2C_RequestP2PHolepunch RequestP2PHolepunch ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerID, const AddrPort & internalAddr, const AddrPort & externalAddr, const CompactFieldMap & fieldMap)
#define PARAM_ProudS2C_RequestP2PHolepunch ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerID, const AddrPort & internalAddr, const AddrPort & externalAddr, const CompactFieldMap & fieldMap)
               
		virtual bool P2P_NotifyDirectP2PDisconnected2 ( ::Proud::HostID, ::Proud::RmiContext& , const HostID & , const ErrorType & , const CompactFieldMap & )		{ 
			return false;
		} 

#define DECRMI_ProudS2C_P2P_NotifyDirectP2PDisconnected2 bool P2P_NotifyDirectP2PDisconnected2 ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerHostID, const ErrorType & reason, const CompactFieldMap & fieldMap) PN_OVERRIDE

#define DEFRMI_ProudS2C_P2P_NotifyDirectP2PDisconnected2(DerivedClass) bool DerivedClass::P2P_NotifyDirectP2PDisconnected2 ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerHostID, const ErrorType & reason, const CompactFieldMap & fieldMap)
#define CALL_ProudS2C_P2P_NotifyDirectP2PDisconnected2 P2P_NotifyDirectP2PDisconnected2 ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerHostID, const ErrorType & reason, const CompactFieldMap & fieldMap)
#define PARAM_ProudS2C_P2P_NotifyDirectP2PDisconnected2 ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerHostID, const ErrorType & reason, const CompactFieldMap & fieldMap)
               
		virtual bool P2P_NotifyP2PMemberOffline ( ::Proud::HostID, ::Proud::RmiContext& , const HostID & , const CompactFieldMap & )		{ 
			return false;
		} 

#define DECRMI_ProudS2C_P2P_NotifyP2PMemberOffline bool P2P_NotifyP2PMemberOffline ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerHostID, const CompactFieldMap & fieldMap) PN_OVERRIDE

#define DEFRMI_ProudS2C_P2P_NotifyP2PMemberOffline(DerivedClass) bool DerivedClass::P2P_NotifyP2PMemberOffline ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerHostID, const CompactFieldMap & fieldMap)
#define CALL_ProudS2C_P2P_NotifyP2PMemberOffline P2P_NotifyP2PMemberOffline ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerHostID, const CompactFieldMap & fieldMap)
#define PARAM_ProudS2C_P2P_NotifyP2PMemberOffline ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerHostID, const CompactFieldMap & fieldMap)
               
		virtual bool P2P_NotifyP2PMemberOnline ( ::Proud::HostID, ::Proud::RmiContext& , const HostID & , const CompactFieldMap & )		{ 
			return false;
		} 

#define DECRMI_ProudS2C_P2P_NotifyP2PMemberOnline bool P2P_NotifyP2PMemberOnline ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerHostID, const CompactFieldMap & fieldMap) PN_OVERRIDE

#define DEFRMI_ProudS2C_P2P_NotifyP2PMemberOnline(DerivedClass) bool DerivedClass::P2P_NotifyP2PMemberOnline ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerHostID, const CompactFieldMap & fieldMap)
#define CALL_ProudS2C_P2P_NotifyP2PMemberOnline P2P_NotifyP2PMemberOnline ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerHostID, const CompactFieldMap & fieldMap)
#define PARAM_ProudS2C_P2P_NotifyP2PMemberOnline ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerHostID, const CompactFieldMap & fieldMap)
               
		virtual bool P2PGroup_MemberLeave ( ::Proud::HostID, ::Proud::RmiContext& , const HostID & , const HostID & , const CompactFieldMap & )		{ 
			return false;
		} 

#define DECRMI_ProudS2C_P2PGroup_MemberLeave bool P2PGroup_MemberLeave ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & memberHostID, const HostID & groupHostID, const CompactFieldMap & fieldMap) PN_OVERRIDE

#define DEFRMI_ProudS2C_P2PGroup_MemberLeave(DerivedClass) bool DerivedClass::P2PGroup_MemberLeave ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & memberHostID, const HostID & groupHostID, const CompactFieldMap & fieldMap)
#define CALL_ProudS2C_P2PGroup_MemberLeave P2PGroup_MemberLeave ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & memberHostID, const HostID & groupHostID, const CompactFieldMap & fieldMap)
#define PARAM_ProudS2C_P2PGroup_MemberLeave ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & memberHostID, const HostID & groupHostID, const CompactFieldMap & fieldMap)
               
		virtual bool NotifyDirectP2PEstablish ( ::Proud::HostID, ::Proud::RmiContext& , const HostID & , const HostID & , const AddrPort & , const AddrPort & , const AddrPort & , const AddrPort & , const int & , const int & , const CompactFieldMap & )		{ 
			return false;
		} 

#define DECRMI_ProudS2C_NotifyDirectP2PEstablish bool NotifyDirectP2PEstablish ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & aPeer, const HostID & bPeer, const AddrPort & X0, const AddrPort & Y0, const AddrPort & Z0, const AddrPort & W0, const int & reliableRTT, const int & unreliableRTT, const CompactFieldMap & fieldMap) PN_OVERRIDE

#define DEFRMI_ProudS2C_NotifyDirectP2PEstablish(DerivedClass) bool DerivedClass::NotifyDirectP2PEstablish ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & aPeer, const HostID & bPeer, const AddrPort & X0, const AddrPort & Y0, const AddrPort & Z0, const AddrPort & W0, const int & reliableRTT, const int & unreliableRTT, const CompactFieldMap & fieldMap)
#define CALL_ProudS2C_NotifyDirectP2PEstablish NotifyDirectP2PEstablish ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & aPeer, const HostID & bPeer, const AddrPort & X0, const AddrPort & Y0, const AddrPort & Z0, const AddrPort & W0, const int & reliableRTT, const int & unreliableRTT, const CompactFieldMap & fieldMap)
#define PARAM_ProudS2C_NotifyDirectP2PEstablish ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & aPeer, const HostID & bPeer, const AddrPort & X0, const AddrPort & Y0, const AddrPort & Z0, const AddrPort & W0, const int & reliableRTT, const int & unreliableRTT, const CompactFieldMap & fieldMap)
               
		virtual bool ReliablePong ( ::Proud::HostID, ::Proud::RmiContext& , const int & , const int & , const CompactFieldMap & )		{ 
			return false;
		} 

#define DECRMI_ProudS2C_ReliablePong bool ReliablePong ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int & localTimeMs, const int & messageID, const CompactFieldMap & fieldMap) PN_OVERRIDE

#define DEFRMI_ProudS2C_ReliablePong(DerivedClass) bool DerivedClass::ReliablePong ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int & localTimeMs, const int & messageID, const CompactFieldMap & fieldMap)
#define CALL_ProudS2C_ReliablePong ReliablePong ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int & localTimeMs, const int & messageID, const CompactFieldMap & fieldMap)
#define PARAM_ProudS2C_ReliablePong ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int & localTimeMs, const int & messageID, const CompactFieldMap & fieldMap)
               
		virtual bool EnableLog ( ::Proud::HostID, ::Proud::RmiContext& , const CompactFieldMap & )		{ 
			return false;
		} 

#define DECRMI_ProudS2C_EnableLog bool EnableLog ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap) PN_OVERRIDE

#define DEFRMI_ProudS2C_EnableLog(DerivedClass) bool DerivedClass::EnableLog ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap)
#define CALL_ProudS2C_EnableLog EnableLog ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap)
#define PARAM_ProudS2C_EnableLog ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap)
               
		virtual bool DisableLog ( ::Proud::HostID, ::Proud::RmiContext& , const CompactFieldMap & )		{ 
			return false;
		} 

#define DECRMI_ProudS2C_DisableLog bool DisableLog ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap) PN_OVERRIDE

#define DEFRMI_ProudS2C_DisableLog(DerivedClass) bool DerivedClass::DisableLog ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap)
#define CALL_ProudS2C_DisableLog DisableLog ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap)
#define PARAM_ProudS2C_DisableLog ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap)
               
		virtual bool NotifyUdpToTcpFallbackByServer ( ::Proud::HostID, ::Proud::RmiContext& , const CompactFieldMap & )		{ 
			return false;
		} 

#define DECRMI_ProudS2C_NotifyUdpToTcpFallbackByServer bool NotifyUdpToTcpFallbackByServer ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap) PN_OVERRIDE

#define DEFRMI_ProudS2C_NotifyUdpToTcpFallbackByServer(DerivedClass) bool DerivedClass::NotifyUdpToTcpFallbackByServer ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap)
#define CALL_ProudS2C_NotifyUdpToTcpFallbackByServer NotifyUdpToTcpFallbackByServer ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap)
#define PARAM_ProudS2C_NotifyUdpToTcpFallbackByServer ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap)
               
		virtual bool NotifySpeedHackDetectorEnabled ( ::Proud::HostID, ::Proud::RmiContext& , const bool & )		{ 
			return false;
		} 

#define DECRMI_ProudS2C_NotifySpeedHackDetectorEnabled bool NotifySpeedHackDetectorEnabled ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const bool & enable) PN_OVERRIDE

#define DEFRMI_ProudS2C_NotifySpeedHackDetectorEnabled(DerivedClass) bool DerivedClass::NotifySpeedHackDetectorEnabled ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const bool & enable)
#define CALL_ProudS2C_NotifySpeedHackDetectorEnabled NotifySpeedHackDetectorEnabled ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const bool & enable)
#define PARAM_ProudS2C_NotifySpeedHackDetectorEnabled ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const bool & enable)
               
		virtual bool ShutdownTcpAck ( ::Proud::HostID, ::Proud::RmiContext& , const CompactFieldMap & )		{ 
			return false;
		} 

#define DECRMI_ProudS2C_ShutdownTcpAck bool ShutdownTcpAck ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap) PN_OVERRIDE

#define DEFRMI_ProudS2C_ShutdownTcpAck(DerivedClass) bool DerivedClass::ShutdownTcpAck ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap)
#define CALL_ProudS2C_ShutdownTcpAck ShutdownTcpAck ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap)
#define PARAM_ProudS2C_ShutdownTcpAck ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap)
               
		virtual bool RequestAutoPrune ( ::Proud::HostID, ::Proud::RmiContext& , const CompactFieldMap & )		{ 
			return false;
		} 

#define DECRMI_ProudS2C_RequestAutoPrune bool RequestAutoPrune ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap) PN_OVERRIDE

#define DEFRMI_ProudS2C_RequestAutoPrune(DerivedClass) bool DerivedClass::RequestAutoPrune ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap)
#define CALL_ProudS2C_RequestAutoPrune RequestAutoPrune ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap)
#define PARAM_ProudS2C_RequestAutoPrune ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap)
               
		virtual bool NewDirectP2PConnection ( ::Proud::HostID, ::Proud::RmiContext& , const HostID & )		{ 
			return false;
		} 

#define DECRMI_ProudS2C_NewDirectP2PConnection bool NewDirectP2PConnection ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerID) PN_OVERRIDE

#define DEFRMI_ProudS2C_NewDirectP2PConnection(DerivedClass) bool DerivedClass::NewDirectP2PConnection ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerID)
#define CALL_ProudS2C_NewDirectP2PConnection NewDirectP2PConnection ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerID)
#define PARAM_ProudS2C_NewDirectP2PConnection ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerID)
               
		virtual bool RequestMeasureSendSpeed ( ::Proud::HostID, ::Proud::RmiContext& , const bool & )		{ 
			return false;
		} 

#define DECRMI_ProudS2C_RequestMeasureSendSpeed bool RequestMeasureSendSpeed ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const bool & enable) PN_OVERRIDE

#define DEFRMI_ProudS2C_RequestMeasureSendSpeed(DerivedClass) bool DerivedClass::RequestMeasureSendSpeed ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const bool & enable)
#define CALL_ProudS2C_RequestMeasureSendSpeed RequestMeasureSendSpeed ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const bool & enable)
#define PARAM_ProudS2C_RequestMeasureSendSpeed ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const bool & enable)
               
		virtual bool S2C_RequestCreateUdpSocket ( ::Proud::HostID, ::Proud::RmiContext& , const NamedAddrPort & )		{ 
			return false;
		} 

#define DECRMI_ProudS2C_S2C_RequestCreateUdpSocket bool S2C_RequestCreateUdpSocket ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const NamedAddrPort & serverUdpAddr) PN_OVERRIDE

#define DEFRMI_ProudS2C_S2C_RequestCreateUdpSocket(DerivedClass) bool DerivedClass::S2C_RequestCreateUdpSocket ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const NamedAddrPort & serverUdpAddr)
#define CALL_ProudS2C_S2C_RequestCreateUdpSocket S2C_RequestCreateUdpSocket ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const NamedAddrPort & serverUdpAddr)
#define PARAM_ProudS2C_S2C_RequestCreateUdpSocket ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const NamedAddrPort & serverUdpAddr)
               
		virtual bool S2C_CreateUdpSocketAck ( ::Proud::HostID, ::Proud::RmiContext& , const bool & , const NamedAddrPort & )		{ 
			return false;
		} 

#define DECRMI_ProudS2C_S2C_CreateUdpSocketAck bool S2C_CreateUdpSocketAck ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const bool & succeed, const NamedAddrPort & serverudpaddr) PN_OVERRIDE

#define DEFRMI_ProudS2C_S2C_CreateUdpSocketAck(DerivedClass) bool DerivedClass::S2C_CreateUdpSocketAck ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const bool & succeed, const NamedAddrPort & serverudpaddr)
#define CALL_ProudS2C_S2C_CreateUdpSocketAck S2C_CreateUdpSocketAck ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const bool & succeed, const NamedAddrPort & serverudpaddr)
#define PARAM_ProudS2C_S2C_CreateUdpSocketAck ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const bool & succeed, const NamedAddrPort & serverudpaddr)
               
		virtual bool NotifyChangedTimeoutTime ( ::Proud::HostID, ::Proud::RmiContext& , const int32_t & )		{ 
			return false;
		} 

#define DECRMI_ProudS2C_NotifyChangedTimeoutTime bool NotifyChangedTimeoutTime ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int32_t & val) PN_OVERRIDE

#define DEFRMI_ProudS2C_NotifyChangedTimeoutTime(DerivedClass) bool DerivedClass::NotifyChangedTimeoutTime ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int32_t & val)
#define CALL_ProudS2C_NotifyChangedTimeoutTime NotifyChangedTimeoutTime ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int32_t & val)
#define PARAM_ProudS2C_NotifyChangedTimeoutTime ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int32_t & val)
               
		virtual bool NotifyChangedAutoConnectionRecoveryTimeoutTimeMs ( ::Proud::HostID, ::Proud::RmiContext& , const int32_t & )		{ 
			return false;
		} 

#define DECRMI_ProudS2C_NotifyChangedAutoConnectionRecoveryTimeoutTimeMs bool NotifyChangedAutoConnectionRecoveryTimeoutTimeMs ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int32_t & val) PN_OVERRIDE

#define DEFRMI_ProudS2C_NotifyChangedAutoConnectionRecoveryTimeoutTimeMs(DerivedClass) bool DerivedClass::NotifyChangedAutoConnectionRecoveryTimeoutTimeMs ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int32_t & val)
#define CALL_ProudS2C_NotifyChangedAutoConnectionRecoveryTimeoutTimeMs NotifyChangedAutoConnectionRecoveryTimeoutTimeMs ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int32_t & val)
#define PARAM_ProudS2C_NotifyChangedAutoConnectionRecoveryTimeoutTimeMs ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int32_t & val)
               
		virtual bool RoundTripLatencyPong ( ::Proud::HostID, ::Proud::RmiContext& , const int32_t & )		{ 
			return false;
		} 

#define DECRMI_ProudS2C_RoundTripLatencyPong bool RoundTripLatencyPong ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int32_t & pingTime) PN_OVERRIDE

#define DEFRMI_ProudS2C_RoundTripLatencyPong(DerivedClass) bool DerivedClass::RoundTripLatencyPong ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int32_t & pingTime)
#define CALL_ProudS2C_RoundTripLatencyPong RoundTripLatencyPong ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int32_t & pingTime)
#define PARAM_ProudS2C_RoundTripLatencyPong ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int32_t & pingTime)
 
		virtual bool ProcessReceivedMessage(::Proud::CReceivedMessage &pa, void* hostTag) PN_OVERRIDE;
		static const PNTCHAR* RmiName_P2PGroup_MemberJoin;
		static const PNTCHAR* RmiName_RequestP2PHolepunch;
		static const PNTCHAR* RmiName_P2P_NotifyDirectP2PDisconnected2;
		static const PNTCHAR* RmiName_P2P_NotifyP2PMemberOffline;
		static const PNTCHAR* RmiName_P2P_NotifyP2PMemberOnline;
		static const PNTCHAR* RmiName_P2PGroup_MemberLeave;
		static const PNTCHAR* RmiName_NotifyDirectP2PEstablish;
		static const PNTCHAR* RmiName_ReliablePong;
		static const PNTCHAR* RmiName_EnableLog;
		static const PNTCHAR* RmiName_DisableLog;
		static const PNTCHAR* RmiName_NotifyUdpToTcpFallbackByServer;
		static const PNTCHAR* RmiName_NotifySpeedHackDetectorEnabled;
		static const PNTCHAR* RmiName_ShutdownTcpAck;
		static const PNTCHAR* RmiName_RequestAutoPrune;
		static const PNTCHAR* RmiName_NewDirectP2PConnection;
		static const PNTCHAR* RmiName_RequestMeasureSendSpeed;
		static const PNTCHAR* RmiName_S2C_RequestCreateUdpSocket;
		static const PNTCHAR* RmiName_S2C_CreateUdpSocketAck;
		static const PNTCHAR* RmiName_NotifyChangedTimeoutTime;
		static const PNTCHAR* RmiName_NotifyChangedAutoConnectionRecoveryTimeoutTimeMs;
		static const PNTCHAR* RmiName_RoundTripLatencyPong;
		static const PNTCHAR* RmiName_First;
		virtual ::Proud::RmiID* GetRmiIDList() PN_OVERRIDE { return g_RmiIDList; }
		virtual int GetRmiIDListCount() PN_OVERRIDE { return g_RmiIDListCount; }
	};

#ifdef SUPPORTS_CPP11 
	
	class StubFunctional : public Stub 
	{
	public:
               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const HostID & , const HostID & , const ByteArray & , const int & , const ByteArray & , const ByteArray & , const int & , const Proud::Guid & , const bool & , const bool & , const int & , const int & , const CompactFieldMap & ) > P2PGroup_MemberJoin_Function;
		virtual bool P2PGroup_MemberJoin ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & groupHostID, const HostID & memberHostID, const ByteArray & message, const int & eventID, const ByteArray & p2pAESSessionKey, const ByteArray & p2pFastSessionKey, const int & p2pFirstFrameNumber, const Proud::Guid & connectionMagicNumber, const bool & allowDirectP2P, const bool & pairRecycled, const int & reliableRTT, const int & unreliableRTT, const CompactFieldMap & fieldMap) 
		{ 
			if (P2PGroup_MemberJoin_Function==nullptr) 
				return true; 
			return P2PGroup_MemberJoin_Function(remote,rmiContext, groupHostID, memberHostID, message, eventID, p2pAESSessionKey, p2pFastSessionKey, p2pFirstFrameNumber, connectionMagicNumber, allowDirectP2P, pairRecycled, reliableRTT, unreliableRTT, fieldMap); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const HostID & , const AddrPort & , const AddrPort & , const CompactFieldMap & ) > RequestP2PHolepunch_Function;
		virtual bool RequestP2PHolepunch ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerID, const AddrPort & internalAddr, const AddrPort & externalAddr, const CompactFieldMap & fieldMap) 
		{ 
			if (RequestP2PHolepunch_Function==nullptr) 
				return true; 
			return RequestP2PHolepunch_Function(remote,rmiContext, remotePeerID, internalAddr, externalAddr, fieldMap); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const HostID & , const ErrorType & , const CompactFieldMap & ) > P2P_NotifyDirectP2PDisconnected2_Function;
		virtual bool P2P_NotifyDirectP2PDisconnected2 ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerHostID, const ErrorType & reason, const CompactFieldMap & fieldMap) 
		{ 
			if (P2P_NotifyDirectP2PDisconnected2_Function==nullptr) 
				return true; 
			return P2P_NotifyDirectP2PDisconnected2_Function(remote,rmiContext, remotePeerHostID, reason, fieldMap); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const HostID & , const CompactFieldMap & ) > P2P_NotifyP2PMemberOffline_Function;
		virtual bool P2P_NotifyP2PMemberOffline ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerHostID, const CompactFieldMap & fieldMap) 
		{ 
			if (P2P_NotifyP2PMemberOffline_Function==nullptr) 
				return true; 
			return P2P_NotifyP2PMemberOffline_Function(remote,rmiContext, remotePeerHostID, fieldMap); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const HostID & , const CompactFieldMap & ) > P2P_NotifyP2PMemberOnline_Function;
		virtual bool P2P_NotifyP2PMemberOnline ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerHostID, const CompactFieldMap & fieldMap) 
		{ 
			if (P2P_NotifyP2PMemberOnline_Function==nullptr) 
				return true; 
			return P2P_NotifyP2PMemberOnline_Function(remote,rmiContext, remotePeerHostID, fieldMap); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const HostID & , const HostID & , const CompactFieldMap & ) > P2PGroup_MemberLeave_Function;
		virtual bool P2PGroup_MemberLeave ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & memberHostID, const HostID & groupHostID, const CompactFieldMap & fieldMap) 
		{ 
			if (P2PGroup_MemberLeave_Function==nullptr) 
				return true; 
			return P2PGroup_MemberLeave_Function(remote,rmiContext, memberHostID, groupHostID, fieldMap); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const HostID & , const HostID & , const AddrPort & , const AddrPort & , const AddrPort & , const AddrPort & , const int & , const int & , const CompactFieldMap & ) > NotifyDirectP2PEstablish_Function;
		virtual bool NotifyDirectP2PEstablish ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & aPeer, const HostID & bPeer, const AddrPort & X0, const AddrPort & Y0, const AddrPort & Z0, const AddrPort & W0, const int & reliableRTT, const int & unreliableRTT, const CompactFieldMap & fieldMap) 
		{ 
			if (NotifyDirectP2PEstablish_Function==nullptr) 
				return true; 
			return NotifyDirectP2PEstablish_Function(remote,rmiContext, aPeer, bPeer, X0, Y0, Z0, W0, reliableRTT, unreliableRTT, fieldMap); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const int & , const int & , const CompactFieldMap & ) > ReliablePong_Function;
		virtual bool ReliablePong ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int & localTimeMs, const int & messageID, const CompactFieldMap & fieldMap) 
		{ 
			if (ReliablePong_Function==nullptr) 
				return true; 
			return ReliablePong_Function(remote,rmiContext, localTimeMs, messageID, fieldMap); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const CompactFieldMap & ) > EnableLog_Function;
		virtual bool EnableLog ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap) 
		{ 
			if (EnableLog_Function==nullptr) 
				return true; 
			return EnableLog_Function(remote,rmiContext, fieldMap); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const CompactFieldMap & ) > DisableLog_Function;
		virtual bool DisableLog ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap) 
		{ 
			if (DisableLog_Function==nullptr) 
				return true; 
			return DisableLog_Function(remote,rmiContext, fieldMap); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const CompactFieldMap & ) > NotifyUdpToTcpFallbackByServer_Function;
		virtual bool NotifyUdpToTcpFallbackByServer ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap) 
		{ 
			if (NotifyUdpToTcpFallbackByServer_Function==nullptr) 
				return true; 
			return NotifyUdpToTcpFallbackByServer_Function(remote,rmiContext, fieldMap); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const bool & ) > NotifySpeedHackDetectorEnabled_Function;
		virtual bool NotifySpeedHackDetectorEnabled ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const bool & enable) 
		{ 
			if (NotifySpeedHackDetectorEnabled_Function==nullptr) 
				return true; 
			return NotifySpeedHackDetectorEnabled_Function(remote,rmiContext, enable); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const CompactFieldMap & ) > ShutdownTcpAck_Function;
		virtual bool ShutdownTcpAck ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap) 
		{ 
			if (ShutdownTcpAck_Function==nullptr) 
				return true; 
			return ShutdownTcpAck_Function(remote,rmiContext, fieldMap); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const CompactFieldMap & ) > RequestAutoPrune_Function;
		virtual bool RequestAutoPrune ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const CompactFieldMap & fieldMap) 
		{ 
			if (RequestAutoPrune_Function==nullptr) 
				return true; 
			return RequestAutoPrune_Function(remote,rmiContext, fieldMap); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const HostID & ) > NewDirectP2PConnection_Function;
		virtual bool NewDirectP2PConnection ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const HostID & remotePeerID) 
		{ 
			if (NewDirectP2PConnection_Function==nullptr) 
				return true; 
			return NewDirectP2PConnection_Function(remote,rmiContext, remotePeerID); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const bool & ) > RequestMeasureSendSpeed_Function;
		virtual bool RequestMeasureSendSpeed ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const bool & enable) 
		{ 
			if (RequestMeasureSendSpeed_Function==nullptr) 
				return true; 
			return RequestMeasureSendSpeed_Function(remote,rmiContext, enable); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const NamedAddrPort & ) > S2C_RequestCreateUdpSocket_Function;
		virtual bool S2C_RequestCreateUdpSocket ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const NamedAddrPort & serverUdpAddr) 
		{ 
			if (S2C_RequestCreateUdpSocket_Function==nullptr) 
				return true; 
			return S2C_RequestCreateUdpSocket_Function(remote,rmiContext, serverUdpAddr); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const bool & , const NamedAddrPort & ) > S2C_CreateUdpSocketAck_Function;
		virtual bool S2C_CreateUdpSocketAck ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const bool & succeed, const NamedAddrPort & serverudpaddr) 
		{ 
			if (S2C_CreateUdpSocketAck_Function==nullptr) 
				return true; 
			return S2C_CreateUdpSocketAck_Function(remote,rmiContext, succeed, serverudpaddr); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const int32_t & ) > NotifyChangedTimeoutTime_Function;
		virtual bool NotifyChangedTimeoutTime ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int32_t & val) 
		{ 
			if (NotifyChangedTimeoutTime_Function==nullptr) 
				return true; 
			return NotifyChangedTimeoutTime_Function(remote,rmiContext, val); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const int32_t & ) > NotifyChangedAutoConnectionRecoveryTimeoutTimeMs_Function;
		virtual bool NotifyChangedAutoConnectionRecoveryTimeoutTimeMs ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int32_t & val) 
		{ 
			if (NotifyChangedAutoConnectionRecoveryTimeoutTimeMs_Function==nullptr) 
				return true; 
			return NotifyChangedAutoConnectionRecoveryTimeoutTimeMs_Function(remote,rmiContext, val); 
		}

               
		std::function< bool ( ::Proud::HostID, ::Proud::RmiContext& , const int32_t & ) > RoundTripLatencyPong_Function;
		virtual bool RoundTripLatencyPong ( ::Proud::HostID remote, ::Proud::RmiContext& rmiContext , const int32_t & pingTime) 
		{ 
			if (RoundTripLatencyPong_Function==nullptr) 
				return true; 
			return RoundTripLatencyPong_Function(remote,rmiContext, pingTime); 
		}

	};
#endif

}


