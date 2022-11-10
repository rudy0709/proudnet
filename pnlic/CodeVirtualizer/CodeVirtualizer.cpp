#include "CodeVirtualizer.h"

/*
	To test the library, include "CodeVirtualizer.h" from an application project
	and call CodeVirtualizerTest().
	
	Do not forget to add the library to Project Dependencies in Visual Studio.
*/

static int s_Test = 0;

extern "C" int CodeVirtualizerTest();

int CodeVirtualizerTest()
{
	return ++s_Test;
}