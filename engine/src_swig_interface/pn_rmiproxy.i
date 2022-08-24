
%feature("director") Proud::IRmiProxy;

%extend Proud::IRmiProxy
{
	inline bool RmiSend(void* remotes, int remoteCount, Proud::RmiContext* rmiContext, void* msgBuffer, int bufferLength, Proud::String RMIName, Proud::RmiID RMIId)
	{
		assert(self);
		assert(remotes);
		assert(rmiContext);
		assert(msgBuffer);

		if ((bufferLength <= 0) || (remoteCount <= 0))
		{
			return false;
		}

		Proud::CMessage msg;
		msg.UseExternalBuffer(static_cast<uint8_t*>(msgBuffer), bufferLength);
		msg.SetLength(bufferLength);
		
		return self->RmiSend(static_cast<Proud::HostID*>(remotes), remoteCount, *rmiContext, msg, RMIName, RMIId);
	}
}

%ignore Proud::IRmiProxy::m_core;

%ignore Proud::IRmiProxy::NotifySendByProxy;

%ignore Proud::IRmiProxy::RmiSend;

%ignore Proud::IRmiProxy::ProxyBadSignatureErrorText;