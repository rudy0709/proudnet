/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once

#include "../include/ProudNetCommon.h"
#include "../include/CriticalSect.h"
#include "../include/Tracer.h"
#include "../include/Crypto.h"
#include "../include/CryptoRsa.h"
#include "ISendDest_C.h"
#include "PooledObjects.h"
//#include "../include/ReaderWriterMonitor.h"
//#include "../include/MessageSummary.h"
//#include "../include/CompressEnum.h"
//#include "enumimpl.h"
#include "SocketPtrAndSerial.h"
#include "SuperSocket.h"
#include "SendReadyList.h"
#include "PooledObjects.h"
#include "UserTaskQueue.h"
#include "TimeEventThreadPool.h"
#include "FastArrayImpl.h"
#include "NetSettings.h"
#include "TimeAlarm.h"



namespace Proud
{
	extern const PNTCHAR* NoServerConnectionErrorText;


	class INetCoreEvent;	
	class CSendFragRefs;
	class CVizAgent;

	typedef CFastMap2 < HostID, CHostBase*, int > AuthedHostMap;

	// main 객체의 베이스 클래스
	class CNetCoreImpl : 
		public IRmiHost
		,public IUserTaskQueueOwner
		,public IThreadReferrer // custom value event를 처리하기 위해
	{
	protected:
		bool Send_SecureLayer(const CSendFragRefs& payload, const CSendFragRefs* compressedPayload, const SendOpt& sendContext, const HostID* sendTo, int numberOfsendTo );
		bool Send_CompressLayer(const CSendFragRefs &payload,const SendOpt& sendContext,const HostID *sendTo, int numberOfsendTo ,int &compressedPayloadLength);

		virtual bool Send_BroadcastLayer(
			const CSendFragRefs& payload,   // 쌩 메시지 내용.
			const CSendFragRefs* encryptedPayload,		// 암호화&압축 혹은 암호화만 된 메시지. may be null.
			const SendOpt& sendContext,
			const HostID* sendTo, int numberOfsendTo) = 0;

		virtual bool AsyncCallbackMayOccur() = 0;

		// 메모리 고갈 상태가 되면 NetCore는 가능한한 네트워킹 정상 종료 처리를 해주어야 한다.
		// (물론 클라의 경우 대부분의 경우 그래픽 등 다른 이유로 고갈이겠지만)
		// 그런데 종료 처리 과정도 제대로 수행되려면 약간의 메모리가 더 필요하다.
		// 이것은 미리 일정량 메모리를 갖고 있다가 메모리 고갈 상황에서 그때를 위한 잔여 공간을 반환해 주는 역할을 한다.
		// 아래 두 변수는 그러한 역할.
		CriticalSection m_preventOutOfMemoryCritSec;
		void* m_preventOutOfMemoryDisconnectError;

		void AllocPreventOutOfMemory();
		void FreePreventOutOfMemory();
	public:		
		virtual void EnqueError( ErrorInfoPtr info ) = 0;
		virtual void EnqueWarning( ErrorInfoPtr info ) = 0;

		//CReaderWriterMonitor_NORECURSE m_callbackMon;
		CFastArray<IRmiProxy*> m_proxyList_NOCSLOCK;
		CFastArray<IRmiStub*> m_stubList_NOCSLOCK;

		// RmiID 중복 검사를 하기위한 CFastArray
		CFastSet<RmiID> m_proxyRmiIDList_NOCSLOCK;
		CFastSet<RmiID> m_stubRmiIDList_NOCSLOCK;

		virtual INetCoreEvent *GetEventSink_NOCSLOCK() = 0;

		void AttachProxy(IRmiProxy *proxy);
		void AttachStub(IRmiStub *stub);

		void DetachProxy(IRmiProxy *proxy);
		void DetachStub(IRmiStub *stub); 

		virtual bool NextEncryptCount(HostID remote, CryptCount &output) = 0;
		virtual void PrevEncryptCount(HostID remote) = 0;
		virtual bool GetExpectedDecryptCount(HostID remote, CryptCount &output) = 0;
		virtual bool NextDecryptCount(HostID remote) = 0;

		void DoUserTask(CWorkResult* workResult);

		// 처리해야 하는 것이 local event인 경우 이것이 실행된다.
		// NC,NS등에서 이를 구현해야 한다.
		virtual void ProcessOneLocalEvent(LocalEvent& e, CHostBase* subject, const PNTCHAR*& outFunctionName, CWorkResult* workResult) = 0;

		// remote와의 통신에서 필요한 암호키 객체를 얻는다.
		// remote가 존재하는 뿐만 아니라 암호키 내용물도 모두 채워져 있어야 한다.
		virtual CSessionKey *GetCryptSessionKey(HostID remote, String &errorOut, bool& outEnqueError) = 0;

#ifdef _DEBUG
		void CheckCriticalsectionDeadLock(const PNTCHAR* functionName);
#define CHECK_CRITICALSECTION_DEADLOCK(netCore) { netCore->CheckCriticalsectionDeadLock(__FUNCTIONT__);}
#else
#define CHECK_CRITICALSECTION_DEADLOCK(netCore) __noop
#endif // _DEBUG

#ifdef _DEBUG
		// 하부 객체들 가령 SuperSocket이 하나라도 lock 걸려 있으면 에러를 토한다.
		// main lock 걸기 전에 데드락 없음을 검사하는 용도. 디버그에서만 쓰도록 하자. 매우 느리다.
		virtual void CheckCriticalsectionDeadLock_INTERNAL(const PNTCHAR* functionName) = 0;
#endif // _DEBUG

		/* 주고받을 수 있는 RMI 혹은 사용자 정의 메시지의 최대 크기다. */
		virtual int GetMessageMaxLength() = 0;

#ifdef _WIN32
		virtual void AddEmergencyLogList(int logLevel, LogCategory logCategory, const String &logMessage, const String &logFunction = _PNT(""), int logLine = 0) = 0;
#endif

		// 거참 이상하네...쓰지도 않는 메서드인데 왜 이걸 넣어야 정상 작동할까
		//virtual CFastHeap* GetUnsafeHeap() = 0;

		// 이 값이 true이면, 넷클라에서 패킷캡처-복제 재현을 할 수 있는 상태가 된다.
		// 더미 클라 테스트를 하려고 할때는 유용하지만 라이브 서비스에서는 반드시 꺼야 한다.
		// 클라가 서버에 접속하기 전에 클라 자체에서 미리 세팅되어야. 즉 Connect() 안에서 세팅됨.
		bool m_simplePacketMode;

		// 이 값이 true이면 유저 콜백에서 Exception 발생 시 OnException으로 넘겨주고,
		// false이면 그냥 크래시나도록 방치한다.
		bool m_allowOnExceptionCallback;

		CNetSettings m_settings;

		// cs locked인 경우 local event를 enque하기만 해야 한다. cs unlock인 경우에는 콜백 허용
		INetCoreEvent* m_eventSink_NOCSLOCK;

		void ShowError_NOCSLOCK(ErrorInfoPtr errorInfo);
		void ShowNotImplementedRmiWarning(const PNTCHAR* RMIName);
		void PostCheckReadMessage(CMessage &msg, const PNTCHAR* RMIName);

		bool ProcessMessage_Encrypted( MessageType msgType, CReceivedMessage &receivedInfo, CMessage& decryptedOutput );
		bool ProcessMessage_Compressed( CReceivedMessage &receivedInfo, CMessage& decryptedOutput );

		bool SendUserMessage(const HostID* remotes, int remoteCount, const RmiContext &rmiContext, uint8_t* payload, int payloadLength);

		CNetCoreImpl();
		virtual ~CNetCoreImpl();

		void CleanupEveryProxyAndStub();

		int GetFinalUserWotkItemCount();
		void ClearFinalUserWorkItemList();

		/* 우선, CNetCoreImpl.m_UdpAddrPortToRemoteClientIndex 를 NetCore로 옮깁시다.
			CNetCoreImpl.m_UdpAddrPortToRemoteClientIndex 즉 X는 UDP로 데이터그램을 받을 때 확인되는 송신자의 주소 즉 recvfrom의 출력 인자를 key로, 그리고 대응하는 remote 객체의 hostID를 value로 하는 맵입니다.
			그러나 이 맵은 결함이 있습니다.
			- 송신자의 NAT가 bad behavior인 경우 수신자의 주소가 서로 다름에도 불구하고 정상적인 host 식별을 못하는 문제가 있습니다.
			- hostID를 아직 안 가진 경우의 host 객체를 식별하는 것도 문제가 있습니다.

			이러한 문제를 해결하기 위해 이렇게 합시다.
			X를 이중맵으로 만듭시다. 1단계는 socket to Y map, 2단계는 Y to (CHostBase*,serial) map으로 갑시다.

			Y는 recvfrom으로 받는 송신자의 주소 혹은 wildcard입니다. wildcard가 뭔지는 아래에서 설명.

			자 이렇게 되면, CNetCore.OnMessageReceived에서 UDP로 패킷을 받으면 다음과 같이 처리해야 합니다.
			패킷을 받은 (UDP 소켓의 ptr and serial)을 key로, X로부터 Y를 찾습니다.
			이제, 받은 패킷의 sender addrPort를 근거로 Y로부터 (CHostBase*,CHostBase.m_serialNum)를 찾습니다.
			(CHostBase*,CHostBase.m_serialNum)로부터 CNetCoreImpl.m_hostInstances에서 CHostBase*를 찾습니다. (CSuperSOcket.m_serialNum처럼, ABA problem을 막기 위함)
			이제, CHostBase는 받은 UDP패킷에 대한 이벤트 처리합니다.

			위에처럼 X가 잘  작동하려면, X에 등록하거나 해제하는 루틴을 추가해야 합니다.
			NC의 경우, 피어 혹은 서버(!)와의 홀펀칭 과정에서 X에 등록을 해야 합니다.
			NS의 경우 클라와의 홀펀칭 과정에서 X에 등록해야 하고요.
			반대로, 해당 host가 파괴될 때 X에서 등록 해제를 해주어야 합니다.
			engine/main에서 기존 소스를 뒤지면 어디서 위 동록/등록해제를 해주면 되는지 답이 나올겁니다.

			map에서 wildcard란? ==> 어떤 값을 key로 넣든 무조건 동일한 값이 나오는 것을 의미합니다. 한마디로 key 값을 무시하는거죠.
			static assigned UDP socket의 경우 1개 UDP socket이 여러 host와의 통신을 담당하기 떄문에 addrPort to host map이 필요하지만 per-client UDP 및 per-peer UDP socket의 경우 딱 하나와의 host와의 통신을 담당하므로 map이 불필요 즉 wildcard입니다.
			class Y
			{
			value m_wildCardValue;
			value Find(key)
			{
			if (m_wildCardValue exists)
			{
			return m_wildCardValue;
			}
			else
			{
			return map.find(key);
			}
			}
			}

			한편 Proud.CSuperSocket.m_receivedAddrPortToVolatileHostIDMap는 여전히 존재해야 합니다. filterTag 처리를 위해 main lock을 걸 수는 없는 노릇이기 때문입니다.

			*/


		/*=> 이해 어려운 부분은 해당 부분을 부분을 집어주시지... 어쨌던 아래에 제가 구조를 달겠습니다.
			정리하다보니, 기존 SocketToHostMap까지 흡수해 버리는 구조로 가는 것이 좋겠다고 판단되어, 
			흡수 후 구조로 정리하겠습니다. 아래 소스를 확인 바랍니다.
			그리고 정리하다보니 CHostBase는 ABA problem이 없네요. 따라서 CHostBase.m_serialNum이 불필요.
			*/

#ifdef VIZAGENT
		// viz client 
		CHeldPtr<CVizAgent> m_vizAgent;
#endif
		// 파괴 순서를 보장하기 위해서 ptr을 가지고 있게 추가
		CSingleton<CFavoritePooledObjects>::Holder m_singletonDepedends;
	private:

		/* superSocket이 어떤 remote와 대응하는지를 가리키는 맵 객체.
		key: CSuperSocket X의 주소와 X의 tag 
		value: 이것이 대응하는 IHostObject 객체
		
		superSocket의 tag가 직접 remote를 가리키게 하면 안되는 이유: 
		OnMessageReceived에서 tag가 가리키는 객체가 돌발 사라질 수 있다.
		그렇다고 tag가 smart pointer라 하더라도, 
		이미 main에서 제외된 remote 객체이면 정상 처리를 하는 것도 안될 일.
		(main lock 후 알고보니 tag가 가리키는 remote 객체는 이미 사라진 상태.
		이러면 메모리는 안 긋더라도 이미 사라진 객체에 대한 행동을 취함으로
		비정상적 결과를 유발할 수 있다.)
		게다가, 어차피 UDP의 경우 수신자 주소로 remote를 lookup한다. 
		TCP라고 그러지 말란 법 없으므로, tag 대신 lookup을 한다.
		
		key값이 superSocket의 주소 뿐만 아니라 tag도 가지는 이유:
		delete X1 후 new X2를 직후 했을 때 X1주소=X2주소일 수 있다. 
		이러면 super socket이 가리키는 remote는 딴놈이 될 수 있다. 
		즉 ABA problem.
		따라서 이를 구별하기 위해 +1씩 증가하는 tag도 같이 key로 가진다.

		remote 객체는 즉시 파괴될 때 이 map에서도 사라진다.
		이러면 superSocket이 아직 살아있어도 map에서 매치되는 것이 없으므로 ok.

		remote는 superSocket과 일시적으로 1:1 관계를 어길 수 있어야 한다.
		연결유지기능 때문임.
		*/
// 		typedef CFastMap2<std::pair<CSuperSocket*, intptr_t>, CHostBase*, int> SocketToHostMap;
// 		SocketToHostMap m_socketToHostMap; // 직접 억세스하지 말고 SocketToHostMap_Get을 사용할 것
		
		// UDP로 수신된 주소를 근거로 host를 찾는 맵.
		// static-assigned UDP socket인 경우에나 맵이 사용되며, per-remote인 경우에는 맵이 무시된다.
		// per-remote인 경우 이 map은 key값이 뭐가 되던 항상 동일한 host를 리턴한다. 
		class CAddrPortToHostMap
		{
			// static assigned UDP socket인 경우 false, 여타의 경우 언제나 1개의 host를 가리키므로 true
			bool m_hasWildcard;
			// m_ignoreAddrPort=true일 때만 유효.
			// m_hasWildcard=true이나 이것이 NULL인 경우도 있으므로 m_wildcardHost가 m_hasWildcard를 대체할 수 없음.
			CHostBase* m_wildcardHost;
			CFastMap2<AddrPort, CHostBase*, int> m_recvAddrPortToHostMap;

		public:
			CAddrPortToHostMap() :
				m_hasWildcard(false),
				m_wildcardHost(NULL)
			{
				// code profile 결과 이거 무시 못하므로.
				m_recvAddrPortToHostMap.SetOptimalLoad_BestLookup();
			}

			CHostBase* Find(const AddrPort& recvAddrPort)
			{
				if (m_hasWildcard)
				{
					return m_wildcardHost;
				}

				CHostBase* hostBase = NULL;

				m_recvAddrPortToHostMap.TryGetValue(recvAddrPort, hostBase);

				return hostBase;
			}
			
			void SetWildcard(CHostBase* host)
			{
				m_hasWildcard	= true;
				m_wildcardHost	= host;
			}

			void Add(const AddrPort& recvAddrPort, CHostBase* host)
			{
				assert(recvAddrPort != AddrPort::Unassigned);
				m_recvAddrPortToHostMap.Add(recvAddrPort, host);
			}

			void Remove(const AddrPort& recvAddrPort)
			{
				m_recvAddrPortToHostMap.RemoveKey(recvAddrPort);
			}
		};

		//typedef RefCount<CAddrPortToHostMap> CAddrPortToHostMapPtr;

		// socket ptr이 hosts를 가리킨다. host가 아님에 주의.
		// code profile 결과, CAddrPortToHostMapPtr의 부하가 크다. 따라서, CAddrPortToHostMap*를 쓴다.
		typedef CFastMap2 < SocketPtrAndSerial, CAddrPortToHostMap*, int, SocketPtrAndSerialTraits > SocketsToHostsMap;
		SocketsToHostsMap m_socketToHostsMap;

	public:
		CHostBase* SocketToHostsMap_Get_NOLOCK(CSuperSocket* socket, const AddrPort& recvAddrPort);

		void SocketToHostsMap_SetForAnyAddr(CSuperSocket* socket, CHostBase* remote);
		void SocketToHostsMap_SetForAddr(CSuperSocket* socket, const AddrPort& recvAddrPort, CHostBase* remote);
		void SocketToHostsMap_RemoveForAnyAddr(CSuperSocket* socket);
		void SocketToHostsMap_RemoveForAddr(CSuperSocket* socket, const AddrPort& recvAddrPort);
	private:
		void SocketToHostsMap_AssertConsist();

	protected:
		typedef CFastMap2<CHostBase*, int64_t, int> RemoteToInt64Map;
		
		// 여기에 등록된 클라들은 곧 파괴될 것이다
		// issue중이던 것들은 바로 dispose시 에러가 발생한다. 혹은 issue recv/send에서 이벤트 없이 에러가 발생한다.
		// 그래서 이 변수가 쓰이는거다.
		// HostID=0인 경우, 즉 비인증 상태의 객체가 파괴되는 경우도 감안, HostID를 키로 두지 않는다.
		// key: object, value: disposed time
		RemoteToInt64Map m_garbagedHosts;
	public:
		void UngarbageHost(CHostBase* r);
		void ClearGarbagedHostMap();
		virtual void GarbageAllHosts();
		virtual bool CanDeleteNow();
		
		CHostBase* AuthedHostMap_Get(HostID hostID);
		void Candidate_Remove(CHostBase* rc);

	public:
		/* HostID가 발급된 remote host들이다. 
		서버에서는 remote client, 클라에서는 remote peer 및 remote server 등이 되겠다.
		loopback host도 여기 들어갈 수 있다. 자기 자신도 HostID를 가질 수 있으니까.

		CHostBase H의 소유권은 m_authedHostMap or m_candidateHosts에서 가진다. 
		따라서 위 목록들 모두에서 제거될 때 명시적인 delete H가 실행된다. (DoGarbageCollect_Host()에서.) */
		AuthedHostMap m_authedHostMap;

	protected:
		/* 아직 hostID가 발급되지 않은 remote host들이다.
		서버에서는, 인증 과정(세션키 발급, OnConnectionRequest 통과 등)이 안 끝나서 hostID가 없는 것들.
		클라에서는, 아직 hostID가 없는 p2p peer들. (그런 경우가 있긴 하겠냐?)

		여기 있는 것들은 빨리 m_authedHosts로 옮겨가 주어야 한다. 오래되면 자연 도태된다.
		
		소유권에 대해서는 m_authedHostMap에 기재되어 있으니 필독. */
		CFastMap2<CHostBase*, CHostBase*, int> m_candidateHosts;

		// 송신할 데이터는 갖고 있지만 아직 issue send or non block send가 되지 않은 것들.
		// coalesce를 위한 onTick에서 꺼내다 처리한다.
		CSendReadySockets* m_sendReadyList;

		/* 위 m_sendReadyList는 main lock 없이 접근된다.
		따라서 m_sendReadyList로부터 얻은 SuperSocket는 무효할 수 있다.
		이것의 유효성 검사는 main lock 상태에서 해야 한다. 그것을 위한 목록이다.
		이 목록에는 TCP socket들, UDP socket들이 들어간다. 
		per-remote UDP socket, per-remote TCP socket, static assigned UDP socket들이 들어간다. 
		즉 인덱스인 셈이다. m_sendReadyList에 있더라도 이 목록에 없으면 무효다.

		이 목록에 있는 객체들은 실제로 파괴되기 직전까지 항상 들어있는다. 
		즉 send ready 조건을 만족할 때만 들어가는 것이 아님.
		이 목록에 접근할 때는 main lock일 때만 해야 한다. */
		CFastMap2<CSuperSocket*, CSuperSocketPtr, int> m_validSendReadyCandidates;

		// 2개 이상의 스레드에서 issue send on need를 진행하는 것을 막기 위한 용도.
		// 한 NetCore의 여러 custom value가 여러 스레드에서 동시에 실행될 수 있기 때문에 필요하다.
		PROUDNET_VOLATILE_ALIGN int32_t m_issueSendOnNeedWorking;

		// ProudNet 전용 내부 RMI나 메시지, 수신 처리, 타이머 콜백 등을 처리하는 thread pool.
		// 주의: 절대 블러킹이 일어나서는 안된다. 모든 처리는 CPU time만 해야 한다.
		// 참고: 사용자 정의 루틴을 실행하는 스레드풀은 m_userThreadPool에서 별도로 다룬다.
		// 사용자가 의도/우발로 만드는 device time이 있는 루틴이 PN 내부 로직을 블러킹시켜 발생하는 문제
		// 가령 핑 타임아웃, 시간 측정 기능의 오동작을 예방하기 위해서다.
		CThreadPoolImpl* m_netThreadPool;

		// 사용자 정의 루틴이 실행되는 스레드풀.
		// 사용자가 블러킹 로직을 넣어도 된다.
		CThreadPoolImpl* m_userThreadPool;

		// true이면 사용자가 정의한 다른 thread pool object에 의존중.
		// false이면, 직접 this가 이 thread pool object를 가짐.
		// false인 경우, 종료 과정에서, 
		bool m_netThread_useExternal;
		bool m_userThread_useExternal;

		// 새 super socket의 최초의 issueRecv는 thread pool 안에서 해야 한다. 
		// 그것이 되게 하기 위한 목록
		// => 불필요. net worker thread에서 recv completion and accept completion을 모두 처리하므로.
		//CFastArray<CSuperSocketPtr> m_issueFirstRecvNeededSockets;

	public:
 //#ifdef _WIN32
 //		void IssueFirstRecv();
 //		void IssueFirstRecvNeededSocket_Set(CSuperSocketPtr s);
 //#endif

		void DeleteSendReadyList();

	public:
		/* [작업된 노트. 그러나 중요한 내용으로 쓰일 것을 대비해서, 지우지 않고 남겨놓음.]
		
		m_serverAsSendDest를 제거합시다. 그리고 NC,NS,LC,LS가 CHostBase를 상속받는데요, 이 상속관계도 제거해 버립시다.

		그리고, local 및 toServer라는 것을 모두 m_authedHosts에 추가함으로써, local 및 toServer가 final user work item을
		netcore가 어떻게 가질 것인지에 대한 문제를 해결할 수 있습니다.

		Loopback의 final user work item을 위해:

		NS,LS.Start()에서는 m_authedHosts에 루프백 즉 (HostID_Server, new CLoopbackHost)를 add합시다.

		LC,LC.Connect()에서는 m_candidateHosts에 new CLoopbackHost를 add합시다.
		그리고 서버로부터 HostID를 발급받으면 m_candidateHosts로부터 m_authedHosts로 옮기면 됩니다.
			
		to-server의 final user work item을 위해:
		NC,LC.Connect()에서는 m_candidateHosts에 new CRemoteServer_C or CLanRemoteServer_C를 add합시다.
		그리고 서버로부터 HostID를 발급받으면 m_candidateHosts로부터 m_authedHosts로 그걸 옮기면 됩니다.

		CLoopbackHost는 SuperSocket을 가질 필요가 없습니다.
		하지만 CRemoteServer_C or CLanRemoteServer_C는 SuperSocket을 가져야 합니다. 
		m_toServerTcp, m_toServerUdp_Fallbackable이 여기로 옮겨져야 하겠죠.

		상기와 같이 만들어 놓으면 toServer에 대한 예외코드 등이 모두 사라지면서 안전하게 netcore 모듈로 필요한 로직들을 
		옮길 수 있죠.
			
		Disconnect() or Stop()에서는 host에 대해서 garbage 처리 후 garbage-collect를 수행할텐데요,
		이때 위 to-server or loopback host들이 모두 dispose 즉 delete가 될겁니다.

		*/

		// 최근에 사용되었다가 버려진 UDP socket들.
		// C/S or P2P 최근 홀펀칭 재사용을 위해 다시 활용될 수 있음.
		CFastMap2<HostID, CSuperSocketPtr, int> m_recycles;

		void UdpSocketToRecycleBin(HostID hostID, CSuperSocketPtr udpSocket, int garbageDelayMs);
		void GarbageTooOldRecyclableUdpSockets();
		void AllClearRecycleToGarbage();

		// task queue
		// 실제로는 task subject(remote 혹은 local)의 queue이고 
		// 각 task는 CHostBase.m_finalUserWorkItemList에 있다.
		CUserTaskQueue m_userTaskQueue;
		// m_userTaskQueue를 사용하는 모든 루틴을 찾아, NetCore로 옮기도록 하자.

		// 현재 몇개 스레드에서 사용자 정의 OnTick이 실행중인지
		PROUDNET_VOLATILE_ALIGN int32_t m_timerCallbackParallelCount;
		// 사용자 정의 OnTick이 최대 몇개 스레드에서 호출 가능한지
		int32_t m_timerCallbackParallelMaxCount;
		// OnTick이 호출되는 주기
		uint32_t m_timerCallbackInterval;
		// 사용자가 호출하는 OnTick에 전달되는 사용자 정의 인자
		void*  m_timerCallbackContext;

		void SetTimerCallbackParameter(uint32_t interval, int32_t maxCount, void* context);
		
		void EnqueueHackSuspectEvent(CHostBase* rc, const char* statement, HackType hackType);
		void EnqueLocalEvent(LocalEvent& e, CHostBase* rc);

		// 사용자 정의 루틴 OnTick을 처리하기 위한 custom event를 thread pool에 던지는 역할
		CHeldPtr<CThreadPoolPeriodicPoster> m_periodicPoster_Tick;
		// 내부 루틴 heartbeat을 호출하는 custom event를 thread pool에 던지는 역할
		CHeldPtr<CThreadPoolPeriodicPoster> m_periodicPoster_Heartbeat;
		// coalesce를 위함
		CHeldPtr<CThreadPoolPeriodicPoster> m_periodicPoster_SendEnqueued;

		CTimeAlarm m_disposeGarbagedHosts_Timer;
		CTimeAlarm m_DisconnectRemoteClientOnTimeout_Timer;
		CTimeAlarm m_acceptedPeerdispose_Timer;
		CTimeAlarm m_lanRemotePeerHeartBeat_Timer;
	private:
		// g_barbages 등을 보호한다.
		// garbage.add를 호출하는 케이스가 per-socket lock을 한 상태에서도 하기 때문에 이것이 별도로 존재해야 한다.
		CriticalSection m_garbageSocketQueueCS;
		Deque<CSuperSocketPtr> m_garbageSocketQueue;

		// remote가 한때 사용했으나 더 이상 안쓰이는 소켓들. 
		// i/o completion 및 이슈 더 이상 없음 보장시 파괴될 것들이다.
		// Proud.CNetClientImpl.m_recycles와는 다름에 주의!
		CFastMap2<intptr_t, CSuperSocketPtr, int> m_garbagedSockets;
	protected:
		/* *주의* OnHostGarbaged, OnHostGarbageCollected에서 하는 것의 주의사항이 있다.
		
		connect lost 상황 즉 ProcessDisconnect()에서,
		socket의 owner host(socket to host map에서 찾음)가 있는지 확인하여, 
		없으면 바로  GarbageSocket을 하고, 있으면 owner host에 대해서만 
		GarbageHost를 호출하되 socket에 대해서는 GarbageSocket을 호출 하지 말자. 
		그러면 추후 호출될 OnHostGarbageCollected에서 종속 socket에 대한 
		GarbageSocket 및 참조 해제를 해버리면 된다.

		자세한 내용 => Host와 Socket의 Garbage 처리.pptx */ 

		// garbage에 있던 socket이 delete를 비로소 할 떄 호출된다.
		// NC의 경우 unused socket port list를 갱신하던가 해야 한다.
		// 이 함수가 호출될 당시 이미 main lock이 되어있다.
		virtual void OnSocketGarbageCollected(CSuperSocket* socket) = 0;

		// remote가 가진 task가 완전히 소진되어, remote를 안전 파괴해도 될 때 호출된다.
		// delete remote 및 각종 목록에서의 제거는 이 함수 바깥에서 한다. 따라서 이 함수 안에서 그걸 하지 말 것.
		// 여기서는 상속된 클래스에 특화된 일만 하도록 만들자. 
		// 이 함수가 호출될 당시 이미 main lock이 되어있다.
		virtual bool OnHostGarbageCollected(CHostBase* remote) = 0;

		// remote에 대해 issue dispose를 호출할 때 호출되는 핸들러.
		// 한 remote에 대해 최초 1회만 호출된다.
		// NS,NC,LC,LS등이 커스텀 로직을 수행함. 가령 갖고 있는 socket들을 GarbageSocket()하거나
		// P2P 그룹 해제나 local event enque 등을 수행.
		virtual void OnHostGarbaged(CHostBase* remote, ErrorType errorType, ErrorType detailType, const ByteArray& comment, SocketErrorCode socketErrorCode) = 0;

		virtual void BadAllocException(Exception& ex) = 0;

	public:
		void GarbageSocket(CSuperSocketPtr socket);
		void DoGarbageCollect();
		const int GetGarbagedSocketsAndHostsCount_NOLOCK();
		
		void UserTaskQueue_Add(CHostBase * rc, CReceivedMessage &ri, FinalUserWorkItemType type, bool stripMessageTypeHeader = false);
		void PostEvent();

	private:
		void DoGarbageCollect_Socket();
		void DoGarbageCollect_Host();

		void OnTick();
		//void EndCompletion();
	public:
		virtual bool Stub_ProcessReceiveMessage(IRmiStub* stub, CReceivedMessage& receivedMessage, void* hostTag, CWorkResult* workResult) = 0;
		void UserWork_FinalReceiveRmi(CFinalUserWorkItem& UWI, CHostBase* subject, CWorkResult* workResult);
		void UserWork_FinalReceiveUserMessage(CFinalUserWorkItem& UWI, CHostBase* subject, CWorkResult* workResult);
		void UserWork_FinalReceiveHla(CFinalUserWorkItem& UWI, CHostBase* subject, CWorkResult* workResult);
		void UserWork_LocalEvent(CFinalUserWorkItem &UWI, CHostBase* subject, CWorkResult* workResult);
		void SetTimerCallbackIntervalMs(int newVal);

	public:
		//////////////////////////////////////////////////////////////////////////
		// SuperSocket 콜백 처리

		/* 메시지를 non block send를 마치거나 overlapped send가 마쳤을 때의 이벤트 핸들러.
		doneBytes: 송신 처리된 메시지들 혹은 policy text의 바이트수.
		socket: 송신 처리된 당사자의 socket. CNetCoreImpl.SocketToHostMap_Get()로 이 socket을 처리하는 remote를 찾을 수 있다.

		이 함수가 실행되는 동안 caller가 per-socket lock을 안 하는 이유:
		main lock을 할 경우 데드락이 되어버리니까.
		그리고 이 함수 안에서는 per-socket lock을 웬만하면 하지 말자.
		다른 곳에서 per-socket lock 후 syscall을 하기 때문에, per-socket lock은 큰 병목이 될 수 있다.




		이 함수를 구현할 때 주의 사항:
		iocp의 경우 이 함수 호출 후 next issue send를 하므로 이 함수 안에서 또 하지 말자.

		OnMessageSent은 CustomValueEvent_SendEnqueued or send avail or completion event에 의해
		콜백된다. 즉 서로 다른 스레드에서 동시에 호출될 수 있다.
		그러하다보니 send done에서 보낸 데이터가 뭔지를 로그를 남길 경우 순서가 뒤바뀔 수 있다.
		이 함수가 보낸 데이터 갯수만 주는 이유가 그 이유.
		이 함수에서는 통계누적 등 순서 뒤바뀌어도 상관없는 로직만을 구현하도록 하자. */
		virtual void OnMessageSent(int doneBytes, SocketType socketType/*, CSuperSocket* socket*/) = 0;

		/* 메시지를 non block recv를 마치거나 overlapped recv가 마쳤을 때의 이벤트 핸들러.
		doneBytes: 수신 처리된 메시지들 혹은 policy text의 바이트수.
		아래 received messages의 총 바이트수보다 높을 수 있다. policy text는 message list에 불포함이므로.
		socket: 위 함수와 동일

		이 함수를 구현할 때 주의 사항:
		iocp의 경우 이 함수 호출 후 next issue recv를 하므로 이 함수 안에서 또 하지 말자.

		잠금 정책 관련 위 함수 주석 숙지할 것.

		안전하게도, 이 함수가 실행되는 동안 다른 스레드에서 OnMessageReceived이 또 호출되지는 않는다.
		왜냐하면 OnMessageReceived이 또 호출되려면 다른 스레드에서 issue recv를 하거나,
		다른 스레드의 epoll에서 이 socket에 대한 signal을 토해낼 수 있어야 하는데 둘다 불만족이니까.
		따라서 이 함수가 per-socket lock을 하지 않은 채 호출되어도 안전하다.

		**주의**
		CReceivedMessage.m_remoteHostID는 안 채워진 채로 콜백된다. 
		이것을 채우는 것은 이 함수 안에서 main lock 후 하도록 하자. */
		virtual void OnMessageReceived(int doneBytes, SocketType socketType, CReceivedMessageList& messages, CSuperSocket* socket) = 0;

		/* Accept이 완료된 경우의 이벤트 핸들러. */
		virtual void OnAccepted(CSuperSocket* socket, AddrPort tcpLocalAddr, AddrPort tcpRemoteAddr) = 0;

		/* Connect가 완료된 경우의 이벤트 핸들러. 
		socket: TCP connect를 시도했던 socket. 
		연결유지기능에서는 기 TCP 연결 socket이 아닌, 다른 socket이 인자로 채워질 수도 있다. */
		virtual void OnConnectSuccess(CSuperSocket* socket) = 0;

		// 위 함수와 동일한 역할을 하되, 실패했을때, 왜 실패했는지가 같이 인자로 전달된다.
		virtual void OnConnectFail(CSuperSocket* socket, SocketErrorCode code) = 0;

		// TCP socket의 disconnect 처리 작업.
		// listen socket의 경우, accept 중 에러가 발생해서 listen socket이 닫힌 경우도 여기에 포함된다.
		// 혹은, ACR을 가동시키거나 디스 모드로 돌입하거나 한다.
		virtual void ProcessDisconnecting(CSuperSocket* socket, const ErrorInfo& errorInfo) = 0;

		//////////////////////////////////////////////////////////////////////////

		// defrag 과정에서 이상한 상황이 생겼을 때 콜백된다.
		// 다행히 NC,NS가 이미 있어서 이 함수를 pure virtual로 뺌.
		// LC,LS는 UDP를 안 다루므로 이것을 빈 함수로 두자.
		virtual void EnquePacketDefragWarning(CSuperSocket* superSocket, AddrPort sender, const PNTCHAR* text) = 0;

		void ProcessCustomValueEvent(CWorkResult* workResult, CustomValueEvent customValue);

		virtual void ValidSendReayList_Add(CSuperSocketPtr superSocket);		

		virtual void SendReadyList_Add(CSuperSocket* socket, bool issueSendNow);

		virtual void EveryRemote_IssueSendOnNeed();

		virtual void LockMain_AssertIsLockedByCurrentThread() = 0;
		virtual void LockMain_AssertIsNotLockedByCurrentThread() = 0;

		//////////////////////////////////////////////////////////////////////////
		// frag board delegate로부터 옮겨옴
		virtual bool IsSimplePacketMode() = 0;	// 이 모듈이 simple packet mode로 작동중인가?	

		/* 잠금 없이 HostID를 얻어온다.
		for filter tag.
		packet frag board에서 per-socket send queue lock을 한 상태에서
		main lock을 하면 deadlock이 생기니까. */
		virtual HostID GetVolatileLocalHostID() const = 0;

		// 위의 경우를 제외하고는 원칙적으로 이것을 쓰도록 한다.
		virtual HostID GetLocalHostID() = 0;

		// super socket이 lock 검사할 때 쓰임
		virtual CriticalSection& GetCriticalSection() = 0;

		//////////////////////////////////////////////////////////////////////////
		// defrag board delegate로부터 옮겨옴
		int GetOverSendSuspectingThresholdInBytes(CSuperSocket* socket);

		//////////////////////////////////////////////////////////////////////////
		// remote 관련

		void GarbageHost(
			CHostBase* rc,
			ErrorType errorType,
			ErrorType detailType,
			const ByteArray& comment,
			const PNTCHAR* where,
			SocketErrorCode socketErrorCode);

		inline void AssertIsLockedByCurrentThread()
		{
			Proud::AssertIsLockedByCurrentThread(GetCriticalSection());
		}
		inline void AssertIsNotLockedByCurrentThread()
		{
			Proud::AssertIsNotLockedByCurrentThread(GetCriticalSection());
		}

		virtual void ProcessMessageOrMoveToFinalRecvQueue(SocketType socketType, CSuperSocket* socket, CReceivedMessageList &extractedMessages) = 0;
	};

	extern const PNTCHAR* DuplicatedRmiIDErrorText;
	extern const PNTCHAR* BadRmiIDErrorText;
	extern const PNTCHAR* AsyncCallbackMayOccurErrorText;

}
