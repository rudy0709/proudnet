// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#if defined(__linux__)
#else
#include <atlbase.h>
#include <stdio.h>
#include <tchar.h>

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#endif

#include <iostream>


// TODO: reference additional headers your program requires here

//#include "../../AuthNetLib/include/AdoWrap.h"
//#include "../../AuthNetLib/include/CoInit.h"
//#include "../../AuthNetLib/include/Tracer.h"
//#include "../../AuthNetLib/include/NTService.h"
//#include "../../AuthNetLib/include/ProudNetServer.h"

//using namespace Proud;
