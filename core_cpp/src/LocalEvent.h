/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once

#include "../include/AddrPort.h"
#include "../include/Enums.h"
#include "../include/NetClientInfo.h"
#include "../include/NetPeerInfo.h"
//#include "TypedPtr.h"
#include "enumimpl.h"
#include "../include/ErrorInfo.h"

namespace Proud
{
	using namespace std;

	class CNetClientInfo;
	typedef shared_ptr<CNetClientInfo> CNetClientInfoPtr;


	// 복사 연산자의 비용이 크므로 꼭 배열에 사용시 주의할 것
	class LocalEvent
	{
	public:
		LocalEventType m_type;
		ErrorInfoPtr m_errorInfo;
		HackType m_hackType;
		CNetClientInfoPtr m_netClientInfo;
		ByteArray m_byteArrayComment;
		ByteArrayPtr m_userData;  // 이건 user callback에서 참조되므로 unsafe이어서는 즐
		HostID m_groupHostID, m_memberHostID, m_remoteHostID;
		// ByteArray는 fast heap을 쓰므로 고속 처리가 가능하다.
		ByteArray m_customField;
		int m_memberCount;
		AddrPort m_remoteAddr;
		SocketErrorCode m_socketErrorCode;
		ByteArray m_connectionRequest;

#ifdef _WIN32
#pragma push_macro("new")
#undef new
		// 이 클래스는 ProudNet DLL 경우를 위해 커스텀 할당자를 쓰되 fast heap을 쓰지 않는다.
		DECLARE_NEW_AND_DELETE
#pragma pop_macro("new")
#endif
	};
}
