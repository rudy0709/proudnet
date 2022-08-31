#pragma once

#include <cstddef>
#include "FreeList.h"
#include <limits>

#ifdef max
#undef max
#endif

namespace Proud
{
	// thread unsafe하고, 내부적으로 free list알고리즘으로, CRT heap보다 더 빠른 속도로 할당,해제를 한다.
	// 대신 free list이기 때문에 미사용 공간이 오래 남을 수 있다.
	// 웹문서 'STL) 나만의 Allocator( 할당자 ) 만들기 - 2' 참고하면서 제작됨.
	// 그냥 복붙했고, 아래 #ThreadUnsafeAllocator-MainFunctions 만 구현.
	template<typename T>
	class ThreadUnsafeAllocator
	{
	public:
		using value_type = T;

		using pointer = T*;
		//using const_pointer = const T*;

		using void_pointer = void*;
		using const_void_pointer = const void*;

		using size_type = size_t;

		//using difference_type = std::ptrdiff_t;

		ThreadUnsafeAllocator() = default;
		~ThreadUnsafeAllocator() = default;

		template <typename U>
		struct rebind {
			using other = ThreadUnsafeAllocator<U>;
		};
		template <typename U>
		ThreadUnsafeAllocator(const ThreadUnsafeAllocator<U>& other)
		{
			// 이것을 사용하는 collection가 복사되는 경우, 이것도 복사되어서는 안된다. 이건 복사된 새 collection 내부용으로 독립적으로 존재해야 한다.
			// 이후 allocator가 새 collection를 대상으로 반복 실행될거다.
			// 여기서는 this에 대해서 새 ThreadUnsafeAllocator 상태를 따끈따끈하게 유지시키자.
			// 따라서 여기서는 달리 할게 없다. other를 갖고 할거도 없다.
		}

		// #ThreadUnsafeAllocator-MainFunctions
		struct MemoryBlock {  // CObjectPool은 생성자,파괴자를 실행한다. std::allocator::allocate 스펙상 메모리 할당만 해야 한다. 따라서 이렇게 한다.
			unsigned char m_block[sizeof(value_type)];

			void SuspendShrink() {}	// CObjectPool에서 요구한다.
			void OnRecycle() {}	// CObjectPool에서 요구한다.
			void OnDrop() {}	// CObjectPool에서 요구한다.
		};
		CObjectPool<MemoryBlock> m_freeList;

		/*pointer allocate(size_type ObjectNum, const_void_pointer hint) {
			allocate(ObjectNum);
		}*/
		// #ThreadUnsafeAllocator - MainFunctions
		pointer allocate(size_type ObjectNum) {
			assert(ObjectNum == 1);		// free list를 이용할 때 여러 객체는 동시에 처리 못한다.
			return (pointer)m_freeList.NewOrRecycle();
			//return static_cast<pointer>(operator new(sizeof(T) * ObjectNum));
		}
		// #ThreadUnsafeAllocator - MainFunctions
		void deallocate(pointer p, size_type ObjectNum) {
			assert(ObjectNum == 1);			// free list를 이용할 때 여러 객체는 동시에 처리 못한다.
			m_freeList.Drop((MemoryBlock*)p);
			//operator delete(p);
		}

		size_type max_size() const { return std::numeric_limits<size_type>::max() / sizeof(value_type); }

		template<typename U, typename... Args>
		void construct(U* p, Args&& ...args) {
			new(p) U(std::forward<Args>(args)...);
		}

		template <typename U>
		void destroy(U* p) {
			p->~U();
		}
	};
}
