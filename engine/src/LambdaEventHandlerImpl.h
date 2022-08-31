#pragma once

#include "../include/LambdaEventHandler.h"

/* [넷텐션 개발자 전용 도움말]
이벤트 콜백을 간편하게 람다식으로도 쓸 수 있게 해주는 매크로입니다.

사용법(예: OnClientLeave):

파라메터 타입 매크로를 먼저 정의한다. 이벤트 콜백 함수의 파라메터들을 채우면 된다.
파라메터 타입만 넣고,이름은 빼야.
예: #define PARAM_OnClientLeave CNetClientInfo *, ErrorInfo *, const ByteArray&

PN_EVENT_PREDEFINE을 클래스 바깥의 헤더 파일에 추가한다. 이때 파라메터 갯수를 숫자로도 넣는다.

PN_EVENT_DECLARE를 사용자에게 노출되는 class 에 추가한다.

PN_EVENT_DECLARE_IMPL을 비노출되는 class 즉 PIMPL의 클래스 정의 헤더 파일에 추가한다.

PN_EVENT_IMPLEMENT_IMPL을 비노출되는 class 즉 PIMPL의 클래스 구현 cpp 파일에 추가한다.

이벤트 람다식 객체를 포장하고 있는 class object의 Run 메서드를 호출한다.
if(m_event_OnClientLeave) m_event_OnClientLeave->Run(param1,param2...);

INetCoreEvent에 있는 이벤트 콜백의 경우는 다음과 같이 작업해야 합니다.

NetCore에는 PN_EVENT_DECLARE_IMPL_VARIABLE 를 추가.
NetClient or NetServer에는 종전과 동일하게 PN_EVENT_DECLARE를 추가.
NetClientImpl or NetServerImpl에는 PN_EVENT_DECLARE 대신 PN_EVENT_DECLARE_IMPL_OTHER을 추가.
NetClient or NetServer에는 종전과 동일하게 PN_EVENT_IMPLEMENT_IMPL을 추가.

PN_EVENT_DECLARE_WITHOUT_VARIABLE
*/



// 인자: 파라메터 이름
#define PN_EVENT_DECLARE_IMPL(NAME) \
	void Set_##NAME(NAME##_BaseParamType* handler); \
	std::shared_ptr<NAME##_BaseParamType> m_event_##NAME;

// 인자: 모 클래스 이름, 파라메터 이름
#define PN_EVENT_IMPLEMENT_IMPL(CLASS, NAME) \
	void CLASS::Set_##NAME(NAME##_BaseParamType* handler) \
	{ \
		m_event_##NAME = std::shared_ptr<NAME##_BaseParamType>(handler); \
	}

// NetCore에서 멤버 변수를 가져야 하는 것들은 이것을 써야.
#define PN_EVENT_DECLARE_IMPL_VARIABLE(NAME) \
	std::shared_ptr<NAME##_BaseParamType> m_event_##NAME;

#define PN_EVENT_DECLARE_IMPL_OTHER(NAME) \
	void Set_##NAME(NAME##_BaseParamType* handler);
