#pragma once 

#include "NetCoreStats.h"

namespace Proud
{
	/**
	\~korean
	\brief 서버 상황 통계 정보
	- 성능 측정를 위한 용도이다.

	\~english
	\brief server status statistics information
	- To check performance

	\~chinese
	\brief 服务器状态统计信息。
	- 为了性能测定的用途。

	\~japanese
	\~
	*/
	class CNetServerStats:public CNetCoreStats
	{
	public:
		/**
		\~korean
		클라이언트간의 직간접 P2P 연결의 총 갯수
		- 단, 아직 P2P 통신을 한 적도 없는지라 P2P 연결 자체를 시도조차 안한 P2P 연결은 집계에서 제외된다.

		\~english
		Number of all direct/indirect P2P connection among clients
		- However, this does not include the P2P connections that are not even attempted yet since there has been no P2P communication attempted.

		\~chinese
		Client之间直间接P2P连接的总个数。
		- 但尚未进行过P2P通信，因此如果是连P2P连接本身也都没有试过的P2P连接除外。

		\~japanese
		\~
		*/
		uint32_t m_p2pConnectionPairCount;

		/**
		\~korean
		클라이언트간의 직접 P2P 연결의 총 갯수
		- 단, 아직 P2P 통신을 한 적도 없는지라 P2P 연결 자체를 시도조차 안한 P2P 연결은 집계에서 제외된다.

		\~english
		Number of all direct P2P connection among clients
		- However, this does not include the P2P connections that are not even attempted yet since there has been no P2P communication attempted.

		\~chinese
		Client之间直间接P2P连接的总个数。
		- 但尚未进行过P2P通信，因此如果是连P2P连接本身也都没有试过的P2P连接除外。

		\~japanese
		\~
		*/
		uint32_t m_p2pDirectConnectionPairCount;

		/**
		\~korean
		총 클라이언트 갯수

		\~english
		Number of all clients

		\~chinese
		总client个数。

		\~japanese
		\~
		*/
		uint32_t m_clientCount;

		/**
		\~korean
		UDP 통신을 유지하고 있는 (TCP fallback을 안하고 있는) 클라이언트 갯수

		\~english
		Number of clients that sustain UDP communications(not doing TCP fallback)

		\~chinese
		维持UDP通信的（没有进行TCP fallback）client个数。

		\~japanese
		\~
		*/
		uint32_t m_realUdpEnabledClientCount;

		/**
		\~korean
		사용중인 UDP 포트의 갯수입니다.
		- ServerUdpAssignMode_Static을 사용중이면 이 값은 무의미합니다.
		- ServerUdpAssignMode_PerClient를 사용중이면 이 값은 현재 사용중인 포트의 갯수를 리턴합니다.
		일반적으로, 서버에 연결된 클라이언트의 갯수와 거의 동일합니다만 진단 목적으로 이를
		사용하셔도 됩니다.

		\~english
		Number of UDP ports that are in use
		- This value is meaningless if ServerUdpAssignMode_Static is in use.
		- If ServerUdpAssignMode_PerClient is in use then this value returns the number of ports that are currently in use.
		In general, this is same as the numer of clients connnected to server but you can use this as diagnosis.

		\~chinese
		使用中的UDP端口个数。
		- 使用ServerUdpAssignMode_Static中的话此值就无意义了。
		- 使用ServerUdpAssignMode_PerClient中的话此值返回现在使用中的端口个数。
		一般与在服务器连接的client个数几乎相同，可以以诊断目的使用。

		\~japanese
		\~
		*/
		int m_occupiedUdpPortCount;

		CNetServerStats();
		virtual String ToString() const;
	};
}