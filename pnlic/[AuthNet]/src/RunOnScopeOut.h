#pragma once 

namespace Proud
{

#ifdef SUPPORTS_LAMBDA_EXPRESSION


	/* 로컬 변수가 스코프를 벗어날 때 자동으로 사용자가 지정한 람다식이 실행되게 해주는 클래스.	

	이것을 scope exit guard라고 부르지만, 용어가 어려워서 그냥 이렇게 이름 쉽게 ^^
	boost.ScopeExit가 한 예이다.

	사용법:
	1. 로컬 변수를 생성하되, RunOnScopeOut()을 이용한다. 로컬 변수 타입으로 auto keyword를 쓰자. 
	   RunOnScopeOut()의 인자로 람다식을 넣자.
	2. 로컬 변수가 파괴되면 람다식이 실행된다. 그 전에 람다식을 실행하고자 하면, Run()을 실행하자.
	   한번 실행하면 또 실행되지 않는다.

	사용예:
	{
		RunScopeOut dd([&]() { ... });
		... 
	}
	// 위 람다식이 막 실행된 상태가 된다.

	*/
	class RunOnScopeOut
	{
		// 파괴자가 호출된 적 있는지
		bool m_called;

		/* std.function을 안쓰면 assign이 없이 리턴값을 할당해주는 묘기[1]가 필요한데 괜히 찜찜하다. 

		T func1()
		{	
			return T(...); // 의외로, T에 대한 copy 함수가 호출되지 않는다.
		}
		*/
		typedef std::function<void()> Lambda;
		Lambda m_lambda;
	public:
		RunOnScopeOut(const Lambda& lambda)
			: m_lambda(lambda)
		{
			m_called = false;
		}

		// 이 함수를 호출하면 스코프 아웃이 안되어 있더라도 사용자 지정 루틴이 실행된다.
		// 이것이 실행되고 나면 스코프 아웃 시 아무것도 안한다.
		void Run()
		{
			if (!m_called)
			{
				m_called = true;
				m_lambda();
			}
		}

		~RunOnScopeOut()
		{
			Run();
		}

		NO_COPYABLE(RunOnScopeOut)
	};


#endif // SUPPORTS_LAMBDA_EXPRESSION


}