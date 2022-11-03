#pragma once

#include "../include/CriticalSect.h"
#include "../include/Singleton.h"
#include "FastMap2.h"
#include "PooledObjectAsLocalVar.h"
#include "FastList2.h"

namespace Proud
{
	class IClassObjectPool;
	class CFavoritePooledObjects_pimpl;

	/* NetCore가 자주 사용하는 object pool의 싱글톤이 netcore보다 먼저 파괴되지 않게 하며,
	ShrinkOnNeed를 호출해 주어 불필요하게 오랫동안 잔존하는 free object를 제거하는 역할을 한다.
	*/
	class CFavoritePooledObjects :public CSingleton < CFavoritePooledObjects >
	{
		CFavoritePooledObjects_pimpl* m_pimpl;

		class ISingletonHolder
		{
		public:
			virtual ~ISingletonHolder() {}
			virtual IClassObjectPool* GetSubstance() = 0;
		};

		template <typename Type>
		class SingletonHolder :public ISingletonHolder
		{
			RefCount<Type> m_holdingPtr;
		public:
			SingletonHolder() {}
			SingletonHolder(RefCount<Type> singletonRefPtr)
			{
				m_holdingPtr = singletonRefPtr;
			}
			virtual IClassObjectPool* GetSubstance()
			{
				return m_holdingPtr.get();
			}

		};

		CriticalSection m_critSec; // 이 안의 singleton list를 보호하기 위해.

		// 보장되는 파괴 순서:
		// NC,NS 등 => CFavoritePooledObjects => 각 CClassObjectPool
		// 이를 위해 value가 ISingletonPtr이며 이는 CClassObjectPool를 가리킴
		// 이 안에서도, 등록한 순서의 반대로 파괴가 되어야 한다. 따라서 map이 아니라 list임.
		typedef CFastList2<RefCount<ISingletonHolder>, int> RegisteredPoolSingletons;
		RegisteredPoolSingletons m_registeredPoolSingletons;
	public:
		void ShrinkOnNeed();

		CFavoritePooledObjects();
		~CFavoritePooledObjects();

		template <typename Type>
		void Register(RefCount<Type> singletonHolder)
		{
			CriticalSectionLock lock(m_critSec, true);
			m_registeredPoolSingletons.AddTail(RefCount<ISingletonHolder>(new SingletonHolder<Type>(singletonHolder)));
		}

		//void Unregister(IClassObjectPool* singletonHolder);
	};

	// 각 타입별 CClassObjectPool는 CFavoritePooledObjects에 등록되어야 한다. 이를 위한 공통 베이스 클래스.
	class IClassObjectPool
	{
	public:
		virtual ~IClassObjectPool() {}

		// 오랫동안 미사용되는 free list를 제거하여 메모리를 절약한다.
		// 일정 시간마다 이것을 호출되는 함수.
		virtual void ShrinkOnNeed() = 0;
	};

	// 사용자는 이것을 직접 사용하지 말 것. 위 클래스가 자동으로 이 클래스를 사용한다.
	template <typename T>
	class CClassObjectPool :public CSingleton < CClassObjectPool<T> >, public IClassObjectPool
	{
		// 굉장히 자주 액세스되지만 spin count가 있는 lock이므로 대부분 CPU time에서 끝난다. 즉 락프리에 준함.
		CriticalSection m_critSec;
    // NOTE: SpinMutex m_critSec; 를 실험해 봤지만 성능상 이익이 없다. 즉 win32 critsec은 충분히 spin lock의 역할을 해낸다.

		CObjectPool<T> m_pool;

		bool m_registered;

		// NOTE: CFavoritePooledObjects를 depend하지 않는다. 그 반대로 참조한다.
	public:
		CClassObjectPool()
		{
			m_registered = false;
		}

		~CClassObjectPool()
		{
			// 			NTTNTRACE(__FUNCTION__);
			// 			NTTNTRACE("\n");
		}

		/* NOTE: 아래 함수는 의도적으로 없다. 다시 살리지 말 것!
		CClassObjectPool보다 CFavoritePooledObjects가 먼저 파괴될 수 있기 때문이다.
		~CClassObjectPool()
		{
		CFavoritePooledObjects::Instance().Unregister(this);
		} */

		T* NewOrRecycle()
		{
			CriticalSectionLock lock(m_critSec, true);

			if (!m_registered)
			{
				// GetSharedPtr는 싱글톤 생성자에서 호출하면 무한 루프에 빠지기 때문에 불가피하게 이 함수에서 이것을 한다.
				// 이 싱글톤이 CFavoritePooledObjects보다 늦게 파괴되게 한다.
				// 또한 ShrinkOnNeed가 정기적으로 호출되게 한다.
				CFavoritePooledObjects::Instance().Register<CClassObjectPool<T> >(this->GetSharedPtr());
				m_registered = true;
			}

			return m_pool.NewOrRecycle();
		}
		void Drop(T* obj)
		{
			CriticalSectionLock lock(m_critSec, true);
			m_pool.Drop(obj);
		}
		void ShrinkOnNeed()
		{
			CriticalSectionLock lock(m_critSec, true);
			m_pool.ShrinkOnNeed();
		}
	};

	// 로컬 변수로 T를 만들되 obj-pool을 이용해서 heap access를 최소화한다.
	// 로컬 변수로만 이 객체의 인스턴스를 만들 것.
	// 배열 객체 등이 효과적. resize cost가 시간이 지나면서 사라지니까.
	// 이 클래스를 쓰면 알아서 ShrinkOnNeed도 자동으로 호출되므로 당신에게 좋을거임.
	// 주의: 배열 객체를 풀링하는 것이라면 내용물이 남아있으므로 쓰기 전에 가령 ClearAndKeepCapacity를 호출한다던지 해야. 
	// 주의: NetCore보다 나중에 파괴되는 객체에 이것을 쓰지 말것. NetCore보다 나중에 파괴됨이 보장되긴 하지만.
	template<typename T>
	class CPooledObjectAsLocalVar
	{
		T* m_obj;
	public:
		inline CPooledObjectAsLocalVar()
		{
			m_obj = CClassObjectPool<T>::Instance().NewOrRecycle();
		}
		inline ~CPooledObjectAsLocalVar()
		{
			CClassObjectPool<T>::Instance().Drop(m_obj);
		}


		// 실제 객체를 얻는다.
		inline T& Get()
		{
			return *m_obj;
		}
	};

	// 위 클래스와 유사하나, ClearAndKeepCapacity가 추가되어 있음.
	// 위 클래스의 주의사항 필독.
	template<typename ArrayType>
	class CPooledArrayObjectAsLocalVar
	{
		ArrayType* m_obj;
	public:
		inline CPooledArrayObjectAsLocalVar()
		{
			m_obj = CClassObjectPool<ArrayType>::Instance().NewOrRecycle();
			m_obj->ClearAndKeepCapacity();
		}
		inline ~CPooledArrayObjectAsLocalVar()
		{
			CClassObjectPool<ArrayType>::Instance().Drop(m_obj);
		}

		// 실제 객체를 얻는다.
		inline ArrayType& Get()
		{
			return *m_obj;
		}

		// 		inline operator ArrayType&()
		// 		{
		// 			return Get();
		// 		}
	};
}

// 로컬 변수로서 object pooling을 할 때 사용하는 매크로. 로컬 변수 선언시 쓴다.
// 가급적이면 이 매크로를 쓰자. 두번째 줄에서 &를 빠뜨리는 사고도 있었으니까.
#define POOLED_LOCAL_VAR(type, name)					\
	CPooledObjectAsLocalVar<type> name##_LV;			\
	type& name = name##_LV.Get();

// 로컬 변수로서 object pooling을 할 때 사용하는 매크로. 로컬 변수 선언시 쓴다.
// 배열 같은 동적 크기 객체를 풀링하므로, 더 효과적인 '빠른 할당/해제' 효과를 얻는다.
// 가급적이면 이 매크로를 쓰자. 두번째 줄에서 &를 빠뜨리는 사고도 있었으니까.
#define POOLED_ARRAY_LOCAL_VAR(type, name)					\
	CPooledArrayObjectAsLocalVar<type> name##_LV;			\
	type& name = name##_LV.Get();
