/*
ProudNet v1.x.x


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

//#pragma pack(push,8)

namespace Proud
{
	/** \addtogroup net_group
	*  @{
	*/

	// 주의: 이것도 바꾸면 ProudClr namespace의 동명 심볼도 수정해야 한다.


	/**
	\~korean
	메시지 송신 우선순위
	- \ref message_priority 참고.

	\~english TODO:translate needed.

	\~chinese
	信息传送优先顺序。
	- 参考\ref message_priority%。

	\~japanese
	\~
	*/
	enum MessagePriority
	{
		/**
		\~korean
		매우 높은 우선순위. connectivity test를 위한 ping 등에 사용됨. PN 전용

		\~english TODO:translate needed.

		\~chinese
		非常高的优先顺序。使用在为了connectivity test的ping等。PN 专用。

		\~japanese
		\~
		*/
		MessagePriority_Ring0 = 0,

		/**
		\~korean
		PN 전용

		\~english TODO:translate needed.

		\~chinese
		PN 专用。

		\~japanese
		\~
		*/
		MessagePriority_Ring1,

		/**
		\~korean
		높은 우선순위

		\~english TODO:translate needed.

		\~chinese
		高的优先顺序。

		\~japanese
		\~
		*/
		MessagePriority_High,

		/**
		\~korean
		보통 우선순위. 가장 많이 사용됨.

		\~english TODO:translate needed.

		\~chinese
		一般优先顺序。用的最多。

		\~japanese
		\~
		*/
		MessagePriority_Medium,

		/**
		\~korean
		낮은 우선순위

		\~english TODO:translate needed.

		\~chinese
		低的优先顺序。

		\~japanese
		\~
		*/
		MessagePriority_Low,

		/**
		\~korean
		매우 낮은 우선순위
		PN 전용

		\~english TODO:translate needed.

		\~chinese
		非常低的优先顺序。
		PN 专用。

		\~japanese
		\~
		*/
		MessagePriority_Ring99,

		/**
		\~korean
		사용자는 사용할 수 없는 값

		\~english TODO:translate needed.

		\~chinese
		用户不能使用的值。

		\~japanese
		\~
		*/
		MessagePriority_LAST,
	};

	// 홀펀칭 패킷은 많을 수 있다. 이것이 트래픽을 방해하면 안되므로 우선순위를 최하위로 둔다.
	static const MessagePriority MessagePriority_Holepunch = MessagePriority_Ring99;

	// 주의: 이것도 바꾸면 ProudClr namespace의 동명 심볼도 수정해야 한다.

	enum MessageReliability
	{
		MessageReliability_Unreliable = 0,
		MessageReliability_Reliable,
		MessageReliability_LAST,
	};

	// 주의: 이것도 바꾸면 ProudClr namespace의 동명 심볼도 수정해야 한다.

	/**
	\~korean
	에러 타입 코드

	\~english TODO:translate needed.

	\~chinese
	错误类型代码。

	\~japanese
	\~
	*/
	enum ErrorType
	{
		/**
		\~korean
		성공. 문제 없음.

		\~english TODO:translate needed.

		\~chinese
		成功。没有问题。

		\~japanese
		\~
		*/
		ErrorType_Ok = 0,

		/**
		\~korean
		예외 상황 발생

		\~english TODO:translate needed.

		\~chinese
		发生例外情况。

		\~japanese
		\~
		*/
		ErrorType_Unexpected,

		/**
		\~korean
		이미 연결되어 있음

		\~english TODO:translate needed.

		\~chinese
		已经是连接状态
		\~japanese
		\~
		*/
		ErrorType_AlreadyConnected,

		/**
		\~korean
		TCP 연결 실패

		\~english TODO:translate needed.

		\~chinese
		TCP 连接失败。

		\~japanese
		\~
		*/
		ErrorType_TCPConnectFailure,

		/**
		\~korean
		잘못된 대칭키

		\~english TODO:translate needed.

		\~chinese
		错误的对称key。

		\~japanese
		\~
		*/
		ErrorType_InvalidSessionKey,

		/**
		\~korean
		암호화가 실패했음

		\~english TODO:translate needed.

		\~chinese
		加密失败。

		\~japanese
		\~
		*/
		ErrorType_EncryptFail,

		/**
		\~korean
		해커가 깨진 데이터를 전송했거나 복호화가 실패했음

		\~english TODO:translate needed.

		\~chinese
		黑客可能传送了乱码数据，或者是解码失败。

		\~japanese
		\~
		*/
		ErrorType_DecryptFail,

		/**
		\~korean
		서버 연결 과정이 너무 오래 걸려서 실패 처리됨

		\~english TODO:translate needed.

		\~chinese
		因服务器连接时间过长而失败。

		\~japanese
		\~
		*/
		ErrorType_ConnectServerTimeout,

		/**
		\~korean
		서버 연결을 위한 프로토콜 버전이 다름

		\~english TODO:translate needed.

		\~chinese
		服务器连接的网络协议版本不同。

		\~japanese
		\~
		*/
		ErrorType_ProtocolVersionMismatch,

		/**
		\~korean
		서버가 연결을 의도적으로 거부했음

		\~english TODO:translate needed.

		\~chinese
		服务器故意拒绝连接。

		\~japanese
		\~
		*/
		ErrorType_NotifyServerDeniedConnection,

		/**
		\~korean
		서버 연결 성공!

		\~english TODO:translate needed.

		\~chinese
		服务器连接成功！

		\~japanese
		\~
		*/
		ErrorType_ConnectServerSuccessful,

		/**
		\~korean
		상대측 호스트에서 연결을 해제했음

		\~english TODO:translate needed.

		\~chinese
		对方主机解除了连接。

		\~japanese
		\~
		*/
		ErrorType_DisconnectFromRemote,

		/**
		\~korean
		이쪽 호스트에서 연결을 해제했음

		\~english TODO:translate needed.

		\~chinese
		这边主机解除了连接。

		\~japanese
		\~
		*/
		ErrorType_DisconnectFromLocal,

		/**
		\~korean
		위험한 상황을 불러올 수 있는 인자가 있음

		\~english TODO:translate needed.

		\~chinese
		存在可能引起危险状况的因素。

		\~japanese
		\~
		*/
		ErrorType_DangerousArgumentWarning,

		/**
		\~korean
		알 수 없는 인터넷 주소

		\~english TODO:translate needed.

		\~chinese
		未知的互联网地址。

		\~japanese
		\~
		*/
		ErrorType_UnknownAddrPort,

		/**
		\~korean
		서버 준비 부족

		\~english TODO:translate needed.

		\~chinese
		服务器准备不足。

		\~japanese
		\~
		*/
		ErrorType_ServerNotReady,

		/**
		\~korean
		서버 소켓의 listen을 시작할 수 없습니다. TCP 또는 UDP 소켓이 이미 사용중인 포트인지 확인하십시오.

		\~english TODO:translate needed.

		\~chinese
		无法开始服务器socket的listen。请确认TCP或者UDPsocket是否已是使用中的端口。

		\~japanese
		\~
		*/
		ErrorType_ServerPortListenFailure,

		/**
		\~korean
		이미 개체가 존재합니다.

		\~english TODO:translate needed.

		\~chinese
		对象已存在。

		\~japanese
		\~
		*/
		ErrorType_AlreadyExists,

		/**
		\~korean
		접근이 거부되었습니다.
		- \ref dbc2_nonexclusive_overview 기능에서, DB cache server가 비독점적 접근 기능을 쓰지 않겠다는 설정이 되어 있으면 발생할 수 있습니다.
		- 윈도우 비스타이상의 OS에서 관리자권한이 없이 CNetUtil::EnableTcpOffload( bool enable ) 함수가 호출되면 발생할 수 있습니다.

		\~english
		Access denied.
		- At \ref dbc2_nonexclusive_overview function, it could occur if a function that DB cache server does not use exclusive access function has been set.
		- At OS over window vista, It could occur when CNetUtil::EnableTcpOffload( bool enable ) function is called without administration authority.

		\~chinese
		接近被拒绝。
		- 在\ref dbc2_nonexclusive_overview%功能上，如果DB cache server被设置成不使用共享接近功能的话便会发生此状况。
		- 在window，vista 以上的OS中，没有管理员权限呼出 CNetUtil::EnableTcpOffload ( bool enable )函数的话，可能会发生此状况。

		\~japanese
		接近が拒否されました。
		- \ref dbc2_nonexclusive_overview 機能で, DB cache serverが非独占的接近機能を使用しないと設定されていると発生する可能性があります。
		- Window Vista以上のOSで管理者権限なしに CNetUtil::EnableTcpOffload( bool enable ) 関数が呼び出されると発生する可能性があります。

		\~
		*/
		ErrorType_PermissionDenied,

		/**
		\~korean
		잘못된 session Guid입니다.

		\~english TODO:translate needed.

		\~chinese
		错误的session Guid。

		\~japanese
		\~
		*/
		ErrorType_BadSessionGuid,

		/**
		\~korean
		잘못된 credential입니다.

		\~english TODO:translate needed.

		\~chinese
		错误的credential。

		\~japanese
		\~
		*/
		ErrorType_InvalidCredential,

		/**
		\~korean
		잘못된 hero name입니다.

		\~english TODO:translate needed.

		\~chinese
		错误的hero name。

		\~japanese
		\~
		*/
		ErrorType_InvalidHeroName,

		/**
		\~korean
		로딩 과정이 unlock 후 lock 한 후 꼬임이 발생했다

		\~english TODO:translate needed.

		\~chinese
		加载过程unlock以后，lock 后发生云集。

		\~japanese
		\~
		*/
		ErrorType_LoadDataPreceded,

		/**
		\~korean
		출력 파라메터 AdjustedGamerIDNotFilled가 채워지지 않았습니다.

		\~english TODO:translate needed.

		\~chinese
		输出参数AdjustedGamerIDNotFilled没有被填充。

		\~japanese
		\~
		*/
		ErrorType_AdjustedGamerIDNotFilled,

		/**
		\~korean
		플레이어 캐릭터가 존재하지 않습니다.

		\~english TODO:translate needed.

		\~chinese
		玩家角色不存在。

		\~japanese
		\~
		*/
		ErrorType_NoHero,

		/**
		\~korean
		유닛 테스트 실패

		\~english
		Unit test failed

		\~chinese
		单元测试失败。

		\~japanese
		\~
		*/
		ErrorType_UnitTestFailed,

		/**
		\~korean
		peer-to-peer UDP 통신이 막혔습니다.

		\~english TODO:translate needed.

		\~chinese
		peer-to-peer UDP 通信堵塞。

		\~japanese
		\~
		*/
		ErrorType_P2PUdpFailed,

		/**
		\~korean
		P2P reliable UDP가 실패했습니다.

		\~english TODO:translate needed.

		\~chinese
		P2P reliable UDP失败。

		\~japanese
		\~
		*/
		ErrorType_ReliableUdpFailed,

		/**
		\~korean
		클라이언트-서버 UDP 통신이 막혔습니다.

		\~english TODO:translate needed.

		\~chinese
		玩家-服务器的UDP通信堵塞。

		\~japanese
		\~
		*/
		ErrorType_ServerUdpFailed,

		/**
		\~korean
		더 이상 같이 소속된 P2P 그룹이 없습니다.

		\~english TODO:translate needed.

		\~chinese
		再没有一同所属的P2P组。

		\~japanese
		\~
		*/
		ErrorType_NoP2PGroupRelation,

		/**
		\~korean
		사용자 정의 함수(RMI 수신 루틴 혹은 이벤트 핸들러)에서 exception이 throw되었습니다.

		\~english TODO:translate needed.

		\~chinese
		从用户定义函数（RMI 收信例程或者事件handler）exception 被throw。

		\~japanese
		\~
		*/
		ErrorType_ExceptionFromUserFunction,

		/**
		\~korean
		사용자의 요청에 의한 실패입니다.

		\~english TODO:translate needed.

		\~chinese
		用户邀请造成的失败。

		\~japanese
		\~
		*/
		ErrorType_UserRequested,

		/**
		\~korean
		잘못된 패킷 형식입니다. 상대측 호스트가 해킹되었거나 버그일 수 있습니다.

		\~english TODO:translate needed.

		\~chinese
		错误的数据包形式。可能是对方主机被黑或者是存在漏洞。

		\~japanese
		\~
		*/
		ErrorType_InvalidPacketFormat,

		/**
		\~korean
		너무 큰 크기의 메시징이 시도되었습니다. 기술지원부에 문의하십시오. 필요시 \ref message_length  를 참고하십시오.

		\~english TODO:translate needed.

		\~chinese
		试图使用了过大的messaging。请咨询技术支援部。必要时请参考\ref message_length%。

		\~japanese
		\~
		*/
		ErrorType_TooLargeMessageDetected,

		/**
		\~korean
		Unreliable 메세지는 encrypt 할수 없습니다.

		\~english TODO:translate needed.

		\~chinese
		Unreliable 信息不能encrypt。

		\~japanese
		\~
		*/
		ErrorType_CannotEncryptUnreliableMessage,

		/**
		\~korean
		존재하지 않는 값입니다.

		\~english TODO:translate needed.

		\~chinese
		不存在的值。

		\~japanese
		\~
		*/
		ErrorType_ValueNotExist,

		/**
		\~korean
		타임아웃 입니다.

		\~english TODO:translate needed.

		\~chinese
		超时。

		\~japanese
		\~
		*/
		ErrorType_TimeOut,

		/**
		\~korean
		로드된 데이터를 찾을수 없습니다.

		\~english TODO:translate needed.

		\~chinese
		无法找到加载的数据。

		\~japanese
		\~
		*/
		ErrorType_LoadedDataNotFound,

		/**
		\~korean
		\ref send_queue_heavy . 송신 queue가 지나치게 커졌습니다.

		\~english TODO:translate needed.

		\~chinese
		\ref send_queue_heavy。传送queue过大。

		\~japanese
		\~
		*/
		ErrorType_SendQueueIsHeavy,

		/**
		\~korean
		Heartbeat가 평균적으로 너무 느림

		\~english TODO:translate needed.

		\~chinese
		Heartbeat 平均太慢。

		\~japanese
		\~
		*/
		ErrorType_TooSlowHeartbeatWarning,

		/**
		\~korean
		메시지 압축이 실패 하였습니다.

		\~english TODO:translate needed.

		\~chinese
		信息压缩失败。

		\~japanese
		\~
		*/
		ErrorType_CompressFail,

		/**
		\~korean
		클라이언트 소켓의 listen을 시작할 수 없습니다. TCP 또는 UDP 소켓이 이미 사용중인 포트인지 확인하십시오.

		\~english
		Unable to start listening of client socket. Must check if either TCP or UDP socket is already in use.

		\~chinese
		无法开始玩家socket的listen。请确认TCP或者UDP socket端口是否已在使用中。

		\~japanese
		\~
		*/
		ErrorType_LocalSocketCreationFailed,

		/**
		\~korean
		Socket을 생성할 때 Port Pool 내 port number로의 bind가 실패했습니다. 대신 임의의 port number가 사용되었습니다. Port Pool의 갯수가 충분한지 확인하십시요.

		\~english
		Failed binding to local port that defined in Port Pool. Please check number of values in Port Pool are sufficient.

		\~chinese
		生成socket的时候往Port Pool内port number的bind失败。任意port number被代替使用。请确认Port Pool的个数是否充分。

		\~japanese
		\~
		*/
		Error_NoneAvailableInPortPool,

		/**
		\~korean
		Port pool 내 값들 중 하나 이상이 잘못되었습니다. 포트를 0(임의 포트 바인딩)으로 하거나 중복되지 않았는지 확인하십시요.

		\~english
		Range of user defined port is wrong. Set port to 0(random port binding) or check if it is overlaped.

		\~chinese
		Port pool 中一个以上的内值出错。确认把端口设为0（任意端口binding）或者是否重复。

		\~japanese
		\~
		*/
		ErrorType_InvalidPortPool,

		/**
		\~korean
		유효하지 않은 HostID 값입니다.

		\~english
		Invalid HostID.

		\~chinese
		无效的HostID。

		\~japanese
		\~
		*/
		ErrorType_InvalidHostID, // CODEREV-FORCERELAY: enum ErrorType에 변경을 가하셨군요. 이러한 경우 TypeToString_Kor, TypeToString_Eng,TypeToString_Chn에도 변경을 가해야 합니다. C#,AS,Java버전에서도 마찬가지입니다.

		/**
		\~korean
		사용자가 소화하는 메시지 처리 속도보다 내부적으로 쌓이는 메시지의 속도가 더 높습니다.
		지나치게 너무 많은 메시지를 송신하려고 했는지, 혹은 사용자의 메시지 수신 함수가 지나치게 느리게 작동하고 있는지 확인하십시오.

		\~english
		The speed of stacking messages are higher than the speed of processing them.
		Check that you are sending too many messages, or your message processing routines are running too slowly.

		\~chinese
		消息的堆叠速度高于处理它们的速度。
		检查您要发送的邮件太多，或消息处理程序运行过慢。
		\~
		\~
		*/
		ErrorType_MessageOverload,


		/** Accessing database failed. For example, query statement execution failed. You may see the details from comment variable.
		*/
		ErrorType_DatabaseAccessFailed,

		/**
		\~korean 메모리가 부족합니다.

		\~english Out of memory.

		\~chinese 内存不足.

		\~japanese メモリーが足りません。
		\~
		*/
		ErrorType_OutOfMemory,

		/** 서버와의 연결이 끊어져서 연결 복구 기능이 가동되었지만, 이것 마저도 실패했습니다. */
		ErrorType_AutoConnectionRecoveryFailed,
	};

	/**
	\~korean
	ProudNet 호스트 식별자

	\~english TODO:translate needed.

	\~chinese
	ProudNet 主机识别者。

	\~japanese
	\~
	*/
	enum HostID
	{
		/**
		\~korean
		없음

		\~english
		None

		\~chinese
		无

		\~japanese
		\~
		*/
		HostID_None = 0,

		/**
		\~korean
		서버

		\~english
		Server

		\~chinese
		服务器

		\~japanese
		\~
		*/
		HostID_Server = 1,

		/**
		\~korean
		(사용금지)

		\~english
		(Do not use it)

		\~chinese
		（禁止使用）

		\~japanese
		\~
		*/
		HostID_Last = 2
	};

	typedef int8_t MessageType;

#ifdef _WIN32
	enum _MessageType;
#endif

	enum RmiID
	{
		RmiID_None = 0,
		RmiID_Last = 65535, // marmalade에서 RMI ID가 word가 됨을 보장하기 위해.
	};

	typedef char LocalEventType;

#ifdef _WIN32
	enum _LocalEventType;
#endif

    enum ReliableUdpFrameType
    {
        ReliableUdpFrameType_None = 0,
        ReliableUdpFrameType_Data,
        ReliableUdpFrameType_Ack,
        //Disconnect
    };

	/** \~korean 모듈 내에서 발생하는 로그의 범주 타입입니다.
	\~english It is a category type of the log occurred within the module.
	\~chinese 在模块内发生的Log范畴类
	\~japanese モジュール内で発生するログのカテゴリータイプです。
	\~ */
	enum LogCategory
	{
		/** \~korean 기본 시스템
		\~english Basic System
		\~chinese 基础系统
		\~japanese 基本システム
		\~ */
		LogCategory_System,
		/** \~korean TCP 통신 관련
		\~english Relevant to TCP communication
		\~chinese 相关 TCP 通信
		\~japanese TCP通信関連
		\~ */
		LogCategory_Tcp,
		/** \~korean UDP 통신 관련
		\~english Relevant to UDP communication
		\~chinese 相关 UDP 通信
		\~japanese UDP通信関連
		\~ */
		LogCategory_Udp,
		/** \~korean P2P 통신 관련
		\~english Relevant to P2P communication
		\~chinese 相关 P2P 通信
		\~japanese P2P通信関連
		\~ */
		LogCategory_P2P
	};

	/**
	\~korean
	해킹의 종류

	\~english
	Type of hacking

	\~chinese
	黑客的种类。

	\~japanese
	\~
	*/
	enum HackType
	{
		/**
		\~korean
		해킹 아님

		\~english
		No hacking

		\~chinese
		不是黑客。

		\~japanese
		\~
		*/
		HackType_None,

		/**
		\~korean
		스피드핵

		\~english
		Speek hack

		\~chinese
		速度hack。

		\~japanese
		\~
		*/
		HackType_SpeedHack,

		/**
		\~korean
		패킷 조작

		\~english
		Handle packet

		\~chinese
		数据包操作。

		\~japanese
		\~
		*/
		HackType_PacketRig,
	};

	/**
	\~korean
	타 호스트와의 연결 상태

	\~english
	Connection status with other hosts

	\~chinese
	与其他主机的连接状态。

	\~japanese
	\~
	*/
	enum ConnectionState
	{
		/**
		\~korean
		연결이 끊어진 상태

		\~english
		Disconnected condition

		\~chinese
		连接中断的状态。

		\~japanese
		\~
		*/
		ConnectionState_Disconnected,

		/**
		\~korean
		연결 시도를 했지만 아직 결과를 알 수 없는 상태

		\~english
		Tried connecting but result is unknown

		\~chinese
		虽已试图连接，但无法知道结果的状态。

		\~japanese
		\~
		*/
		ConnectionState_Connecting,

		/**
		\~korean
		연결 과정이 성공한 상태

		\~english
		Succeed to connect

		\~chinese
		连接过程成功的状态。

		\~japanese
		\~
		*/
		ConnectionState_Connected,

		/**
		\~korean
		연결 해제 과정이 진행중인 상태
		주의 : RMI나 Callback 함수 내부에서 Disconnect 함수를 호출한 것이었으면 Connection_State는 ConnectionState_Disconnecting 상태로 인식됩니다.

		\~english
		Disconnecting is in progress
		caution : If you called the Disconnect function in a RMI or a Callback function, the Connection_State will be recognized as ConnectionState_Disconnecting.

		\~chinese
		在进行连接解除过程的状态。
		注意：RMI或者Callback函数内有Disconnect函数被调用的话，Connection_State会识别为ConnectionState_Disconnecting 状态。

		\~japanese
		連結解除の過程が進行中の状態
		注意 : RMIや Callback 関数の内部で Disconnect 関数を呼び出したら Connection_Stateは ConnectionState_Disconnecting 状態に認識されます。
		\~
		*/
		ConnectionState_Disconnecting,
	};

	PROUD_API const PNTCHAR* ToString(LogCategory logCategory);
	PROUD_API const PNTCHAR* ToString(ConnectionState val);

	/**
	\~korean
	TCP fallback을 의도적으로 시행할 때의 방법

	\~english
	How to intentially use TCP fallback

	\~chinese
	故意实行TCP fallback时的方法。

	\~japanese
	\~
	*/
	enum FallbackMethod // 강도가 낮은 순으로 enum 값을 정렬할 것
	{
		/**
		\~korean
		Fallback을 안함. 즉 서버 및 peer와의 UDP 통신을 모두 사용함.

		\~english
		No Fallback. In other words, UDP communication to both server and peer are in use.

		\~chinese
		不做Fallback。即使用全部的服务器及与peer的UDP通信。

		\~japanese
		\~
		*/
		FallbackMethod_None,
		/**
		\~korean
		서버와의 UDP 통신은 유지하되 타 Peer들과의 UDP 통신만이 차단된 것으로 처리한다. 일시적 포트매핑 실패와 유사한 상황을 재현한다. 가장 강도가 낮다.

		\~english
		Regards that UDP with server is sustained but UDP with other peers to be disconnected. Reproduce a circumstance similar to a temporary port mapping failure. This is the lowest option with weakest impact.

		\~chinese
		维持与服务器的UDP通信，处理为只与其他peer的UDP通信中断。再现一时的端口映像失败和类似的情况。强度最低。

		\~japanese
		\~
		*/
		FallbackMethod_PeersUdpToTcp,
		/**
		\~korean
		서버와의 UDP 통신을 차단된 것으로 처리한다. 일시적 포트매핑 실패와 유사한 상황을 재현한다. 아울러 Peer들과의 UDP 통신도 차단된다. 중간 강도다.

		\~english
		 Regards that UDP with server is disconnected. Reproduce a circumstance similar to a temporary port mapping failure. On top of that, it also disconnects UDP with peers. Intermediate impact.


		\~chinese
		处理为只与其他peer的UDP通信中断。再现一时的端口映像失败和类似的情况。与peer的UDP通信也被中断。强度一般。

		\~japanese
		\~
		*/
		FallbackMethod_ServerUdpToTcp,
		/**
		\~korean
		클라이언트의 UDP 소켓을 아예 닫아버린다. 영구적인 UDP 복구를 못하게 한다. 가장 강도가 높다.
		- Proud.CNetServer.SetDefaultFallbackMethod 에서는 사용할 수 없다.

		\~english
		 All UDP sockets of client will be shut down. UDP restoration will never be possible. Strongest impact.
		- Unable to use in Proud.CNetServer.SetDefaultFallbackMethod

		\~chinese
		直接关闭玩家的UDPsocket。不让做永久性的UDP修复。强度最高。

		\~japanese
		\~
		*/
		FallbackMethod_CloseUdpSocket,
	};

	/**
	\~korean
	클라이언트간 직접 P2P 통신을 위한 홀펀칭을 시작하는 조건

	\~english
	Conditions to start hole-punching for direct P2P communication among clients

	\~chinese
	为了玩家之间开始直接P2P通信的打洞条件。

	\~japanese
	\~
	*/
	enum DirectP2PStartCondition
	{
		/**
		\~korean
		꼭 필요한 상황이 아닌 이상 홀펀칭을 하지 않는다. 웬만하면 이것을 쓰는 것이 좋다.

		\~english
		Unless really needed, it is recommended not to do hole-punching. Using this is strongly recommended.

		\~chinese
		不是必要的情况的话不进行打洞。最好用这个。

		\~japanese
		\~
		*/
		DirectP2PStartCondition_Jit,
		/**
		\~korean
		CNetServer.CreateP2PGroup 이나 CNetServer.JoinP2PGroup 등에 의해 클라이언트간 P2P 통신이 허용되는 순간에 무조건
		홀펀칭 과정을 시작한다. 예를 들어 \ref super_peer  에서 수퍼 피어를 게임 플레이 도중 종종 바꿔야 한다면 이것이
		필요할 수도 있을 것이다.

		\~english
		This forcefully begins hole-punching at the moment when P2P communication among clients is allowed by CNetServer.CreateP2PGroup or CNetServer.JoinP2PGroup or others.
		For an example, if there is a need to change super peer at \ref super_peer during game play, this may be needed.

		\~chinese
		被 CNetServer.CreateP2PGroup%或者 CNetServer.JoinP2PGroup ，玩家之间P2P通信允许的瞬间一定要开始打洞过程。例如在\ref super_peer%，游戏进行中要把super peer改变时可能需要这个。

		\~japanese
		\~
		*/
		DirectP2PStartCondition_Always,
		DirectP2PStartCondition_LAST,
	};
	/**  @} */

	/**
	\~korean
	사용자 실수로 인한 에러를 ProudNet이 보여주는 방법

	\~english
	The way Proudnet shows error caused by user

	\~chinese
	ProudNet 显示由用户的失误造成的错误的方法。

	\~japanese
	\~
	*/
	enum ErrorReaction
	{
		/**
		\~korean
		대화 상자를 보여준다. 개발 과정에서는 요긴하지만 엔드유저 입장에서는 풀스크린인 경우 프리징으로 보일 수 있어서 되레 문제 찾기가 더 어려울 수 있다.

		\~english
		Shows chat box. This may be useful during development process but it can also be very troublesome since the chat box can be seen to end users as full screen freezing.

		\~chinese
		显示对话框。虽在开发过程中非常重要，但对于最终用户在全屏中可能会视其为freezing，查找问题可能反而更加困难。

		\~japanese
		\~
		*/
		ErrorReaction_MessageBox,
		/**
		\~korean
		크래시를 유발한다. 엔드유저 입장에서는 크래시로 나타나므로 릴리즈된 경우 문제를 더 쉽게 찾을 수 있다.
		단, \ref minidump  등을 혼용해야 그 가치가 있다.

		\~english
		Causes a crash. From end users' point of view, this is definitely a crash so it can be rather easier to find what the problem is when released.
		But, it is more effective when used along with \ref minidump.

		\~chinese
		诱发crash。在最终用户显示为crash，可能更容易找到release的原因。
		但要混用\ref minidump%等才会有其价值。

		\~japanese
		\~
		*/
		ErrorReaction_AccessViolation,
		/**
		\~korean
		디버거 출력창에서만 보여집니다. 크래시나 대화상자를 띄워줄만한 상황이 아니라면 이옵션을 사용하십시오.

		\~english
		It shows only debugger output screen. If you can not pop up crash or message box, please use this option.

		\~chinese
		只显示在调试输出窗。不是显示crash或者对话框的情况的话，请使用此选项。

		\~japanese
		\~
		*/
		ErrorReaction_DebugOutput,
		/**
		\~korean
		디버거 브레이크를 겁니다. 해당옵션은 디버그 모드에서만 사용하십시오.
		\~english
		\~
		 */
		ErrorReaction_DebugBreak
	};

	/**
	\~korean
	연산 종류

	\~english
	Operation type

	\~chinese
	运算种类。

	\~japanese
	\~
	*/
	enum ValueOperType
	{
		/**
		\~korean
		덧셈

		\~english
		Addition

		\~chinese
		加法。

		\~japanese
		\~
		*/
		ValueOperType_Plus,
		/**
		\~korean
		뺄셈

		\~english
		Subtraction

		\~chinese
		减法

		\~japanese
		\~
		*/
		ValueOperType_Minus,
		/**
		\~korean
		곱셈

		\~english
		Multiplication

		\~chinese
		乘法。

		\~japanese
		\~
		*/
		ValueOperType_Multiply,
		/**
		\~korean
		나눗셈

		\~english
		Division

		\~chinese
		除法。

		\~japanese
		\~
		*/
		ValueOperType_Division
	};

	/**
	\~korean
	비교 종류

	\~english
	Comparison type

	\~chinese
	比较种类。

	\~japanese
	\~
	*/
	enum ValueCompareType
	{
		/**
		\~korean
		크거나 같으면

		\~english
		Greater or equal

		\~chinese
		大或等于。

		\~japanese
		\~
		*/
		ValueCompareType_GreaterEqual,

		/**
		\~korean
		크면

		\~english
		Greater

		\~chinese
		大的话。

		\~japanese
		\~
		*/
		ValueCompareType_Greater,

		/**
		\~korean
		작거나 같으면

		\~english
		Less or equal

		\~chinese
		小或等于。

		\~japanese
		\~
		*/
		ValueCompareType_LessEqual,

		/**
		\~korean
		작으면

		\~english
		Less

		\~chinese
		小的话。

		\~japanese
		\~
		*/
		ValueCompareType_Less,

		/**
		\~korean
		같으면

		\~english
		Equal

		\~chinese
		等于。

		\~japanese
		\~
		*/
		ValueCompareType_Equal,

		/**
		\~korean
		다르면

		\~english
		Not equal

		\~chinese
		不同的话。

		\~japanese
		\~
		*/
		ValueCompareType_NotEqual
	};

	/**
	\~korean
	암호화 수준

	\~english
	Encryption Level

	\~chinese
	加密水平。

	\~japanese
	\~
	*/
	enum EncryptLevel
	{
		/**
		\~korean
		AES 암호화 수준 하

		\~english
		AES encryption level Low

		\~chinese
		AES 加密水平低。

		\~japanese
		\~
		*/
		EncryptLevel_Low = 128,
		/**
		\~korean
		AES 암호화 수준 중

		\~english
		AES encryption level Middle

		\~chinese
		AES 加密水平中。

		\~japanese
		\~
		*/
		EncryptLevel_Middle = 192,
		/**
		\~korean
		AES 암호화 수준 상

		\~english
		AES encryption level High

		\~chinese
		AES 加密水平上。

		\~japanese
		\~
		*/
		EncryptLevel_High = 256,
	};

	/**
	\~korean
	Fast 암호화 수준

	\~english
	Fast encryption level

	\~chinese
	Fast 加密水平。

	\~japanese
	\~
	*/
	enum FastEncryptLevel
	{
		/**
		\~korean
		Fast 암호화 수준 하

		\~english
		Fast encryption level Low

		\~chinese
		Fast加密水平低。

		\~japanese
		\~
		*/
		FastEncryptLevel_Low = 512,
		/**
		\~korean
		Fast 암호화 수준 중

		\~english
		Fast encryption level Middle

		\~chinese
		Fast加密水平中。

		\~japanese
		\~
		*/
		FastEncryptLevel_Middle = 1024,
		/**
		\~korean
		Fast 암호화 수준 상

		\~english
		Fast encryption level High

		\~chinese
		Fast加密水平上。

		\~japanese
		\~
		*/
		FastEncryptLevel_High = 2048,
	};

	enum HostType
	{
		HostType_Server,
		HostType_Peer,
		HostType_All,
	};

	/**
	\~korean
	HostID 재사용 기능 옵션입니다.

	\~english
	It is an option for HostID reuse function.

	\~chinese
	HostID是再使用技能选项。

	\~japanese
	HostID再使用機能オプションです。

	\~
	*/
	enum HostIDGenerationPolicy
	{
		/**
		\~korean
		HostID 재사용 기능을 사용합니다.
		-기본값입니다.

		\~english
		It uses a HostID reuse function.
		-It is a default value.

		\~chinese
		HostID 使用再使用技能。
		-为基本值。.

		\~japanese
		HostID再使用機能を使います。
		‐デフォルト値です

		\~
		*/
		HostIDGenerationPolicy_Recycle = 1,

		/**
		\~korean
		HostID 재사용 기능을 사용하지 않습니다.

		\~english
		It does not use a HostID reuse function.

		\~chinese
		HostID 不使用再使用技能。.

		\~japanese
		HostID再使用機能を使いません。

		\~
		*/
		HostIDGenerationPolicy_NoRecycle,
	};

	/**
	\~korean

	\~english

	\~chinese

	\~japanese

	\~
	*/
	enum ThreadModel
	{
		/**
		\~korean

		\~english

		\~chinese

		\~japanese

		\~
		*/
		ThreadModel_SingleThreaded = 1,

		/**
		\~korean

		\~english

		\~chinese

		\~japanese

		\~
		*/
		ThreadModel_MultiThreaded,

		/**
		\~korean

		\~english

		\~chinese

		\~japanese

		\~
		*/
		ThreadModel_UseExternalThreadPool,
	};
}


//#pragma pack(pop)
