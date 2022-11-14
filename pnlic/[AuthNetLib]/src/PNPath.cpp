#include "stdafx.h"
#include "../include/PNPath.h"

namespace Proud
{
	Proud::String Path::GetExtension(const Proud::String& stringInput)
	{
		Proud::String outExtension;

		const int32_t length(stringInput.GetLength());
		if (length <= 1)
			return outExtension;


		const PNTCHAR* const stringPointer(stringInput.GetString());

		uint32_t indexAfterDot(length);
		while (indexAfterDot--)
		{
			if (stringPointer[indexAfterDot] == _PNT('.'))
			{
				++indexAfterDot;
				break;
			}
		}

		if (indexAfterDot == 0 || (indexAfterDot == length))
			return outExtension;


		outExtension = stringInput.Mid(indexAfterDot, length - indexAfterDot);
		return outExtension;
	}

	Proud::String Path::RemoveExtension(const Proud::String& stringInput)
	{
		Proud::String stringOutput(stringInput);

		const int32_t length(stringOutput.GetLength());
		if (length <= 1)
			return stringOutput;


#ifdef _PNUNICODE
		const wchar_t* const stringPointer(stringOutput.GetString());
#else
		const char* const stringPointer(stringOutput.GetString());
#endif

		uint32_t indexAfterDot(length);
		while (indexAfterDot--)
		{
			if (stringPointer[indexAfterDot] == _PNT('.'))
			{
				++indexAfterDot;
				break;
			}
		}

		if (indexAfterDot == 0 || (indexAfterDot == length))
			return stringOutput;


		PNTCHAR* buf = stringOutput.GetBuffer();
		buf[indexAfterDot] = 0;
		stringOutput.ReleaseBuffer();

		return stringOutput;
	}
}