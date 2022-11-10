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

#include "ProudNetServer.h"
#include "ProudNetClient.h"

//#pragma pack(push,8)

namespace Proud
{
#if (_MSC_VER>=1400)
#pragma managed(push, off)
#endif


	/**
	\~korean
	일정 시간 혹은 OnReportStatsCommand 가 왔을때 Agent 로 보고할 Report 클래스
	- 사용자가 임의로 값을 넣어서 사용할수 있습니다.
	- 여기서 입력한 값은 PNServerAgentConsole 에서 확인가능합니다.

	\~english TODO:translate needed.

	\~chinese
	一定的时间或者OnReportStatsCommand的时候用Agent报告的Report类。
	- 使用者可以输入任意值使用。
	- 在这里输入的值在PNServerAgentConsole可以确认。

	\~japanese
	\~
	*/
	class CReportStatus
	{
	public:
		/**
		\~korean
		이값에 따라서 PNLicenseAgentConsole 에서 글자색이 달라집니다.

		\~english TODO:translate needed.

		\~chinese
		随着此值，在PNLicenseAgentConsole的字体颜色会改变。

		\~japanese
		\~
		*/
		enum StatusType { StatusType_OK, StatusType_Warning, StatusType_Error };

		/**
		\~korean
		AgentConsole 로 보낼 정보의 타입

		\~english TODO:translate needed.

		\~chinese
		往AgentConsole发送信息的类型。

		\~japanese
		\~
		*/
		StatusType m_statusType;

		/**
		\~korean
		정보의 내용. 이 String이 비면  Data를 보내지 않습니다..

		\~english TODO:translate needed.

		\~chinese
		信息的内容。如果此String空的话不发送Data。

		\~japanese
		\~
		*/
		String m_statusText;

		typedef CFastMap<String, String> KeyValueList;

		/**
		\~korean
		추가적으로 필요한 Data들의 정보 List
		- AgentConsole의 property View의 Log를 통하여 확인하실 수 있습니다.
		- AgentConsole의 property View에서 Key : Value로 출력됩니다.

		\~english TODO:translate needed.

		\~chinese
		额外需要的Data信息List。
		-AgentConsole 的property可以通过View Log可以确认.
		-AgentConsole 的property在Key : Value输出.

		\~japanese
		\~
		*/
		KeyValueList m_list;
	};

	/**
	\~korean
	CAgentConnector 가 요구하는 delegate 인터페이스. CAgentConnector 를 생성할 때 인자로 필요하다.

	\~english TODO:translate needed.

	\~chinese
	CAgentConnector 需要的delegate界面。作为因子，生成 CAgentConnector%的时候需要。

	\~japanese
	\~
	*/
	class IAgentConnectorDelegate
	{
	public:
		/**
		\~korean
		Agnet connect의 인증이 성공된 경우 callback됩니다.

		\~english TODO:translate needed.

		\~chinese
		Agnet connect 的认证成功时会callback。

		\~japanese
		\~
		*/
		virtual void OnAuthentication(ErrorInfo* errorinfo) = 0;

		/**
		\~korean
		Server app을 종료시키라는 명령이 온경우 callback됩니다.

		\~english TODO:translate needed.

		\~chinese
		接到结束Server app的命令时会callback。

		\~japanese
		\~
		*/
		virtual void OnStopCommand() = 0;

		/**
		\~korean
		Agent Server 로부터 Report 요청 명령이 온경우 callback됩니다.

		\~english TODO:translate needed.

		\~chinese
		从Agent Server接到Report邀请命令时会callback。

		\~japanese
		\~
		*/
		virtual void OnReportStatsCommand() = 0;

		/**
		\~korean
		Agent를 사용자가 잘못된 방법으로 사용하였을 시에 호출됩니다.

		\~english TODO:translate needed.

		\~chinese
		用户以错误的方法使用Agent的时候呼出。

		\~japanese
		\~
		*/
		virtual void OnWarning(ErrorInfo* errorinfo) = 0;
	};

	/**
	\~korean
	Agent와 연결하여 PNServerAgentConsole 에서 사용자의 서버앱을 원격으로 관리할수 있습니다.

	\~english TODO:translate needed.

	\~chinese
	连接Agent以后在PNServerAgentConsole可以远程管理用户的server app。

	\~japanese
	\~
	*/
	class CAgentConnector
	{
	public:
		virtual ~CAgentConnector() {}

		PROUD_API static CAgentConnector* Create(IAgentConnectorDelegate* dg);

		/**
		\~korean
		Start 후 객체를 삭제하기 전까지 Agent와 계속 연결을 유지합니다.
		- ServerAgent에 의하여 실행 된 것이 아닌 경우 Start되지 않습니다.

		\~english TODO:translate needed.

		\~chinese
		Start 以后，把对象删除之前会继续维持与Agent的连接。
		- 不是被ServerAgent实行的情况不Start。

		\~japanese
		\~
		*/
		virtual bool Start() = 0;

		/**
		\~korean
		일정 시간 혹은 IAgentConnectorDelegate::OnReportStatsCommand 가 왔을때 이 함수를 이용하여 CReportStatus 를 Agent로 보낼수 있습니다.
		\return ServerAgent에 의하여 실행되지 않아 Start 되지 않았거나 reportStatus의 Data가 잘못되었을 시에 false

		\~english TODO:translate needed.

		\~chinese
		一定时间或者IAgentConnectorDelegate::OnReportStatsCommand 的时候，利用此函数可以把 CReportStatus%发送到Agent。
		\return 没有被ServerAgent实行而没有Start的或者reportStatus的Data出错的时候fals。

		\~japanese
		\~
		*/
		virtual bool SendReportStatus(CReportStatus& reportStatus) = 0;

		/**
		\~korean
		Agent로 Log를 보내는 기능
		- 상세한 데이터가 아닌 간단한 로그를 보내고자 할 때 SendReportStatus를 사용하지 않고 EventLog를 통하여 간편이 이용하실 수 있습니다.
		\return text가 비었거나 ServerAgent가 Start되지 못하였을 때 false

		\~english TODO:translate needed.

		\~chinese
		把Log发送到Agent的功能
		- 想发送不是详细数据而是简单log的时候，不用SendReportStatus，而是可以通过EventLog简单利用。
		\return text空着或者ServerAgent没Start的时候false。

		\~japanese
		*/
		virtual bool EventLog(CReportStatus::StatusType logType, const TCHAR* text) = 0;

		/**
		\~korean
		일정 시간에 한번씩 해주어야 하는 일들을 처리합니다.
		- 연결이 되어 있지 않다면 일정 시간마다 자동으로 연결을 시도 합니다.

		\~english TODO:translate needed.

		\~chinese
		处理在一定时间每进行一次的事情。
		- 没有连接的话会每一定时间自动尝试连接。

		\~japanese
		*/
		virtual void FrameMove() = 0;

		/**
		\~korean
		FrameMove 내의 실시간 상태 정보 전송에 대한 Delay Time을 정합니다.
		- 1/1000초 단위입니다.
		- default 는 1000입니다.

		\~english TODO:translate needed.

		\~chinese
		决定对FrameMove内的实时状态信息传送的Delay Time。
		- 是1/1000秒单位。
		- default是1000。


		\~japanese
		\~
		*/
		virtual void SetDelayTimeAboutSendAgentStatus(uint32_t delay) = 0;

	};


#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif
}

//#pragma pack(pop)
