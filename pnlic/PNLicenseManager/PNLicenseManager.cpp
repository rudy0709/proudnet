/*
	To test the library, include "LinuxLicenseManager.h" from an application project
	and call LinuxLicenseManagerTest().
	
	Do not forget to add the library to Project Dependencies in Visual Studio.
*/
#include "stdafx.h"

static int s_Test = 0;

extern "C" int PNLicenseManagerTest();

int PNLicenseManagerTest()
{
	return ++s_Test;
}