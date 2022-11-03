/*
ProudNet HERE_SHALL_BE_EDITED_BY_BUILD_HELPER


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
\~korean
CNetServer.Start 에 의해 서버가 시작할 때 서버의 초기 설정값입니다.
서버 초기 설정값은 서버 성능에 영향을 줍니다. 자세한 것은 \ref performance_tips 를 참고하십시오.

\~english
This is an initial setup value when the server initializes.
The initial values of the server directly affect server functions. Please refer \ref performance_tips for more details.

\~chinese
服务器由 CNetServer.Start%开始的时候的初始设定值。
服务器的初始设定值对服务器性能产生影响。详细请参考\ref performance_tips%。

\~japanese
\~
*/
	class CStartServerParameterBase
	{
	public:
		/**
		\~korean
		서버의 주소입니다.
		- 초기값은 빈 문자열입니다.
		- 통상적으로 빈 문자열을 지정하지만, L4 스위치나 인터넷 공유기(NAT) 뒤에서 서버가 작동할 경우
		클라이언트가 인식할 서버의 호스트 이름이나 IP address을 넣어줘야 합니다.
		- IP 주소(111.111.111.111 등) 혹은 인식 가능한 이름(aaa.mydomain.com)을 넣을 수 있습니다. 하지만 포트 번호는 넣지 못합니다.

		\~english
		This is a server address.
		- The initial vlaue is an empty string.
		- It is usual to designate an empty value for this but it is necessary to input either the host name of the server or IP address
		when the server is working behind an L4 switch or an Internet router(NAT) for the clients to recognize.
		- An IP address (e.g. 111.111.111.111) or a recognizable name (www.mydomain.com) can be used, but not the port number

		\~chinese
		服务器地址。
		- 初始值是空字符串。
		- 一般不指定空字符串，但是服务器在L4 switch或者网络路由器（NAT）后面运转的时候要输入client能识别的服务器主机名或者IP address。
		- 可以输入IP地址(111.111.111.111等)或者可以识别的名字(aaa.mydomain.com)。但是不能输入端口号码。

		\~japanese
		\~
		*/
		String m_serverAddrAtClient;

		/**
		\~korean
		서버의 리스닝 소켓이 바인딩될 주소입니다.
		- 초기값은 빈 문자열입니다.
		- 통상적으로 빈 문자열을 지정하지만 서버가 네트워크 인터페이스(NIC)를 2개 이상 가지는 경우,
		그리고 그것들 중 하나만이 CNetClient 에서 오는 접속 요청을 받을 수 있는 경우 여기에 해당됩니다.
		NIC에 지정된 IP 또는 호스트 이름을 지정해야 합니다. 호스트에 있는 NIC 주소 리스트를 얻으려면 Proud.GetLocalIPAddresses 를 쓸 수 있습니다.
		- 호스트 이름을 지정할 경우 클라이언트는 서버로 접속하기 위한 주소로 localhost를 입력할 경우 연결 실패를 할 수 있습니다. 이러한 경우
		클라이언트는 서버로 접속하기 위해 서버 호스트의 주소를 명시적으로 입력해야 합니다.
		- \ref multiple_nic_at_server

		\~english
		This is the address that the listening socket of the server to be bound.
		- The initial value is an empty value.
		- It is usual to designate an empty value for this but it is used in the case that the server has more than
		2 Network Interfces (NIC) and when only one of them is able to receive a connection request from CNetClient.
		Either an IP address or a host name that is registered by the NIC must be designated.
		It is possible to use Proud.GetLocalIPAddresses to acquire the NIC address list in the host.
		- When a hostname is designated, itis possible to cause a connection failure when a localhost is used for the address to connect at the client.
		It is necessary to input correct address of the server host in order to connect to client server.
		- \ref multiple_nic_at_server

		\~chinese
		服务器的listing socket要被bound的地址。
		- 初始值是空字符串。
		- 一般指定为空字符串，但是服务器拥有2个以上网络界面（NIC）的时候，或者是只有其中之一可以接收从 CNetClient%发来的连接邀请的时候归于这里。
		要指定在NIC指定的IP或者主机名称。想获得在主机的NIC地址list的话可以使用 Proud.GetLocalIPAddresses%。
		- 指定主机名的时候，client 为了连接服务器而输入的地址是localhost的话可能会连接失败。这时候client为了连接服务器，要明示地输入服务器主机的地址。
		- \ref multiple_nic_at_server

		\~japanese
		\~
		*/
		String m_localNicAddr;
		/**
		\~korean
		서버와 클라이언트간 맞추는 프로토콜 버전입니다.
		- 만약 프로토콜 버전이 서로 다르게 지정된 클라이언트가
		서버에 연결(CNetClient.Connect)하려고 시도하면
		클라이언트에서 ErrorType_ProtocolVersionMismatch 가 트리거됩니다.

		\~english
		This is a protocaol version that syncs the servers and the clients.
		- When a client with different protocol version to ther server attempt to connect(CNetClient.Connect), the client will trigger ErrorType_ProtocolVersionMismatch.

		\~chinese
		服务器和client对接的协议版本。
		- 如果协议版本指定相互不同的client试图连接（CNetClient.Connect）服务器的话，client会引起ErrorType_ProtocolVersionMismatch。

		\~japanese
		\~
		*/
		Guid m_protocolVersion;

		/**
		\~korean
		사용자 정의 루틴이 실행되는 스레드가 이 모듈의 내장 스레드 풀에서 실행될 경우, 그 스레드 풀의 스레드 갯수입니다.
		- 기본값은 0입니다. 0을 지정하면 CPU 갯수로 지정됩니다.
		- m_externalUserWorkerThreadPool이 설정 되었을 경우 이 값은 무시됩니다.

		\~english
		The number of treads in thread pool. It must be at least bigger than 1.
		- The initial value is 0. If 0 is designated, then it is to be disignated as the number of CPUs.
		- When you set m_externalUserWorkerThreadPool then this value is ignored.

		\~chinese
		用户定义routine实行的线程在此模块的内置线程槽里实行时候，那个线程槽的线程个数。
		- 默认值是0。指定0的话便会指定为CPU个数。
		- m_externalUserWorkerThreadPool被设置的时候此值会被忽视。

		\~japanese
		\~
		*/
		int m_threadCount;

		/**
		\~korean
		ProudNet은 내부적으로 I/O 처리를 담당하는 스레드가 있습니다. 이것의 갯수를 지정합니다.
		- 기본값은 0입니다. 0을 지정하면 CPU 갯수로 지정됩니다.
		- 만약 ProudNet이 작동할 서버가 1개의 머신에서 CPU 코어 갯수만큼 작동하는 경우 사실상 서버는 단일 스레드 기반이나 다름없습니다. 이러한 경우에는 이 값을 1로 설정해서 1개의 CPU 만 사용하도록 설정해주면 ProudNet의 처리 성능이 향상됩니다. 그 외에는 본 값은 그냥 두는 것이 좋습니다.
		- m_externalNetWorkerThreadPool이 설정 되었을 경우 이 값은 무시됩니다.

		\~english
		ProudNet has a thread that deals with internal I/O. This value designates how many for this.
		- If it is 0 then it is regarded as the number of CPU cores.
		- In case when the server where Proudnet to be working works as many as the number of CPU cores within a machine, it is reasonable to say that the server is one thread base.
		If this is the case, it is ideal to set this value as 1 to use 1 CPU in order to increase the CPU's overall efficiency.
		- If this is not the case, then it is ideal to leave the value as it is.

		\~chinese
		ProudNet 内置着担任I/O处理的线程。指定此个数。
		- 默认值是0。指定0的话便会指定为CPU个数。
		- 如果运转ProudNet的服务器在一个机器运转相当于CPU core数量的时候，实际上服务器与单一线程基础没有不同。这种情况要把此值设置为1，设置成只使用一个CPU的话，会提高ProudNet的处理性能。此外不碰此值的为好。
		- m_externalNetWorkerThreadPool被设置的时候此值会被忽视。

		\~japanese
		\~
		*/
		int m_netWorkerThreadCount;
		/**
		\~korean
		ProudNet에서 암호화된 메시징을 주고 받을 때의 암호키의 길이입니다. (참고: \ref key_length)
		- AES기반의 프라우드넷의 자체 암호화된 키를 생성합니다.
		- Proud::EncryptLevel 값을 참조하여 세팅하여야 하며 초기값은 EncryptLevel_Low 입니다.

		\~english
		This is the length of the coded key when ProudNet communicates with encrypted messeges. (Please refer to \ref key_length.)
		- It generates encrypted key that based on AES.
		- You need to set it up with refer to Proud::EncryptLevel value and defualt value is EncryptLevel_Low.

		\~chinese
		在ProudNet里传接加密的messaging的时候的密钥长度。（参考：\ref key_length）
		- 生成AES基础的ProudNet本身加密的key。
		- 要参照 Proud::EncryptLevel%值而设置，初始值是EncryptLevel_Low。

		\~japanese
		\~
		*/
		EncryptLevel m_encryptedMessageKeyLength;

		/**
		\~korean
		ProudNet에서 암호화된 메시징을 주고 받을 때의 암호키의 길이입니다. (참고: \ref key_length)
		- Fast기반의 프라우드넷의 자체 암호화된 키를 생성합니다.
		- 스트리밍 암호화 방식이기 때문에 키값이 길어도 암복호화의 속도에는 영향을 주지 않습니다.
		- Proud::FastEncryptLevel 값을 참조하여 세팅하여야 하며 초기값은 FastEncryptLevel_Low 입니다.

		\~english
		This is the length of the coded key when ProudNet communicates with encrypted messeges. (Please refer to \ref key_length.)
		- It generates encrypted key that based on Fast
		- It does not affect speed of encryption even key value is long because it use streaming encryption type.
		- You need to set it up with refer to Proud::FastEncryptLevel value and defualt value is FastEncryptLevel_Low.

		\~chinese
		在ProudNet里传接加密的messaging的时候的密钥长度。（参考：\ref key_length）
		- 生成Fast基础的ProudNet本身加密的key。
		- 因为是串流加密方式，key 值长也不会对加密、解密速度产生影响。
		- 要参照 Proud::FastEncryptLevel%值而设置，初始值是FastEncryptLevel_Low。

		\~japanese
		\~
		*/
		FastEncryptLevel m_fastEncryptedMessageKeyLength;

		/**
		\~korean
		P2P RMI 메세지의 encrypt 를 켜거나 끄는기능입니다. 기본값은 false입니다.
		- 만약, 사용자가 false로 설정하면 P2P 암호화된 RMI는 불가능합니다. 대신에 서버성능은 향상됩니다.
		- 사용자가 false인 상태에서 encrypt 메세징을 할경우에는 OnError가 콜백되고, P2P 메세지는 비암호화 RMI로 전송됩니다.

		\~english
		This is used to turn on/off the encryption function of P2P RMI message. The default value is false.
		- If the user sets this as false then it is impossible to use encrypted P2P RMI. Instead, the server performance is improved.
		- If the user uses encrypted messaging with its value being false, then OnError will be called back and the P2P message will be sent as not-encrypted RMI.

		\~chinese
		开启或关闭P2P RMI信息的encrypt的功能。默认值是false。
		- 如果用户设置为false的话P2P加密的RMI是不可能的。但是服务器性能会提高。
		- 用户在false的状态下进行encrypt messaging的时候会回拨 OnError，P2P%信息会传送到非加密RMI。

		\~japanese
		\~
		*/
		bool m_enableP2PEncryptedMessaging;

		/**
		\~korean
		서버를 \ref p2p_group 의 멤버로 포함시킬 수 있는지를 결정하는 설정값입니다.
		- 기본값은 false입니다.
		- 자세한 내용은 \ref server_as_p2pgroup_member  를 참고하십시오.

		\~english
		This is a value to decide whether the server can be included as the member of \ref p2p_group or not.
		- The default value is false.
		- Please refer to \ref server_as_p2pgroup_member for more details.

		\~chinese
		决定是否能把服务器包含为\ref p2p_group%的成员的设定值。
		- 默认值是false。
		- 详细内容请参考\ref server_as_p2pgroup_member%。

		\~japanese
		\~
		*/
		bool m_allowServerAsP2PGroupMember;


		/**
		\~korean
		Timer callback 주기 입니다. \ref server_timer_callback  기능입니다.
		이것을 세팅하면 milisec단위로 한번씩 INetServerEvent.OnTick 가 호출됩니다.
		- 0이면 콜백하지 않습니다.
		- 기본값은 0입니다.

		\~english
		This sets the period of timer callback. A function described in \ref server_timer_callback.
		When this is activated, INetServerEvent.OnTimerCallback will be called at every period set by millisecond unit.
		- Ther is no callback when this is set as 0.
		- The default value is 0

		\~chinese
		Timer callback周期。\ref server_timer_callback  功能。
		设置这个的话， INetServerEvent.OnTick%会以milisec单位呼叫一次。
		- 0的的话不会回拨。
		- 默认值是0。

		\~japanese
		\~
		*/
		uint32_t m_timerCallbackIntervalMs;

		/**
		\~korean
		Timer callback이 동시 몇 개의 user worker thread에서 호출될 수 있는지를 정합니다.
		기본값은 1입니다. 모든 user worker thread를 사용하고 싶다면 INT_MAX를 입력해도 됩니다.

		\~english TODO:translate needed.

		\~chinese
		决定Timer callback同时能在几个user worker thread被呼叫。
		默认值是1。想使用所有user worker thread的话也可以输入INT_MAX。

		\~japanese
		\~
		*/
		int32_t m_timerCallbackParallelMaxCount;

		/**
		\~korean
		서버에서 일정주기에 한번씩 콜백을 할시에 인자로 사용되는 유저 데이터입니다.
		Proud.INetServerEvent.OnTick 가 호출될시에 인자값으로 들어갑니다. \ref server_timer_callback 기능입니다.
		- 기본값은 NULL입니다.

		\~english
		This is a user data used as an index when the server calls back each time in every period.
		This value is to be input as an index when Proud.INetServerEvent.OnTimerCallback is called. A function described in \ref server_timer_callback.
		- The default value is NULL.

		\~chinese
		在服务器每一定周期回拨一次的时候用于因素的用户数据。
		Proud.INetServerEvent.OnTick%被呼叫的时候输入成因素值。是\ref server_timer_callback%功能。
		- 默认值是NULL。

		\~japanese
		\~
		*/
		void* m_timerCallbackContext;

		/**
		\~korean
		\ref delayed_send 기능을 켜거나 끕니다. TCP를 주로 사용하면서 레이턴시에 민감하며, 통신량이 적은 앱은 이 값을 false로 지정하기도 합니다.
		- 기본값은 true입니다.
		- true인 경우 TCP Nagle 알고리즘이 켜지며 ProudNet 자체의 지연 송신 기능을 끕니다.
		- false인 경우 TCP Nagle 알고리즘이 꺼지며 ProudNet 자체의 지연 송신 기능을 켭니다.
		- TCP Nagle 알고리즘은 운영체제에 따라 다릅니다. Windows간 호스트에서 WAN에서는 최장 0.7초가 보고되었습니다.
		- ProudNet 자체의 지연 송신 기능은 최장 0.01초입니다.

		\~english
		Turn on or off \ref delayed_send function. It normally use TCP and sensitive with latency. If application does not require high traffic, sometimes set this value to false.
		- Default value is true
		- If it is true, it will trun on TCP Nagle algorism and turn off delayed sending function.
		- If it is false, it will turn off TCP Nagle algorism and turn on delayed sending function.
		- TCP Nagle algorism is different depends on OS. Longest time that reported is 0.7 second in WAN, Host between Windows.
		- ProudNet's delayed sending function is 0.01 second for longest.

		\~chinese
		开或者关\ref delayed_send%功能。主要使用TCP，对latency较为敏感，通信量少的应用也把此值指定为false。
		- 默认值是true。
		- true的话开启TCP Nagle算法，关闭ProudNet本身的延迟传送功能。
		- false的话关闭TCP Nagle算法，开启ProudNet本身的延迟传送功能。
		- TCP Nagle算法根据运营体系而不同。Windows 间的主机中，在WAN报告最长为0.7秒。
		- ProudNet本身的延迟传送功能的最长是0.01秒。

		\~japanese
		\~
		*/
		bool m_enableNagleAlgorithm;

		/**
		\~korean
		P2P Group 생성에 사용할 예약된 범위의 HostID의 시작값입니다. 범위의 크기는 m_reservedHostIDCount로 설정하십시오.
		- 클라이언트들의 HostID는 이 예약 범위를 피해서 생성됩니다.
		- CreateP2PGroup()에 이 예약 범위에 있는 값을 넣어서 사용자가 원하는 HostID값을 가진 P2P group을 생성할 수 있습니다.

		\~english TODO:translate needed.
		It is the start value of HostID whose range was reserved for P2P Group creation. Need to set the size of range as m_reservedHostIDCount.
		- HostIDs of clients that are about to be connected must be created beyond the reserved range.
		- P2P group that has HostID value that a user wants can be created by inserting the value (in the reserved range) into CreateP2PGroup().

		\~chinese
		在生成P2P Group 时要使用的预定范围内的 HostID 初始值。请将范围大小设置为 m_reservedHostIDCount。
		- 所要连接的客户端 HostID会避开这个预定范围生成。
		- 在CreateP2PGroup()中输入此预定范围内的值可以生成拥有用户所希望 HostID值的 P2P group。

		\~japanese
		P2P Group 生成に使用する予約された範囲のHostIDのスタート値です。範囲の大きさは m_reservedHostIDCountに設定してください。
		- 接続されたクライアントたちのHostIDはこの予約範囲を避けて生成されます。
		- CreateP2PGroup()にこの予約範囲にある値を入れてユーザが望むHostID値をもったP2P groupを生成できます。
		\~
		*/
		HostID m_reservedHostIDFirstValue;

		/**
		\~korean
		P2P Group 생성에 사용할 예약된 범위의 HostID의 갯수입니다. 시작값은 m_reservedHostIDFirstValue로 설정하십시오.

		\~english
		It is the number of HostID in the reserved range that are going to be used for P2P group creation. Start value must be set as m_reservedHostIDFirstValue.

		\~chinese
		在生成P2P Group 时要使用的预定范围内的 HostID 个数。请将初始值设置为m_reservedHostIDFirstValue。

		\~japanese
		P2P Group生成に使用する予約された範囲のHostIDの数です。スタート値はm_reservedHostIDFirstValueに設定してください。

		\~
		*/
		int m_reservedHostIDCount;

		/**
		\~korean
		사용자 정의 메서드를 사용자가 수동으로 생성한 thread pool에서 실행하게 합니다. 자세한 것은 \ref thread_pool_sharing 을 참고하십시오.
		- 기본값은 null입니다.
		- 이 값을 설정했을 경우 m_threadCount는 무시됩니다.

		\~english
		Running user defined method in thread pool that created manually by user. Please refer to \ref thread_pool_sharing for more detail.
		- Default value is null
		- When you set this value, m_threadCount is ignored.

		\~chinese
		在用户手动生成的thread pool里实行用户定义方法。详细请参考\ref thread_pool_sharing%。
		- 默认值是null。
		- 设定此值的话会忽视m_threadCount。

		\~japanese
		\~
		*/
		CThreadPool* m_externalUserWorkerThreadPool;
		
		/**
		\~korean
		network I/O 관련 루틴을 사용자가 수동으로 생성한 thread pool에서 실행하게 합니다. 자세한 것은 \ref thread_pool_sharing 을 참고하십시오.
		- 기본값은 null입니다.
		- 이 값을 설정했을 경우 m_networkerThreadCount는 무시됩니다. m_externalNetWorkerThreadPool가 사용자가 수동으로 생성한 thread pool인

		\~english
		Running network I/O related routine in thread pool that created manually by user. Please refer to  \ref thread_pool_sharing for more detail.
		- Default value is null
		- When you set this value, m_networkerThreadCount is ignored.

		\~chinese
		在用户手动生成的thread pool里实行network I/O相关routine。详细请参考\ref thread_pool_sharing%。
		- 默认值是null。
		- 设定此值的话会忽视m_networkerThreadCount。

		\~japanese
		\~
		*/
		CThreadPool* m_externalNetWorkerThreadPool;

		/**
		\~korean
		- OnException 콜백 기능을 사용할 지 여부를 선택합니다. 
		- 기본값은 true입니다.
		- 만약 false를 지정하면 유저 콜백에서 예상치 못한 Exception 발생 시 OnException이 호출되지 않고 크래시가 발생합니다.

		\~english
		- Decide whether it uses OnException callback function or not.
		- Default value is true.
		- If setting it as false, when exception occurs during  user callback, crash will occur as OnException is not called.
		
		\~chinese
		- 选择是否使用 OnException Callback功能。
		- 基本值为 true。
		- 如果指定false，用户在Callback过程中发生意想不到的 Exception 时，OnException将不被呼出，且发生Crash。

		\~japanese
		- OnException Callback機能を使用するかどうかを選択します。
		- デフォルト値はtrueです。
		- もしfalseを指定するとユーザーCallbackから予想できないExceptionが発生した時、OnExceptionが呼び出されずにクラッシュが発生します。
		\~
		*/
		bool m_allowOnExceptionCallback;

		/**
		\~korean
		- 암호화 기능을 켜거나 끕니다.
		- 일반적으로는 암호화 기능을 켜는 것이 좋습니다. 하지만 클라이언트가 서버에 접속할 때 보안 key 교환을 위해 서버의 연산 시간을 더 많이 소요하게 됩니다.
		- 암호화 기능이 전혀 필요 없는 게임 서버라면 본 기능을 끄시는 것이 좋습니다.
		- 기본값은 true입니다.

		\~english
		- Turn on or off encryption function.
		- Generally, It had better turn on encryption function. But it takes lots of server’s operation time to exchange security key when connecting to server.
		- If the game server does not need encryption function, it had better turn it off.
		- Default value is “true”.

		\~chinese
		- 开启或关闭加密功能
		- 建议开启加密功能。但在客户端连接服务器交换安全密钥过程会消耗一些服务器演算时间。
		- 如果游戏服务器不需要任何加密可以关闭本功能。
		- 默认值为“true”。

		\~japanese
		- 暗号化機能をオンまたはオフにします。
		- 一般的に暗号化機能をオンにする方がよいです。しかし、クライアントがサーバーに接続するときにセキュリティキーの交換のためにサーバーの演算時間をもっとかかるようになります。
		- 暗号化機能が不用のゲームサーバーであれば、本機能をオフにする方がよいです。
		- デフォルトではtrueです。
		\~
		*/
		bool m_enableEncryptedMessaging; 
	protected:
		CStartServerParameterBase();

	};

	/**  @} */

#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif

}


//#pragma pack(pop)
