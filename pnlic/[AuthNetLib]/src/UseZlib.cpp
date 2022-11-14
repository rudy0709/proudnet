#include "stdafx.h"
#include "UseZlib.h"
#include "../include/ProcHeap.h"

namespace Proud 
{
	voidpf Zlib_calloc(voidpf opaque, unsigned items, unsigned size)
	{
		void* ret = CProcHeap::Alloc(items * size);
		if (ret == NULL)
			throw std::bad_alloc();

		return ret;
	}

	void Zlib_free(voidpf opaque, voidpf ptr)
	{
		CProcHeap::Free(ptr);
	}

	int ZEXPORT ZlibCompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen)
	{
		int level = Z_DEFAULT_COMPRESSION;
		z_stream stream;
		int err;

		stream.next_in = (Bytef*)source;
		stream.avail_in = (uInt)sourceLen;
#ifdef MAXSEG_64K
		/* Check for source > 64K on 16-bit machine: */
		if ((uLong)stream.avail_in != sourceLen) return Z_BUF_ERROR;
#endif
		stream.next_out = dest;
		stream.avail_out = (uInt)*destLen;
		if ((uLong)stream.avail_out != *destLen) return Z_BUF_ERROR;

		stream.zalloc = Zlib_calloc;
		stream.zfree = Zlib_free;

		stream.opaque = (voidpf)0;

		err = deflateInit(&stream, level);
		if (err != Z_OK) return err;

		err = deflate(&stream, Z_FINISH);
		if (err != Z_STREAM_END) {
			deflateEnd(&stream);
			return err == Z_OK ? Z_BUF_ERROR : err;
		}
		*destLen = stream.total_out;

		err = deflateEnd(&stream);
		return err;
	}

	int ZEXPORT ZlibUncompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen)
	{
		z_stream stream;
		int err;

		stream.next_in = (Bytef*)source;
		stream.avail_in = (uInt)sourceLen;
		/* Check for source > 64K on 16-bit machine: */
		if ((uLong)stream.avail_in != sourceLen) return Z_BUF_ERROR;

		stream.next_out = dest;
		stream.avail_out = (uInt)*destLen;
		if ((uLong)stream.avail_out != *destLen) return Z_BUF_ERROR;

		stream.zalloc = Zlib_calloc;
		stream.zfree = Zlib_free;

		err = inflateInit(&stream);
		if (err != Z_OK) return err;

		err = inflate(&stream, Z_FINISH);
		if (err != Z_STREAM_END) {
			inflateEnd(&stream);
			if (err == Z_NEED_DICT || (err == Z_BUF_ERROR && stream.avail_in == 0))
				return Z_DATA_ERROR;
			return err;
		}
		*destLen = stream.total_out;

		err = inflateEnd(&stream);
		return err;
	}

}