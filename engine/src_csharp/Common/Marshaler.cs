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
	Message Marshaler class
	- 사용자 정의 타입을 쓸때에는 이 클래스를 상속받아서 써야 합니다.
	- PIDL컴파일러 결과물에서 사용할 marshaler입니다.

	\~english TODO:translate needed.

	\~chinese
	Message Marshaler class
	- 在使用用户定义类型时需接续此类进行使用。
	- 在PIDL编译结果物中要使用的marshaler。

	\~japanese

	\~
	*/
	public class Marshaler
	{
		public static void Read(Message msg, out byte b)
		{
			b = 0;
			msg.Read(out b);
		}

		public static void Read(Message msg, out sbyte b)
		{
			b = 0;
			msg.Read(out b);
		}

		public static void Read(Message msg, out ushort b)
		{
			b = 0;
			msg.Read(out b);
		}

		public static void Read(Message msg, out short b)
		{
			b = 0;
			msg.Read(out b);
		}

		public static void Read(Message msg, out uint b)
		{
			b = 0;
			msg.Read(out b);
		}

		public static void Read(Message msg, out int b)
		{
			b = 0;
			msg.Read(out b);
		}

		public static void Read(Message msg, out UInt64 b)
		{
			b = 0;
			msg.Read(out b);
		}

		public static void Read(Message msg, out long b)
		{
			b = 0;
			msg.Read(out b);
		}


		public static void Read(Message msg, out float b)
		{
			b = 0;
			msg.Read(out b);
		}

		public static void Read(Message msg, out double b)
		{
			b = 0;
			msg.Read(out b);
		}

		public static void Read(Message msg, out HostID b)
		{
			b = HostID.HostID_None;
			msg.Read(out b);
		}

		public static void Read(Message msg, out ByteArray b)
		{
			b = new ByteArray();
			msg.Read(out b);
		}

		public static void Read(Message msg, out HostIDArray b)
		{
			b = new HostIDArray();
			msg.Read(out b);
		}

		public static void Read(Message msg, out IPEndPoint b)
		{
			b = new IPEndPoint(0, 0);
			msg.Read(out b);
		}

		public static void Read(Message msg, out ErrorType b)
		{
			b = ErrorType.Ok;

			msg.Read(out b);
		}

		public static void Read(Message msg, out bool b)
		{
			b = false;

			msg.Read(out b);
		}

		//internal static void Read(Message msg, out LogCategory b)
		//{
		//	b = LogCategory.System;
		//	Byte read = 0;

		//	msg.Read(out read);

		//	b = (LogCategory)read;
		//}

		public static void Read(Message msg, out String b)
		{
			b = "";

			msg.Read(out b);
		}

		//public static void Read(Message msg, out FrameNumber b)
		//{
		//	b = 0;

		//	msg.Read(out b);
		//}

		public static void Read(Message msg, out System.Guid b)
		{
			msg.Read(out b);
		}

		public static void Read(Message msg, out NamedAddrPort b)
		{
			b = new NamedAddrPort();

			msg.Read(out b);
		}

		public static void Write(Message msg, byte b)
		{
			msg.Write(b);
		}

		public static void Write(Message msg, sbyte b)
		{
			msg.Write(b);
		}

		public static void Write(Message msg, ushort b)
		{
			msg.Write(b);
		}
		public static void Write(Message msg, short b)
		{
			msg.Write(b);
		}

		public static void Write(Message msg, uint b)
		{
			msg.Write(b);
		}

		public static void Write(Message msg, int b)
		{
			msg.Write(b);
		}

		public static void Write(Message msg, UInt64 b)
		{
			msg.Write(b);
		}

		public static void Write(Message msg, long b)
		{
			msg.Write(b);
		}

		public static void Write(Message msg, float b)
		{
			msg.Write(b);
		}

		public static void Write(Message msg, double b)
		{
			msg.Write(b);
		}

		public static void Write(Message msg, HostID b)
		{
			msg.Write(b);
		}

		public static void Write(Message msg, ErrorType b)
		{
			msg.Write(b);
		}

		public static void Write(Message msg, bool b)
		{
			msg.Write(b);
		}

		public static void Write(Message msg, IPEndPoint b)
		{
			msg.Write(b);
		}

		public static void Write(Message msg, ByteArray b)
		{
			msg.Write(b);
		}

		public static void Write(Message msg, HostIDArray b)
		{
			msg.Write(b);
		}

		//internal static void Write(Message msg, LogCategory b)
		//{
		//	msg.Write((Byte)b);
		//}

		public static void Write(Message msg, String b)
		{
			msg.Write(b);
		}

		//public static void Write(Message msg, FrameNumber b)
		//{
		//	msg.Write(b);
		//}

		public static void Write(Message msg, System.Guid b)
		{
			msg.Write(b);
		}

		public static void Write(Message msg, NamedAddrPort b)
		{
			msg.Write(b);
		}
	}

	/**  @} */
}
