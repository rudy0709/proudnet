#pragma once 

#include "zlib/zlib.h"

namespace Proud 
{
	int ZEXPORT ZlibCompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
	int ZEXPORT ZlibUncompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
}