#pragma once

#if !defined(_WIN32)
    #include "quicksort.h"
#endif

namespace Proud
{
/** [min1,max1]과 [min2,max2] 사이의 정수 범위의 교집합의 숫자 갯수를 리턴한다. */
template<typename T>
inline bool IsCombinationEmpty(T min1, T max1, T min2, T max2)
{
#if !defined(_WIN32)
	if (min1 > max1) quickswap(min1, max1);
#else
	if (min1 > max1) swap(min1, max1);
#endif

#if !defined(_WIN32)
	if (min2 > max2) quickswap(min2, max2);
#else
	if (min2 > max2) swap(min2, max2);
#endif
	if (min2 <= min1 && max1 <= max2)
		return false;
	if (min1 <= min2 && max2 <= max1)
		return false;
	if (min1 <= min2 && min2 <= max1 && max1 <= max2)
		return false;
	if (min2 <= min1 && min1 <= max2 && max2 <= max1)
		return false;

	return true;
}
class NetUtil
{
public:
	static int SetManualOrAutoCoalesceInterval(bool autoCoalesceInterval, int recentPingMs, int manualInterval);
};
}