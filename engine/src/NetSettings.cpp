#include "stdafx.h"
#include <new>
#include "NetSettings.h"
#include "../include/NetConfig.h"

namespace Proud
{
	CNetSettings::CNetSettings()
	{
		m_defaultTimeoutTimeMs = CNetConfig::DefaultNoPingTimeoutTimeMs;
		m_autoConnectionRecoveryTimeoutTimeMs = CNetConfig::AutoConnectionRecoveryTimeoutTimeMs;
		m_directP2PStartCondition = CNetConfig::DefaultDirectP2PStartCondition;
		m_fallbackMethod = FallbackMethod_None;
		m_serverMessageMaxLength = CNetConfig::MessageMaxLengthInOrdinaryCase;
		m_clientMessageMaxLength = CNetConfig::MessageMaxLengthInOrdinaryCase;
		m_overSendSuspectingThresholdInBytes = CNetConfig::DefaultOverSendSuspectingThresholdInBytes;
		m_enableNagleAlgorithm = true;
		m_allowServerAsP2PGroupMember = false;
		m_enableP2PEncryptedMessaging = true;
		m_enableLookaheadP2PSend = true;
		m_emergencyLogLineCount = 0;

		// 128, 192, 256 bit 만을 지원합니다.
		m_encryptedMessageKeyLength = 128;

		// 스트림형 암호방식으로 최대 2048을 지원합니다. 키값이 길어도 속도에 지장을 주지 않습니다.
		m_fastEncryptedMessageKeyLength = 0;

		m_upnpDetectNatDevice = CNetConfig::UpnpDetectNatDeviceByDefault;
		m_upnpTcpAddPortMapping = CNetConfig::UpnpTcpAddPortMappingByDefault;
		m_enablePingTest = false;

		m_ignoreFailedBindPort = false;
	}
}
