#include "stdafx.h"
#include "../include/CoInit.h"

#if defined(_WIN32)

namespace Proud
{

	CCoInitializer::CCoInitializer()
	{
		// CoInitialize(0)은 STA로 작동. 즉, C# BeginInvoke처럼 작동해서 느리다. 따라서 아래와 같이 옵션을 붙여야.
		HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if (SUCCEEDED(hr))
			m_success = true;
		else
			m_success = false;
	}

	CCoInitializer::~CCoInitializer()
	{
		if (m_success)
			CoUninitialize();
	}
}

#endif // _WIN32