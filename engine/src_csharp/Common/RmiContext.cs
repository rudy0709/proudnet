/*
ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.

ProudNet

This program is soley copyrighted by Nettention.
Any use, correction, and distribution of this program are subject to the terms and conditions of the License Agreement.
Any violated use of this program is prohibited and will be cause of immediate termination according to the License Agreement.

** WARNING : PLEASE DO NOT REMOVE THE LEGAL NOTICE ABOVE.

ProudNet

此程序的版权归Nettention公司所有。
与此程序的修改、使用、发布相关的事项要遵守此程序的所有权者的协议。
不遵守协议时要原则性的禁止擅自使用。
擅自使用的责任明示在与此程序所有权者的合同书里。

** 注意：不要移除关于制作物的上述明示。

*/
using System;
using System.Collections.Generic;
using System.Text;

namespace Nettention.Proud
{
	/** \addtogroup net_group
	*  @{
	*/


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
	public class RmiContext : ICloneable
	{
		/**
		\~korean
		relay된 메시지인가?
		- RMI stub에서 채워지는 값입니다.
		- 만약 클라이언트가 보낸 RMI가 서버를 통해 릴레이(바이패스)되었거나,
		서버가 보낸 RMI가 다른 클라이언트를 통해 route되었으면
		이 값은 true가 된 채 RMI 함수가 콜백됩니다.

		\~english
		Is this a relayed message?
		- The value filled at RMI stub
		- If RMI from client is relayed(relayed) via server or RMI sent by server is routed by other client,
		then RMI function wil be called back while this value keeps to be true.

		\~chinese
		是被relay的信息吗?
		- 是在RMI stub中填充的值。
		- 如果客户端发送的RMI通过服务器进行了relay（Bypass），或服务器发送的RMI通过其他客户端进行了route，在此值为true的状态下RMI函数将被回调。

		\~japanese

		\~
		*/
		public bool relayed = false;

		/**
		\~korean
		RMI를 송신한 peer의 HostID이다.
		- RMI stub에서 채워지는 값이다.

		\~english
		HostID of peer that transmitted RMI
		- The value filled at RMI stub

		\~chinese
		发送RMI的peer的HostID。
		- 在RMI stub当中被填充的值。

		\~japanese

		\~
		*/
		public HostID sentFrom = HostID.HostID_None;

		/**
		\~korean
		직접 멀티캐스트를 할 수 있는 최대 갯수
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
		- When RMI transmitting to P2P group and the value is set as N, for N many of other peers of P2P group members, it will be trasmitted as direct P2P. (Of course when other peers communicate with direct P2P)
		However, it will be transmitted to the other peers as relayed P2P even if they are performing direct P2P communications. If 0 is set then it means there is no P2P transmission at all.
		- This function is useful when client performs a large amount of multicasts and the speed limit of client upload (e.g. ADSL line with slower upload speed).
		During broadcasting relayed P2P in Proudnet, client sends only 1 of message to be relayed then server performs the role that sends it to many other clients.
		- Default: infinite
		- User can designate this value to RMI parameter before it is called by client. At server, this value is not used.

		\~chinese
		可以直接进行Multicast的最大个数
		- 在向P2P组发送RMI时，此值被指定为N的话，对P2P组成员中相当于N个数量的其他Peer直接用P2P进行发送。（只在其他Peer进行直接P2P通信的情况下）而对于其余的Peer则用Relayed P2P进行传送。即使其余Peer也正进行直接P2P通信。如果指定为0，则表示不进行Direct P2P。
		- 此功能可以在客户端进行大量的Multicast时，客户端的Upload速度局限较大（如Upload速度慢的ADSL线路）的情况下使用。因为在ProudNet内部，进行relayed P2P的Broadcast过程中，客户端向服务器发送一个要被relay的信息，而服务器将其向多个客户端进行传送。
		- 基本值：无限。
		- 在客户端呼出的RMi参数，用户可以在之前指定此值。服务器不使用此值。

		\~japanese

		\~
		*/
		public int maxDirectP2PMulticastCount = 0;

		/**
		\~korean
		메시지의 단일화하기 위한 고유값
		- 기본값은 0이다. 0인 경우 단일화되지 않는다.
		- 송신할 메시지가 송신 큐에서 단일화하고자 한다면 이 값을 0 이외의 값을 지정하면 된다.

		\~english
		Unique value to unify messages
		- Default is 0 and does not unify if 0.
		- If the mesage to be sent is to be unified at transmission queue then this value must not designate 0 but other value.

		\~chinese
		为对信息进行单一化的固有值。
		- 基本值为0。如果是0不会被单一化。
		- 如果要发送的信息想在送信Queue中进行单一化，将此值指定为0以外的值即可。

		\~japanese

		\~
		*/
		public Int64 uniqueID = 0;

		/**
		\~korean
		메시지 송신 우선순위
		- 사용자가 지정해야 한다.
		- reliability가 reliable로 지정된 경우 이 값은 무시된다.

		\~english
		Message trnasmission priority
		- User must define.
		- If reliability = reliable then this value is ignored.

		\~chinese
		信息传送优先顺序
		- 要由用户指定。
		- reliability被指定为reliable时此值将被无视。

		\~japanese

		\~
		*/
		public MessagePriority priority = MessagePriority.MessagePriority_Medium;

		/**
		\~korean
		메시지 송신 메서드
		- 사용자가 지정해야 한다.

		\~english
		Message trnasmission method
		- User must define.

		\~chinese
		信息传送方法
		- 要由用户指定。

		\~japanese

		\~
		*/
		public MessageReliability reliability = MessageReliability.MessageReliability_Reliable;

		/**
		\~korean
		이 값이 false이면 RMI 수신자가 P2P 그룹 등 복수개인 경우 자기 자신에게 보내는 메시징(loopback)을 제외시킵니다.
		기본값은 true입니다.

		\~english
		While this value is false if there are 2 or more RMI receivers then excludes the messaging to itself(loopback).
		Default is true.

		\~chinese
		如果此值为false，且RMI收信者是P2P组等复数形式，可以将发送给自己的信息（loopback）除外。
		基本值为 true。

		\~japanese

		\~
		*/
		public bool enableLoopback = true;

		/**
		\~korean
		사용자가 지정한 tag 값입니다. \ref host_tag  기능입니다.
		- 주의! : tag는 네트웍 동기화가 되지 않는 값입니다.

		\~english
		User defined tag value. A \ref host_tag function.
		- Caution!: tag is a value that cannot be network synchronized.

		\~chinese
		是用户指定的tag值，是\ref host_tag%功能。
		- 注意！：tag是没有进行网络同步化的值。

		\~japanese

		\~
		*/
		public Object hostTag = null;

		/**
		\~korean
		내부 용도 입니다. 사용자는 사용하지 마십시오.

		\~english TODO:translate needed.

		\~chinese
		内部使用用途，用户请不要使用。

		\~japanese

		\~
		*/
		public bool enableP2PJitTrigger = true;

		/**
		\~korean
		이 값이 false이면 Unreliable로 보내려 할때, 상대가 relay mode이면, 보내지 않습니다.
		- 기본값은 true입니다.

		\~english
		While this value is false if opponent is relay mode then do not send it.
		- Default is true.

		\~chinese
		如果此值为false，用Unreliable进行传送时，如果对方是relay mode，则不进行传送。
		- 基本值是true。

		\~japanese

		\~
		*/
		public bool allowRelaySend = true;

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

		- For example, If packet sending time is 3 times slower than relay through a server, you may relay it through server even it can send to peer directly. This case set this value to 1/3.
		  Also if it is 5 times slower then set 1/5. If you set 0, it does not do forced relay.
		  Therefore it does forced relay when "Relay p2p ping / Direct p2p ping" is smaller than this value.
		- In China, server-client is faster than P2P.
		- Default is 0

		\~chinese
		强制relay临界比率值。
		如果调节此值，在relay的速度比P2P间的通信速度更快时可以选择用relay进行。

		- 如Peer之间传送数据的时间比通过服务器进行relay的时间慢3倍，那么即使可以直接向Peer进行传送也会想要通过服务器进行relay。此时可以将此值指定为1/3.
		  在慢5倍时，想要强制进行relay，则可以指定为1/5，如果指定为0则不进行指定。
		  即，"Relay p2p ping / Direct p2p ping"比此值小时，可以强制进行relay传送。
		- 在中国，与服务器的通信速度会比P2P间的通信速度更快。
		- 基本值是0.

		\~japanese

		\~
		*/
		public double forceRelayThresholdRatio = 0;

		/**
		\~korean
		EncryptMode 입니다.초기 설정 EM_None으로 되어 암호화 하지 않습니다.

		\~english TODO:translate needed.

		\~chinese
		是EncryptMode。初始设置为EM_None，不进行加密。

		\~japanese

		\~
		*/
		public EncryptMode encryptMode = EncryptMode.EM_None;

		/**
		\~korean
		메시지 압축 기능 입니다. 이 값을 CM_None 이외를 선택할 경우 압축을 하여 메시지를 전송합니다.
		- 현재 지원되지 않는 기능입니다.
		- 기본값은 CM_None 입니다.
		- 보낼 메시지의 크기가 너무 작거나(약 50바이트) 압축을 해도 크기가 작아지지 않으면 압축하지 않고 전송합니다.

		\~english TODO:translate needed.

		\~chinese
		信息压缩功能。如果选择CM_None以外的值，则会对信息先进行压缩后传送。
		- 现不支持的功能。
		- 基本值是CM_None。
		- 如果要传送的信息过小（约50Byte）或即使进行压缩也不会变小的话，则不进行压缩直接传送。

		\~japanese
		\~
		*/
		public CompressMode compressMode = CompressMode.CM_None;

		/**
		\~korean
		RmiID

		\~english
		RmiID

		\~chinese
		RmiID

		\~japanese
		RmiID

		\~
		*/
		public RmiID rmiID = RmiID.RmiID_None;

		Object ICloneable.Clone()
		{
			return this.Clone();
		}

		public RmiContext Clone()
		{
			return (RmiContext)this.MemberwiseClone();
		}

		public RmiContext()
		{

		}

		public RmiContext(MessagePriority priority, MessageReliability reliability, EncryptMode encryptMode)
		{
			this.priority = priority;
			this.reliability = reliability;
			this.encryptMode = encryptMode;

			this.maxDirectP2PMulticastCount = int.MaxValue;
		}

		public void AssureValidation()
		{
			if (this.reliability == MessageReliability.MessageReliability_Unreliable)
			{
				if (this.priority < MessagePriority.MessagePriority_High || this.priority > MessagePriority.MessagePriority_Low)
				{
					throw new Exception("RMI messaging cannot have Engine level priority!");
				}
			}
		}

		/**
		\~korean
		Reliable message로 RMI 호출시 이것을 파라메터로 넣으면 된다.
		- 유저가 원하면 별도로 RmiContext 객체를 둬도 좋지만 통상적인 경우 RMI 호출시 이것을 그냥 써도 된다.

		\~english
		This is to be entered as parameter when calling RMI as reliable message.
		- It is ok to use separate RmiContext object if needed but usually if the case is usual then it is ok to use this calling RMI.


		\~chinese
		用Reliable message呼出RMI时可以将此输入为参数。
		- 如果用户愿意，可以另存留 RmiContext%客体，但一般情况下呼出RMI时也可以直接使用这个。

		\~japanese

		\~
		*/
		public static readonly RmiContext ReliableSend = new RmiContext(MessagePriority.MessagePriority_High, MessageReliability.MessageReliability_Reliable, EncryptMode.EM_None);
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
		用Unreliable message呼出RMI时可以将此输入为参数。
		- 如果用户愿意，可以另存留 RmiContext%客体，但一般情况下呼出RMI时也可以直接使用这个。
		- 被指定为HIGH priority。

		参考事项
		- 在一般的网络游戏当中，所有通信量的很大一部分都是如角色移动，连射攻击等几种信息，其余信息占据很小比重。但在没有进行慎重考虑下使用unreliable send 时，可能会引发较长的问题解决时间。考虑到此，在网络游戏开发初期，将大部分信息用reliable send，之后通过网络通信量情报查找虽占据大部分通信量但可以流失的信息，将这些信息转换为unreliable send ，也会是一个好方法。

		\~japanese

		\~
		*/
		public static readonly RmiContext UnreliableSend = new RmiContext(MessagePriority.MessagePriority_Medium, MessageReliability.MessageReliability_Unreliable, EncryptMode.EM_None);

		/**
		\~korean
		EM_Fast모드로 암호화 하여 Reliable message로 RMI 호출시 이것을 파라메터로 넣으면 된다.
		- 기타 사항은 ReliableSend와 같습니다.

		\~english
		Encrypt with EM_Fast mode then put this as parameter when you call RMI with Reliable message.
		- All other details are same as ReliableSend.


		\~chinese
		加密为EM_Fast模式，用Reliable message呼出RMI时可以将此输入为参数。
		- 其他事项与ReliableSend相同。

		\~japanese

		\~
		*/
		public static readonly RmiContext FastEncryptedReliableSend = new RmiContext(MessagePriority.MessagePriority_High, MessageReliability.MessageReliability_Reliable, EncryptMode.EM_Fast);
		/**
		\~korean
		EM_Fast 모드로 암호화 하여 Unreliable message로 RMI 호출시 이것을 파라메터로 넣으면 된다.
		- 기타 사항은 UnreliableSend와 같습니다.

		\~english
		Encrypt with EM_Fast mode then put this as parameter when you call RMI with Reliable message.
		- All other details are same as ReliableSend.

		\~chinese
		加密为EM_Fast模式，用Unreliable message呼出RMI时可以将此输入为参数。
		- 其他事项与UnreliableSend相同。

		\~japanese

		\~
		*/
		public static readonly RmiContext FastEncryptedUnreliableSend = new RmiContext(MessagePriority.MessagePriority_Medium, MessageReliability.MessageReliability_Unreliable, EncryptMode.EM_Fast);

		/**
		\~korean
		EM_Secure 모드로 암호화 하여 Reliable message로 RMI 호출시 이것을 파라메터로 넣으면 된다.
		- 기타 사항은 ReliableSend와 같습니다.

		\~english
		Encrypt with EM_Secure mode then put this as parameter when you call RMI with Reliable message.
		- All other details are same as ReliableSend.

		\~chinese
		加密为EM_Secure模式，用Reliable message呼出RMI时可以将此输入为参数。
		- 其他事项与ReliableSend相同。

		\~japanese

		\~
		*/
		public static readonly RmiContext SecureReliableSend = new RmiContext(MessagePriority.MessagePriority_High, MessageReliability.MessageReliability_Reliable, EncryptMode.EM_Secure);
		/**
		\~korean
		EM_Secure 모드로 암호화 하여 Unreliable message로 RMI 호출시 이것을 파라메터로 넣으면 된다.
		- 기타 사항은 UnreliableSend와 같습니다.

		\~english
		Encrypt with EM_Secure mode then put this as parameter when you call RMI with Reliable message.
		- All other details are same as ReliableSend.

		\~chinese
		加密为EM_Secure模式，用Unreliable message呼出RMI时可以将此输入为参数。
		- 其他事项与UnreliableSend相同。

		\~japanese

		\~
		*/
		public static readonly RmiContext SecureUnreliableSend = new RmiContext(MessagePriority.MessagePriority_Medium, MessageReliability.MessageReliability_Unreliable, EncryptMode.EM_Secure);
	}

	/**	@} */
}
