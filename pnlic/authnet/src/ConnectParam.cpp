/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "../include/ConnectParam.h"
//#include "../include/netconfig.h"

namespace Proud
{
	CNetConnectionParam::CNetConnectionParam()
	{
		m_tunedNetworkerSendIntervalMs_TEST = 0;
		m_serverPort = 0;
		//m_localPort = 0;
		//m_useTimerType = MilisecTimerType_GetTimeOfDay;
		m_slowReliableP2P = false;	// default value is false
		m_simplePacketMode = false;
		m_allowOnExceptionCallback = true;
		m_enableAutoConnectionRecovery = false;

		m_userWorkerThreadModel = ThreadModel_SingleThreaded;
		m_netWorkerThreadModel = ThreadModel_MultiThreaded;

		m_externalUserWorkerThreadPool = NULL;
		m_externalNetWorkerThreadPool = NULL;

		m_timerCallbackIntervalMs = 0;
		m_timerCallbackParallelMaxCount = 1;
		m_timerCallbackContext = NULL;
	}
}