#pragma once

#include "../include/numutil.h"

namespace Proud
{
	// 사용자에게 노출 시키지 않게 하기 위해서 ../include/numutil.h 가 아닌 이곳에.
	/**
	\~korean
	double 을 int 로 변환합니다.

	\param val int 로 변환될 double 입니다.
	\return double 의 소수점 이하 부분이 제거되어 return 됩니다.
	Int 의 최소값보다 작을경우 -2147483648이,
	Int 의 최대값보다 클경우 2147483647 이 return 됩니다.

	\~english TODO:translate needed.

	\~chinese TODO:translate needed.

	\~japanese TODO:translate needed.

	*/
	inline int DoubleToInt(double val)
	{
		double ret = PNMAX(INT32_MIN, val);
		ret = PNMIN(ret, INT32_MAX);

		return (int)ret;
	}

	PROUD_API void CauseAccessViolation();

	// 일시적으로 atomic +1 then 파괴시 -1을 한다. 예외가 생겨도 마무리를 보장하기 위함.
	// 로컬 변수로서 쓰인다.
	// 사용법: 로컬 변수로 생성하면 +1. GetReturnValue로 최종 값 체크. 파괴시 -1.
	class TemporaryAtomicIncrease
	{
		volatile int32_t* m_value;
		int32_t m_retVal;
	public:
		inline TemporaryAtomicIncrease(volatile int32_t *value)
		{
			m_value = value;
			m_retVal = AtomicIncrement32(m_value);
		}

		inline int32_t GetReturnValue()
		{
			return m_retVal;
		}

		inline ~TemporaryAtomicIncrease()
		{
			AtomicDecrement32(m_value);
		}
	};

	/* 코딩하다 실수를 방지하기 위해, 성공적인 롤백을 위해 이걸 애용하자.
	사용법: 로컬 변수로 생성하면 CAS. GetReturnValue로 최종 값 체크. 파괴시 CAS를 롤백. 단, CAS가 성공했을 때만. */
	class TemporaryAtomicCompareAndSwap
	{
		volatile int32_t* m_pValue;
		int32_t m_retVal; // same to oldValue
		int32_t m_newValue;
		bool m_swapOK;
	public:
		inline TemporaryAtomicCompareAndSwap(int32_t oldValue, int32_t newValue, volatile int32_t *pValue)
		{
			m_pValue = pValue;
			m_newValue = newValue;
			m_retVal = AtomicCompareAndSwap32(oldValue, newValue, m_pValue);
			m_swapOK = (m_retVal == oldValue); // 성공적으로 CAS가 됐으면 true.
		}

		inline int32_t GetReturnValue()
		{
			return m_retVal;
		}

		inline ~TemporaryAtomicCompareAndSwap()
		{
			// CAS 됐었으면 롤백해주자.
			if(m_swapOK)
				AtomicCompareAndSwap32(m_newValue, m_retVal, m_pValue);
		}
	};
}
