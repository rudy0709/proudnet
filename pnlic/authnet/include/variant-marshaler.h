#pragma once

#include "Variant.h"

namespace Proud
{
	class CMessage;
	class CVariant;

#if defined(_WIN32)
	CMessage& operator<<(CMessage &a, const CVariant &b);
	CMessage& operator>>(CMessage &a, CVariant &b);

	void AppendTextOut(String &a, const CVariant &b);
#endif
}
