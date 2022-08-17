﻿/*
ProudNet HERE_SHALL_BE_EDITED_BY_BUILD_HELPER


이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의 : 저작물에 관한 위의 명시를 제거하지 마십시오.


This program is soley copyrighted by Nettention.
Any use, correction, and distribution of this program are subject to the terms and conditions of the License Agreement.
Any violated use of this program is prohibited and will be cause of immediate termination according to the License Agreement.

** WARNING : PLEASE DO NOT REMOVE THE LEGAL NOTICE ABOVE.


此程序的版权归Nettention公司所有。
与此程序的修改、使用、发布相关的事项要遵守此程序的所有权者的协议。
不遵守协议时要原则性的禁止擅自使用。
擅自使用的责任明示在与此程序所有权者的合同书里。

** 注意：不要移除关于制作物的上述明示。


このプログラムの著作権はNettentionにあります。
このプログラムの修正、使用、配布に関する事項は本プログラムの所有権者との契約に従い、
契約を遵守しない場合、原則的に無断使用を禁じます。
無断使用による責任は本プログラムの所有権者との契約書に明示されています。

** 注意：著作物に関する上記の明示を除去しないでください。

*/

#pragma once

#ifdef _MSC_VER
#pragma warning(error:4150) // deletion of pointer to incomplete type 'Proud::CP2PGroup_S'; no destructor called
#endif

// 이게 가장 먼저 있어야 빌드 에러가 준다.
#if defined(_WIN32)
#include <winsock2.h>
#endif

#include "CriticalSect.h"
#include "HostIDArray.h"
#include "IRMIStub.h"
#include "IRMIProxy.h"
#include "Marshaler.h"
#include "MilisecTimer.h"
#include "NetPeerInfo.h"
#include "P2PGroup.h"

#include "FastMap.h"
#include "FastList.h"
#include "FastArray.h"
#include "ReceivedMessage.h"
#include "RMIContext.h"
#include "AddrPort.h"
#include "Enums.h"
#include "MessageSummary.h"
#include "ErrorInfo.h"
#include "FakeClr.h"
#include "Message.h"
#include "Ptr.h"
#include "SendData.h"
#include "FixedLengthArray.h"
#include "sysutil.h"
#include "PNSemaphore.h"
#include "ThreadPool.h"
#include "ThreadUtil.h"
#include "TimerThread.h"
#include "EmergencyLogCommon.h"

#ifdef USE_HLA
#include "Hla_C.h"
#endif // USE_HLA

