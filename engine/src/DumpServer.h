#pragma once

#include "../include/ProudNetServer.h"
#include "../include/dumpcommon.h"
#include "../include/DumpServer.h"
#include "DumpC2S_stub.h"
#include "DumpS2C_proxy.h"

namespace Proud
{
	class CDumpServerImpl;

	class CDumpCli_S
	{
		CDumpServerImpl* m_server;
		HostID m_HostID;
	public:
		HANDLE m_fileHandle;

		CDumpServerImpl* GetServer() {
			return m_server;
		}

		CDumpCli_S(CDumpServerImpl* server, HostID hostID);
		~CDumpCli_S(void);
	};

	typedef RefCount<CDumpCli_S> CDumpCliPtr_S;
	typedef std::map<HostID, CDumpCliPtr_S> CDumpClients;

	class CDumpServerImpl:
		public CDumpServer,
		public DumpC2S::Stub,
		public INetServerEvent
	{
		void FrameMove();

		DumpS2C::Proxy m_s2cProxy;
		//float m_elapsedTime;
		IDumpServerDelegate* m_dg;

		CHeldPtr<CNetServer> m_server;
		CDumpClients m_clients;

		//float GetElapsedTime();

		void OnClientJoin(CNetClientInfo *clientInfo);
		void OnClientLeave(CNetClientInfo *clientInfo, ErrorInfo *errorInfo, const ByteArray& comment);

		virtual void OnError(ErrorInfo* /*errorInfo*/) {}
		virtual void OnWarning(ErrorInfo* /*errorInfo*/) {}
		virtual void OnInformation(ErrorInfo* /*errorInfo*/) {}
		virtual void OnException(const Exception& /*e*/) {}
		virtual void OnUnhandledException() {}
		virtual void OnNoRmiProcessed(Proud::RmiID /*rmiID*/) {}
		virtual void OnUserWorkerThreadBegin(){}
		virtual void OnUserWorkerThreadEnd(){}

		virtual bool OnConnectionRequest(AddrPort /*clientAddr*/, ByteArray& /*userDataFromClient*/, ByteArray& /*reply*/) {
			return true;
		}
		virtual void OnP2PGroupJoinMemberAckComplete(HostID /*groupHostID*/, HostID /*memberHostID*/,ErrorType /*result*/) {}

		CDumpCliPtr_S GetClientByHostID(HostID client);
		CDumpCliPtr_S GetClientByLogonTimeUUID(UUID logonTimeUUID);

		int GetUserNum();

		DECRMI_DumpC2S_Dump_Start;
		DECRMI_DumpC2S_Dump_Chunk;
		DECRMI_DumpC2S_Dump_End;

	public:
		CDumpServerImpl(IDumpServerDelegate* dg);
		~CDumpServerImpl(void);

		void RunMainLoop();

	};
}
