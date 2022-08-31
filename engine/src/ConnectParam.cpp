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
		m_allowExceptionEvent = true;
		m_enableAutoConnectionRecovery = false;

		// true인 경우, 클라-서버간 TCP 연결에서 핑퐁 타임아웃이 발생하면 연결 해제(#TCP_KEEP_ALIVE)를 한다.
		// 기본적으로 true이다. false가 맞으나 하위호환성 때문이다.
		// 이걸 끈 이유: 별별 상황에서 TCP는 살아있는데 정작 어플이 죽은걸로 오탐하는 경우가 있다. 이를 피하고자 한다.
		// 어차피 이거 필요없다. TCP에서 몇 초에 한번씩만 상대에게 송신을 1바이트라도 하면 retransmission timeout으로 10053 에러를 일으키니까.
		// PNTEST에서 TCP 디스를 에뮬하기 위해서 이걸 켜는 경우 말고는 필요없다.
		m_closeNoPingPongTcpConnections = true;

		m_userWorkerThreadModel = ThreadModel_SingleThreaded;
		m_netWorkerThreadModel = ThreadModel_MultiThreaded;

		m_externalUserWorkerThreadPool = NULL;
		m_externalNetWorkerThreadPool = NULL;

		m_timerCallbackIntervalMs = 0;
		m_timerCallbackParallelMaxCount = 1;
		m_timerCallbackContext = NULL;
	}
}
