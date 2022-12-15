/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once

#include "../include/Enums.h"

namespace Proud
{
#if (_MSC_VER>=1400)
// 아래 주석처리된 pragma managed 전처리 구문은 C++/CLI 버전이 있었을 때에나 필요했던 것입니다.
// 현재는 필요없는 구문이고, 일부 환경에서 C3295 "#pragma managed는 전역 또는 네임스페이스 범위에서만 사용할 수 있습니다."라는 빌드에러를 일으킵니다.
//#pragma managed(push,off)
#endif

	class CNetSettings
	{
	public:
		FallbackMethod m_fallbackMethod;
		int m_serverMessageMaxLength;
		int m_clientMessageMaxLength;
		int m_defaultTimeoutTimeMs;	// NOTE: int64로 바꾸지 말 것. 큰 값이 들어가봤자 무의미한데 쓸데없이 패킷량만 늘어남.
		int m_autoConnectionRecoveryTimeoutTimeMs; // 연결 유지 모드에 돌입한 뒤 얼마나 기다려줄 것인지 설정하는 변수. 단위는 밀리세컨드.

		DirectP2PStartCondition m_directP2PStartCondition;
		int m_overSendSuspectingThresholdInBytes;
		bool m_enableNagleAlgorithm;
		int m_encryptedMessageKeyLength;		// AES 대칭키의 사이즈(bit) 입니다. 128, 192, 256bit만을 지원합니다.
		int m_fastEncryptedMessageKeyLength;	// Fast 대칭키의 사이즈(bit) 입니다. 최대 0,512,1024,2048을 지원합니다. 키값이 길어도 속도에 지장을 주지 않습니다.
		bool m_allowServerAsP2PGroupMember;		// Server 를 P2P 그룹에 포함시킬 것인지에 대한 여부. 기본 false이다.
		bool m_enableP2PEncryptedMessaging;
		bool m_enableEncryptedMessaging; // true이면 클라-서버 및 P2P 암호화 기능이 켜짐.
										 // true이면 NAT router를 찾기를 시도함
		bool m_upnpDetectNatDevice;

		bool m_upnpTcpAddPortMapping;
		
		int m_emergencyLogLineCount;
		bool m_enableLookaheadP2PSend;
		bool m_enablePingTest;
		bool m_ignoreFailedBindPort;

		CNetSettings();
	};

#if (_MSC_VER>=1400)
//#pragma managed(pop)
#endif
}
