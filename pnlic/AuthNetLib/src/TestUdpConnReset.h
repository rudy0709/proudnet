#pragma once

#include "FastSocket.h"

namespace Proud 
{
	class CNetServerImpl;

	class CTestUdpConnReset/*:public IFastSocketDelegate*/
	{
		FastSocketPtr m_socket;
		uint8_t m_payload[100];
	public:
		CNetServerImpl* m_owner;

		CTestUdpConnReset(void);
		~CTestUdpConnReset(void);

		void DoTest();

		virtual void OnSocketWarning(CFastSocket* socket, String msg) {}
#ifdef _WIN32
		virtual void OnSocketIoCompletion( CIoEventStatus& comp)
		{
			// none
		}
#endif
	};

}