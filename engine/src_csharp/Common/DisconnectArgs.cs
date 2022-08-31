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
	public class DisconnectArgs
	{
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
		long gracefulDisconnectTimeoutMs = NetConfig.DefaultGracefulDisconnectTimeoutMs;

		/**
		\~korean
		이것으로 Disconnect시의 대기하는 Sleep 시간을 조절할 수 있습니다.

		\~english TODO:translate needed.

		\~chinese TODO:translate needed.
		\~japanese TODO:translate needed.
		\~
		*/
		int disconnectSleepIntervalMs = (int)NetConfig.ClientHeartbeatIntervalMs;

		/**
		\~korean
		서버에서 클라이언트 연결 해제 사유를 이것을 통해 전송할 수 있습니다.

		\~english TODO:translate needed.

		\~chinese
		服务器将通过此传送客户端连接解除原因。
		\~japanese
		\~
		*/
		byte[] commentByte = new byte[0];

		public long gracefulDisconnectTimeout
		{
			get { return gracefulDisconnectTimeoutMs; }
			set { gracefulDisconnectTimeoutMs = value; }
		}

		public int disconnectSleepInterval
		{
			get { return disconnectSleepIntervalMs; }
			set { disconnectSleepIntervalMs = value; }
		}

		public byte[] comment
		{
			get { return commentByte; }
			set { commentByte = value; }
		}
	};
}
