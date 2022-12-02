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
#pragma once 

#include "FastList2.h"
#include "../include/ReceivedMessage.h"

namespace Proud
{
	// Received message list.
	class CReceivedMessageList : public CFastList2<CReceivedMessage, int>
	{
		/* NOTE: 로컬 변수로 쓰는 자주 쓰이고 있어서 heap을 불필요하게 쓰지 않게 하려고 아래와 같이 선언.
		예전에는 CLowFragMemArray<1000>이었으나, 지나치게 많은 생성자 호출 및 클라 과부하시 크래쉬가 발생하므로 CFastList 로 변경!
		*/
#ifndef SWIG
		NO_COPYABLE(CReceivedMessageList)
#endif //SWIG
	public:

		inline CReceivedMessageList() {}

		// called by CObjectPool.
		void SuspendShrink() 
		{
			// do nothing. CFastList2 itself has a object pool already.
		}
	};

}