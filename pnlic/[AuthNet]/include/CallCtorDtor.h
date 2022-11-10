#pragma once

/** 생성자, 파괴자를 명시적으로 호출하는 함수들.
Container class를 정의할 때 주로 사용된다. */

template< typename T >
void CallConstructor(T* pInstance)
{
#ifndef __MARMALADE__
#pragma push_macro("new")
#undef new
#endif

	::new(pInstance)T();

#ifndef __MARMALADE__
#pragma pop_macro("new")
#endif // __MARMALADE__
};

template< typename T, typename Src >
void CallConstructor(T* pInstance, const Src& src)
{
#ifndef __MARMALADE__
#pragma push_macro("new")
#undef new
#endif // __MARMALADE__

	::new(pInstance)T(src);

#ifndef __MARMALADE__
#pragma pop_macro("new")
#endif // __MARMALADE__
};

// 주의: 반드시!! 함수 사용시 <T>를 명료하게 붙일 것. 일부 컴파일러는 엉뚱한 것을 템플릿 인스턴스화한다.
template< typename T, typename Src, typename Src1 >
void CallConstructor(T* pInstance, const Src& src, const Src1 src1)
{
#ifndef __MARMALADE__
#pragma push_macro("new")
#undef new
#endif // __MARMALADE__

	::new(pInstance)T(src, src1);

#ifndef __MARMALADE__
#pragma pop_macro("new")
#endif // __MARMALADE__
};

// 주의: 반드시!! 함수 사용시 <T>를 명료하게 붙일 것. 일부 컴파일러는 엉뚱한 것을 템플릿 인스턴스화한다.
template< typename T >
void CallDestructor(T* pInstance)
{
	pInstance->T::~T();
};

#ifndef __MARMALADE__
#pragma push_macro("new")
#undef new
#endif // __MARMALADE__

// 주의: 반드시!! 함수 사용시 <T>를 명료하게 붙일 것. 일부 컴파일러는 엉뚱한 것을 템플릿 인스턴스화한다.
template< typename T >
static inline void CallConstructors(T* pElements, intptr_t nElements)
{
	int iElement = 0;

	//	try
	//	{
	for (iElement = 0; iElement < nElements; iElement++)
	{
		::new(pElements + iElement) T;

	}
	//	}
	// 	catch(...)
	// 	{
	// 		while( iElement > 0 )
	// 		{
	// 			iElement--;
	// 			pElements[iElement].~T();
	// 		}
	//
	// 		throw;
	// 	}
}

// 주의: 반드시!! 함수 사용시 <T>를 명료하게 붙일 것. 일부 컴파일러는 엉뚱한 것을 템플릿 인스턴스화한다.
template< typename T >
static inline void CallCopyConstructors(T* pElements, const T* pOldElements, intptr_t nElements)
{
	int iElement = 0;

	for (iElement = 0; iElement < nElements; iElement++)
	{
		::new(pElements + iElement) T(pOldElements[iElement]);
	}
}


// 주의: 반드시!! 함수 사용시 <T>를 명료하게 붙일 것. 일부 컴파일러는 엉뚱한 것을 템플릿 인스턴스화한다.
template< typename T >
static inline void CallDestructors(T* pElements, intptr_t nElements) throw()
{
	(void)pElements;

	for (int iElement = 0; iElement < nElements; iElement++)
	{
		pElements[iElement].~T();
	}
}
#ifndef __MARMALADE__
#pragma pop_macro("new")
#endif // __MARMALADE__
