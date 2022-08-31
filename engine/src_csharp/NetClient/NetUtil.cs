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

*/
using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.Net.Sockets;
using System.Net;

namespace Nettention.Proud
{
	public class NetUtil
	{
		/**
		\~korean
		호스트가 갖고 있는 로컬 IP 주소를 모두 얻어냅니다.

		\~english
		Gets every local IP addresses that host has.

		\~chinese
		取得主机拥有的所有本地IP地址。

		\~japanese
		\~
		*/
		public static void GetLocalIPAddresses(ref List<string> list)
		{
			if (list == null)
			{
				throw new System.ArgumentNullException("list");
			}

			System.IntPtr nativeIPAddresses = System.IntPtr.Zero;

			try
			{
				nativeIPAddresses = NativeNetUtil.LocalIPAddresses_New();

				int count = NativeNetUtil.GetLocalIPAddresseCount(nativeIPAddresses);
				for (int i = 0; i < count; ++i)
				{
					string addr = NativeNetUtil.GetLocalIPAddress(nativeIPAddresses, i);
					list.Add(addr);
				}
			}
			finally
			{
				if (nativeIPAddresses != System.IntPtr.Zero)
				{
					NativeNetUtil.LocalIPAddresses_Delete(nativeIPAddresses);
				}
			}
		}

		public static int GetIPVersionFromString(string rhs)
		{
			return NativeNetUtil.GetIPVersionFromString(rhs);
		}

		public static bool IsAddressAny(string address)
		{
			return NativeNetUtil.IsAddressAny(address);
		}

		public static bool IsAddressUnspecified(string address)
		{
			return NativeNetUtil.IsAddressUnspecified(address);
		}

		public static bool IsAddressPhysical(string address)
		{
			return NativeNetUtil.IsAddressPhysical(address);
		}

		public static bool IsAddressLoopback(string address)
		{
			return NativeNetUtil.IsAddressLoopback(address);
		}
	}
}
