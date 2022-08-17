#pragma once 

#include "../include/BasicTypes.h"
//222-원본: #include "zlib/zlib.h"
#include "../OpenSources/zlib-v1.2.8/zlib.h"

namespace Proud 
{
	 PROUD_API int ZEXPORT ZlibCompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
	 PROUD_API int ZEXPORT ZlibUncompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
}
