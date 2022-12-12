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

//#pragma pack(push,8)

namespace Proud
{
#if (_MSC_VER>=1400)
#pragma managed(push, off)
#endif

    
#if defined(_WIN32)    
	/** \addtogroup minidump_group
	*  @{
	*/

	/** 
	\~korean
	CDumpClient 가 요구하는 delegate 인터페이스. CDumpClient 를 생성할 때 인자로 필요하다.

	\~english
	Delegate interface that is requested by CDumpClient. It is needed as as index when creating CDumpClient.

	\~chinese
	CDumpClient 要求的delegate实例。生成 CDumpClient%的时候需要的因子。

	\~japanese
	\~
	*/
	class IDumpClientDelegate
	{
	public:
		virtual ~IDumpClientDelegate() {}

		/** 
		\~korean
		CDumpClient 가 덤프 파일을 서버로 보내는 동안 일정 시간마다 호출된다. 유저에 의해
		보내기를 취소할 경우 이 함수는 true를 리턴하면 된다.

		\~english
		Called periodically while CDumpClient sends dump file to server. This function must return true in order to let user cancel the sending process.

		\~chinese
		CDumpClien 把转储文件发送到服务器的期间，每一定时间都会被呼叫。被用户取消发送的时候，此函数返回true即可。

		\~japanese
		\~
		*/
		virtual bool MustStopNow() = 0;

		/** 
		\~korean
		송신 처리중 예외가 발생할 경우 이 메서드가 콜백된다.
		\param e 예외정보 Exception::what() 을 통하여 문제에 관련된 String 받으실 수 있습니다.

		\~english TODO:translate needed.
		This method will be called back in case when an exception occurs while processing sending.


		\~chinese
		文件传送处理中产生例外的时候回调此方法。
		\param e 通过例外信息 Exception::what()%可以获得问题相关的String。

		\~japanese
		\~
		*/
		virtual void OnException(Exception &e) = 0;
		
		/** 
		\~korean
		송신이 완료되면 이것이 호출된다. 송신 대화 상자를 이때 닫으면 된다.

		\~english
		This will be called once sending is completed. Sending chat box can be closed at this point. 

		\~chinese
		传送完毕的话这个会被呼叫。这时候关闭传送对话框即可。

		\~japanese
		\~
		*/
		virtual void OnComplete() = 0;
	};

	/** 
	\~korean
	덤프 클라이언트
	- CDumpServer 로의 연결을 한 후 서버로 DMP 파일을 보내는 역할을 한다.
	- HTTP 방식에 비해 효율적이며 대부분의 웹 호스팅 서비스가 파일 기록 허가를 꺼놓기 때문에 차라리 자체 서버 구축이
	불가피하다는 점을 감안하면 이렇게 자체 프로토콜이 더 현실적이다.

	일반적 용도
	- 생성은 Create()로 한다.
	- 이 객체를 생성한 후 Start()로 송신을 한다.
	- 매 일정 시간(대략 1초에 10회 이상) FrameMove를 호출한다.
	- 송신중 중도 상황은 GetState(),GetSendProgress(),GetSendTotal()로 얻을 수 있다. 이것을 대화 상자에 표시하면 된다.
	- OnException, OnComplete가 올 때까지 기다린다.

	\~english
	Dump client
	- Manages sending DMP files to server after connected to CDumpServer
	- Inefficeint compared to HTTP and this kind of custom protocol is more realistic since most of web hosting services turn off file record permission pushing publishers to setup own servers.

	General usage
	- Use Create() to create.
	- After creating the object the use Start() to send.
	- Calls FrameMove periodically (approx. 10 or more times per second)
	- The sending status during sending process can be acquired by using GetState(), GetSendProgress() and GetSendTotal(). And those can be displayed in chat box.
	- ait until OnException and OnComplete arrive.

	\~chinese
	转储玩家
	- 与 CDumpServer%的连接后，起着往服务器发送DMP文件的作用。
	- 比HTTP方式有效率，因为大部分web hosting服务关闭文件记录许可，考虑本身服务器构建是不可避免，这样自身的protocol是更现实的。

	一般的用途
	- 由Create()生成。
	- 生成此对象以后用Start()传送。
	- 每一定时间（大约每秒10回以上）呼叫FrameMove。
	- 文件传送的状况由GetState(),GetSendProgress(),GetSendTotal()获得。把这个显示在对话框即可。
	- 等到OnException, OnComplete到来。

	\~japanese
	\~
	*/
	class CDumpClient
	{
	public:
		enum State { Connecting, Sending, Closing, Stopped};

		virtual ~CDumpClient() {};

		/** 
		\~korean
		덤프 서버로 덤프 파일을 보내기 시작한다.
		\param serverAddr 서버 주소
		\param serverPort 서버 포트
		\param filePath 파일의 경로

		\~english TODO:translate needed.
		Start sending dump file to dump server.
		\param serverAddr
		\param serverPort
		\param filePath

		\~chinese
		往转储服务器开始发送转储文件。
		\param serverAddr 服务器地址。
		\param serverPort 服务器端口。
		\param filePath 文件的路径。

		\~japanese
		\~
		*/
		virtual void Start(String serverAddr, uint16_t serverPort, String filePath) = 0;

		/** 
		\~korean
		일정 시간마다 이 메서드를 호출해줘야 송신 과정이 진행될 수 있다.

		\~english
		Sending process is sustained by calling this method periodically.

		\~chinese
		每一定时间呼叫此方法，传送过程才能进行。

		\~japanese
		\~
		*/
		virtual void FrameMove() = 0;

		/** 
		\~korean
		현재 덤프 파일 송신 경과를 얻는다.

		\~english
		Gets current dump file sending status

		\~chinese
		获得现在的转储文件传送结果。

		\~japanese
		\~
		*/
		virtual State GetState() = 0;

		/** 
		\~korean
		현재 덤프 파일 송신이 몇 바이트가 끝난는지 얻는다.

		\~english
		Gets how many bytes of dump file has been completed

		\~chinese
		获得现在转储文件传送结束多少字节。

		\~japanese
		\~
		*/
		virtual int GetSendProgress() = 0;

		/** 
		\~korean
		보내야 하는 총 송신 데이터를 얻는다.

		\~english
		Gets total sending data to be sent

		\~chinese
		获得要发送的总传送数据。

		\~japanese
		\~
		*/
		virtual int GetSendTotal() = 0;

		/** 
		\~korean
		CDumpClient 객체를 생성한다.
		\param dg IDumpClientDelegate 의 포인터

		\~english TODO:translate needed.
		Creates CDumpClient object
		\param dg

		\~chinese
		CDumpClient 生成对象
		\param dg IDumpClientDelegate的pointer。

		\~japanese
		\~
		*/
		PROUD_API static CDumpClient* Create(IDumpClientDelegate* dg);

	};
#endif 

	/**  @} */
#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif
}

//#pragma pack(pop)
