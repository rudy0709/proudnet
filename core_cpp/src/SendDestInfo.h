#pragma once 

#include "../include/AddrPort.h"

namespace Proud
{
	using namespace std;
	
	class ISendDest;
	class IHostObject;
	class CHostBase;
	class CSuperSocket;

	// Send_BroadcastLayer 등에 의해 사용되는, 수신 대상 정보.
	class SendDestInfo
	{
	public:
		// 수신대상
		HostID mHostID;
		// 수신대상 remote 객체. loopback 객체일 수도 있다.
		shared_ptr<CHostBase> mObject;
		// P2P routed broadcast와 연계됐는지 여부를 파악하는데 씀.
		SendDestInfo *mP2PRoutePrevLink, *mP2PRouteNextLink;

		// UDP인 경우, static assigned or per - client UDP socket
		// #NOTE_AFTER_MAIN_UNLOCK 참고.
		shared_ptr<CSuperSocket> m_socket;

		// UDP인 경우, 송신 대상
		AddrPort mDestAddr;

		inline SendDestInfo()
		{
			mHostID = HostID_None;
			mP2PRoutePrevLink = mP2PRouteNextLink = NULL;
		}
	private:
		// 이것을 갖고 비교 연산을 할 일이 없어야 한다. 과거 코드에서 container의 key로 이 클래스를 쓸 경우 컴파일 에러를 내기 위해.
		bool operator==(const SendDestInfo& b);
		bool operator!=(const SendDestInfo& b);
		bool operator<(const SendDestInfo& b);
	};

	// NOTE: 자주 로컬 변수로 사용되므로 Proud.CPooledArrayObjectAsLocalVar를 통해 사용하자.
	class SendDestInfoArray : public CFastArray < SendDestInfo, true, false, int > {};
	
	class SendDestInfoPtrArray : public CFastArray<SendDestInfo*, false, true, int> {};

}