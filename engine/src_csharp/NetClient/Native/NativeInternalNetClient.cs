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
using System.Runtime.InteropServices;

namespace Nettention.Proud
{
	internal class NativeInternalNetClient
	{
		internal static unsafe bool SendUserMessage(NativeNetClient natvieNetClient, HostID remote, RmiContext rmiContext, ByteArray payload)
		{
			if (natvieNetClient == null)
			{
				return false;
			}

			bool ret = false;

			fixed (byte* data = payload.data)
			{
				NativeRmiContext native = null;
				try
				{
					native = ConvertToNative.RmiContextToNative(rmiContext);

					ret = natvieNetClient.SendUserMessage(remote, native, new IntPtr((void*)data), payload.Count);
				}

				finally
				{
					if (native != null)
					{
						native.Dispose();
					}
				}
				return ret;
			}
		}
	}
}
