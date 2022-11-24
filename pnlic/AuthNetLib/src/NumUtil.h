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

	void CauseAccessViolation();
}