/*
ProudNet v1


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

#include "Enums.h"
#include "EncryptEnum.h"
#include "BasicTypes.h"
#include "CompressEnum.h"

//#pragma pack(push,8)

namespace Proud
{
	/** \addtogroup net_group
	*  @{
	*/

	class CReceivedMessage;
	class CMessage;

	/**
	\~korean
	RMI 호출에 관련된 네트워킹 속성 등

	\~english
	Networkign nature related to RMI calling and others

	\~chinese
	关于RMI呼出的网络属性等。

	\~japanese
	\~
	*/
	// 주의: 수정시 ProudClr의 동명 심볼도 수정해야 한다.
	class RmiContext
	{
	public:
		/**
		\~korean
		relay된 메시지인가?
		- RMI stub에서 채워지는 값입니다.
		- 만약 클라이언트가 보낸 RMI가 서버를 통해 릴레이(바이패스)되었거나( \ref p2p_comm_overview  참고),
		서버가 보낸 RMI가 다른 클라이언트를 통해 route되었으면 ( \ref s2c_routed_multicast  참고)
		이 값은 true가 된 채 RMI 함수가 콜백됩니다.

		\~english
		Is this a relaysed message?
		- The value filled at RMI stub
		- If RMI from client is relayed via server(Please refer \ref p2p_comm_overview) or RMI sent by server is routed by other client(Please refer \ref s2c_routed_multicast),
  		then RMI function wil be called back while this value keeps to be true.

		\~chinese
		Relay 的信息吗？
		- 在RMI stub填充的值。
		- 如果client发送的RMI通过服务器relay（bypass）了（参考\ref p2p_comm_overview%）或者服务器发送的RMI通过其他client被route的话（参考\ref s2c_routed_multicast%），此值会在true状态下回拨RMI函数。

		\~japanese
		\~
		*/
		bool m_relayed;

		/**
		\~korean
		RMI를 송신한 peer의 HostID 이다.
		- RMI stub에서 채워지는 값이다.

		\~english
		HostID of peer that transmitted RMI
		- The value filled at RMI stub

		\~chinese
		传送RMI的peer的Host ID。
		- 在RMI stub填充的值。

		\~japanese
		\~
		*/
		HostID m_sentFrom;

		/**
		\~korean
		unreliable send인 경우,
		routed multicast를 할 수 있다. 이때 몇개까지 허용하는지를 여기서 지정한다.
		- 사용자가 지정해야 한다.

		\~english
		If unreliable send, can perform routed multicast. This is where to decide how many of them are allowed.
		- User must define.

		\~chinese
		unreliable send 的情况下可以进行routed multicast。这时在这里指定能允许至几个。
		- 要用户指定。

		\~japanese
		\~
		*/
		int m_unreliableS2CRoutedMulticastMaxCount;

		/**
		\~korean
		unreliable 메시징인 경우에,
		routed multicast를 할 수 있다. 이때 얼마 이상의 랙 이하의 피어간 통신에서만 허용하는지를 여기서 지정한다.
		- 사용자가 지정해야 한다.

		\~english
		If unreliable messaging, can perform routed multicast. This is where to decide which rate of laggy P2P communications are allowed.
		- User must define.

		\~chinese
		如果是Unreliable messaging的情况，可以进行routed multicast。这时在这里指定只允许多少以上的lag以下的peer之间通信。
		- 要用户指定。

		\~japanese
		\~
		*/
		int m_unreliableS2CRoutedMulticastMaxPingMs;

		/**
		\~korean
		직접 멀티캐스트를 할 수 있는 최대 갯수
		- \ref throttling  기법 중 하나이다.
		- P2P 그룹을 대상으로 RMI 송신을 할 경우, 이 값이 N으로 지정된 경우, P2P 그룹 멤버중 N개 만큼의 타 peer에게는
		P2P로 직접 전송한다. (물론 타 peer가 직접 P2P 통신을 하고 있는 경우에) 하지만 나머지 peer들에게는
		Relayed P2P로 전송한다. 설령 직접 P2P 통신을 하고 있더라도 말이다. 0을 지정하면 direct P2P 송신 자체를 안함을 의미한다.
		- 이 기능은 클라이언트가 대량의 멀티캐스트를 하는 경우, 그리고 클라이언트의 업로드 속도의 한계가 큰 경우(예를 들어
		업로드 속도가 느린 ADSL 회선) 유용하다. 왜냐하면 ProudNet에서는 relayed P2P의 브로드캐스트 과정에서 클라이언트는
		1개의 relay될 메시지만을 서버에게 보내고, 서버는 그것을 여러 클라이언트들에게 보내주는 역할을 하기 때문이다.
		- 기본값: 무제한.
		- 클라이언트에서 호출하는 RMI의 파라메터에 전에 이 값을 사용자가 지정할 수 있다. 서버에서는 이 값이 쓰이지 않는다.

		\~english
		The maximum number of direct multicast can be performed
		- One of \ref throttling technique
		- When RMI transmitting to P2P group and the value is set as N, for N many of other peers of P2P group members, it will be trasmitted as direct P2P. (Of course when other peers communicate with direct P2P)
		  However, it will be transmitted to the other peers as relayed P2P even if they are performing direct P2P communications. If 0 is set then it means there is no P2P transmission at all.
		- This function is useful when client performs a large amount of multicasts and the speed limit of client upload (e.g. ADSL line with slower upload speed).
                  During broadcasting relayed P2P in Proudnet, client sends only 1 of message to be relayed then server performs the role that sends it to many other clients.
		- Default: infinite
		- User can designate this value to RMI parameter before it is called by client. At server, this value is not used.

		\~chinese
		能直接进行multi cast的最大个数。
		- \ref throttling%技法中之一。
		- 以P2P组为对象发送RMI的时候，把此值指定为N时，给P2P组成员中相当于N个的其他peer用P2P直接传送（当然，是在其他peer直接进行P2P通信的情况下）。但是给剩下的peer，用relayed P2P传送。即使直接进行P2P通信也是。指定0的话意味着不进行direct P2P本身。
		- 此功能在client进行大量multi cast的时候，还有client的上传速度的上限大的时候（例如上传速度慢的ADSL回路）会有效果。因为在ProudNet进行relayed P2P的broadcast过程中client只把一个relay的信息发送给服务器，服务器则把此内容发送给许多client的作用。
		- 默认值：无限制。
		- 在client呼叫的RMI参数之前用户可以指定此值。在服务器不用此值。

		\~japanese
		\~
		*/
		int m_maxDirectP2PMulticastCount; // NOTE: 서버간 통신의 경우 등 때문에 사용자가 임의 지정하지 않는한 무제한이다.

		/**
		\~korean
		\ref message_unique_id 기능을 위한 고유값입니다.
		- 기본값은 0입니다. 0인 경우 단일화되지 않습니다.
		- 송신할 메시지를 송신 큐에서 단일화하고자 한다면 이 값을 0 이외의 값을 지정하십시오.

		\~english
		Unique ID for \ref message_unique_id.
		- Default is 0 and does not unify if 0.
		- If the mesage to be sent is to be unified at transmission queue then this value must not designate 0 but other value.

		\~chinese
		是为了\ref message_unique_id 技能的固有值。
		- 基本值为0.基本值为0时,不被单一化。
		- 如果想把送信信息在送信queue中单一化，请将基本值设置为0以外的数字。

		\~japanese
		\ref message_unique_id機能のための固有値です。
		- デフォルトは０です。０の場合、単一化されません。
		- 送信するメッセージを送信キューで単一化する際には、この値を０以外の値で指定してください。
		\~
		*/
		int64_t m_uniqueID;

		/**
		\~korean
		메시지 송신 우선순위
		- \ref message_priority  참고.
		- 사용자가 지정해야 한다.
		- m_reliability가 reliable로 지정된 경우 이 값은 무시된다.

		\~english
		Message trnasmission priority
		- Please refer \ref message_priority.
		- User must define.
		- If m_reliability = reliable then this value is ignored.

		\~chinese
		信息传送的优先顺序。
		- 参考\ref message_priority%。
		- 要用户指定。
		- m_reliability 被指定为reliable的话，此值会忽视。

		\~japanese
		\~
		*/
		MessagePriority m_priority;

		/**
		\~korean
		메시지 송신 메서드
		- 사용자가 지정해야 한다.

		\~english
		Message trnasmission method
		- User must define.

		\~chinese
		信息传送方法。
		- 要用户指定。

		\~japanese
		\~
		*/
		MessageReliability m_reliability;

		/**
		\~korean
		이 값이 false이면 RMI 수신자가 P2P 그룹 등 복수개인 경우 자기 자신에게 보내는 메시징(loopback)을 제외시킵니다.
		기본값은 true입니다.

		\~english
		While this value is false if there are 2 or more RMI receivers then excludes the messaging to itself(loopback).
		Default is true.

		\~chinese
		此值是false的话RMI收信者P2P组等复数个的时候把发给自己的messaging（loopback）除外。
		默认值是true。

		\~japanese
		\~
		*/
		bool m_enableLoopback;

		/**
		\~korean
		사용자가 지정한 tag 값입니다. \ref host_tag  기능입니다.
		- 주의! : tag는 네트웍 동기화가 되지 않는 값입니다.

		\~english
		User defined tag value. A \ref host_tag function.
		- Caution!: tag is a value that cannot be network synchronized.

		\~chinese
		用户指定的tag值。是\ref host_tag%功能 。
		- 注意！：tag 是无法进行网络同步的值。

		\~japanese
		\~
		*/
		void* m_hostTag;

		// 내부 용도
		// true인 경우 이 RMI송신은 JIT P2P 홀펀칭을 트리거한다. Non-PN RMI에서만 false이다.
		bool m_enableP2PJitTrigger;

		/**
		\~korean
		이 값이 false이면 Unreliable로 보내려 할때, 상대가 relay mode이면, 보내지 않습니다.
		- 기본값은 true입니다.

		\~english
		While this value is false if opponent is relay mode then do not send it.
		- Default is true.

		\~chinese
		此值是false的话往Unreliable发送的时候，对方是relay mode的话不会发送。
		- 默认值是true。

		\~japanese
		\~
		*/
		bool m_allowRelaySend;

        /**
		\~korean
		강제 릴레이 임계비율 값입니다.
		이 값을 조절하면, P2P간 통신 속도보다 릴레이가 더 통신 속도가 빠른 경우 릴레이를 선택할 수 있습니다.

		- 예를 들어 피어간 패킷 전송 시간이 서버를 통해 릴레이하는 시간보다 3배 느린 경우에는 직접 피어에게 전송할
		수 있다 하더라도 서버를 통해 릴레이하고 싶을 수 있습니다. 그러한 경우 이 값을 1/3으로 지정하면 됩니다.
		5배 느린 경우에 한해 강제 릴레이를 원할 경우 1/5를 지정하면 됩니다. 0을 지정하면 강제 릴레이를 하지 않습니다.
		즉, "Relay p2p ping / Direct p2p ping"이 이 값보다 작은 경우에는 강제로 릴레이로 전송합니다.
		- 중국에서는 P2P간 통신 속도보다 서버와 통신하는 속도가 훨씬 원활한 환경이 있다고 알려져 있습니다.
		- 기본값은 0입니다.

		\~english
		Forced relay critical rate value.
		If you change this value, it can select relay instead of P2P communication when relay is faster than P2p communication.

		-For example, If packet sending time is 3 times slower than relay through a server, you may relay it through server even it can send to peer directly. This case set this value to 1/3.
		Also if it is 5 times slower then set 1/5. If you set 0, it does not do forced relay.
		Therefore it does forced relay when "Relay p2p ping / Direct p2p ping" is smaller than this value.
		- In China, server-client is faster than P2P.
		- Default is 0

		\~chinese
		强制relay临界比率值。
		调整此值的话，当relay通信速度比P2P通信速度更块的时候可以选择relay。

		- 例如，peer 之间数据包传送时间比通过服务器relay的时间慢3倍的时候，即使能够直接往peer传送，也可能想通过服务器relay。这时把此值指定为1/3即可。限为慢5倍的情况，要强制relay的时候指定1/5即可。指定成0的话不会强制relay。即，"Relay p2p ping / Direct p2p ping"比此值小的话，传送为强制relay。
		- 据说在中国与服务器通信的速度比P2P之间通信速度块的多。
		- 默认值是0。

		\~japanese
		\~
		*/
        double m_forceRelayThresholdRatio;

        /**
		\~korean
		ProudNet 전용 메시지인지에 대한 식별값입니다.
        - 기본 false입니다. false이면 udp 메시지 갯수를 측정합니다.
        - 특별한 상황이 아니면 이값을 true로 바꾸지 마십시오.

		\~english
		Identifying value that message for only ProudNet or not
		- Default is false. If it is false then check number of udp messages.
		- Do not change it to true except special case.

		\~chinese
		对ProudNet专用信息的识别值。
		- 默认值是false。False 的话测定udp信息个数。
		- 不是特殊情况，不要把此值修改成true。

		\~japanese
		\~
		*/
        bool m_INTERNAL_USE_isProudNetSpecificRmi;

        /**
		\~korean
		\ref encryption_usage 에서 사용되는 암호화 알고리즘 선택입니다.
		- 기본값은 EM_None 입니다.

		\~english TODO:translate needed.

		\~chinese
		选择在\ref encryption_usage%使用的加密散发。
		- 默认值是EM_None。

		\~japanese
		\~
		*/
        EncryptMode m_encryptMode;

		/**
		\~korean
        메시지 압축 기능 입니다. 이 값을 CM_None 이외를 선택할 경우 압축을 하여 메시지를 전송합니다.
        - 기본값은 CM_None 입니다.
        - 보낼 메시지의 크기가 너무 작거나(약 50바이트) 압축을 해도 크기가 작아지지 않으면 압축하지 않고 전송합니다.

		\~english TODO:translate needed.

		\~chinese
		信息压缩功能。把此值选择为CM_None以外的时候，压缩以后传送信息。
		- 默认值是CM_None。
		- 要发送信息的太小（约50byte，就不压缩直接传送。

		\~japanese
		\~
		*/
        CompressMode m_compressMode;

		/**
		\~korean
		m_rmiID는 ProudNet 내부에서 처리되는 패킷이 어떤 RMI인지 구분하기 위해 사용됩니다.

		\~english
		m_rmiID is used to identify which RMI packets are being processed within ProudNet.
		RmiID

		\~chinese
		m_rmiID分组由内部ProudNet处理使用什么RMI之间进行区分。

		\~japanese
		m_rmiIDは、ProudNet内部で処理されるパケットがどのようなRMIのか区別するために使用されます。

		\~
		*/
		RmiID m_rmiID;

		// 주의: 수정시 ProudClr의 동명 심볼도 수정해야 한다.
		PROUD_API RmiContext();

		// 주의: 수정시 ProudClr의 동명 심볼도 수정해야 한다.
		PROUD_API RmiContext( MessagePriority priority, MessageReliability reliability, int unreliableS2CRoutedMulticastMaxCount, EncryptMode encryptMode = EM_None);

		/**
		\~korean
		Reliable message로 RMI 호출시 이것을 파라메터로 넣으면 된다.
		- 유저가 원하면 별도로 RmiContext 객체를 둬도 좋지만 통상적인 경우 RMI 호출시 이것을 그냥 써도 된다.

		\~english
		This is to be entered as parameter when calling RMI as reliable message.
		- It is ok to use separate RmiContext object if needed but usually if the case is usual then it is ok to use this calling RMI.

		\~chinese
		用Reliable message呼叫RMI时，把这个输入为参数即可。
		- 用户希望的话，另留 RmiContext%对象也好，一般情况下呼叫RMI时也可以直接使用这个。

		\~japanese
		\~
		*/
		PROUD_API static RmiContext ReliableSend;

		/**
		\~korean
		EM_Fast 모드로 암호화 하여 Reliable message 로 RMI 호출시 이것을 파라메터로 넣으면 된다.
		- 기타 사항은 ReliableSend 와 같습니다.

		\~english
		Encrypt with EM_Fast mode then put this as parameter when you call RMI with Reliable message.
		- All other details are same as ReliableSend.

		\~chinese
		以EM_Fast模式加密以后用Reliable message呼叫RMI时，把这个输入为参数即可。
		- 其他项与ReliableSend相同。

		\~japanese
		\~
		*/
		PROUD_API static RmiContext FastEncryptedReliableSend;

		/**
		\~korean
		EM_Secure 모드로 암호화 하여 Reliable message로 RMI 호출시 이것을 파라메터로 넣으면 된다.
		- 기타 사항은 ReliableSend 와 같습니다.

		\~english
		Encrypt with EM_Secure mode then put this as parameter when you call RMI with Reliable message.
		- All other details are same as ReliableSend.

		\~chinese
		以EM_Secure模式加密以后用Reliable message呼叫RMI时，把这个输入为参数即可。
		- 其他项与ReliableSend相同。

		\~japanese
		\~
		*/
		PROUD_API static RmiContext SecureReliableSend;





		/**
		\~korean
		Unreliable message로 RMI 호출시 이것을 파라메터로 넣으면 된다.
		- 유저가 원하면 별도로 RmiContext 객체를 둬도 좋지만 통상적인 경우 RMI 호출시 이것을 그냥 써도 된다.
		- HIGH priority로 지정되어 있다.

		참고 사항
		- 일반적인 온라인 게임에서 전체 통신량의 대부분은 캐릭터 이동, 연사 공격과 같은 몇 종류의 메시지만이 차지하며
		그 외의 수많은 메시지들은 낮은 비중을 차지한다. 그리고 충분히 검토하지 않고 unreliable send를 쓸 경우 종종
		장시간의 문제 해결 시간으로 이어지곤 한다. 이러한 경험을 고려했을때 온라인 게임 개발 초기 과정에서는 웬만한 메시지는
		모두 reliable send를 쓰게 만들다가 네트웍 통신량 프로필링 등을 통해 대부분의 통신량을 차지하지만 누실이 감당되는 메시지들만
		찾아서 unreliable send로 바꿔주는 것도 좋은 개발 방법이라 말할 수 있다.

		\~english
		This is to be entered as parameter when calling RMI as reliable message.
		- It is ok to use separate RmiContext object if needed but usually if the case is usual then it is ok to use this calling RMI.
		- Set as HIGH priority.

		Reference
		- Generally in most of online games, most of overall communications consist of a few different types of mesages such as character movement, attacking actions and so on while others have significantly low proportion.
		And there have been many cases where unreliable send was used without enough considerations then caused to spend hours of debugging time.
		Considering those experiences, it is believed to say that making everything use reliable send at the beginning then replace only those that can handle losses to unreliable can be a good way to develop an online game.

		\~chinese
		用Unreliable message呼叫RMI时，把这个输入为参数即可。
		- 用户希望的话，另留 RmiContext%对象也好，一般情况下呼叫RMI时也可以直接使用这个。
		- 被指定为HIGH priority。

		参考事象
		- 在一般的网络游戏中，全部通信量的大部分是由角色移动，连射攻击等几种信息所占有，之外的许多信息占据比重较低。而且不进行充分的检讨后使用unreliable send的时候，会有种种需要长时间解决的问题的发生。考虑到这种经验，在网络游戏开发初期将一般的信息设置成用reliable send，虽然这样会通过网络通信量profiling等占据大部分通信量，但是可以查找能担当流失的信息，并将其修改成unreliable send也可以说是很好的开发方法。

		\~japanese
		\~
		*/
		PROUD_API static RmiContext UnreliableSend;

		/**
		\~korean
		EM_Fast 모드로 암호화 하여 Unreliable message로 RMI 호출시 이것을 파라메터로 넣으면 된다.
		- 기타 사항은 UnreliableSend 와 같습니다.

		\~english
		Encrypt with EM_Fast mode then put this as parameter when you call RMI with Reliable message.
		- All other details are same as ReliableSend.

		\~chinese
		以EM_Fast模式加密以后用Unreliable message呼叫RMI时，把这个输入为参数即可。
		- 其他项与UnreliableSend相同。

		\~japanese
		\~
		*/
		PROUD_API static RmiContext FastEncryptedUnreliableSend;


		/**
		\~korean
		EM_Secure 모드로 암호화 하여 Unreliable message로 RMI 호출시 이것을 파라메터로 넣으면 된다.
		- 기타 사항은 UnreliableSend와 같습니다.

		\~english
		Encrypt with EM_Secure mode then put this as parameter when you call RMI with Reliable message.
		- All other details are same as ReliableSend.

		\~chinese
		以EM_Secure模式加密以后用Unreliable message呼叫RMI时，把这个输入为参数即可。
		- 其他项与UnreliableSend相同。

		\~japanese
		\~
		*/
		PROUD_API static RmiContext SecureUnreliableSend;





		/**
		\~korean
		Unreliable server to client multicast message로 RMI 호출시 이것을 파라메터로 넣으면 된다.
		- 유저가 원하면 별도로 RmiContext 객체를 둬도 좋지만 통상적인 경우 RMI 호출시 이것을 그냥 써도 된다.
		- Medium priority로 지정되어 있다.
		- 이 기능은, 서버에서 여러개의 클라이언트로 unreliable RMI 브로드캐스트 호출을 할 때 수신자 중 P2P 직접 연결이 되어 있는 것들끼리는
		P2P를 통한 routed send를 하게 한다. 이 기능은 서버에서의 CPU 사용량을 약간 증가시키는 댓가로 서버에서 클라이언트로의 송신
		통신량을 절감한다. (대부분의 게임에서 유용하게 쓰일 수 있다.)

		\~english
		This is unreliable server to client multicast message and this is to be entered as parameter when calling RMI.
		- It is ok to use separate RmiContext object if needed but usually if the case is usual then it is ok to use this calling RMI.
		- Set as HIGH priority.
		- This function performs routed send via P2P for those direct P2P connected among receivers when server calls unreliable RMI broadcast to multiple clients.
		This function saves the amount of transmission from server to client in exchange of increase of CPU use in server. (Can be very useful in most of games.)

		\~chinese
		用Unreliable server to client multicast message呼叫RMI时，把这个输入为参数即可。
		- 用户希望的话，另留 RmiContext%对象也可以，一般情况下呼叫RMI时也可以直接使用这个。
		- 指定为Medium priority。
		- 此功能是，在服务器用几个client进行unreliable RMI broadcast呼叫的时候，P2P 直接连接的收信者之间会进行通过P2P的routed send。此功能是以稍微增加CPU使用量为代价，减少从服务器往client的传送通信量。

		\~japanese
		\~
		*/
		PROUD_API static RmiContext UnreliableS2CRoutedMulticast;

		PROUD_API void AssureValidation() const;
	};

	/**  @} */
}


//#pragma pack(pop)
