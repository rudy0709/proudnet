#pragma once 

#ifndef __ANDROID__ // #TODO Android는 unwind API라는 것을 쓰도록 하자.

#include "../include/PNString.h"
#include "../include/FastArray.h"

namespace Proud
{
	void GetStackTrace_Unix(CFastArray<StringA>& output);
}


#endif