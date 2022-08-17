#pragma once 

#include "../include/CriticalSect.h"
#include "SpinLock.h"

namespace Proud 
{
	/* context switch를 최소화하면서 불특정한(!) 순서로 objList의 항목에 대한 처리를 수행한다.
	사용예:
	MyElement element[1000];
	LowContextSwitchingLoop(element, 1000, MyFunc() );

	objList,length: 처리할 배열
	Func: functor class.
	Functor class의 최소 모양은 다음과 같다.

	class MyFunc
	{
		// obj: objList의 element 객체(remote or socket 등)
		// objLock: 이미 lock이 1회 되어 있는 critsec lock 객체.
		// return: 잘 처리했으므로 더 이상 할 필요가 없으면 true, 아직 처리를 더 해야 할 필요가 있으면 false를 리턴한다.
		//         가령, non-block syscall의 경우, EWOULDBLOCK이 나올 때까지 반복해야 한다. 
		//         이러한 경우 이 함수는 평소 false를 리턴하다가 EWOULDBLOCK 혹은 여타 에러가 나오면 true를 리턴하면 된다.
		bool DoElementAndUnlock(MyElement& obj, CriticalSectionLock& objLock)
		{
			// obj has objLock, and objLock is already locked by current thread.
			// do something here, including unlocking objLock.
			// DoElementAndUnlock가 일단 호출된 element는 더 이상 이 함수에서 사용되지 않는다. 따라서, DoElementAndUnlock 안에서
			// DecreaseUseCount를 호출해서 element가 다른 스레드에 의해 파괴되게 만들어도 안전하다.
			return true or false;
		}

		CriticalSection* GetCriticalSection(MyElement& obj)
		{
			obj가 갖고 있는 per-remote lock 객체를 여기서 주어야 한다.
		}

		// add more members if you want.
	}

	Element type은 복사 비용이 싸며, 복사가 lock free한 객체이어야 한다. 이 함수는 완료된 것들을 아직 처리안된 것으로부터 복사받기 때문이다.
	몇가지 작은 값들을 가지는 구조체 혹은 포인터 변수 타입인 것이 좋은 예다.

	VS2008의 유지보수가 모두 만료되는 2018년에 할일: 
	VS2010나 xcode 어떤 버전 이후부터에서는 람다 정규식을 쓸 수 있게 하면 더 좋다.
	그러면 Functor class대신 그것을 쓰면 코딩이 더 편해진다.
	가령, 이렇게. 물론 이렇게 하려면 GetCriticalSection을 위한 functor가 하나 더 있어야 하겠지만.

	class MyClass
	{
		void Foo(a,b)
		{
			var c,d;
			LowContextSwitchingLoop(list, count, [this, a,b,c,d](obj,lock) {
				// do something for DoElementAndUnlock
				} );
		}
	};

	*/
	template<typename _Func, typename _ElementType, typename _IndexType>
	void LowContextSwitchingLoop(_ElementType* objList, _IndexType length, _Func &Func)
	{
		int turnCount = 0; // 처음부터 끝까지 처리하고 나면 1 턴 끝났다고 지칭하자.
		
		while (length > 0) // 처리할게 아직 있으면(첫 턴은 전부 처리 대상)
		{
			// Q: 첫 루프부터 blocking lock을 거는 것이 CPU 사용율이 덜 나오는데요?
			// A: 그렇게 하면 CPU 사용율이 덜 나오지만 총 처리량이 더 적습니다. 
			// 그리고 논리적으로 따져도 이게 더 효율적이라는 점에서 맞죠. 
			// context switch 한번 덜 하는 것이 불필요한 루프 한번 더 도는 것보다 낫습니다.
			for (_IndexType i = 0; i < length; ++i)
			{
				_ElementType& object = objList[i];

				CriticalSectionLock solock(*Func.GetCriticalSection(object), false);
				if (turnCount>0 && i==0) // 최소 한바퀴는 이미 돌았으며, 첫번째 항목이면
				{
					// 블러킹 잠금
					solock.Lock();
					bool finished = Func.DoElementAndUnlock(object, solock);
					assert(!solock.IsLocked());

					if (finished)
					{
						// 맨 끝에 있는 것을 현재 항목으로 옮겨온다. 그리고 현재 항목의 기존 값은 폐기.
						objList[i] = MOVE_OR_COPY(objList[length - 1]); // item type이 shared_ptr인 경우 move가 빠르다.
						length--;
					}
				}
				else
				{
					// non-block 잠금을 해보고 성공하면 처리, 아니면 다음 턴으로 미룬다.
					bool lockOk = solock.TryLock();
					if (lockOk)
					{
						bool finished = Func.DoElementAndUnlock(object, solock);
						assert(!solock.IsLocked());

						if (finished)
						{
							// 맨 끝에 있는 것을 현재 항목으로 옮겨온다. 그리고 현재 항목의 기존 값은 폐기.
							objList[i] = MOVE_OR_COPY(objList[length - 1]); // item type이 shared_ptr인 경우 move가 빠르다.
							length--;
						}
					}
				}
			}

			turnCount++;
		}
	}

	/* 위에 함수와 동일. 단, 두개의 함수 객체를 받는다.
	첫 2개 파라메터는 앞의 함수와 동일하다.

	GetCritSecFunc: object가 가진 CritSec 객체의 포인터를 리턴한다.
	Func: object에 대한 내부 처리를 한 후 **Unlock을** 한다. 내부 처리를 하지 못했으면 false를 리턴하자. 	

	사용 예:
	MyType[] array;
	LowContextSwitchingLoop(array, arrayLength,
		[](MyType& elem){ return &elem.m_critSec; },
		[](MyType& elem, CriticalSectionLock& lock) {
		DoSomething(elem);
		lock.Unlock();
		return true;
	});
	
	*/
	template<typename _ElementType, typename _IndexType, typename _GetCritSecFunc, typename _Func>
	void LowContextSwitchingLoop(_ElementType* objList, _IndexType length, 
		const _GetCritSecFunc& GetCritSecFunc,
		const _Func& Func)
	{
		int turnCount = 0; // 처음부터 끝까지 처리하고 나면 1 턴 끝났다고 지칭하자.

		while (length > 0) // 처리할게 아직 있으면(첫 턴은 전부 처리 대상)
		{
			// Q: 첫 루프부터 blocking lock을 거는 것이 CPU 사용율이 덜 나오는데요?
			// A: 그렇게 하면 CPU 사용율이 덜 나오지만 총 처리량이 더 적습니다. 
			// 그리고 논리적으로 따져도 이게 더 효율적이라는 점에서 맞죠. 
			// context switch 한번 덜 하는 것이 불필요한 루프 한번 더 도는 것보다 낫습니다.
			for (_IndexType i = 0; i < length; ++i)
			{
				_ElementType& object = objList[i];

				CriticalSectionLock solock(*GetCritSecFunc(object), false);
				if (turnCount > 0 && i == 0) // 최소 한바퀴는 이미 돌았으며, 첫번째 항목이면
				{
					// 블러킹 잠금
					solock.Lock();
					bool finished = Func(object, solock);
					assert(!solock.IsLocked());

					if (finished)
					{
						// 맨 끝에 있는 것을 현재 항목으로 옮겨온다. 그리고 현재 항목의 기존 값은 폐기.
						objList[i] = objList[length - 1];
						length--;
					}
				}
				else
				{
					// non-block 잠금을 해보고 성공하면 처리, 아니면 다음 턴으로 미룬다.
					bool lockOk = solock.TryLock();
					if (lockOk)
					{
						bool finished = Func(object, solock);
						assert(!solock.IsLocked());

						if (finished)
						{
							// 맨 끝에 있는 것을 현재 항목으로 옮겨온다. 그리고 현재 항목의 기존 값은 폐기.
							objList[i] = MOVE_OR_COPY(objList[length - 1]); // item type이 shared_ptr인 경우 move가 빠르다.
							length--;
						}
					}
				}
			}

			turnCount++;
		}
	}

	template<typename _Func, typename _ElementType, typename _IndexType>
	void LowContextSwitchingLoop_SpinMutex(_ElementType* objList, _IndexType length, _Func &Func)
	{
		int turnCount = 0; // 처음부터 끝까지 처리하고 나면 1 턴 끝났다고 지칭하자.

		while (length > 0) // 처리할게 아직 있으면(첫 턴은 전부 처리 대상)
		{
			// Q: 첫 루프부터 blocking lock을 거는 것이 CPU 사용율이 덜 나오는데요?
			// A: 그렇게 하면 CPU 사용율이 덜 나오지만 총 처리량이 더 적습니다. 
			// 그리고 논리적으로 따져도 이게 더 효율적이라는 점에서 맞죠. 
			// context switch 한번 덜 하는 것이 불필요한 루프 한번 더 도는 것보다 낫습니다.
			for (_IndexType i = 0; i < length; ++i)
			{
				_ElementType& object = objList[i];

				SpinLock solock(*Func.GetCriticalSection(object), false);
				if (turnCount > 0 && i == 0) // 최소 한바퀴는 이미 돌았으며, 첫번째 항목이면
				{
					// 블러킹 잠금
					solock.Lock();
					bool finished = Func.DoElementAndUnlock(object, solock);
					assert(!solock.IsLocked());

					if (finished)
					{
						// 맨 끝에 있는 것을 현재 항목으로 옮겨온다. 그리고 현재 항목의 기존 값은 폐기.
						objList[i] = objList[length - 1];
						length--;
					}
				}
				else
				{
					// non-block 잠금을 해보고 성공하면 처리, 아니면 다음 턴으로 미룬다.
					bool lockOk = solock.TryLock();
					if (lockOk)
					{
						bool finished = Func.DoElementAndUnlock(object, solock);
						assert(!solock.IsLocked());

						if (finished)
						{
							// 맨 끝에 있는 것을 현재 항목으로 옮겨온다. 그리고 현재 항목의 기존 값은 폐기.
							objList[i] = objList[length - 1];
							length--;
						}
					}
				}
			}

			turnCount++;
		}
	}

}