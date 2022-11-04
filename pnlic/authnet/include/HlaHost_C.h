/*
ProudNet v1


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

#include "HlaEntity_C.h"
#include "HlaDelagate_Common.h"

namespace Proud
{
	class CHlaEntityManagerBase_C;
	class CriticalSection;
	class IHlaDelegate_C;

	/**
	\~korean
	HLA 세션 클라이언트 메서드 인터페이스입니다.

	\~english TODO:translate needed.

	\~chinese
	是HLA session玩家方法界面。

	\~japanese
	\~
	*/
	class IHlaHost_C
	{
	public:
		virtual ~IHlaHost_C() {};

		/**
		\~korean
		HLA entity class를 등록합니다.
		HLA entity class는 HLA compiler output이어야 합니다.

		\~english TODO:translate needed.

		\~chinese
		登录HLA entity class。
		HLA entity class得是HLA compiler output。

		\~japanese
		\~
		*/
		virtual void HlaAttachEntityTypes(CHlaEntityManagerBase_C* entityManager) = 0;

		/**
		\~korean
		이 모듈에 의해 콜백되는 메서드들을 구현한 객체를 받아들입니다.

		\~english TODO:translate needed.

		\~chinese
		在接收体现被此模块的回拨方法的对象。

		\~japanese
		\~
		*/
 		virtual void HlaSetDelegate(IHlaDelegate_C* dg) = 0;

		/**
		\~korean
		사용자는 이 함수를 일정 시간마다 콜 해야 합니다.

		\~english TODO:translate needed.

		\~chinese
		用户要每隔一段一定时间回拨此函数。

		\~japanese
		\~
		*/
		virtual void HlaFrameMove() = 0;

	};

	/**
	\~korean
	HLA 세션 클라이언트에 의해 콜백되는 인터페이스입니다.

	참고
	- C++ 이외 버전에서는 본 메서드는 delegate callback 형태일 수 있습니다.

	\~english TODO:translate needed.

	\~chinese
	被HLA session玩家回拨的界面。

	参考
	- 在C++以外的版本，此方法可能是delegate callback形式。

	\~japanese
	\~
	*/
	class IHlaDelegate_C:public IHlaDelegate_Common
	{
	public:
		virtual ~IHlaDelegate_C() {}

		/**
		\~korean
		HLA entity가 생성되면 이 메서드가 콜백됩니다.
		HlaOnLockCriticalSection()에 의해 lock을 시행한 상태에서 호출됩니다.

		\~english TODO:translate needed.

		\~chinese
		生成HLA entity的话此方法会回拨。
		被HlaOnLockCriticalSection()执行lock的状态下被呼出。

		\~japanese
		\~
		*/
		virtual void HlaOnEntityAppear( CHlaEntity_C* entity ) = 0;

		/**
		\~korean
		HLA entity가 파괴되면 이 메서드가 콜백됩니다.
		당신은 이 함수에서 entity를 다음과 같이 파괴해야 합니다. 안그러면 memory leak으로 이어집니다.
		(C# 버전에서는 파괴를 할 필요가 없습니다. garbage collector에 의해 제거되기 때문입니다.)
		\code
			delete entity;
		\endcode
		HlaOnLockCriticalSection()에 의해 lock을 시행한 상태에서 호출됩니다.

		\~english TODO:translate needed.

		\~chinese
		HLA entity 被破坏的话此方法会回拨。
		你要在此函数如下破坏entity。否则会造成memory leak。
		（在C\#版本没有必要破坏。因为会被garbage collector删除）
		\code
			delete entity;
		\endcode
		被HlaOnLockCriticalSection()执行lock的状态下被呼出。

		\~japanese
		\~
		*/
		virtual void HlaOnEntityDisappear( CHlaEntity_C* entity ) = 0;
	};
}
