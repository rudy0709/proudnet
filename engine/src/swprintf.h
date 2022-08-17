#include "stdafx.h"


// iOS에서도 sw, vsw류의 표준 함수들이 안되는 관계로 pn~으로 변경한다. by hyeonmin.yoon
#if defined(__linux__) || defined(__MARMALADE__) || defined(__MACH__)// marmalade

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <wctype.h>
#include <wchar.h>
#include <limits.h>
#include <string.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern	int pnswprintf(wchar_t *buf, size_t, const wchar_t *fmt, ...);
extern	int pnsnwprintf(wchar_t *buf, size_t cnt, const wchar_t *fmt, ...);
extern	int pnvswprintf(wchar_t *buf, size_t, const wchar_t *fmt, va_list args);
extern	int pnvsnwprintf(wchar_t *buf, size_t cnt, const wchar_t *fmt, va_list args);

#if defined(__cplusplus)
}
#endif	

#endif //defined(__linux__) || defined(__MARMALADE__) && defined(__GNUC__)  // marmalade ARM GCC