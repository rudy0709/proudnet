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

#include "BasicTypes.h"
#include "Exception.h"
#include "CriticalSect.h"
#include "atomic.h"
#include "Ptr.h"

#if defined __MARMALADE__ || defined __linux__
#include <unistd.h>
#endif

#include "PnThread.h"
#include "SysTime.h"


//#pragma pack(push,8)

#if (_MSC_VER>=1400)
#pragma managed(push, off)
#endif


namespace Proud
{
	extern const uint32_t SingletonTryWaitMilisec;

	/** \addtogroup util_group
	*  @{
	*/

	/**
	\~korean
	JIT 인스턴싱만 thread safe하게 하는 singleton template
	- JIT 인스턴싱 과정만 CS lock을 해서 thread safe하게 하고, 생성된 이후부터는 thread unsafe하게 한다.
	- 생성만 thread 동기화하고 이후부터는 불필요한 경우, 예컨대 CItemTemplate 등에서 이 객체를 써야 드문 빈도의 서버 다운을
	예방 가능.

	일반적 용도
	- 이 클래스의 파생 클래스를 만든다. T에는 파생 클래스의 이름을 넣는다.
	- Instance()를 이용해서 싱글톤의 레퍼런스를 얻는다. Instance()를 최초로 호출하면 객체가 인스턴스화된다.
	- 싱글톤간 파괴 순서를 제어하려면 싱글톤의 생성자에서 의존 대상 싱글톤을 한번 접근해주면 된다.

	사용예
	\code
	class A:public CSingleton<A>
	{
	public:
		void Foo();
	};

	void Aoo()
	{
		A::Instance().Foo();
	}
	\endcode

	DLL과 EXE가 같은 Singleton을 쓰고자 하는 경우
	- CSingleton을 DLL이 제공하고 EXE가 그것을 쓰고자 하는 경우에는 본 클래스를 다음과 같이 구현해야 한다. 안그러면 EXE가 쓰는 singleton과
	DLL이 쓰는 singleton이 서로 다른 인스턴스로 존재하게 된다.
	\code
	class Goofie:protected CSingleton<Goofie> // protected로 선언해서 Instance를 직접 접근하지 못하게 한다.
	{
	public:
		__declspec(dllexport) Goofie& Instance() // CSingleton.Instance()를 오버라이드하되 DLL에서 export되는 함수로 만든다.
		{
			return __super::Instance(); // DLL 모듈 메모리 공간 내에 만들어진 instance를 리턴한다.
		}
	};
	\endcode

	\~english
	Singleton template that makes only JIT instancing to be thread safe.
	- Only CS locks JIT instancing process making it thread safe and makes thread unsafe after created.
	- Only thread synchronized then not needed afterwards, e.g. Server down with little frequency can be prevented by using this object at CItemTemplate ans so on.

	General usage
	- Creates derivative class to this class. The name of the derivative class is to enter at T.
	- Gets reference of Singleton using Instance(). Object is instanced when Instance() is called for the first time.
	- To control the destruction order of Singletons, constructor of Signleton should approach to dependent target Singleton just for once.

	Usage example
	\code
	class A:public CSingleton<A>
	{
	public:
		void Foo();
	};

	void Aoo()
	{
		A::Instance().Foo();
	}
	\endcode

	If DLL and EXE to use same Singleton,
	- This class must be realized as followings for the case that DLL provides CSingleton and EXE uses it. Otherwise, the singleton used by EXE and the singleton used by DLL will exist as different instance.
	\code
	class Goofie:protected CSingleton<Goofie> // Declared as protected to prevent direct access of Instance
	{
	public:
		__declspec(dllexport) Goofie& Instance() // Override CSingleton.Instance() but make it as a function exported from DLL.
		{
			return __super::Instance(); // Returns the instance that is created in DLL module memory space
		}
	};
	\endcode

	\~chinese
	只把JIT instancing进行成thread safe的singleton template。
	- 只把JIT instancing过程CS lock以后使其thread safe，生成以后开始让它thread unsafe。
	- 只同步生成thread，之后不必要的情况，例如要在 CItemTemplate%等使用此对象才能预防很少频率的服务器死机。

	一般用途
	- 创建此类的派生类。往T里输入派生类的名字。
	- 利用Instance()获得singleton的reference。初始呼叫Instance()的话对象会instance化。
	- 想要控制singleton之间的破坏顺序，要在singleton的生成者里接近一次依存对象的singleton。

	使用例
	\code
	class A:public CSingleton<A>
	{
	public:
		void Foo();
	};

	void Aoo()
	{
		A::Instance().Foo();
	}
	\endcode

	DLL 和EXE要使用相同的singleton的时候
	- DLL要提供 CSingleton%，EXE 要使用它的时候要把此类如下体现。否则EXE使用的singleton和DLL使用的singleton会存在成不同的instance。
	\code
	class Goofie:protected CSingleton<Goofie> // 用protected宣告，不让Instance直接接近。
	{
	public:
		__declspec(dllexport) Goofie& Instance() // 覆盖 CSingleton.Instance()，但是制造成在DLL能export的函数。
		{
			return __super::Instance(); // 返回在DLL模块内存空间内制造的instance。
		}
	};
	\endcode

	\~japanese

	\~


	\~korean

	만약 싱글톤끼리 파괴 순서를 보장해야 하는 경우, shared pointer 객체를 얻어서 보관함으로써 파괴 순서를 정의할 수 있습니다.

	\~english

	 You can define destruction order of singletons by keeping shared pointer object from singleton.

	\~chinese

	如果要在单例模式之间保障破坏顺序，可以获取并保存shared pointer对象以定义破坏顺序。

	\~japanese

	\~



	\~korean

	예시 코드

	\~english

	Example code

	\~chinese

	编码例。

	\~japanese

	\~


	\~korean

	\code
	// 싱글톤 클래스입니다.
	class MySingleton:public CSingleton<MySingleton>
	{
		...
		Something();
	};

	// 싱글톤을 접근하는 사용자입니다.
	class MyGoo
	{
		// MyGoo 인스턴스가 존재하는 동안 싱글톤의 생존을 보장하려면 이 멤버를 갖고 있어야 합니다.
		// 물론, 싱글톤 접근을 위해 이 멤버를 사용하십시오.
		MySingleton::PtrType m_mySingleton;

		// 생성자 메서드
		User()
		{
			// 싱글톤 참조 카운트를 증가시키면서, 싱글톤의 생존을 보장시킵니다.
			m_mySingleton = MySingleton::GetInstanceAccessor();
		}

		Foo()
		{
			...
			// 싱글톤을 접근하려면 이미 갖고 있는 싱글톤 스마트포인터 객체를 통해 접근합니다.
			m_mySingleton->Something();
		}
	}

	\endcode

	\~english

	\code
	// Singleton Class
	class MySingleton:public CSingleton<MySingleton>
	{
		...
		Something();
	};

	// It is the user that has access to Singleton.
	class MyGoo
	{
		// While MyGoo instance exists, it must have this member in order to assure Singleton’s survival.
		// Use this member for Singleton access, of course.
		MySingleton::PtrType m_mySingleton;

		// constructor
		User()
		{
			// It guarantee survival of singleton with increasing singleton refer count.
			m_mySingleton = MySingleton::GetInstanceAccessor();
		}

		Foo()
		{
			...
			// To access singleton, use singleton smartpointer object that already has.
			// do not use MySingleton::Instance(), but use this member variable.
			m_mySingleton->Something();
		}
	}

	\endcode

	\~chinese

	\code
	// singleton类。
	class MySingleton:public CSingleton<MySingleton>
	{
		...
		Something();
	};

	// 接近singleton的使用者。
	class MyGoo
	{
		// MyGoo的instance存在期间，想保障singleton生存的话，要拥有此成员。
		// 为了singleton接近，使用此成员吧。
		MySingleton::PtrType m_mySingleton;

		// 生成者方法
		User()
		{
			// 想增加singleton参照count的话，可以保障singleton的生存。
			m_mySingleton = MySingleton::GetInstanceAccessor();
		}

		Foo()
		{
			...
			// 想接近singleton的话，通过已经拥有的singleton smartpointer对象来接近。
			m_mySingleton->Something();
		}
	}
	\endcode

	\~japanese
	\~


	*/
	template<typename T>
	class CSingleton
	{
	public:
		typedef RefCount<T> PtrType;

		// shared ptr로서의 객체를 가져온다.
		_Noinline static PtrType& GetSharedPtr()  // 리턴값이 &인 이유는, 사용자가 weak ptr로 가져오는 경우 쓸데없는 복사 수행을 막기 위해서다.
		{
			// 동시에 여러 스레드에서 값을 얻으려고 할 수 있다.
			// 이때 단 한 스레드에서만 생성자를 호출하고, 그 동안 다른 스레드는 생성자 실행 완료를 기다려야 한다.
			volatile static int state = 0; //0:아직 Instance가 호출된 적 없음, 1: 생성자 실행중,2: 생성자 실행 완료
			static PtrType instancePtr;

			if (AtomicCompareAndSwap32(2, 2, &state) == 2)
			{
				// 이미 인스턴스가 잘 생성된 후다. 그냥 값을 가져오자.
				return instancePtr;
			}

			if (AtomicCompareAndSwap32(0, 1, &state) == 0)
			{
				// 한번도 Instance가 실행된 적이 없었다. 인스턴스를 생성하자.
				instancePtr = RefCount<T>(new T);

				// 생성 끝났다. 상태값을 바꾸자.
				AtomicCompareAndSwap32(1, 2, &state);
				return instancePtr;
			}

			// 다른 스레드에서 인스턴스를 생성중이다. 완료될 때까지 기다리자.
			while (AtomicCompareAndSwap32(2, 2, &state) != 2)
			{
				// device time이 있는 생성자 함수를 위해 0이 아니라 1.
				Proud::Sleep(SingletonTryWaitMilisec);
			}

			return instancePtr;
		}

		// weak pointer로서의 싱글톤 객체를 가져온다.
		inline static T& Instance()
		{
			PtrType& t = GetSharedPtr(); // 좌항이 &이므로 복사는 안 일어난다.
			return *t;
		}

		// 사용자가 이것의 변수를 멤버로 가지는 동안 사용자의 객체가 파괴되기 전에는 이 싱글톤이 파괴 안됨을 보장한다.
		class Holder
		{
			PtrType m_sharedPtr;
		public:
			Holder()
			{
				m_sharedPtr = CSingleton<T>::Instance().GetSharedPtr();
			}
			// 이 함수가 있어야.
			// 컴파일러가 알아서 파생 클래스의 디폴트 파괴자를 만들기는 하겠지만, 확인 사살을 위해.
			virtual ~Holder() {}
		};

	};

	/**  @} */
}

#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif

//#pragma pack(pop)
