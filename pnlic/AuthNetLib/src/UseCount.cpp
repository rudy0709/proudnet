/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "UseCount.h"

namespace Proud
{
	CUseCount::CUseCount( void ):
	m_inUseCount(0)
	{

	}

	CUseCount::~CUseCount( void )
	{
		assert(m_inUseCount == 0);
	}

	// 이 값이 0을 리턴할 경우 더 이상 이 객체를 참조하는 곳이 없음을 보장한다.
	// 이 메서드를 콜 하기 전에 lock(main)이어야 한다.
	int CUseCount::GetUseCount()
	{
		return m_inUseCount;
	}

	// 이 메서드를 콜 하기 전에 lock(main)이어야 한다. 안그러면 UseCount=0인 순간 다른 스레드에 의해 delete하는 사태 발생.
	void CUseCount::IncreaseUseCount()
	{
		int32_t r = AtomicIncrement32(&m_inUseCount);
		assert(r > 0);
	}

	void CUseCount::DecreaseUseCount()
	{
		assert(m_inUseCount>0);
#ifdef PN_LOCK_OWNER_SHOWN
		assert(IsLockedByCurrentThread() == false);
#endif
		int32_t r = AtomicDecrement32(&m_inUseCount);
		assert(r>=0);
	}

	void CUseCount::AssertIsZeroUseCount()
	{
		assert(m_inUseCount >= 0);
	}

	CUseCounter::CUseCounter( CUseCount& useCount )
	{
		m_useCount = &useCount;

		// CUseCount를 가진 객체를 lock해도 정작 CUseCounter 로컬 변수가 unlock하고 있으면 타 스레드에 의해 race condition이 생길 수 있다. 이를 예방하고자 atomic op으로.
		m_useCount->IncreaseUseCount();
	}

	CUseCounter::~CUseCounter()
	{
		assert(m_useCount);
#ifdef PN_LOCK_OWNER_SHOWN
		// add by ulelio : useCount를 상속받은 객체가 lock되어있으면 안된다.
		assert(m_useCount->IsLockedByCurrentThread() == false);
#endif

		// CUseCount를 가진 객체를 lock해도 정작 CUseCounter 로컬 변수가 unlock하고 있으면 타 스레드에 의해 race condition이 생길 수 있다. 이를 예방하고자 atomic op으로.
		m_useCount->DecreaseUseCount();
	}
}
