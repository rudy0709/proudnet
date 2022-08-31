/*
ProudNet HERE_SHALL_BE_EDITED_BY_BUILD_HELPER

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

using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Nettention.Proud
{
	public class GCHandleList
	{
		// C# -> C++로 넘기는 포인터(객체, 함수 등)에 대해서는 가비지 수집이 되지 않도록 하기 위해서
		// 별도의 리스트 자료구조에 GCHandle를 저장합니다.
		// 여기에 attach된 proxy, stub들의 handle이 여기 보관된다.
		private List<GCHandle> m_handleList = new List<GCHandle>();

		internal void AddHandle(GCHandle handle)
		{
			m_handleList.Add(handle);
		}

		internal void FreeAllHandle()
		{
			foreach (GCHandle handle in m_handleList)
			{
				handle.Free();
			}

			m_handleList.Clear();
		}
	}
}
