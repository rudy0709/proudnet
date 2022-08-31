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
using System.Net;

namespace Nettention.Proud
{
	/** \addtogroup net_group
	*  @{
	*/


	/**
	\~korean
	문자열과 ushort port로 구성된 클래스 입니다.
	- 문자열에는 111.222.111.222 형태 또는 game.aaa.com 형식의 이름이 들어갈 수 있다.

	\~english
	It's similar to AddrPort but string can be inserted in m_addr.
	- At string, either 111.222.111.222 format or game.aaa.com format can be added.

	\~chinese
	是以文字列和ushort port所构成的类。
	- 在文字列当中可以进入111.222.111.222形态或game.aaa.com形式的名。

	\~japanese

	\~
	*/
	public class NamedAddrPort
	{
		/**
		\~korean
		호스트 이름
		- 문자열에는 111.222.111.222 형태 또는 game.aaa.com 형식의 이름이 들어갈 수 있다.

		\~english
		Host name
		- At string, either 111.222.111.222 format or game.aaa.com format can be added.

		\~chinese
		主机名称
		- 在文字列当中可以进入111.222.111.222形态或game.aaa.com形式的名。

		\~japanese

		\~
		*/
		public String addr;
		/**
		\~korean
		포트 값

		\~english
		Port Value

		\~chinese
		端口值

		\~japanese

		\~
		*/
		public ushort port;

		/**
		\~korean
		빈 주소

		\~english
		Empty Address

		\~chinese
		空地址

		\~japanese

		\~
		*/
		public static readonly NamedAddrPort Unassigned = new NamedAddrPort("", 0xffff);

		/**
		\~korean
		기본 생성자

		\~english TODO:translate needed.

		\~chinese
		基本生成者

		\~japanese

		\~
		*/
		public NamedAddrPort()
		{
			this.addr = "";
			this.port = 0xffff;
		}

		/**
		\~korean
		문자열과 포트를 받아 세팅하는 생성자.

		\~english TODO:translate needed.

		\~chinese
		接收文字列和端口后进行设置的生成者。

		\~japanese

		\~
		*/
		public NamedAddrPort(String addr, ushort port)
		{
			this.addr = addr;
			this.port = port;
		}

		/**
		\~korean
		IPEndPoint를 받아 생성하는 생성자

		\~english TODO:translate needed.

		\~chinese
		接收IPEndPoint后生成的生成者。

		\~japanese

		\~
		*/
		public NamedAddrPort(IPEndPoint addrPort)
		{
			this.addr = addrPort.Address.ToString();
			this.port = (ushort)addrPort.Port;
		}

		/**
		\~korean
		IPEndPoint로 변환하여 리턴합니다.

		\~english TODO:translate needed.

		\~chinese
		变换为IPEndPoint后进行Return。

		\~japanese

		\~
		*/
		public IPEndPoint ToAddrPort()
		{
			return new IPEndPoint(IPAddress.Parse(addr), port);
		}

		/**
		\~korean
		String형태로 리턴 합니다. "addr:port"

		\~english
		Empty Address

		\~chinese
		Return为String形态。"addr:port"

		\~japanese

		\~
		*/
		public override String ToString()
		{
			return String.Format("{0}:{1}", addr, port);
		}

		/**
		\~korean
		브로드캐스트 주소도 아니고, null 주소도 아닌, 1개 호스트의 1개 포트를 가리키는 정상적인 주소인가?

		\~english
		Is it correct address that point 1 port of 1 host instead of broadcast address, null address?

		\~chinese
		是一个既不是宽带地址，也不是null地址的，指向一个主机的一个端口的正常地址吗？

		\~japanese

		*/
		public bool IsUnicastEndpoint()
		{
			addr = addr.Trim();

			if (port == 0 /*|| port == 0xffff*/)
				return false;

			if (addr == "" || addr == "0.0.0.0" || addr == "255.255.255.255")
				return false;

			return true;
		}

	}

	/**  @}  */
}
