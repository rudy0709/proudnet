/*
ProudNet v1.x.x


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

#include "CallbackContext.h"
#include <functional>

#ifdef SUPPORTS_LAMBDA_EXPRESSION

namespace Proud
{
	// 파라메터 갯수가 몇개든 상관없이 처리 가능한 일반적인 람다식 객체의 베이스 클래스.
	// 내부적으로 std::function이 없으므로, 항상 고정 크기 클래스임을 보장한다.
	// 따라서 미리 빌드된 라이브러리에서의 이식성을 보장한다.
	// RETURN에는 람다식의 리턴 타입을 넣자. void도 가능.
	template<typename RETURN>
	class LambdaBase_Param0
	{
	public:
		virtual RETURN Run() = 0;
		virtual ~LambdaBase_Param0() {}
	};

	// 파라메터 0개짜리 실체
	template<typename RETURN>
	class Lambda_Param0 :public LambdaBase_Param0 < RETURN >
	{
		// thread and context
		std::function<RETURN()> m_lambda;
	public:
		Lambda_Param0(const std::function<RETURN()> &lambda) { m_lambda = lambda; }
		void Run() { m_lambda(); }
	};

	template<typename RETURN, typename PARAM1>
	class LambdaBase_Param1
	{
	public:
		virtual RETURN Run(PARAM1 param1) = 0;
		virtual ~LambdaBase_Param1() {}
	};

	template<typename RETURN, typename PARAM1>
	class Lambda_Param1 :public LambdaBase_Param1 < RETURN,PARAM1 >
	{
		// thread and context
		std::function<RETURN(PARAM1)> m_lambda;
	public:
		Lambda_Param1(const std::function<RETURN(PARAM1)> &lambda) { m_lambda = lambda; }
		void Run(PARAM1 param1) { m_lambda(param1); }
	};
}

#endif
