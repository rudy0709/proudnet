
// PNLicenseHidden.h : main header file for the PROJECT_NAME application
//

#pragma once

#include "stdafx.h"

#if defined(__linux__)
#include <sys/wait.h>
#include <sys/stat.h>
#include "../AuthNetLib/include/pnmutex.h"
#include "../AuthNetLib/include/ThreadUtil.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "PNLicInformer.h"
#include "../PNLicenseSDK/PNLicenseSDK.h"
#include "../PNLicenseManager/PNTypes.h"

#include "../CodeVirtualizer/Include/C/VirtualizerSDK.h"

#include "../PNLicenseManager/LicenseType.h"

#pragma comment(lib, "PNLicenseSDK")
using namespace Proud;

