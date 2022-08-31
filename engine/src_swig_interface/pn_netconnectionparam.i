

%csmethodmodifiers Proud::CNetConnectionParam::GetNativeUserData "private"

%csmethodmodifiers Proud::CNetConnectionParam::SetExternalUserWorkerThreadPool "private"
%csmethodmodifiers Proud::CNetConnectionParam::SetExternalNetWorkerThreadPool "private"

%typemap(cscode) Proud::CNetConnectionParam
%{
	public void SetUserData(ByteArray data)
	{
		if ((data == null) || (data.Count == 0))
		{
			return;
		}

		System.IntPtr nativeData = GetNativeUserData();

		ConvertToNative.CopyByteArrayToNative(data, nativeData);
	}

	public void SetExternalUserWorkerThreadPool(ThreadPool threadPool)
	{
		SetExternalUserWorkerThreadPool(threadPool.GetNativeThreadPool());
	}

	public void SetExternalNetWorkerThreadPool(ThreadPool threadPool)
	{
		SetExternalNetWorkerThreadPool(threadPool.GetNativeThreadPool());
	}
%}

%extend Proud::CNetConnectionParam
{
	inline void* GetNativeUserData()
	{
		assert(self);
		return &(self->m_userData);
	}

	inline void SetExternalUserWorkerThreadPool(Proud::CThreadPool* threadPool) throw (Proud::Exception)
	{
		assert(self);
		self->m_externalUserWorkerThreadPool = threadPool;
	}

	inline void SetExternalNetWorkerThreadPool(Proud::CThreadPool* threadPool) throw (Proud::Exception)
	{
		assert(self);
		self->m_externalNetWorkerThreadPool = threadPool;
	}
}


//%ignore Proud::CNetConnectionParam::m_localUdpPortPool;
%ignore Proud::CNetConnectionParam::m_userData;

//%ignore Proud::CNetConnectionParam::m_userWorkerThreadModel;
//%ignore Proud::CNetConnectionParam::m_netWorkerThreadModel;

%ignore Proud::CNetConnectionParam::m_externalUserWorkerThreadPool;
%ignore Proud::CNetConnectionParam::m_externalNetWorkerThreadPool;

%ignore Proud::CNetConnectionParam::m_timerCallbackContext;
%ignore Proud::CNetConnectionParam::m_allowExceptionEvent;
