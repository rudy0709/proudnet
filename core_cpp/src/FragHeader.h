#pragma once

#include <cstdint>
#include "../include/Message.h"

namespace Proud 
{
	inline bool IsInt8Range(int32_t v)
	{
		return v>=INT8_MIN && v<=INT8_MAX;
	}
	inline bool IsInt16Range(int32_t v)
	{
		return v>=INT16_MIN && v<=INT16_MAX;
	}

	// FragHeader write에 사용
	inline int GetLengthFlag(int v)
	{
		if (IsInt8Range(v))
			return 0;
		else if (IsInt16Range(v))
			return 1;
		else
			return 3;
	}

	inline void WriteCompressedByFlag(CMessage& msg, int v, int flag)
	{
		switch(flag)
		{
		case 0:
			msg.Write((int8_t)v);
			break;
		case 1:
			msg.Write((short)v);
			break;
		case 3:
			msg.Write(v);
			break;
		default:
			// 심각 버그다. 크래시를 내자.
			int* a=0;
			*a=1;
			break;
		}
	}

	inline bool ReadCompressedByFlag(CMessage& msg, int &outV, int flag)
	{
		int8_t v0;
		int16_t v1;
		bool ret;

		switch(flag)
		{
		case 0:
			ret = msg.Read(v0);
			if(ret)
				outV = v0;
			return ret;
		case 1:
			ret = msg.Read(v1);
			if(ret)
				outV = v1;
			return ret;
		case 3:
			return msg.Read(outV);
		default:
			// 읽을 수 없는 플래그다. 읽기 실패 처리를 하자.
			return false;
		}
	}

}