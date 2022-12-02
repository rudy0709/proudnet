#include "NativeType.h"
#include "ProudNetClientPlugin.h"
#include "EventWrap.h"

#if defined(__ORBIS__)

// ============================================================
// This event is triggered for SCE_SYSTEM_SERVICE_EVENT_ENTITLEMENT_UPDATE.
struct OnEntitlementUpdate { SceSystemServiceEvent params; };
REGISTER_EVENT_ID(0xDE76F015C0DE4BE8ULL, 0x9046B1153C877E39ULL, OnEntitlementUpdate)

void OnEntitlementUpdateHandler(const OnEntitlementUpdate &eventData)
{
	Assert(eventData.params.eventType == SCE_SYSTEM_SERVICE_EVENT_ENTITLEMENT_UPDATE);

	printf("SCE_SYSTEM_SERVICE_EVENT_ENTITLEMENT_UPDATE\n");
}

UnityEventQueue::StaticFunctionEventHandler<OnEntitlementUpdate>	g_OnEntitlementUpdateHandler(OnEntitlementUpdateHandler);


// ============================================================
// This event is triggered for SCE_SYSTEM_SERVICE_EVENT_APP_LAUNCH_LINK.
struct OnLaunchLink { SceSystemServiceEvent params; };
REGISTER_EVENT_ID(0x477AFB5C1CA045D6ULL, 0x95E9C61B8365A66AULL, OnLaunchLink)

void OnLaunchLinkHandler(const OnLaunchLink &eventData)
{
	Assert(eventData.params.eventType == SCE_SYSTEM_SERVICE_EVENT_APP_LAUNCH_LINK);

	uint32_t size = eventData.params.data.appLaunchLink.size;
	const uint8_t *  data = eventData.params.data.appLaunchLink.arg;

	printf("SCE_SYSTEM_SERVICE_EVENT_APP_LAUNCH_LINK\n");
}

UnityEventQueue::StaticFunctionEventHandler<OnLaunchLink>	g_OnLaunchLinkHandler(OnLaunchLinkHandler);




extern "C" int module_start(size_t sz, const void* arg)
{
	if (!ProcessPrxPluginArgs(sz, arg, "NativePluginPS4Example"))
	{
		// Failed.
		return -1;
	}

	// An example of handling system service events
	UnityEventQueue::IEventQueue* eventQueue = GetRuntimeInterface<UnityEventQueue::IEventQueue>(PRX_PLUGIN_IFACE_ID_GLOBAL_EVENT_QUEUE);
	eventQueue->AddHandler(&g_OnEntitlementUpdateHandler);
	eventQueue->AddHandler(&g_OnLaunchLinkHandler);

	return 0;
}
#endif //defined(__ORBIS__)


// C# <-> Wrap <-> C++ 연결해 주는 NetClientEvent Wrap 객체를 생성합니다.
void* NativeToNetClientEventWrap_New()
{
	return new CNetClientEventWrap();
}

void NativeToNetClientEventWrap_Delete(void* eventWrap)
{
	if (eventWrap != NULL)
	{
		delete static_cast<CNetClientEventWrap*>(eventWrap);
	}
}

// C# <-> Wrap <-> C++ 연결해 주는 RmiStub Wrap 객체를 생성합니다.
void* NativeToRmiStubWrap_New()
{
	return new CRmiStubWrap();
}

void NativeToRmiStubWrap_Delete(void* stubWrap)
{
	if (stubWrap != NULL)
	{
		delete static_cast<CRmiStubWrap*>(stubWrap);
	}
}

// C# <-> Wrap <-> C++ 연결해 주는 RmiProxy Wrap 객체를 생성합니다.
void* NativeToRmiProxyWrap_New()
{
	return new CRmiProxyWrap();
}

void NativeToRmiProxyWrap_Delete(void* proxyWrap)
{
	if (proxyWrap != NULL)
	{
		delete static_cast<CRmiProxyWrap*>(proxyWrap);
	}
}

// C# <-> Wrap <-> C++ 연결해 주는 RmiContext Wrap 객체를 생성합니다.
void* NativeToRmiContext_New()
{
	return new Proud::RmiContext();
}

void NativeToRmiContext_Delete(void* context)
{
	if (NULL != context)
	{
		delete static_cast<Proud::RmiContext*>(context);
	}
}

// UnityEngine 에디터에서 사용할 때 에디터를 종료하는 과정에서
// GC에 의해 메모리가 정리되는 과정에서 불규칙적으로 CriticalSection 소멸자에서 ShowUserMisuseError 함수가 호출됩니다.
// 비 정상적인 상황이긴 하지만 C++ 코드에서는 발생하지 않으며 에디터가 종료가 될 때에만 발생하기 때문에 유니티일 경우에는
// ErrorReactionType을 DebugOutputType으로 변경합니다. (throw가 발생해도 무시합니다. 동작에는 문제가 없습니다).
void ChangeErrorReactionTypeToDebugOutputTypeWhenUnityEngine()
{
	using namespace Proud;
	CNetConfig::UserMisuseErrorReaction = ErrorReaction_DebugOutput;
}

//////////////////////////////////////////////////////////////////////////

// C# NetClientEvent와 C++ NetClientWrapper 클래스를 연결합니다.(콜백 이벤트)
void NetClientEvent_SetCSharpHandle(void* obj1, void* obj2)
{
	assert(obj1);
	assert(obj2);

	CNetClientEventWrap* netEventWrap = static_cast<CNetClientEventWrap*>(obj1);
	netEventWrap->m_csharpHandle = obj2;
}


void NetClientEvent_SetCallbackJoinServerComplete(void* obj, CallbackJoinServerComplete callback)
{
	assert(obj);
	assert(callback);

	CNetClientEventWrap* netEventWrap = static_cast<CNetClientEventWrap*>(obj);
	netEventWrap->m_callBackJoinServerComplete = callback;
}

void NetClientEvent_SetCallbackLeaveServer(void* obj, CallbackLeaveServer callback)
{
	assert(obj);
	assert(callback);

	CNetClientEventWrap* netEventWrap = static_cast<CNetClientEventWrap*>(obj);
	netEventWrap->m_callbackLeaveServer = callback;
}

void NetClientEvent_SetCallbackP2PMemberJoin(void* obj, CallbackP2PMemberJoin callback)
{
	assert(obj);
	CNetClientEventWrap* netEventWrap = static_cast<CNetClientEventWrap*>(obj);
	netEventWrap->m_callbackP2PMemberJoin = callback;
}

void NetClientEvent_SetCallbackP2PMemberLeave(void* obj, CallbackP2PMemberLeave callback)
{
	assert(obj);
	CNetClientEventWrap* netEventWrap = static_cast<CNetClientEventWrap*>(obj);
	netEventWrap->m_callbackP2PMemberLeave = callback;
}

void NetClientEvent_SetCallbackChangeP2PRelayState(void* obj, CallbackChangeP2PRelayState callback)
{
	assert(obj);
	CNetClientEventWrap* netEventWrap = static_cast<CNetClientEventWrap*>(obj);
	netEventWrap->m_callbackChangeP2PRelayState = callback;
}

void NetClientEvent_SetCallbackChangeServerUdpState(void* obj, CallbackChangeServerUdpState callback)
{
	assert(obj);
	CNetClientEventWrap* netEventWrap = static_cast<CNetClientEventWrap*>(obj);
	netEventWrap->m_callbackChangeServerUdpState = callback;
}

void NetClientEvent_SetCallbackSynchronizeServerTime(void* obj, CallbackSynchronizeServerTime callback)
{
	assert(obj);
	CNetClientEventWrap* netEventWrap = static_cast<CNetClientEventWrap*>(obj);
	netEventWrap->m_callbackSynchronizeServerTime = callback;
}

void NetClientEvent_SetCallbackError(void* obj, CallbackError callback)
{
	assert(obj);
	CNetClientEventWrap* netEventWrap = static_cast<CNetClientEventWrap*>(obj);
	netEventWrap->m_callbackError = callback;
}

void NetClientEvent_SetCallbackWarning(void* obj, CallbackWarning callback)
{
	assert(obj);
	CNetClientEventWrap* netEventWrap = static_cast<CNetClientEventWrap*>(obj);
	netEventWrap->m_callbackWarning = callback;
}

void NetClientEvent_SetCallbackInformation(void* obj, CallbackInformation callback)
{
	assert(obj);
	CNetClientEventWrap* netEventWrap = static_cast<CNetClientEventWrap*>(obj);
	netEventWrap->m_callbackInformation = callback;
}

void NetClientEvent_SetCallbackException(void* obj, CallbackException callback)
{
	assert(obj);
	CNetClientEventWrap* netEventWrap = static_cast<CNetClientEventWrap*>(obj);
	netEventWrap->m_callbackException = callback;
}

void NetClientEvent_SetCallbackServerOffline(void* obj, CallbackServerOffline callback)
{
	assert(obj);
	CNetClientEventWrap* netEventWrap = static_cast<CNetClientEventWrap*>(obj);
	netEventWrap->m_callbackServerOffline = callback;
}

void NetClientEvent_SetCallbackServerOnline(void* obj, CallbackServerOnline callback)
{
	assert(obj);
	CNetClientEventWrap* netEventWrap = static_cast<CNetClientEventWrap*>(obj);
	netEventWrap->m_callbackServerOnline = callback;
}

void NetClientEvent_SetCallbackP2PMemberOffline(void* obj, CallbackP2PMemberOffline callback)
{
	assert(obj);
	CNetClientEventWrap* netEventWrap = static_cast<CNetClientEventWrap*>(obj);
	netEventWrap->m_callbackP2PMemberOffline = callback;
}

void NetClientEvent_SetCallbackP2PMemberOnline(void* obj, CallbackP2PMemberOnline callback)
{
	assert(obj);
	CNetClientEventWrap* netEventWrap = static_cast<CNetClientEventWrap*>(obj);
	netEventWrap->m_callbackP2PMemberOnline = callback;
}

void NetClientEvent_SetCallbackNoRmiProcessed(void* obj, CallbackNoRmiProcessed callback)
{
	assert(obj);
	CNetClientEventWrap* netEventWrap = static_cast<CNetClientEventWrap*>(obj);
	netEventWrap->m_callbackNoRmiProcessed = callback;
}

void NetClientEvent_SetCallbackReceiveUserMessage(void* obj, CallbackReceiveUserMessage callback)
{
	assert(obj);
	CNetClientEventWrap* netEventWrap = static_cast<CNetClientEventWrap*>(obj);
	netEventWrap->m_callbackReceiveUserMessage = callback;
}

//////////////////////////////////////////////////////////////////////////

// C# RmiStub와 C++ RmiStubWrapper 클래스를 연결합니다.(콜백 이벤트)
void RmiStub_SetCSharpHandle(void* obj1, void* obj2)
{
	assert(obj1);
	assert(obj2);

	CRmiStubWrap* stubWrap = static_cast<CRmiStubWrap*>(obj1);
	stubWrap->m_csharpHandle = obj2;
}

void RmiStub_SetCallbackGetRmiIDList(void* obj, CallbackGetRmiIDList callback)
{
	assert(obj);
	assert(callback);

	CRmiStubWrap* stubWrap = static_cast<CRmiStubWrap*>(obj);
	stubWrap->m_callbackGetRmiIDList = callback;
}

void RmiStub_SetCallbackGetRmiIDListCount(void* obj, CallbackGetRmiIDListCount callback)
{
	assert(obj);
	assert(callback);

	CRmiStubWrap* stubWrap = static_cast<CRmiStubWrap*>(obj);
	stubWrap->m_callbackGetRmiIDListCount = callback;
}

void RmiStub_SetCallbackProcessReceivedMessage(void* obj, CallbackProcessReceivedMessage callback)
{
	assert(obj);
	assert(callback);

	CRmiStubWrap* stubWrap = static_cast<CRmiStubWrap*>(obj);
	stubWrap->m_callbackProcessReceivedMessage = callback;
}

//////////////////////////////////////////////////////////////////////////

// C# RmiProxy와 C++ RmiProxyWrapper 클래스를 연결합니다.(콜백 이벤트)
// C++ RmiProxy 멤버 변수에 접근하는 함수들입니다.
void RmiProxy_SetCSharpHandle(void* obj1, void* obj2)
{
	assert(obj1);
	assert(obj2);

	CRmiProxyWrap* proxyWrap = static_cast<CRmiProxyWrap*>(obj1);
	proxyWrap->m_csharpHandle = obj2;
}

void RmiProxy_SetCallbackGetRmiIDList(void* obj, CallbackGetRmiIDList callback)
{
	assert(obj);
	assert(callback);

	CRmiProxyWrap* stubWrap = static_cast<CRmiProxyWrap*>(obj);
	stubWrap->m_callbackGetRmiIDList = callback;
}

void RmiProxy_SetCallbackGetRmiIDListCount(void* obj, CallbackGetRmiIDListCount callback)
{
	assert(obj);
	assert(callback);

	CRmiProxyWrap* stubWrap = static_cast<CRmiProxyWrap*>(obj);
	stubWrap->m_callbackGetRmiIDListCount = callback;
}

bool RmiProxy_RmiSend(void* obj1, void* remotes, int remoteCount, Proud::RmiContext* context, void* data, int dataLength, std::string RMIName, int RMIId)
{
	using namespace Proud;

	if (dataLength <= 0)
	{
		return false;
	}

	assert(obj1);
	assert(remotes);
	assert(context);
	assert(data);

	CRmiProxyWrap* proxy = static_cast<CRmiProxyWrap*>(obj1);
	//Proud::RmiContext* context = static_cast<Proud::RmiContext*>(obj2);

	Proud::CMessage msg;
	msg.UseExternalBuffer(static_cast<uint8_t*>(data), dataLength);
	msg.SetLength(dataLength);

	Proud::String rmiName = StringA2T(RMIName.c_str());

	return proxy->RmiSend(static_cast<Proud::HostID*>(remotes), remoteCount, *context, msg, rmiName.c_str(), static_cast<Proud::RmiID>(RMIId));
}

// C++ ReceivedMessage 멤버함수에 접근하는 함수를 정의합니다.

//////////////////////////////////////////////////////////////////////////

// C++ RmiContext 멤버변수를 핸들링하는 함수를 정의합니다.

void RmiContext_SetHostTag(Proud::RmiContext* context, int64_t value)
{
	assert(context);
	context->m_hostTag = reinterpret_cast<void*>(value);

}

bool RmiContext_GetRelayed(void* obj)
{
	assert(obj);

	Proud::RmiContext* context = static_cast<Proud::RmiContext*>(obj);
	return context->m_relayed;
}

int RmiContext_GetSentFrom(void* obj)
{
	assert(obj);

	Proud::RmiContext* context = static_cast<Proud::RmiContext*>(obj);
	return context->m_sentFrom;
}

int RmiContext_GetMaxDirectP2PMulticastCount(void* obj)
{
	assert(obj);

	Proud::RmiContext* context = static_cast<Proud::RmiContext*>(obj);
	return context->m_maxDirectP2PMulticastCount;
}

int RmiContext_GetPriority(void* obj)
{
	assert(obj);

	Proud::RmiContext* context = static_cast<Proud::RmiContext*>(obj);
	return context->m_priority;
}

int RmiContext_GetReliability(void* obj)
{
	assert(obj);
	Proud::RmiContext* context = static_cast<Proud::RmiContext*>(obj);
	return context->m_priority;
}

int RmiContext_GetEncryptMode(void* obj)
{
	assert(obj);
	Proud::RmiContext* context = static_cast<Proud::RmiContext*>(obj);
	return context->m_encryptMode;

}

int RmiContext_GetCompressModee(void* obj)
{
	assert(obj);
	Proud::RmiContext* context = static_cast<Proud::RmiContext*>(obj);
	return context->m_compressMode;
}

/*CReceivedMessage에서 address 부분을 얻는다. ipv4 or 6 주소 데이터의 위치를 얻어오므로, ptr*이다.
주소 데이터의 원본 위치다. 객체 따로 할당한 것 아님. 따라서 free or delete 불필요. */
void* ReceivedMessage_GetRemoteAddress(void* obj)
{
	assert(obj);
	Proud::CReceivedMessage* recvMsg = static_cast<Proud::CReceivedMessage*>(obj);
	return &(recvMsg->m_remoteAddr_onlyUdp.m_addr);
}


int64_t RmiContext_GetHostTag(void* obj)
{
	assert(obj);
	Proud::RmiContext* context = static_cast<Proud::RmiContext*>(obj);
	return reinterpret_cast<int64_t>(context->m_hostTag);
}

//////////////////////////////////////////////////////////////////////////

// ByteArray를 C# <-> C++를 변환하는 함수를 정의합니다.

void NativeToByteArray_Delete(void* byteArray)
{
	if (byteArray != NULL)
	{
		delete static_cast<Proud::ByteArray*>(byteArray);
	}
}

//////////////////////////////////////////////////////////////////////////

int AddrPort_GetAddrSize()
{
	return sizeof(Proud::AddrPort::ExtendAddr);
}

//////////////////////////////////////////////////////////////////////////

void* ByteArrayToNative(void* data, int dataLength)
{
	if (dataLength <= 0)
	{
		return NULL;
	}

	Proud::ByteArray* byteArray = new Proud::ByteArray();

	byteArray->SetCount(dataLength);
	memcpy(byteArray->GetData(), data, dataLength);

	return byteArray;
}

void CopyManagedByteArrayToNativeByteArray(void* data, int dataLength, void* nativeData)
{
	if (dataLength <= 0)
	{
		return;
	}

	assert(nativeData);

	Proud::ByteArray* byteArray = static_cast<Proud::ByteArray*>(nativeData);
	byteArray->SetCount(dataLength);
	memcpy(byteArray->GetData(), data, dataLength);
}

int ByteArray_GetCount(void* obj)
{
	assert(obj);

	Proud::ByteArray* byteArray = static_cast<Proud::ByteArray*>(obj);

	return byteArray->GetCount();
}

void CopyNativeByteArrayToManageByteArray(void* dst, void* obj)
{
	assert(obj);
	assert(dst);

	Proud::ByteArray* src = static_cast<Proud::ByteArray*>(obj);

	memcpy(dst, src->GetData(), src->GetCount());
}

void CopyNativeByteArrayToManageByteArray(void* dst, void* src, int length)
{
	assert(dst);
	assert(src);
	assert(0 < length);

	memcpy(dst, src, length);
}

Proud::String ConvertNatvieStringToManagedString(void* obj)
{
	assert(obj);

	Proud::String* str = static_cast<Proud::String*>(obj);
	return (*str);
}

void CopyNativeAddrToManagedAddr(void* dst, void* src, int length)
{
	assert(dst);
	assert(src);
	assert(length > 0);

	memcpy(dst, src, length);
}
