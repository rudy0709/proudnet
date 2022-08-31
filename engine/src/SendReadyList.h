#pragma once

#include "../include/AddrPort.h"
#include "FastList2.h"
#include "FastMap2.h"
#include "SuperSocket.h"
#include "SpinLock.h"

namespace Proud
{
	//////////////////////////////////////////////////////////////////////////
	// !!Integ할때 참고!!
	// 이 객체가 사용되면, R0,R1,R2에 있는 send ready list 객체들과 관련 critsec 들을 제거할 것.
	// CListNode<CRemoteClient_S>, CListNode<CSuperSocket>를 쓰는 부분도 제거할 것. 랜클라도 마찬가지.
	//////////////////////////////////////////////////////////////////////////

	class IHostObject;

	/* 송신큐게 뭔가가 들어있는 socket들의 리스트.
	coalesce timer에서 여기에서 항목을 꺼내 issue send or non block send를 한다.

	TCP socket을 가리키는 경우: TCP socket 객체 자체를 가리킨다.
	UDP socket을 가리키는 경우: UDP socket 내 frag board 내 sendToAddr을 키값으로 하는 packet queue 하나를 가리킨다. 수신 대상마다 네트워크 품질이 다를 수 있기 때문이다.

	Q: 직접 packet queue ptr을 가리키면 더 성능이 좋지 않나요?
	A: 그렇게 하고 싶다. 그러나 생각해보자.
	send ready list는 main lock을 하지 않은 채로 접근되어야 하며 이때 얻은 결과물은 socket ptr이다.
	이 socket ptr은 main lock을 한 상태로 얻어야 안전하다. 그렇지만 그러하지 않은 상황이 있을 수 있다. 가령 send completion 루틴에서.
	따라서 얻은 socket ptr은 validation check를 한 후 접근해야 한다.
	그리고 packet queue ptr만 얻어도 다루기 불충분하다. 그것을 소유한 UDP socket ptr도 얻어와야 한다.
	이미 frag board에서는 addrPort to queue map을 가지고 있기 때문에 그것의 key값을 사용해서 validation check을 하는 것이 더 효율적이다.

	Q: socket가 삭제될 때 관련된 항목들을 제거하는 기능은 왜 없나요?
	A: CSendReadySockets에서 이제는 제거 안해도, 시간이 지나면 자연히 도태된다.
	쓸데없이 지우는 루틴 넣느라 CSendReadySockets에 invert index 억세스하는 추가 루틴 만들지 말고 이렇게 냅두자. */
	class CSendReadySockets
	{
	public:
		PROUD_API CSendReadySockets();

		void AddOrSet(const shared_ptr<CSuperSocket>& socket);
		void Remove(const shared_ptr<CSuperSocket>& socket);

		void PopKeys(CSuperSocketArray& output);

		//bool PopEarlistKey(int64_t currTime, IHostObject** outRemote, AddrPort* outUdpSendTo);

		int GetCount() const;

	private:
		// code profile 결과 blocking이 꽤 크다.
		// 성능에 민감한 부분이므로 context switch를 절대 일으키지 않게 하기 위해, spin mutex를 쓴다.
		mutable SpinMutex m_critSec;

		/* send queue에 뭔가가 들어있는 socket들.
		예전에는 map이었다. code profile 결과 성능 임팩트가 커서 list로 변경.
		이것이 처리 못하는 동안 SuperSocket 객체가 계속 남는 것이 PS4나 안드로이드 같은
		리소스 협소 플랫폼에서 잠재적 문제가 될 수 있다. 따라서 weak_ptr을 두어,
		소유자가 놔버리면 같이 덩달아 놓치게 하자. 물론 weak_ptr 특성상 돌연 댕글링은 없다. */
		typedef CFastList2<weak_ptr<CSuperSocket>, int> SendReadySocketList;
		SendReadySocketList m_sendReadySockets;

	};
}
