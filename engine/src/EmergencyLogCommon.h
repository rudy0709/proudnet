/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once

//#include "ProudNet.h"

//#pragma pack(push,8)

namespace Proud
{
#if (_MSC_VER>=1400)
#pragma managed(push, off)
#endif

	extern const GUID EmergencyProtocolVersion;


	/** EmergencyLog 서버가 요구하는 delegate
	*/
	class IEmergencyLogServerDelegate
	{
	public:
		virtual ~IEmergencyLogServerDelegate() {}

		/** 서버 실행 파라메터를 설정하는 메서드.
		서버가 시작되는 순간 콜백된다. 사용자는 이 메서드를 통해 서버에게 서버 실행 옵션을 설정해야 한다.

		\param refParam 서버 실행 옵션. 이 함수에서 사용자는 Proud.CStartServerParameter.m_tcpPort는 반드시 설정해야 한다.
		Proud.CStartServerParameter.m_localNicAddr,Proud.CStartServerParameter.m_serverAddrAtClient는 필요시 설정하도록 한다.
		나머지 파라메터는 설정하지 않아도 된다.
		주의! CEmergencyLogServer는 UDP 사용을 하지 않기때문에 m_udpPorts, m_udpAssignMode를 설정해도 UDP 통신이 되지 않는다.
		*/
		virtual void OnStartServer(CStartServerParameter &refParam) = 0;

		/** 서버가 종료해야 하는 상황(유저의 요청 등)이면 이 함수가 true를 리턴하면 된다. */
		virtual bool MustStopNow() = 0;
		
		/** Critical section 객체를 리턴한다. 개발자는 이 함수를 통해 이미 서버가 사용중인 critical section이나
		별도로 준비한 critical section 객체를 공급해야 한다. */
		virtual CriticalSection* GetCriticalSection() = 0;
		
		/** 서버 시작이 완료됐음을 알리는 이벤트
		- \param err 서버 시작이 성공했으면 NULL이, 그렇지 않으면 ErrorInfo 객체가 들어있다. */
		virtual void OnServerStartComplete(Proud::ErrorInfo *err) = 0;

		/** 일정 시간마다 호출된다. */
		virtual void OnFrameMove() {}
	};

	/** EmergencyLog 서버

	일반적 용도
	- 클라이언트는 따로 실행할 필요없음 CNetClient.SendEmergencyLog를 호출하면 알아서 로그서버로 보냄
	- 생성은 Create()로 한다.
	- RunMainLoop()를 실행하면 로그 서버가 종료할 때까지 역할을 수행한다.
	*/
	class CEmergencyLogServer
	{
	public:
		virtual ~CEmergencyLogServer(void) {}

		/** 이 메서드를 실행하면 로그 서버가 활성화된다. 이 메서드는 서버가 작동을 중지하라는 요청이 IEmergencyLogServerDelegate에
		의해 오기 전까지 리턴하지 않는다. */
		virtual void RunMainLoop() = 0;

		/** CEmergencyLogServer 객체를 생성한다. */
		PROUDSRV_API static CEmergencyLogServer* Create(IEmergencyLogServerDelegate* dg);
	};

#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif
}


//#pragma pack(pop)