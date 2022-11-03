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

//#define TEST_DISCONNECT_CLEANUP 테스트 용도


#include "BasicTypes.h"
//#include "ConnectParam.h"
#include "HostIDArray.h"
#include "AddrPort.h"
#include "Enums.h"
#include "EncryptEnum.h"
#include "RMIContext.h"
#include "ErrorInfo.h"

#define PN_DEPRECATED /*__declspec(deprecated) */

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4324)
#endif

//#pragma pack(push,8)

namespace Proud
{
#if (_MSC_VER>=1400)
#pragma managed(push, off)
#endif

	/** \addtogroup net_group
	*  @{
	*/

	class CMessage;
	class CSendFragRefs;
	class ErrorInfo;
	class IRmiProxy;
	class IRmiStub;
	class RmiContext;
	class MessageSummary;

	struct SendOpt;

	/**
	\~korean
	\brief ProudNet RMI가 참조하는 네트워크 호스트 인터페이스입니다.
	유저는 이 클래스를 직접 사용할 필요가 없습니다.

	\~english
	\brief ProudNet Network host interface that is referred by RMI.
	User does not have to use this class directly.

	\~chinese
	\brief ProudNet RMI参照的联网主机界面。
	用户不用直接使用此类。

	\~japanese
	\~
	*/
	class IRmiHost
	{
		friend class IRmiProxy;
	public:
		virtual ~IRmiHost() {}

		/**
		\~korean
		PIDL 컴파일러의 결과물 중 proxy를 이 객체에 등록한다.

		\~english
		Registers proxy among the results of PIDL compiler to this object

		\~chinese
		PIDL 编译器的产物中把proxy登录到此对象。

		\~japanese
		\~
		*/
		virtual void AttachProxy(IRmiProxy* proxy) = 0;
		// no-throw version for UE4, etc.
		void AttachProxy(IRmiProxy *proxy, ErrorInfoPtr& outError);

		/**
		\~korean
		PIDL 컴파일러의 결과물 중 stub을 이 객체에 등록한다.

		\~english
		Registers stub among the results of PIDL compiler to this object

		\~chinese
		PIDL 编译器产物中把stub登录到此对象。

		\~japanese
		\~
		*/
		virtual void AttachStub(IRmiStub* stub) = 0;
		// no-throw version for UE4, etc.
		void AttachStub(IRmiStub* stub, ErrorInfoPtr& outError);
		/**
		\~korean
		AttachProxy 의 반대로서, 이 객체에서 등록 해제를 한다.

		\~english
		As opposite to AttachProxy, this object cancels the registration.

		\~chinese
		是AttachProxy的相反，解除在此对象的登录。

		\~japanese
		\~
		*/
		virtual void DetachProxy(IRmiProxy* proxy) = 0;
		// no-throw version for UE4, etc.
		void DetachProxy(IRmiProxy* proxy, ErrorInfoPtr& outError);
		/**
		\~korean
		AttachStub 의 반대로서, 이 객체에서 등록 해제를 한다.

		\~english
		As opposite to AttachStub, this object cancels the registration.

		\~chinese
		是AttachStub的相反，解除在此对象的登录。

		\~japanese
		\~
		*/
		virtual void DetachStub(IRmiStub* stub) = 0;
		// no-throw version for UE4, etc.
		void DetachStub(IRmiStub* stub, ErrorInfoPtr& outError);
	protected:
		// 내부 용도 함수이다. 사용자는 호출하지 말 것.
		// 이 함수에 대한 설명은 CNetClientImpl에 있음
		virtual bool Send(const CSendFragRefs &msg,const SendOpt& sendContext,const HostID *sendTo, int numberOfsendTo,int &compressedPayloadLength) = 0;

	public:

		/**
		\~korean
		simple packet mode를 사용하고 있는지 확인한다.

		\~english
		TODO:translate needed.

		\~chinese
		TODO:translate needed.

		\~japanese
		TODO:translate needed.
		\~
		*/
		virtual bool IsSimplePacketMode() = 0;

		/**
		\~korean
		내부 함수. 사용자는 호출 금지.

		\~english
		Internal function. User must not call this.

		\~chinese
		内部函数。用户禁止呼叫。

		\~japanese
		\~
		*/
		virtual void ShowError_NOCSLOCK( ErrorInfoPtr errorInfo ) = 0;
		virtual void ShowNotImplementedRmiWarning(const PNTCHAR* RMIName) = 0;
		virtual void PostCheckReadMessage(CMessage& msg, const PNTCHAR* RMIName) = 0;

		/**
		\~korean
		내부 함수. 사용자는 호출 금지.

		\~english
		Internal function. User must not call this.

		\~chinese
		内部函数。用户禁止呼叫。

		\~japanese
		\~
		*/
		virtual void ConvertGroupToIndividualsAndUnion( int numberOfsendTo, const HostID * sendTo, HostIDArray& output) = 0;

		virtual CriticalSection& GetCriticalSection() = 0;

#ifdef VIZAGENT
		virtual void EnableVizAgent(const PNTCHAR* vizServerAddr, int vizServerPort, const String &loginKey) = 0;

		virtual void Viz_NotifySendByProxy(const HostID* remotes, int remoteCount, const MessageSummary &summary, const RmiContext& rmiContext) = 0;
		virtual void Viz_NotifyRecvToStub(HostID sender, RmiID RmiID, const PNTCHAR* RmiName, const PNTCHAR* paramString) = 0;
#endif
	};

	/**
	\~korean
	\brief clientWorker의 정보를 담는 구조체.

	\~english
	\brief Structure that contain information of clientWorker

	\~chinese
	\brief 包含clientWorker信息的构造体。

	\~japanese
	\~
	*/
	class CClientWorkerInfo
	{
	public:

		bool m_isWorkerThreadNull;

		//bool m_isWorkerThreadEnded;

		int m_connectCallCount;

		int m_disconnectCallCount;

		ConnectionState m_connectionState;

		int m_finalWorkerItemCount;

//		int64_t m_lastTcpStreamTimeMs;

		int64_t m_currentTimeMs;

		int m_peerCount;
#if !defined(_WIN32)
		pthread_t m_workerThreadID;
#else
		uint32_t m_workerThreadID;
#endif
	};

	/**
	\~korean
	\brief socket의 정보를 담는 구조체입니다.

	\~english
	\brief Structure that contain information of socket

	\~chinese
	\brief 包含socket信息的构造体。

	\~japanese
	\~
	*/
	class CSocketInfo
	{
	public:
		/**
		\~korean
		TCP socket handle입니다. 서버와의 연결을 위한 socket입니다.

		\~english
		It is TCP socket handle. This socket is for connecting to server

		\~chinese
		是TCP socket handle。为了与服务器连接的socket。

		\~japanese
		\~
		*/
		SOCKET m_tcpSocket;

		/**
		\~korean
		UDP socket handle입니다. 서버 혹흔 P2P peer와의 연결을 위한 socket입니다.

		\~english
		It is UDP socket handle. This socket is for connecting to server or P2P peer.

		\~chinese
		是UDP socket handle。为了与服务器或者P2P peer的连接的socket。

		\~japanese
		\~
		*/
		SOCKET m_udpSocket;

		inline CSocketInfo()
		{
#if !defined(_WIN32)
			m_tcpSocket = -1;
			m_udpSocket = -1;
#else
			m_tcpSocket = INVALID_SOCKET;
			m_udpSocket = INVALID_SOCKET;
#endif
		}
	};

	/**
	\~korean
	\brief 1개의 스레드에 대한 정보가 담겨져 있는 구조체.

	\~english
	\brief Construct contains information of 1 thread

	\~chinese
	\brief 包含对一个线程信息的构造体。

	\~japanese
	\~
	*/
	class CThreadInfo
	{
	public:
		/**
		\~korean
		Thread의 ID

		\~english
		ID of thread

		\~chinese
		Thread的ID。

		\~japanese
		\~
		*/
#if defined (_WIN32)
		uint32_t m_threadID;
#endif

		/**
		\~korean
		Thread의 Handle

		\~english
		Handle of thread

		\~chinese
		Thread的Handle。

		\~japanese
		\~
		*/
#if defined(_WIN32)
		HANDLE m_threadHandle;
#else
		pthread_t m_threadHandle;
#endif
	};

	/**
	\~korean
	\brief 사용자 프로그램에서 엔진에 전달하는 입력 파라메터

	\~english
	Input parameter delivered from \brief user program to engine

	\~chinese
	\brief 在用户程序中，往引擎传达的输入参数。

	\~japanese
	\~
	*/
	class CApplicationHint
	{
	public:
		/**
		\~korean
		사용자가 측정한, 응용 프로그램의 프레임 레이트입니다.

		\~english
		Frame rate of application program measured by user

		\~chinese
		用户测定的，应用程序的帧速率。

		\~japanese
		\~
		*/
		double m_recentFrameRate;

		CApplicationHint()
		{
			m_recentFrameRate = 0;
		}
	};

	/**
	\~korean
	\brief 통계 정보. 디버깅이나 성능 측정을 위함

	\~english
	\brief statistics information. For debugging or performance test

	\~chinese
	\brief 统计信息。为了调试或性能测定。

	\~japanese
	\~
	*/
	class ReliableUdpHostStats
	{
	public:
		// 수신 윈도에 들어있는 프레임 갯수, 아직 뽑아내지 않은 스트림 크기, 여지껏 받은 스트림 크기, 여지껏 받은 프레임 갯수, 최근에 프레임 받는 갯수의 속도
		int m_receivedFrameCount, m_receivedStreamCount, m_totalReceivedStreamLength, m_totalAckFrameCount;
		int m_senderWindowLength;
		int m_senderWindowMaxLength;
		int m_senderWindowWidth;	// first~last의 number 차이
		int m_expectedFrameNumberToReceive;
		int m_nextFrameNumberToSend;

		/**
		\~korean
		송신큐에 쌓여있는 스트림 크기

		\~english
		Stream size that stacked at sending queue

		\~chinese
		堆积在传送queue的流大小。

		\~japanese
		\~
		*/
		int m_sendStreamCount;

		/**
		\~korean
		초송신 윈도에 들어있는 프레임 갯수

		\~english
		Number of frame that located in first sending window

		\~chinese
		在初始传送窗的帧个数。

		\~japanese
		\~
		*/
		int m_senderWindowDataFrameCount;

        /**
		\~korean
		재송신 윈도에 들어있는 프레임 갯수

		\~english
		Number of frame that located in resend window

		\~chinese
		在再次传送窗的帧个数。

		\~japanese
		\~
		*/
		int m_resendFrameCount;

		/**
		\~korean
		여지껏 보낸 스트림 크기

		\~english
		Total size of sent stream

		\~chinese
		至今发送的流大小。

		\~japanese
		\~
		*/
		int m_totalSendStreamLength;

		/**
		\~korean
		여지껏 초송신한 프레임 갯수
		- 초송신한 프레임 갯수에 비해 여지껏 재송신한 프레임 갯수가 지나치게 증가하면 P2P Reliable 메시징이 잘 이루어지지 않음을 의미한다.

		\~english
		Total number of first send frame
		- If number of resent frame is dramarically increased compare to number of first send frame, it mean, it does not working P2P Reliable messaing unstably

		\~chinese
		至今初始传送的帧个数。
		- 比起初始传送的帧个数，至今传送的帧个数过分增加的话，意味着P2P Reliable messaging没有很好的完成。

		\~japanese
		\~
		*/
		int m_totalFirstSendCount;

		/**
		\~korean
		여지껏 재송신한 프레임 갯수

		\~english
		Total number of resent frame

		\~chinese
		至今再次传送的帧个数。

		\~japanese
		\~
		*/
		int m_totalResendCount;

		/// if zero, it means nothing to show, or the number is zero indeed.
		int m_senderWindowLastFrame;

		int m_totalReceiveDataCount;
		int m_lastReceivedAckNumber;
		String ToString();
	};

	// 주의: 이걸 여러 스레드에서 너무 자주 접근할 시,멀티코어 환경에서 메인보드상에서의 병목이 유발되더라.
	class CTestStats
	{
	public:
		static double TestAllowedMaxSendSpeed;
		static double TestRecentReceiveSpeed;
		static double TestRecentSendSpeedAtReceiverSide;
	};

	// 주의: 이걸 여러 스레드에서 너무 자주 접근할 시,멀티코어 환경에서 메인보드상에서의 병목이 유발되더라.
	class CTestStats2
	{
	public:
		bool m_C2SReplyRecentReceiveSpeedDone;
		bool m_C2CRequestRecentReceiveSpeedDone;
		bool m_C2CReplyRecentReceiveSpeedDone;

		CTestStats2()
		{
			m_C2SReplyRecentReceiveSpeedDone = false;
			m_C2CRequestRecentReceiveSpeedDone = false;
			m_C2CReplyRecentReceiveSpeedDone = false;
		}
	};

	/**
	\~korean
	Disconnect()에서 사용하는 인자입니다.

	\~english TODO:translate needed.

	\~chinese
	Disconnect()使用的因子。
	\~japanese
	\~
	*/
	class CDisconnectArgs
	{
	public:
		/**
		\~korean
		graceful disconnect를 수행하는데 걸리는 최대 시간입니다.
		이 시간을 넘어서면 Proud.CNetClient.Disconnect()는 무조건 return하게 되고, 서버에서는 클라이언트의 연결 해제를
		즉시 인식하지 못합니다.
		꼭 필요한 경우가 아니면 이 값을 변경하지 마십시오.

		\~english TODO:translate needed.

		\~chinese
		执行graceful disconnect所消耗的最长时间。
		如果超出此时间，则 Proud.CNetClient.Disconnect()%将会随机return，服务器将无法立刻识别客户端的连接解除。
		如不是必要情况，请匆变更此值。

		\~japanese
		\~
		*/
		int64_t m_gracefulDisconnectTimeoutMs;

		/**
		\~korean
		이것으로 Disconnect시의 대기하는 Sleep 시간을 조절할 수 있습니다.

		\~english TODO:translate needed.

		\~chinese TODO:translate needed.
		\~japanese TODO:translate needed.
		\~
		*/
		uint32_t m_disconnectSleepIntervalMs;

		/**
		\~korean
		서버에서 클라이언트 연결 해제 사유를 이것을 통해 전송할 수 있습니다.

		\~english TODO:translate needed.

		\~chinese
		服务器将通过此传送客户端连接解除原因。
		\~japanese
		\~
		*/
		ByteArray m_comment;

		CDisconnectArgs();
	};

	extern StringA policyFileText;

#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif

	/**  @} */
}

//#pragma pack(pop)

#ifdef _MSC_VER
#pragma warning(pop)
#endif
