#pragma once 

#include "../include/BasicTypes.h"
#include "../OpenSources/zlib/zlib.h"

namespace Proud 
{
	 PROUD_API int ZEXPORT ZlibCompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
	 PROUD_API int ZEXPORT ZlibUncompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
}
