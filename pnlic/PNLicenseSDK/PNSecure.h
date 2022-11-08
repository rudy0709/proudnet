#pragma once

#include "stdafx.h"
#include "PNExport.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>

typedef int errno_t;

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __linux__
	errno_t  fopen_s(FILE **stream, const char *name, const char *mode);
	errno_t  memcpy_s(void *dest, size_t dest_size, const void* src, size_t max_count);
	int  sprintf_s(char *dest, size_t bufferSize, const char *format, ...);
	//size_t  fread_s(void *buf, size_t bufferSize, size_t elementSize, size_t count, FILE *stream);
	//errno_t  strcpy_s(char *strDestination, size_t numberOfElements, const char *strSource);

#endif
#ifdef __cplusplus
}
#endif

