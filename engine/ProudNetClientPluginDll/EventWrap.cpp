#include "EventWrap.h"

//////////////////////////////////////////////////////////////////////////

// ikpil.choi : 2016-12-15, Proud::String 으로 전파합니다 A2T
Proud::String GetExceptionMessage(Proud::Exception const& e)
{
	using namespace Proud;

	if (e.m_exceptionType == Proud::ExceptionType_ComError)
	{
#if defined(_WIN32)
		Proud::String msg = StringA2T(e.chMsg.GetString());
		return msg;
#endif //defined(_WIN32)
	}
	else if (e.m_exceptionType == Proud::ExceptionType_Std)
	{
		return StringA2T(e.m_pStdSource->what());
	}
	else if (e.m_exceptionType == Proud::ExceptionType_Void)
	{
		return StringA2T("exception of voidptr type!");
	}
	else if (e.m_exceptionType == Proud::ExceptionType_Unhandled)
	{
		return StringA2T("Unhandle Exception!");
	}

	return StringA2T(e.what());
}

//////////////////////////////////////////////////////////////////////////

CEventWrap::CEventWrap()
	: m_csharpHandle(NULL)
{
}


CEventWrap::~CEventWrap()
{
}

//////////////////////////////////////////////////////////////////////////

CNetClientEventWrap::CNetClientEventWrap()
	: CEventWrap()
	, m_callBackJoinServerComplete(NULL)
	, m_callbackLeaveServer(NULL)
	, m_callbackP2PMemberJoin(NULL)
	, m_callbackP2PMemberLeave(NULL)
	, m_callbackChangeP2PRelayState(NULL)
	, m_callbackChangeServerUdpState(NULL)
	, m_callbackSynchronizeServerTime(NULL)
	, m_callbackError(NULL)
	, m_callbackWarning(NULL)
	, m_callbackInformation(NULL)
	, m_callbackException(NULL)
	, m_callbackServerOffline(NULL)
	, m_callbackServerOnline(NULL)
	, m_callbackP2PMemberOffline(NULL)
	, m_callbackP2PMemberOnline(NULL)
	, m_callbackNoRmiProcessed(NULL)
	, m_callbackReceiveUserMessage(NULL)
{
}

CNetClientEventWrap::~CNetClientEventWrap()
{

}

void CNetClientEventWrap::OnJoinServerComplete(Proud::ErrorInfo* info, const Proud::ByteArray& replyFromServer)
{
	assert(m_callBackJoinServerComplete);
	assert(m_csharpHandle);
	m_callBackJoinServerComplete(m_csharpHandle, info, const_cast<Proud::ByteArray*>(&replyFromServer));
}

void CNetClientEventWrap::OnLeaveServer(Proud::ErrorInfo *errorinfo)
{
	assert(m_callbackLeaveServer);
	assert(m_csharpHandle);
	m_callbackLeaveServer(m_csharpHandle, errorinfo);
}

void CNetClientEventWrap::OnP2PMemberJoin(Proud::HostID memberHostID, Proud::HostID groupHostID, int memberCount, const Proud::ByteArray &message)
{
	assert(m_callbackP2PMemberJoin);
	assert(m_csharpHandle);
	m_callbackP2PMemberJoin(m_csharpHandle, memberHostID, groupHostID, memberCount, const_cast<Proud::ByteArray*>(&message));
}

void CNetClientEventWrap::OnP2PMemberLeave(Proud::HostID memberHostID, Proud::HostID groupHostID, int memberCount)
{
	assert(m_callbackP2PMemberLeave);
	assert(m_csharpHandle);
	m_callbackP2PMemberLeave(m_csharpHandle, memberHostID, groupHostID, memberCount);
}

void CNetClientEventWrap::OnChangeP2PRelayState(Proud::HostID remoteHostID, Proud::ErrorType reason)
{
	assert(m_callbackChangeP2PRelayState);
	assert(m_csharpHandle);
	m_callbackChangeP2PRelayState(m_csharpHandle, remoteHostID, (int)reason);
}

void CNetClientEventWrap::OnChangeServerUdpState(Proud::ErrorType reason)
{
	assert(m_callbackChangeServerUdpState);
	assert(m_csharpHandle);
	m_callbackChangeServerUdpState(m_csharpHandle, (int)reason);
}

void CNetClientEventWrap::OnSynchronizeServerTime()
{
	assert(m_callbackSynchronizeServerTime);
	assert(m_csharpHandle);
	m_callbackSynchronizeServerTime(m_csharpHandle);
}

void CNetClientEventWrap::OnError(Proud::ErrorInfo *errorInfo)
{
	assert(m_callbackError);
	assert(m_csharpHandle);
	m_callbackError(m_csharpHandle, errorInfo);
}


void CNetClientEventWrap::OnWarning(Proud::ErrorInfo *errorInfo)
{
	assert(m_callbackWarning);
	assert(m_csharpHandle);
	m_callbackWarning(m_csharpHandle, errorInfo);
}

void CNetClientEventWrap::OnInformation(Proud::ErrorInfo *errorInfo)
{
	assert(m_callbackInformation);
	assert(m_csharpHandle);
	m_callbackInformation(m_csharpHandle, errorInfo);
}

void CNetClientEventWrap::OnException(const Proud::Exception& e)
{
	assert(m_callbackException);

	// ikpil.choi : 2016-12-15, Proud::String 으로 받아, Swig에 그 주소를 천파합니다.
	Proud::String msg = GetExceptionMessage(e);
	assert(m_csharpHandle);
	m_callbackException(m_csharpHandle, e.m_remote, &msg);
}

void CNetClientEventWrap::OnNoRmiProcessed(Proud::RmiID rmiID)
{
	assert(m_callbackNoRmiProcessed);
	assert(m_csharpHandle);
	m_callbackNoRmiProcessed(m_csharpHandle, rmiID);
}

void CNetClientEventWrap::OnReceiveUserMessage(Proud::HostID sender, const Proud::RmiContext &rmiContext, uint8_t* payload, int payloadLength)
{
	assert(m_callbackReceiveUserMessage);
	assert(m_csharpHandle);
	m_callbackReceiveUserMessage(m_csharpHandle, sender, const_cast<Proud::RmiContext*>(&rmiContext), payload, payloadLength);
}

void CNetClientEventWrap::OnServerOffline(Proud::CRemoteOfflineEventArgs &args)
{
	assert(m_callbackServerOffline);
	assert(m_csharpHandle);
	m_callbackServerOffline(m_csharpHandle, &args);
}

void CNetClientEventWrap::OnServerOnline(Proud::CRemoteOnlineEventArgs &args)
{
	assert(m_callbackServerOnline);
	assert(m_csharpHandle);
	m_callbackServerOnline(m_csharpHandle, &args);
}

void CNetClientEventWrap::OnP2PMemberOffline(Proud::CRemoteOfflineEventArgs &args)
{
	assert(m_callbackP2PMemberOffline);
	assert(m_csharpHandle);
	m_callbackP2PMemberOffline(m_csharpHandle, &args);
}

void CNetClientEventWrap::OnP2PMemberOnline(Proud::CRemoteOnlineEventArgs &args)
{
	assert(m_callbackP2PMemberOnline);
	assert(m_csharpHandle);
	m_callbackP2PMemberOnline(m_csharpHandle, &args);
}

//////////////////////////////////////////////////////////////////////////

CRmiStubWrap::CRmiStubWrap()
	: CEventWrap()
	, m_callbackGetRmiIDList(NULL)
	, m_callbackGetRmiIDListCount(NULL)
	, m_callbackProcessReceivedMessage(NULL)
{
}

CRmiStubWrap::~CRmiStubWrap()
{
}

Proud::RmiID* CRmiStubWrap::GetRmiIDList()
{
	assert(m_csharpHandle);
	assert(m_callbackGetRmiIDList);
	return static_cast<Proud::RmiID*>(m_callbackGetRmiIDList(m_csharpHandle));
}

int CRmiStubWrap::GetRmiIDListCount()
{
	assert(m_csharpHandle);
	assert(m_callbackGetRmiIDListCount);
	return m_callbackGetRmiIDListCount(m_csharpHandle);
}

bool CRmiStubWrap::ProcessReceivedMessage(Proud::CReceivedMessage& pa, void* hostTag)
{
	assert(m_csharpHandle);
	assert(m_callbackProcessReceivedMessage);
	return m_callbackProcessReceivedMessage(m_csharpHandle, &pa, reinterpret_cast<int64_t>(hostTag));
}

//////////////////////////////////////////////////////////////////////////

CRmiProxyWrap::CRmiProxyWrap()
	: CEventWrap()
	, m_callbackGetRmiIDList(NULL)
	, m_callbackGetRmiIDListCount(NULL)
{
}

CRmiProxyWrap::~CRmiProxyWrap()
{
}

Proud::RmiID* CRmiProxyWrap::GetRmiIDList()
{
	assert(m_csharpHandle);
	assert(m_callbackGetRmiIDList);
	return static_cast<Proud::RmiID*>(m_callbackGetRmiIDList(m_csharpHandle));
}

int CRmiProxyWrap::GetRmiIDListCount()
{
	assert(m_csharpHandle);
	assert(m_callbackGetRmiIDListCount);
	return m_callbackGetRmiIDListCount(m_csharpHandle);
}
