// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "..\include\sysutil.h"

namespace Proud
{
	volatile bool g_ProudDllsAreUnloading = false;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		NTTNTRACE("DLL_THREAD_DETACH\n");
		break;
	case DLL_PROCESS_DETACH:
		Proud::g_ProudDllsAreUnloading = true;
		Proud::Thread::NotifyDllProcessDetached();

		break;
	}
	return TRUE;
}
