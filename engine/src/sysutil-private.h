#pragma once

#include "../include/BasicTypes.h"

// https://github.com/owent-utils/c-cpp/blob/a0f4c1f8b6a5ef5546521bae2f0b39ffe6129c9a/include/Lock/SpinLock.h 에서 복사해온 소스이다.
// 이것때문에 프라우드넷 제품 설명서에서 언급되는 외부라이브러리 중에는 owent-utils (MIT License)도 언급되어야 한다.

/**
 * ==============================================
 * ======            asm pause             ======
 * ==============================================
 */
#if defined(_MSC_VER)
#include <windows.h> // YieldProcessor

/*
 * See: http://msdn.microsoft.com/en-us/library/windows/desktop/ms687419(v=vs.85).aspx
 * Not for intel c++ compiler, so ignore http://software.intel.com/en-us/forums/topic/296168
 */
#define __UTIL_LOCK_SPIN_LOCK_PAUSE() YieldProcessor()

#elif defined(__GNUC__) || defined(__clang__)
#if defined(__i386__) || defined(__x86_64__)
/**
 * See: Intel(R) 64 and IA-32 Architectures Software Developer's Manual V2
 * PAUSE-Spin Loop Hint, 4-57
 * http://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-instruction-set-reference-manual-325383.html?wapkw=instruction+set+reference
 */
define __UTIL_LOCK_SPIN_LOCK_PAUSE() __asm__ __volatile__("pause")
#elif defined(__ia64__) || defined(__ia64)
/**
 * See: Intel(R) Itanium(R) Architecture Developer's Manual, Vol.3
 * hint - Performance Hint, 3:145
 * http://www.intel.com/content/www/us/en/processors/itanium/itanium-architecture-vol-3-manual.html
 */
#define __UTIL_LOCK_SPIN_LOCK_PAUSE() __asm__ __volatile__ ("hint @pause")
#elif defined(__arm__) && !defined(__ANDROID__)
/**
 * See: ARM Architecture Reference Manuals (YIELD)
 * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.subset.architecture.reference/index.html
 */
#define __UTIL_LOCK_SPIN_LOCK_PAUSE() __asm__ __volatile__ ("yield")
#endif

#endif /*compilers*/

// set pause do nothing
#if !defined(__UTIL_LOCK_SPIN_LOCK_PAUSE)
	#define __UTIL_LOCK_SPIN_LOCK_PAUSE()
#endif/*!defined(CAPO_SPIN_LOCK_PAUSE)*/


namespace Proud
{
	PROUD_API void YieldThread();
}
