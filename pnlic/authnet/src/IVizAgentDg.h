#pragma once 

namespace Proud 
{
	class CNetClientImpl;
	class CNetServerImpl;
	class IVizAgentDg
	{
	public:
		virtual ~IVizAgentDg() {}
		
		virtual CNetClientImpl* QueryNetClient() = 0;
		virtual CNetServerImpl* QueryNetServer() = 0;
	};
}