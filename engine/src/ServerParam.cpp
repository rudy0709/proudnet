/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "../include/ServerParam.h"
#include "../include/NetConfig.h"

namespace Proud
{
	CStartServerParameterBase::CStartServerParameterBase()
	{
		m_encryptedMessageKeyLength = EncryptLevel_Low;				// default encrypt level low
		m_fastEncryptedMessageKeyLength = FastEncryptLevel_Low;	// default fast encrypt level low
		m_threadCount = 0;
		m_netWorkerThreadCount = 0;
		m_allowServerAsP2PGroupMember = false;
		m_enableP2PEncryptedMessaging = false; // 하위호환성 문제보다도 성능 강하가 훨씬 큰 문제. 그리고 아직 암호화 P2P를 실용하는 업체가 없으므로 안전.
		m_timerCallbackIntervalMs = 0;
		m_timerCallbackParallelMaxCount = 1;
		m_timerCallbackContext = NULL;
		m_enableNagleAlgorithm = true;
		m_externalNetWorkerThreadPool = NULL;
		m_externalUserWorkerThreadPool = NULL;
		m_reservedHostIDCount = 0;
		m_reservedHostIDFirstValue = HostID_None;
		m_allowExceptionEvent = true;
		m_enableEncryptedMessaging = true;
	}


	CStartServerParameter::CStartServerParameter()
	{
		m_enableIocp = true;
		m_udpAssignMode = ServerUdpAssignMode_PerClient;
		m_upnpDetectNatDevice = CNetConfig::UpnpDetectNatDeviceByDefault;
		m_upnpTcpAddPortMapping = CNetConfig::UpnpTcpAddPortMappingByDefault;
		m_usingOverBlockIcmpEnvironment = false;
		m_enablePingTest=false;
		m_clientEmergencyLogMaxLineCount = 0;
		m_ignoreFailedBindPort = false;
		m_tunedNetworkerSendIntervalMs_TEST = 0;
		m_failedBindPorts.Clear();
		m_simplePacketMode = false;
		m_hostIDGenerationPolicy = HostIDGenerationPolicy_Recycle;
		m_enableAutoConnectionRecoveryOnServer = true;

#ifdef SUPPORTS_WEBGL
		m_webSocketParam.webSocketType = WebSocket_None;
		m_webSocketParam.listenPort = 0;
		m_webSocketParam.endpoint = _PNT("");
		m_webSocketParam.threadCount = 0;
		m_webSocketParam.timeoutRequest = 5;
		m_webSocketParam.timeoutIdle = 0;
		m_webSocketParam.certFile = _PNT("");
		m_webSocketParam.privateKeyFile = _PNT("");
#endif
	}
}
