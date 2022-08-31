/*
ProudNet HERE_SHALL_BE_EDITED_BY_BUILD_HELPER


이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의 : 저작물에 관한 위의 명시를 제거하지 마십시오.


This program is soley copyrighted by Nettention.
Any use, correction, and distribution of this program are subject to the terms and conditions of the License Agreement.
Any violated use of this program is prohibited and will be cause of immediate termination according to the License Agreement.

** WARNING : PLEASE DO NOT REMOVE THE LEGAL NOTICE ABOVE.


此程序的版权归Nettention公司所有。
与此程序的修改、使用、发布相关的事项要遵守此程序的所有权者的协议。
不遵守协议时要原则性的禁止擅自使用。
擅自使用的责任明示在与此程序所有权者的合同书里。

** 注意：不要移除关于制作物的上述明示。


このプログラムの著作権はNettentionにあります。
このプログラムの修正、使用、配布に関する事項は本プログラムの所有権者との契約に従い、
契約を遵守しない場合、原則的に無断使用を禁じます。
無断使用による責任は本プログラムの所有権者との契約書に明示されています。

** 注意：著作物に関する上記の明示を除去しないでください。

*/
#pragma once

#include "Relayer.h"
#include "NetSettings.h"
#include "CompatibleMode.h"
#include "../include/NetVariant.h"

namespace Proud
{
	class CompactFieldMap;

#define SERIALIZE_ENUM(ENUMTYPE, REALTYPE) \
	inline void Message_Write(CMessage& msg, ENUMTYPE a) \
	{ \
		msg.Write(static_cast<REALTYPE>(a)); \
	} \
	inline bool Message_Read(CMessage& msg, ENUMTYPE& a) \
	{ \
		REALTYPE a0 = 0; \
		bool ok = msg.Read(a0); \
		if (ok) \
		{ \
			a = static_cast<ENUMTYPE>(a0); \
		} \
		return ok; \
	}

	SERIALIZE_ENUM(MessagePriority, int8_t);
	SERIALIZE_ENUM(ReliableUdpFrameType, int8_t);
	SERIALIZE_ENUM(MessageType, int8_t);
	SERIALIZE_ENUM(FallbackMethod, int8_t);
	SERIALIZE_ENUM(DirectP2PStartCondition, int8_t);
	SERIALIZE_ENUM(ErrorType, int32_t);
	SERIALIZE_ENUM(RmiID, int16_t);
	SERIALIZE_ENUM(LogCategory, int8_t);
	SERIALIZE_ENUM(CompactFieldName, int16_t);
	SERIALIZE_ENUM(NetVariantType, int8_t);
	SERIALIZE_ENUM(EncryptMode, int8_t);

	// src/ 내 private type에 대한 배열 시리얼라이저
	template<typename T, typename COLLT>
	inline bool Message_ReadArrayT(CMessage &a, COLLT &b)
	{
		int length;
		if (a.ReadScalar(length) == false)
			return false;

		// count가 해킹되어 말도 안되는 값이면 실패 처리하기
		// 물론 모든 경우를 잡지는 못하지만 (sizeof elem 무용지물) 그래도 최소 1바이트는 쓸테니.
		if (length < 0 || length > a.GetLength() - a.GetReadOffset())
			return false;

		b.SetCount(length);

		for (int i = 0; i < length; i++)
		{
			if (Message_Read(a, b[i]) == false)
				return false;
		}
		return true;
	}

	// src/ 내 private type에 대한 배열 시리얼라이저
	template<typename T, typename COLLT>
	inline void Message_WriteArrayT(CMessage &a, const COLLT &b)
	{
		int length = (int)b.GetCount();
		a.WriteScalar(length);

		for (int i = 0; i < length; i++)
			Message_Write(a, b[i]);
	}

	inline bool Message_Read(CMessage& msg, CNetSettings &b)
	{
		return Message_Read(msg, b.m_fallbackMethod) &&
			msg.Read(b.m_serverMessageMaxLength) &&
			msg.Read(b.m_clientMessageMaxLength) &&
			msg.Read(b.m_defaultTimeoutTimeMs) &&
			msg.Read(b.m_autoConnectionRecoveryTimeoutTimeMs) &&
			Message_Read(msg, b.m_directP2PStartCondition) &&
			msg.Read(b.m_overSendSuspectingThresholdInBytes) &&
			msg.Read(b.m_enableNagleAlgorithm) &&
			msg.Read(b.m_encryptedMessageKeyLength) &&
			msg.Read(b.m_fastEncryptedMessageKeyLength) &&
			msg.Read(b.m_allowServerAsP2PGroupMember) &&
			msg.Read(b.m_enableEncryptedMessaging) &&  // 암호화 테스트 youngjunko
			msg.Read(b.m_enableP2PEncryptedMessaging) &&
			msg.Read(b.m_upnpDetectNatDevice) &&
			msg.Read(b.m_upnpTcpAddPortMapping) &&
			msg.Read(b.m_enableLookaheadP2PSend) &&
			msg.Read(b.m_enablePingTest) &&
			msg.Read(b.m_ignoreFailedBindPort) &&
			msg.Read(b.m_emergencyLogLineCount);
	}

	inline void Message_Write(CMessage& msg, const CNetSettings &b)
	{
		Message_Write(msg, b.m_fallbackMethod);
		msg.Write(b.m_serverMessageMaxLength);
		msg.Write(b.m_clientMessageMaxLength);
		msg.Write(b.m_defaultTimeoutTimeMs);
		msg.Write(b.m_autoConnectionRecoveryTimeoutTimeMs);
		Message_Write(msg, b.m_directP2PStartCondition);
		msg.Write(b.m_overSendSuspectingThresholdInBytes);
		msg.Write(b.m_enableNagleAlgorithm);
		msg.Write(b.m_encryptedMessageKeyLength);
		msg.Write(b.m_fastEncryptedMessageKeyLength);
		msg.Write(b.m_allowServerAsP2PGroupMember);
		msg.Write(b.m_enableEncryptedMessaging); // 암호화 테스트 youngjunko
		msg.Write(b.m_enableP2PEncryptedMessaging);
		msg.Write(b.m_upnpDetectNatDevice);
		msg.Write(b.m_upnpTcpAddPortMapping);
		msg.Write(b.m_enableLookaheadP2PSend);
		msg.Write(b.m_enablePingTest);
		msg.Write(b.m_ignoreFailedBindPort);
		msg.Write(b.m_emergencyLogLineCount);
	}

	inline bool Message_Read(CMessage& msg, RelayDestList &relayDestList)
	{
		return Message_ReadArrayT<RelayDest, RelayDestList>(msg, relayDestList);
	}

	inline void Message_Write(CMessage& msg, const RelayDestList &relayDestList)
	{
		Message_WriteArrayT<RelayDest, RelayDestList>(msg, relayDestList);
	}

	inline bool Message_Read(CMessage& msg, RelayDest &rd)
	{
		return (msg.Read(rd.m_frameNumber) && msg.Read(rd.m_sendTo));
	}

	inline void Message_Write(CMessage& msg, const RelayDest &rd)
	{
		msg.Write(rd.m_frameNumber);
		msg.Write(rd.m_sendTo);
	}

	inline void Message_Write(CMessage& msg, const HostIDArray &lst)
	{
		CMessage::WriteArrayT<HostID, true, HostIDArray>(msg, lst);
	}

	inline bool Message_Read(CMessage& msg, HostIDArray &lst)
	{
		return CMessage::ReadArrayT<HostID, true, HostIDArray>(msg, lst);
	}

	PROUD_API void Message_Write(CMessage& msg, const CompactFieldMap &fieldMap);
	PROUD_API bool Message_Read(CMessage& msg, CompactFieldMap &fieldMap);
}

#include "MessagePrivateImpl.inl"
