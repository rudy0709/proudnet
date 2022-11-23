#pragma once

#include "SuperSocket.h"
#include "SendOpt.h"
#include "../include/ListNode.h"
#include "../include/AddrPort.h"

namespace Proud
{
	class CSuperSocket;
	class CRemoteClient_S;

	// 서버 입장에서 클라와의 UDP 연결. 홀펀칭 안되어 있으면 TCP fallback을 해주는 역할도 한다.
	// socket, event 등의 객체가 같은 lifetime을 가지므로 이렇게 뜯어냈다.
	class CFallbackableUdpLayer_S
	{
	public:
		// 클라와의 UDP 통신이 가능한가 여부
		bool m_realUdpEnabled;

		// 소켓사용을 요청했는가
		bool m_clientUdpReadyWaiting;

		// 소켓 생성 실패를 한적이 있는가?
		bool m_createUdpSocketHasBeenFailed;
	private:
		/* 서버 입장에서 인식된, 클라의 주소. 즉 클라의 external addr.
		주의: m_UdpAddrFromHere를 바꿀 경우 AddOrUpdateUdpAddrToRemoteClientIndex를
		반드시 호출해야 한다! 인덱스된 값이니까. */
		AddrPort m_UdpAddrFromHere_USE_OTHER;
	public:
		// 클라 로컬에서 인식된 주소
		AddrPort m_UdpAddrInternal;

		// 클라-서버간 UDP 홀펀칭 할 때 사용되는 식별 데이터.
		Guid m_holePunchMagicNumber;

		// 소유자
		CRemoteClient_S* m_owner;

		// UDP 소켓들 중 이 객체에 배정된 소켓. per-client이면 owned이고, static-assigned이면 NS가 직접 가지고 있는 공용 소켓.
		CSuperSocketPtr m_udpSocket;

		// 서버 입장에서 인식된, 클라의 주소. 즉 클라의 external addr.
		inline AddrPort GetUdpAddrFromHere()
		{
			return m_UdpAddrFromHere_USE_OTHER;
		}

		void SetUdpAddrFromHere(AddrPort val);
		void AddToSendQueueWithSplitterAndSignal_Copy(HostID sendHostID,const CSendFragRefs& sendData,const SendOpt &sendOpt);
		void ResetPacketFragState();

		CFallbackableUdpLayer_S(CRemoteClient_S* owner);
		~CFallbackableUdpLayer_S();
	private:
//		CTcpLayer_S &GetFallbackTcpLayer();
	};

}
