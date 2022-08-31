#pragma once

#include "../include/CriticalSect.h"
#include "../include/Singleton.h"
#include "FastMap2.h"
#include "PooledObjectAsLocalVar.h"
#include "FastList2.h"
#include "SpinLock.h"


namespace Proud
{
	// 전방선언
	template <typename T>
	class CClassObjectPool;

	// 주의: 이 함수를 실행하는 동안 CClassObjectPool 싱글톤의 비파괴 보장이 되도록 할 것.
	template<typename T>
	void GetClassObjectPoolInDll(CClassObjectPool<T>** output);

	// 사용자는 이것을 직접 사용하지 말 것. 위 클래스가 자동으로 이 클래스를 사용한다.
	template <typename T>
	class CClassObjectPool :public DllSingleton<CClassObjectPool<T>>
	{
	public: // SubPool_ShrinkOnNeed_Functor에서 액세스하기 위해
		// contention을 줄이기 위해, CPU 갯수만큼의 object pool을 하위로 둔다.
		struct SubPool
		{
			/* 굉장히 자주 액세스되지만 spin count가 있는 lock이므로 대부분 CPU time에서 끝난다. 즉 락프리에 준함.
			critical section lock은 CAS보다 2배 차이. 큰 차이는 아니지만, 여기는 사용 빈도가 극도로 높기 때문에,
			sub pool을 두어 이렇게 하는 것이 차라리 낫다. contention이 거의 발생하지 않는다.

			그러나 CObjectPool 안에서 낮은 확률로나마 malloc or free를 하고 있다. 가령 free list가 없거나 ShrinkOnNeed에 의해서.
			이런 경우 spin lock은 부적절하다. 짧은 시간안에의 완료가 not primising이기 때문이다.
			따라서 spin lock을 쓰지 말도록 하자. */
			CriticalSection m_critSec;

			CObjectPool<T> m_pool;
		};

		// 최대 8개의 스레드가 돌테니, 이를 위한 sub object pool 배열.
		SubPool* m_subPools;
		// 위 배열의 크기. 액세스 속도에 민감하므로 non-template type.
		int m_subPoolCount;

		// GetAnySubPool을 몇번째 항목부터 탐색 시작을 할 것인지?
		// 사실상 0~SubPool 갯수 사이의 랜덤값이다.
		int m_lastSubPoolSelection;

		class ShrinkOnNeed_Functor
		{
		public:
			SubPool* m_subPools[MaxCpuCoreCount];
			int m_subPoolCount;

			ShrinkOnNeed_Functor(CClassObjectPool<T>* pThis)
			{
				// SubPool의 주소들을 모은 배열을 만든다.
				m_subPoolCount = pThis->m_subPoolCount;
				for (int i = 0; i < m_subPoolCount; i++)
				{
					m_subPools[i] = &pThis->m_subPools[i];
				}
			}

			bool DoElementAndUnlock(SubPool* subPool, CriticalSectionLock& objLock)
			{
				subPool->m_pool.ShrinkOnNeed();
				objLock.Unlock();
				return true;
			}

			CriticalSection* GetCriticalSection(SubPool* subPool)
			{
				return &subPool->m_critSec;
			}
		};

	private:

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4172)
#else
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wall"
#endif
		// 대략적인 로컬스택 주소를 리턴한다. 저질 랜덤값을 빠르게 리턴하는 용도로 쓴다.
		inline intptr_t getApproximateStackPointer()
		{
			// 그냥 로컬 변수 하나 만들고 그것의 주소값을 리턴하면 된다.
			int a;
			return (intptr_t)&a;
		}
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma clang diagnostic pop
#endif

		// NOTE: CFavoritePooledObjects를 depend하지 않는다. 그 반대로 참조한다.

		// +0
		inline int getCurrentAsNextSubPoolIndex()
		{
			return m_lastSubPoolSelection;
		}

		// +1
		// contention 확률을 줄여서 CPU 사용량을 줄인다.
		// 이건 쓰지 말자. StressTest 예제로, 메모리 사용량이 크다. 그래도 기록 취지로 남겨두자.
		inline int getUniformlySelectedNextSubPoolIndex()
		{
			int sel = m_lastSubPoolSelection;
			sel++;
			if (sel >= m_subPoolCount)
				sel = 0;
			return sel;
		}

		// +random
		// contention 확률을 줄여서 CPU 사용량을 줄인다.
		// 이건 쓰지 말자. StressTest 예제로, 메모리 지속 증가가 일어난다. 그래도 기록 취지로 남겨두자.
		inline int getRandomSelectedNextSubPoolIndex()
		{
			// 이 값은 마지막에 사용되었던 것을 가리킨다. 마지막에 사용되었던 것은 높은 확률로 다른 스레드에서 사용중일 수 있다. 즉 contention 확률이 있다.
			// 따라서 여기서는 다음 것부터 시도를 하자.
			// 다른 스레드에서도 다음 것을 시도하더라도 같은 것을 시도하면, contention이 여전히 높을 것이다.
			// 따라서 랜덤값으로서 stackPointer값을 쓰도록 하자. 다만 64bit에서는 8의 배수일테니 >>3을 해서 /8 연산을 해버리자.
			//0-3-4-7-0-3-4-7-...을 반복한다. 다른 방법 없을까? =>어쩌면 나쁜 방법이 아닐지도! =>이거 적용하니까 메모리가 계속 느는거같은데? 빼보고 다시 체크해보자.
			int randomValue = (0x0fffffff & getApproximateStackPointer()) >> 3;

			int sel = (m_lastSubPoolSelection + 1/*getStackPointer가 0을 리턴해도 next를 가리키게 하기 위함 */ + randomValue) % m_subPoolCount;

			return sel;
		}

		// CPU 갯수만큼의 sub object pool에 대해서 try-lock을 수행한 후 성공하는 것을 리턴한다.
		// 마지막 것에 대해서는 blocked lock이다. 따라서 성공이 보장된다.
		// 당연히 이것을 통해 얻은 SubPool 객체는 unlock을 꼭 해주어야 한다.
		inline SubPool* GetAnyLockedSubPool()
		{
			// NOTE: 예전에는 atomic inc % (CpuCount)를 하는 연산이 있었으나, 이렇게 하니까,
			// 0번째 sub pool에서 얻은 것을 1번째에 drop하는 일이 많아지고 이 때문에 0번째 sub pool에서 얻을게 없어 계속 할당하게 되고
			// 이로 인해 RAM 폭주가 일어났다.
			// 그래서 지금은 그렇게 안한다.
			// => 한 변수에만 몰아서 검사하면, 똑같은 atomic op이라도 contention으로 더 느려진다고.
			// 따라서 여러 Sub pool을 다룬다.

			// 사실상 랜덤값이다. 여러 곳에서 마구잡이로 접근되니까.

			int sel = getCurrentAsNextSubPoolIndex();

			for (int i = 0; i < m_subPoolCount; i++) // 무한 루프는 아니고, 한번 끝까지는 돌도록 하자.
			{
				SubPool* subPool = &m_subPools[sel];
				if (subPool->m_critSec.TryLock())
				{
					// 이번에 sel에서 일을 했으니, 다음에 누군가가 여기 일을 걸면 다음 subpool을 건드리게 만들자.
					// (그냥 이번에 처리했던 subpool이 다음에 누군가가 건드리게 했었으나, lock contention의 확률이 증가할 뿐이다.)
					m_lastSubPoolSelection = sel;
					return subPool;
				}

				// non block lock failed. try next one.
				sel++;
				if (sel >= m_subPoolCount)
					sel = 0;
			}

			// 끝까지 돌았으나 얻지를 못한다. 다른 스레드들 모두가 모든 sub pool 안에서 낮은 확률로나마 작동하는 heap alloc or free라도 하고 있는지도?
			// 그렇다면 현재 것에 대해서 blocked alloc을 하도록 하자.
			SubPool* subPool = &m_subPools[sel];
			subPool->m_critSec.Lock();

			// 이번에 sel에서 일을 했으니, 다음에 누군가가 여기 일을 걸면 다음 subpool을 건드리게 만들자.
			// (그냥 이번에 처리했던 subpool이 다음에 누군가가 건드리게 했었으나, lock contention의 확률이 증가할 뿐이다.)
			sel++;
			if (sel >= m_subPoolCount)
				sel = 0;

			m_lastSubPoolSelection = sel;
			return subPool;
		}

	public:
		CClassObjectPool()
		{
			m_lastSubPoolSelection = 0;

			int c = GetNoofProcessors();

			m_subPools = new SubPool[c];
			m_subPoolCount = c;

		}

		~CClassObjectPool()
		{
			delete []m_subPools;
		}

		/* NOTE: 아래 함수는 의도적으로 없다. 다시 살리지 말 것!
		CClassObjectPool보다 CFavoritePooledObjects가 먼저 파괴될 수 있기 때문이다.
		~CClassObjectPool()
		{
		CFavoritePooledObjects::GetUnsafeRef().Unregister(this);
		} */

		inline T* NewOrRecycle()
		{
			SubPool* subPool = GetAnyLockedSubPool();

			T* ret = subPool->m_pool.NewOrRecycle();
			subPool->m_critSec.Unlock();

			return ret;
		}
		inline void Drop(T* obj)
		{
			SubPool* subPool = GetAnyLockedSubPool();
			subPool->m_pool.Drop(obj);
			subPool->m_critSec.Unlock();
		}
		void ShrinkOnNeed();

		// ProudNet client DLL이나 server DLL 안에 싱글톤으로 존재하는 전역 CClassObjectPool 객체를 얻는다.
		// 어디 안에 존재하는지 여부는 GetClassObjectPool(아래 참고)를 client DLL 혹은 server DLL안에 구현하고 PROUD_API나 PROUDSRV_API 속성 추가 여부에 따라 결정된다.
		inline static CClassObjectPool& GetUnsafeRef()
		{
			CClassObjectPool<T>* ret;
			// 만약 여기서 컴파일 에러간 나면
			// void GetClassObjectPool(CClassObjectPool<T>** output)
			// 이 함수를 구현하세요. 구현하는 함수는 PROUD_API나 PROUDSRV_API 속성이어야 합니다.
			GetClassObjectPoolInDll(&ret);
			return *ret;
		}
	};

	// ProudNet client DLL에서 singleton을 관리한다. 따라서 이것은 사용자에게 노출될 수 없다.
	//
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
			// 이 로컬 변수 생성/파괴는 성능에 민감하고,
			// NC or NS 안에서만 사용되므로, GetUnsafeRef를 쓴다.
			m_obj = CClassObjectPool<T>::GetUnsafeRef().NewOrRecycle();
		}
		inline ~CPooledObjectAsLocalVar()
		{
			CClassObjectPool<T>::GetUnsafeRef().Drop(m_obj);
		}


		// 실제 객체를 얻는다.
		inline T& Get()
		{
			return *m_obj;
		}
	};

}

/* ProudNet client DLL이 존재하는한 이것을 관리하는 singleton이 유지된다.

로컬 변수로서 object pooling을 할 때 사용하는 매크로. 로컬 변수 선언시 쓴다.
가급적이면 이 매크로를 쓰자. 두번째 줄에서 &를 빠뜨리는 사고도 있었으니까.

Q: 그냥 각 메인 객체가 object pool을 가지면 되지, 굳이 이렇게 singleton을 두나요?
A: 메인A에서 생성하고 메인B에서 버리는 객체가 잦으면, A와 B가 각각 object pool을 가지면 안됩니다. 그런데 A부터 Z까지 있다고 칩시다.
프로그램 덩치가 커지게 되면 pool은 가급적 단일화되어야 합니다. 그래서 singleton을 둡니다.
물론 singleton은 dll unload시 전역변수의 파괴 순서를 지키기 위한 추가 코딩 등 번거롭긴 합니다. 그럼에도 불구하고요.

사용법:
로컬 변수 선언으로써 POOLED_LOCAL_VAR(T, varName) 를 그냥 쓰면 된다.

사전 준비:
CFavoritePooledObject_Client,CFavoritePooledObject_Server 중 하나에다가 T를 추가해야 한다. 멤버 변수 선언, ShrinkOnNeed 에 라인 추가, 템플릿 함수 추가 등.
자세한 것은 CFavoritePooledObject_Client,CFavoritePooledObject_Server의 소스 형태를 살펴보면 추정해서 만드세요.
T가 ProudNet 클라&서버 공통이면 CFavoritePooledObject_Client에다가, 서버 전용이면 CFavoritePooledObject_Server에다가 만드세요.
*/
#define POOLED_LOCAL_VAR(type, name)					\
	CPooledObjectAsLocalVar<type> name##_LV;			\
	type& name = name##_LV.Get();

// BiasManagedPointer.AllocTombstone, FreeTombstone을 구현한다.
// 중복코드 방지차.
// NOTE: object pooling과 smart pointer化를 한번에 하는 유틸리티 클래스는 사용자에게 노출시킬 필요가 없으므로 이렇게 template specialization을 한다.
#define BiasManagedPointer_IMPLEMENT_TOMBSTONE(__Type, __ClearOnDrop) \
	template<> \
	PROUD_API BiasManagedPointer<__Type, __ClearOnDrop>::Tombstone* BiasManagedPointer<__Type, __ClearOnDrop>::AllocTombstone() \
	{ \
		typedef  BiasManagedPointer<__Type, __ClearOnDrop>::Tombstone TombstoneType; \
		Tombstone* ret = CClassObjectPool<TombstoneType>::GetUnsafeRef().NewOrRecycle(); \
		return ret; \
	} \
	template<> \
	PROUD_API void BiasManagedPointer<__Type, __ClearOnDrop>::FreeTombstone(Tombstone* tombstone) \
	{ \
		typedef  BiasManagedPointer<__Type, __ClearOnDrop>::Tombstone TombstoneType; \
		CClassObjectPool<TombstoneType>::GetUnsafeRef().Drop(tombstone); \
	}

#include "PooledObjects.inl"
