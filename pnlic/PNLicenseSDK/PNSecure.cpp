#include "stdafx.h"
#include "PNSecure.h"
#include <exception>

using namespace std;

#if defined(__linux__)

errno_t fopen_s(FILE **stream, const char *name, const char *mode)
{
	errno_t ret = 0;

	if(stream == 0) {
		return EINVAL;
	}

	*stream = fopen(name, mode);
	if(!*stream) {
		ret = errno;
	}

	return ret;
}

errno_t memcpy_s(void *dest, size_t dest_size, const void* src, size_t max_count)
{
	size_t count = max_count;

	if (dest == 0 || src == 0) {
		return EINVAL;
	}

	if (count < 0) {
		return ERANGE;
	}

	if (dest_size < count) {
		return ERANGE;
	}

	memcpy(dest, src, count);
	return 0;
}

int sprintf_s(char *buffer, size_t sizeOfBuffer, const char *format, ...)
{
	va_list argp;

	if (buffer == 0 || format == 0) {
		return EINVAL;
	}

	va_start(argp, format);
	int ret = snprintf(buffer, sizeOfBuffer, format, argp);
	va_end(argp);
	return ret;
}


//size_t fread_s(void *buf, size_t bufferSize, size_t elementSize, size_t count, FILE *stream)
//{
//	size_t readSize;
//
//	if(!buf) {
//		throw "Not declare buffer";
//	}
//
//	if(!stream) {
//		throw "Not declare file discripter";
//	}
//
//	readSize = fread(buf, elementSize, count, stream);
//	return readSize;
//}
//

//errno_t strcpy_s(char *strDestination, size_t numberOfElements, const char *strSource)
//{
//	if(strDestination == 0 || strSource == 0) {
//		return EINVAL;
//	}
//
//	if(numberOfElements < 0 || numberOfElements > sizeof(strDestination))
//	{
//		return ERANGE;
//	}
//
//	strlen(strSource)
//	strcpy(strDestination, strSource);
//	return 0;
//}

#endif