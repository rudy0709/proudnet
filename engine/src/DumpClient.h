#pragma once

#if defined(_WIN32)
#include "../include/ProudNetClient.h"

#include "../include/dumpcommon.h"
#include "../include/DumpClient.h"
#include "dumps2c_stub.h"
#include "dumpc2s_proxy.h"

namespace Proud
{
	class CDumpClientImpl:
		public CDumpClient,
		public DumpS2C::Stub,
		public INetClientEvent
	{
		volatile State m_state;
		volatile bool m_stopNow;
		IDumpClientDelegate* m_dg;
		int m_sendProgress;
		int m_sendTotal;

		DumpC2S::Proxy m_c2sProxy;

		HANDLE m_fileHandle;

		bool SendNextChunk();

		virtual void OnJoinServerComplete(ErrorInfo *info, const ByteArray &replyFromServer);
		virtual void OnLeaveServer(ErrorInfo *errorInfo);
		virtual void OnP2PMemberJoin(HostID memberHostID, HostID groupHostID, int memberCount, const ByteArray &message);
		virtual void OnP2PMemberLeave(HostID memberHostID, HostID groupHostID, int memberCount);
		virtual void OnError(ErrorInfo *errorInfo);
		virtual void OnWarning(ErrorInfo *errorInfo) { errorInfo; }
		virtual void OnInformation(ErrorInfo *errorInfo) { errorInfo; }
		virtual void OnException(const Exception &e) { e; }
		virtual void OnUnhandledException() {}
		virtual void OnNoRmiProcessed(Proud::RmiID rmiID) { rmiID; }

		virtual void OnChangeP2PRelayState(HostID remoteHostID, ErrorType reason) { remoteHostID; reason; }
		virtual void OnSynchronizeServerTime() {}

		DECRMI_DumpS2C_Dump_ChunkAck;

		CHeldPtr<CNetClient> m_client;
	public:

		CDumpClientImpl(IDumpClientDelegate* dg);
		~CDumpClientImpl(void);

		void Start(String serverAddr, uint16_t serverPort, String filePath);
		void FrameMove();
		State GetState();
		int GetSendProgress();
		int GetSendTotal();
	};
}

#endif
