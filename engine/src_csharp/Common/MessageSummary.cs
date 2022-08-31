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
	전송되는 메시지의 요약 정보입니다. 예를 들어 RMI 로그를 추적할 때, 보낸 메시지가 어떤 형식인지 등을 요약하여 사용자에게 제공합니다.

	\~english
	This is a quick summary of messages being sent. For instance, when tracking RMI log, this shows a summary of the sent messages to user such as which type they are in.

	\~chinese
	传送信息的摘要情报，例如在追踪RMI Log时，总结所发送的信息是何种形式等内容后，将其摘要提供给用户。

	\~japanese

	\~
	*/
	public class MessageSummary
	{
		/**
		\~korean
		메시지의 크기입니다. ProudNet의 메시지 계층 이하의 계층은 포함하지 않습니다.

		\~english
		Size of message. This doesn't include other classes than the ProudNet message class.

		\~chinese
		是信息的大小。不包含ProudNet的信息阶层以下的阶层。

		\~japanese

		\~
		*/
		public int payloadLength = 0;

		/**
		\~korean
		이 메시지가 RMI 메시지인 경우 RMI의 ID값입니다.

		\~english
		RMI ID if a message is RMI.

		\~chinese
		此信息为RMI信息时RMI的ID值。

		\~japanese

		\~
		*/
		public RmiID rmiID = RmiID.RmiID_None;

		/**
		\~korean
		이 메시지가 RMI 메시지인 경우 RMI의 함수명입니다.

		\~english
		RMI function name if a message is RMI.

		\~chinese
		此信息为RMI信息时RMI的函数名。

		\~japanese

		\~
		*/
		public String rmiName = "";

		/**
		\~korean
		이 메시지에 동원된 암호화 기법입니다.

		\~english
		The encrypted method of a message.

		\~chinese
		在此信息当中被调动的加密技术。

		\~japanese

		\~
		*/
		public EncryptMode encryptMode = EncryptMode.EM_None;

		/**
		\~korean
		이 메시지에 동원된 압축방식입니다.

		\~english
		The compressed method of a message.

		\~chinese
		在此信息当中被调动的压缩方法。

		\~japanese

		\~
		*/
		public CompressMode compressMode = CompressMode.CM_None;
	}

	/**
	\~korean
	IRmiStub::BeforeRmiInvocation에서 수신 메시지의 요약 정보입니다. 예를 들어 RMI 로그를 추적할 때, 보낸 메시지가 어떤 형식인지 등을 요약하여 사용자에게 제공합니다.

	\~english
	Summary of received message at IRmiStub::BeforeRmiInvocation. For exmaple, it provide summary to user such as type of message when you tracking RMI log.

	\~chinese
	在 IRmiStub::BeforeRmiInvocation%中的收信信息摘要情报。例如在追踪RMI Log时，总结所发送的信息是何种形式等内容后，将其摘要提供给用户。

	\~japanese

	\~
	*/
	public class BeforeRmiSummary
	{
		/**
		\~korean
		이 메시지가 RMI 메시지인 경우 RMI의 ID값입니다.

		\~english
		RMI ID if a message is RMI.

		\~chinese
		此信息为RMI信息时RMI的ID值。

		\~japanese

		\~
		*/
		public RmiID rmiID = RmiID.RmiID_None;

		/**
		\~korean
		이 메시지가 RMI 메시지인 경우 RMI의 함수명입니다.

		\~english
		RMI function name if a message is RMI.

		\~chinese
		此信息为RMI信息时RMI的函数名。

		\~japanese

		\~
		*/
		public String rmiName = "";

		/**
		\~korean
		보낸 Host의 HostID 입니다.

		\~english
		HostID of Host who sent

		\~chinese
		所发送Host的HostID。

		\~japanese

		\~
		*/
		public HostID hostID = HostID.HostID_None;

		/**
		\~korean
		사용자가 지정한 hostTag의 레터런스입니다.

		\~english
		Reference of user defined hostTag.

		\~chinese
		用户所指定的hostTag的Reference。

		\~japanese

		\~
		*/
		public Object hostTag = null;
	}

	/**
	\~korean
	IRmiStub::AfterRmiInvocation 에서 수신 메시지의 요약 정보입니다. 예를 들어 RMI 로그를 추적할 때, 보낸 메시지가 어떤 형식인지 등을 요약하여 사용자에게 제공합니다.

	\~english
	Summary of received message at IRmiStub::AfterRmiInvocation. For exmaple, it provide summary to user such as type of message when you tracking RMI log.

	\~chinese
	在 IRmiStub::AfterRmiInvocation%当中收信信息的摘要情报。 例如在追踪RMI Log时，总结所发送的信息是何种形式等内容后，将其摘要提供给用户。

	\~japanese

	\~
	*/
	public class AfterRmiSummary
	{
		/**
		\~korean
		이 메시지가 RMI 메시지인 경우 RMI의 ID값입니다.

		\~english
		RMI ID if a message is RMI.

		\~chinese
		此信息为RMI信息时RMI的ID值。

		\~japanese

		\~
		*/
		public RmiID rmiID = RmiID.RmiID_None;

		/**
		\~korean
		이 메시지가 RMI 메시지인 경우 RMI의 함수명입니다.

		\~english
		RMI function name if a message is RMI.

		\~chinese
		此信息为RMI信息时RMI的函数名。

		\~japanese

		\~
		*/
		public String rmiName = "";

		/**
		\~korean
		보낸 Host의 HostID 입니다.

		\~english
		HostID of Host who sent

		\~chinese
		所发送Host的HostID。

		\~japanese

		\~
		*/
		public HostID hostID = HostID.HostID_None;

		/**
		\~korean
		사용자가 지정한 hostTag의 포인터입니다.

		\~english
		Pointer of user defined hostTag.

		\~chinese
		用户所指定的 hostTag的指针。

		\~japanese

		\~
		*/
		public Object hostTag = null;

		/**
		\~korean
		수신 RMI함수가 처리되는데 걸리는 시간

		\~english
		Time to process received RMI function.

		\~chinese
		处理收信RMI函数时的所需时间

		\~japanese

		\~
		*/
		public long elapsedTime = 0;
	}

	/**  @} */
}
