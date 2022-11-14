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

//#include "../../AuthNet/include/AdoWrap.h"
// #include "../../AuthNet/include/CoInit.h"
//#include "../../AuthNet/include/Tracer.h"
//#include "../../AuthNet/include/NTService.h"
//#include "../../AuthNet/include/ProudNetServer.h"

//using namespace Proud;