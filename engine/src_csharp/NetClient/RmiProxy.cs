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
//using System.Linq;
using System.Text;

namespace Nettention.Proud
{
	/**
	\~korean
	PIDL 컴파일러가 생성한 Proxy 클래스의 베이스 클래스

	주의 사항
	- 이 클래스를 유저가 직접 구현하지 말 것. PIDL 컴파일러에서 구현한 것을 쓰도록 해야 한다.

	\~english
	Base class of Proxy class created by PIDL compiler

	Note
	- User must not create this class. Must be realized by PIDL compiler.

	\~chinese
	PIDL编译所生成的Proxy类的基础类。

	注意事项
	- 用户不可直接体现此类。要使用在PIDL编译中体现的类。

	\~japanese

	\~
	*/
	public abstract class RmiProxy
	{
		internal NativeInternalProxy m_native_Internal = null;

		public IRmiHost m_core = null;

		public IRmiHost core
		{
			get { return m_core; }
		}

		public bool internalUse = false;

		public bool enableNotifySendByProxy = true;

		public RmiProxy()
		{
			try
			{
				m_native_Internal = new NativeInternalProxy(this);
			}
			catch (System.TypeInitializationException ex)
			{
				// c++ ProudNetServerPlugin.dll, ProudNetClientPlugin.dll 파일이 작업 경로에 없을 때
				throw new System.Exception(ClientNativeExceptionString.TypeInitializationExceptionString);
			}
		}

		~RmiProxy()
		{
		}

		public NativeInternalProxy GetNativeInternalProxy()
		{
			return m_native_Internal;
		}

		/**
		\~korean
		RMI ID 목록 객체

		\~english
		RMI ID List object

		\~chinese
		RMI ID目录客体

		\~japanese

		\~
		*/
		public RmiID[] RmiIDList
		{
			get
			{
				return GetRmiIDList();
			}
		}

		public abstract RmiID[] GetRmiIDList();

		public virtual int GetRmiIDListCount
		{
			get;
			set;
		}

		/**
		\~korean
		메시지 송신을 위해 RMI를 호출할 때(즉 proxy에서 호출하기)마다 이 함수가 callback됩니다.
		프로필러나 RMI 사용 로그를 남기고자 할 때 이 함수를 사용하시면 됩니다. 자세한 내용은 \ref monitor_rmi_proxy  를 참고하십시오.
		- 수신자가 여럿인 경우 여러번 호출됩니다.
		- 기본 함수는 아무것도 하지 않습니다.

		또한 송신 직전에 RmiContext를 최종 수정할 수 있는 기회를 제공합니다.
		(기회를 주는 이유: 송신시 잘못된 값이 있으면 경고와 함께 최종 수정을 위함입니다.
		오픈베타시점에서는 문제 분석과 해결을 동시에 해야 하니까요. )
		\param sendTo 수신자
		\param summary 보내는 RMI 메시지의 요약 정보
		\param rmiContext 사용자가 호출한 RmiContext 값입니다.

		\~english
		This function is called back every time RMI is called for message send(e.g. calling from proxy).
		This function is used when there is a need to leave porfiler or RMI use log. Please refer to \ref monitor_rmi_proxy.
		- Multiple reciever cause multiple calling.
		- Base function does not do anything.

		Plus, this provides a cance to finally modify RmiContext before sending.
		(Why the last chance is given: for the case there is an incorrect value and to notify, warn and modify finally. During OBT, we need to do prob analysis and solving at the same time.)
		\param sendTo reciever
		\param summary gist of RMI message to be sent
		\param rmiContext RmiContext value called by user

		\~chinese
		每次为发送信息呼出 RMI时(即在 proxy进行呼出时)此函数会被callback。
		如果想要保留情报或保留RMI的使用Log，可以使用此函数。详细内容请参考\ref monitor_rmi_proxy%。
		-  如果收信者为多数，将进行多次呼出。
		-  基本函数不进行任何操作。

		在发送信息前，提供可最终修改 RmiContext%的机会。
		(提供机会的理由：如果在发送信息时存在错误值，则可以在发出警报的同时进行最终修改。因为在进行公测时要同时进行问题的分析和解决。)

		\param sendTo 收信者
		\param summary 发送的RMI信息总结情报。
		\param rmiContext 用户呼出的 RmiContext%值。

		\~japanese

		\~
		*/
		public virtual void NotifySendByProxy(HostID[] remotes, MessageSummary summary, RmiContext rmiContext, Message msg) { }

		/**
		\~korean
		내부 함수입니다. 사용자는 이 함수를 오버라이드하지 마십시오.

		\~english
		User must not override this function.

		\~chinese
		是内部函数，用户不要Override此函数。

		\~japanese

		\~
		*/
		public virtual bool RmiSend(HostID[] remotes, RmiContext rmiContext, Message msg, String rmiName, RmiID rmiID)
		{
			if (null == this.core)
			{
				throw new NullReferenceException("ProudNet RMI Proxy is not attached yet!");
			}

			bool ret = NativeInternalProxy.RmiSend(m_native_Internal, remotes, rmiContext, msg, rmiName, rmiID);

			if (internalUse == false)
			{
				MessageSummary msgSumm = new MessageSummary();
				msgSumm.payloadLength = sizeof(Byte) + msg.Data.Count;
				msgSumm.rmiID = rmiID;
				msgSumm.rmiName = rmiName;
				msgSumm.encryptMode = rmiContext.encryptMode;
				msgSumm.compressMode = rmiContext.compressMode;

				if (enableNotifySendByProxy)
				{
					NotifySendByProxy(remotes, msgSumm, rmiContext, msg);
				}
			}

			return ret;
		}

	}

	/**	@} */
}
