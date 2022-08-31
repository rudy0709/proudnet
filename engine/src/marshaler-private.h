#pragma once

#include "../include/pnstdint.h"
#include "MessagePrivateImpl.h"

namespace Proud
{
	//////////////////////////////////////////////////////////////////////////
	// PN 내부 전용 RMI에서 사용되는 객체 타입에 대한 마샬링 정의.

	inline CMessage& operator>>(CMessage &a, RelayDestList &b)
	{
		Message_Read(a, b);
		return a;
	}

	inline CMessage& operator<<(CMessage &a, const RelayDestList &b)
	{
		Message_Write(a, b);
		return a;
	}


	inline CMessage& operator>>(CMessage &a, HostIDArray &b)
	{
		Message_Read(a, b);
		return a;
	}

	inline CMessage& operator<<(CMessage &a, const HostIDArray &b)
	{
		Message_Write(a, b);
		return a;
	}

	PROUD_API void AppendTextOut(String &a, HostIDArray &b);

	inline CMessage& operator>>(CMessage &a, LogCategory &b)
	{
		Message_Read(a, b);

		return a;
	}

	inline CMessage& operator<<(CMessage &a, const LogCategory &b)
	{
		Message_Write(a, b);
		return a;
	}

	inline void AppendTextOut(String &a, const LogCategory &b)
	{
		a += ToString(b);
	}

	inline CMessage& operator >> (CMessage &a, CompactFieldMap &b)
	{
		Message_Read(a, b);
		return a;
	}

	inline CMessage& operator<<(CMessage &a, const CompactFieldMap &b)
	{
		Message_Write(a, b);
		return a;
	}

	PROUD_API void AppendTextOut(String &a, const CompactFieldMap &b);

	inline CMessage& operator >> (CMessage &a, CompactFieldName &b)
	{
		Message_Read(a, b);
		return a;
	}

	inline CMessage& operator<<(CMessage &a, const CompactFieldName &b)
	{
		Message_Write(a, b);
		return a;
	}

	inline void AppendTextOut(String &a, const CompactFieldName &b)
	{
		a += String::NewFormat(_PNT("FieldType:%d"), b);
	}
}
