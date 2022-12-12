/*
ProudNet v1.7


이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의 : 저작물에 관한 위의 명시를 제거하지 마십시오.


This program is soley copyrighted by Nettention.
Any use, correction, and distribution of this program are subject to the terms and conditions of the License Agreement.
Any violated use of this program is prohibited and will be cause of immediate termination according to the License Agreement.

** WARNING : PLEASE DO NOT REMOVE THE LEGAL NOTICE ABOVE.


此程序的版权归Nettention公司所有。
与此程序的修改、使用、发布相关的事项要遵守此程序的所有权者的协议。
不遵守协议时要原则性的禁止擅自使用。
擅自使用的责任明示在与此程序所有权者的合同书里。

** 注意：不要移除关于制作物的上述明示。


このプログラムの著作権はNettentionにあります。
このプログラムの修正、使用、配布に関する事項は本プログラムの所有権者との契約に従い、
契約を遵守しない場合、原則的に無断使用を禁じます。
無断使用による責任は本プログラムの所有権者との契約書に明示されています。

** 注意：著作物に関する上記の明示を除去しないでください。

*/

#pragma once

#include "BasicTypes.h"
#include "FakeClr.h"
#include "ThreadPool.h"
#include "IRmiHost.h"
#include "P2PGroup.h"
#include "StartServerParameterBase.h"
#include "ServerUdpAssignMode.h"

//#pragma pack(push,8)

namespace Proud
{
#if (_MSC_VER>=1400)
#pragma managed(push, off)
#endif

	/** \addtogroup net_group
	*  @{
	*/

	/**
Parameters for NetServer startup.
*/
	class CStartServerParameter : public CStartServerParameterBase
	{
	public:
		/**
		\~korean
		TCP 리스닝 포트 목록.
		- 포트를 여러개 만들어서 여러개의 Listening socket을 생성할수 있습니다.
		- 목록을 채우지 않으면 0이 들어갑니다. 0을 지정하면 TCP 리스닝 포트 한개가 자동 할당됩니다. 자동 할당이 유용한 경우는,
		클라이언트가 서버에 접속하기 전에 자동 할당된 포트 번호를 받은 경우,
		예를 들어 스트레스 테스트를 위해 불특정 다수의 서버를 띄워야 하는 경우입니다.
		이때 클라이언트에게 자동할당된 포트 주소를 알려줘야 합니다.
		자동 할당된 포트 번호는 GetServerAddrPort()로 얻을 수 있습니다.

		\~english
		TCP listening port
		- The initial value is 0. The TCP listening port is to be allocated automatically when 0 is designated.
		It is useful to use the automatic allocation when a client received an automatically allocated port number before connected to the server,
		for exmple, when you need to launch multiple anonymous servers for a stress test.
		If this is the case, then the server must let the clients know the automatically allocated port addresses.
		The automatically allocated port numbers can be acquired using GetServerAddrPort().

		\~chinese
		TCP listing端口目录
		- 创建几个端口以后可以生成几个Listening socket。
		- 不填充目录的话会输入0。指定0的话TCP listing一个port会自动分配。自动分配有用的情况是，client在连接服务器之前接收自动分配的port号码的时候，例如为了线程测试，要打开不特定的多数服务器的时候。
		这时候要告诉client自动分配的port地址。
		自动分配的port号码可以由GetServerAddrPort()获得。

		\~japanese
		\~
		*/
		CFastArray<int> m_tcpPorts;

		/**
		\~korean
		UDP 호스트 포트 목록입니다.
		m_udpAssignMode가 Proud.ServerUdpAssignMode_PerClient 인 경우 본 목록은 무시됩니다.

		m_udpAssignMode가 Proud.ServerUdpAssignMode_Static 인 경우 본 목록은 다음과 같이 작동합니다.
		- 0이 들어있는 배열인 경우: 0이 들어있는 갯수만큼 임의 포트 번호를 가지는 UDP 소켓들이 미리 준비됩니다.
		이때 실제 배정된 포트 번호를 얻으려면 CNetServer.GetServerUdpAddrPort()를 사용하면 됩니다.
		- 0 이외의 값들이 들어있는 배열인 경우: 배열에 들어있는 값을 가진 UDP 소켓들이 미리 준비됩니다.

		- UDP 호스트 포트 목록은 서버용 방화벽 설정에 영향을 줍니다. \ref firewall_setting 을 참고하십시오.
		- 포트 갯수의 적정선에 대해서는 \ref server_udp_assign 를 참고하십시오.
		- 개요에 대해서는 \ref server_listen 를 참고하십시오.

		\~english
		This is a list of UDP host port
		When m_udpAssignMode is same as Proud.ServerUdpAssignMode_PerClient, this list will be ignored;
		
		When m_udpAssignMode is same as Proud.ServerUdpAssignMode_Static, this list will work as followings;
		- in the case the allocation containing 0 value: The UDP sockets, as many as of the number of 0s containing, with random port numbers will be prepared.
		CNetServer.GetServerUdpAddrPort() is used to get the port numbers that are actually allocated.
		- in the case the allocation containing other values other than 0: The UDP sockets with the values contained within the allocation will be prepared.

		- The list of UDP host port affects a firewall settings for servers. Please refer to \ref firewall_setting for more details.
		- About reasonable number of port, please refer to \ref server_udp_assign.
		- For the summary, please refer to \ref server_listen.

		\~chinese
		是UDP主机端口目录。
		m_udpAssignMode是 Proud.ServerUdpAssignMode_PerClient%的时候此无关目录。
		
		m_udpAssignMode是 Proud.ServerUdpAssignMode_Static%的时候此目录会如下运转。
		- 有0的数组的时候：会预先准备拥有相当于有0数量的任意port号码的UDP socket。
		此时想获得实际分配的port号码的话，可以使用 CNetServer.GetServerUdpAddrPort()。
		- 有0以外值的数组的时候：预先准备拥有数组里的值的UDP socket。

		- UDP主机port会对服务器用防火墙设置产生影响。请参考\ref firewall_setting%。
		- 对port个数的合理线请参考\ref server_udp_assign%。
		- 对概要请参考\ref server_listen%。

		\~japanese
		\~
		*/
		CFastArray<int> m_udpPorts;

		/**
		\~korean
		UDP 호스트 포트 사용 정책입니다.
		- UDP 호스트 포트 목록은 서버용 방화벽 설정에 영향을 줍니다. \ref firewall_setting 을 참고하십시오.
		- 본 설정은 서버의 성능에 영향을 줍니다. (자세한 내용: \ref server_udp_assign)
		- 개요에 대해서는 \ref server_listen 를 참고하십시오.

		\~english
		This is the policy of using UDP port.
		- The UDP host port list affects the firewall settings for the servers. Please refer \ref firewall_setting.
		- This setting affects server functions. (for more details, please refer \ref server_udp_assign.)
		- For the smmary, please refer \ref server_listen.

		\~chinese
		UDP主机端口使用政策。
		- UDP主机port目录对服务器防火墙设置产生影响。请参考\ref firewall_setting%。
		- 此设置对服务器的性能产生影响。（详细内容：\ref server_udp_assign）
		- 对概要请参考\ref server_listen%。

		\~japanese
		\~
		*/
		ServerUdpAssignMode m_udpAssignMode;

		/**
		\~korean

		\~english

		\~chinese

		\~japanese
		\~
		FOR TEST USE! DO NOT MODIFY THIS
		*/
		bool m_enableIocp;

		/**
		\~korean
		클라이언트에서 필요시 universal plug and play(UPNP) 기능을 시도할 것인지에 대한 여부입니다.
		- 기본값은 true입니다.
		- 서버간 통신에서 CNetClient 를 쓰는 경우 불필요할 수 있는데 이때 이 값을 false로 설정해주면 됩니다.

		\~english
		This is used to decide wheter to use universal plug and play function for the clients when required.
		- The default vlaue is true.
		- It may not be necessary when CNetClient is used between server communications then set tis value as false.

		\~chinese
		Client需要时是否要试图universal plug and play(UPNP)功能的与否。
		- 默认值是true。
		- 服务器之间通信中使用 CNetClient%的时候可能会不需要，这时候把此值设置为false即可。

		\~japanese
		\~
		*/
		bool m_upnpDetectNatDevice;

		/**
		\~korean
		true이면 클라이언트에서 필요시 universal plug and play(UPNP) 기능을 이용하여 TCP 홀펀칭 연결을 강제 포트 매핑
		시킵니다.
		- 이 기능이 켜져 있으면 TCP와의 연결을 안정화 하기 위해 TCP 홀펀칭 연결에 대한 upnp 포트매핑 연결을 제어합니다.
		- 일반적으로 TCP 연결이 열려 있으면 이 기능이 굳이 필요하지는 않습니다. 따라서 기본값은 false입니다.

		\~english TODO:translate needed.

		\~chinese
		True的话client需要时利用universal plug and play(UPNP)功能把TCP打洞连接进行强制port mapping。
		- 此功能开启的话，为了稳定与TCP的连接，制约对TCP打洞连接的upnp port mapping连接。
		- 一般情况下TCP连接开着的话不需要此功能。默认值是fasle。

		\~japanese
		\~
		*/
		bool m_upnpTcpAddPortMapping;


		/**
		\~korean
		서버측 방화벽에서 ICMP 패킷이 오는 호스트가 있을시 그 호스트로부터의 모든 종류의 통신을
		다 차단해버리는 '과잉진압형' 정책이 있는 경우 본 값을 true로 설정해 주어야 합니다. 단, Per client
		UDP assign mode를 사용하는 경우 본 값을 true로 설정하지 않아도 잘 작동할 수 있는데, 이러한 경우
		false를 쓰는 것을 권장합니다.
		- 상기 과잉진압형 정책은 별로 권장되지 않습니다. 왜냐하면 TTL을 제한한 패킷 교환이 불가능하기
		때문에, 일부 공장에서 나오는 인터넷 공유기(라우터)가 서버를 멀웨어로 감지해버리는 사태가
		있을 수 있기 때문입니다.
		- 기본값은 false입니다.

		\~english
		If there exists a strong policy that totally eliminates all types of communications from the host with ICMP packet from server firewalls, then this value must be set as true.
		However, there could be some cases when Per client UDP asigne mode is used and working well while this value is not set as true, but it is recommended to set this value as false.
		- The policy mentioned above is not recommended to stick with since it makes the exchange of the packets that restricted TTL impossible, causing some routers detect the servers as mullware.
		- The default value is false.

		\~chinese
		存在从服务器防火墙传来的ICMP数据包的主机的时候，有切断全部此主机的所有种类通信的‘过剩镇压型’政策的话，把此值设置为true。但是使用Per client UDP assign mode的时候，即使不把此值设置为true也会运转，这时候建议用fasle。
		- 不太建议上述过剩镇压型政策。因为限制TTL的数据包不可能交换，可能会发生从一部分工厂出来的网络路由器会把服务器感知为mullware的情况。
		- 默认值是false。

		\~japanese
		\~
		*/
		bool m_usingOverBlockIcmpEnvironment;

		/**
		\~korean
		서버가 HostID를 발급하는 방식을 지정합니다.
		- default는 HostIDGenerationPolicy_NoRecycle 입니다.

		\~english
		Server will select method of issuing HostID
		- Default is HostIDGenerationPolicy_NoRecycle.

		\~chinese
		服务器指定发放Host ID的方式。
		- default是HostIDGenerationPolicy_NoRecycle。

		\~japanese
		\~
		*/
		//HostIDGenerationPolicy m_HostIDGenerationPolicy;

		/**
		\~korean
		클라이언트가 남겨야 할 비상 로그 라인수입니다.
		- default는 0입니다.
		- 이값을 지정하면, NetClient 에서 SendEmergencyLog 를 호출하는 경우
		EmergencyLogServer 로그파일에 해당 라인수 만큼 비상로그가 저장됩니다.

		\~english
		This is number of emergency log line for client
		- Default is 0
		- Once you set this value, client create LogFile when NetClient call DumpEmergencyLog.

		\~chinese
		Client要留下的紧急log line数。
		- default是0。
		- 指定此值的话，在NetClient呼叫SendEmergencyLog的时候，往EmergencyLogServer log文件储存相当于相关line数的紧急log。

		\~japanese
		\~
		*/
		uint32_t	m_clientEmergencyLogMaxLineCount;

		/*
		\~korean
		모든 피어간의 핑을 수집하는 기능입니다.
		- 초기값은 false 입니다.
		- true로 하면 Peer로 부터 연결되어 있는 모든 피어간의 핑값을 모두 얻어 옵니다.
		- 이 기능이 켜져있지 않다면 Superpeer 선정에 있어 Peer간 핑을 받을 수 없음으로, m_peerLagWeight값을 계산하지 않습니다.

		\~english TODO:translate needed.
		This is function that collect ping between all peers
		- Default is false
		- When you set it true, it will get all ping value from Peer and all other peer that connected.
		-

		\~chinese
		收集所有peer之间的ping功能。
		- 初始值是false。
		- 设为true的话，会全部获取从peer开始连接的所有peer之间ping值。
		- 此功能没有开启的话，在Superpeer选定上不能接收peer之间的ping，不能计算m_peerLagWeight值。

		\~japanese
		\~
		*/
		bool m_enablePingTest;


		/**
		\~korean
		이값을 true로 했을경우 m_udpports에 이미 사용중인 port가 있을경우 실패하지않고 다음포트를 bind하게 됩니다.
		- 초기값은 false입니다.
		- 실패한 port 목록은 m_failedBindPorts 안에 들어갑니다.

		\~english
		When you set this value to true, it bind next port if it has port that already using in m_udpports.
		- Default is false
		- Failed port list will go through to m_failedBindPorts

		\~chinese
		把此值设为true的话，在m_udpports已经有使用中的port的时候不会失败，而是会bind下一个端口。
		- 初始值是false。
		- 失败的port目录进入m_failedBindPorts。

		\~japanese
		\~
		*/
		bool m_ignoreFailedBindPort;

		/**
		\~korean
		m_ignoreFailedBindPort를 true로 했을경우, 이안에 bind 실패한 port목록이 들어가게 됩니다.
		- m_ignoreFailedBindPort가 fasle일때는 값이 채워지지 않습니다.

		\~english
		When you set m_ignoreFailedBindPort to true, port list of failed bind will be there.
		- When m_ignoreFailedBindPort is false, value does not fill in.

		\~chinese
		把m_ignoreFailedBindPort设为true的话，往这里进入bind失败的port目录。
		- m_ignoreFailedBindPort为fasle的时候不会填充此值。

		\~japanese
		\~
		*/
		CFastArray<int> m_failedBindPorts;

		// coalesce interval. 테스트용이므로 평소에는 손대지 말 것. 0이면 기본값 인터벌 값으로 대체됨을 의미하며, 이 값 자체의 기본값은 0이다.
		int m_tunedNetworkerSendIntervalMs_TEST;

		/**
		\~korean
		\brief Simple network protocol mode.

		기본값은 false 입니다.
		패킷 캡쳐 및 복제 방식으로 더미 클라이언트 테스트를 가능하게 하기 위해서 이 값을 true로 설정하십시오.
		단, 서비스가 해커의 공격에 취약해 지며 UDP networking과 direct P2P 통신을 사용할 수 없습니다. (대신 relay 로 전송합니다.)
		라이브 서비스를 위해서는 false로 설정하십시오.

		패킷 캡쳐와 리플레이 테스트관련 내용:
		각각의 더미 클라이언트는 자신의 HostID를 확인할 수 없습니다.
		CreateP2PGroup() 과 같이 P2P 그룹 관련 함수를 호출할 경우 예상치 못한 상황이 발생할 수 있습니다.

		\~english
		\brief Simple network protocol mode.

		Default is false.
		Setting this to true allows dummy client test via packet capture and replication method.
		However, it will make service vulnerable to hackers, and does not allow UDP networking
		and direct P2P communication (will be relayed instead.)
		You should set this to false for live service.

		Notice for packet capture and replay test:
		- Each dummy client cannot identify self HostID.
		Unexpected behavior may occur if you call P2P group functions such as CreateP2PGroup().

		\~chinese
		\brief Simple network protocol mode.

		基本值为false。
		是数据包截取及复制的方式，为能够进行客户端测试请将此值设置为true。
		但这时可能会容易受到黑客攻击且无法使用UDP networking和Direct P2P通信。（但这时会用relay传送）。
		为能够提供实时服务请将此值设置为false。

		数据包截取与Replay测试的相关内容：
		各测试客户端无法确认自己的HostID。
		如CreateP2PGroup()，呼叫P2P组相关函数时可能会发生没有预测到的状况。

		\~japanese
		\brief Simple network protocol mode.

		デフォルト値は false です。
		Packet capture及び複製方式でdummy client テストを可能にするためにこの値を trueに設定してください。
		ただし、サービスがHackerの攻撃に脆弱になり、 UDP networkingと direct P2P通信を使うことができません。 (代わりに relayで転送します。)
		Liveサービスのためには falseに設定してください。

		Packet captureとリプレーテスト関連内容
		各々の dummy client は自分の HostIDの確認ができません。
		CreateP2PGroup() のように P2Pグループ関連関数を呼び出す場合、予想外の状況が発生する恐れがあります。

		\~
		*/
		bool m_simplePacketMode;

		/**
		\~korean
		HostID 재사용 기능 옵션입니다.
		- HostIDGenerationPolicy_Recycle로 설정되어 있을경우 CNetConfig의 HostIDRecycleAllowTimeMs값에 따라 HostID가 재사용되어 집니다.
		- HostIDGenerationPolicy_NoRecycle로 설정되어 있을경우 INT_MAX까지 Unique한 HostID값을 보장합니다.
		
		기본값은 HostIDGenerationPolicy_Recycle입니다.

		\~english
		It is an option for HostID reuse function.
		- If HostIDGenerationPolicy_Recycle is set, HostID will be reused according to HostIDRecycleAllowTimeMs of CNetConfig.
		- If HostIDGenerationPolicy_NoRecycle is set, unique HostID (until INT_MAX) will be guaranteed.

		Default value is HostIDGenerationPolicy_Recycle.
		
		\~chinese
		HostID是再使用技能选项。
		- 已设定为 HostIDGenerationPolicy_Recycle 时，随着 CNetConfig的 HostIDRecycleAllowTimeMs 值， HostID被再使用。
		- 已设定为 HostIDGenerationPolicy_NoRecycle 时 ，保障 Unique 到 INT_MAX 的 HostID值。

		基本值为 HostIDGenerationPolicy_Recycle。
		
		\~japanese
		HostID 再使用機能オプションです。
		- HostIDGenerationPolicy_Recycleと設定されている場合、 CNetConfigのHostIDRecycleAllowTimeMs値によりHostIDが再使用されます。
		- HostIDGenerationPolicy_NoRecycleと設定されている場合、INT_MAXまでUniqueなHostID値を保障します。

		デフォルト値はHostIDGenerationPolicy_Recycleです。
		
		\~
		*/
		HostIDGenerationPolicy m_hostIDGenerationPolicy;

		/**
		\~korean
		해당 값이 설정되면 NetServer 병목 발생시 경고와 덤프 파일이 생성됩니다.
		특별한 경우가 아니라면 함부로 이 값을 설정하지 마십시오.
		
		\~english
		
		
		\~chinese
		
		
		\~japanese
		
		
		\~
		*/
		CriticalSectionSettings m_bottleneckWarningSettings;

		/**
		\~korean
		생성자 메서드입니다.

		\~english
		Constructor method

		\~chinese
		生成者方法。

		\~japanese
		\~
		*/
		PROUD_API CStartServerParameter();
	};


	/**  @} */

#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif

}


//#pragma pack(pop)
