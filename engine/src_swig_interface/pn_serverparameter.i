
%csmethodmodifiers Proud::CStartServerParameter::SetExternalUserWorkerThreadPool "private"
%csmethodmodifiers Proud::CStartServerParameter::SetExternalNetWorkerThreadPool "private"


%typemap(cscode) Proud::CStartServerParameter
%{
	public void SetExternalUserWorkerThreadPool(ThreadPool threadPool)
	{
		SetExternalUserWorkerThreadPool(threadPool.GetNativeThreadPool());
    } 

	public void SetExternalNetWorkerThreadPool(ThreadPool threadPool)
	{
		SetExternalNetWorkerThreadPool(threadPool.GetNativeThreadPool());
    } 
%}


%extend Proud::CStartServerParameter
{
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


// CStartServerParameterBase
%ignore Proud::CStartServerParameterBase::m_externalUserWorkerThreadPool;
%ignore Proud::CStartServerParameterBase::m_externalNetWorkerThreadPool;
%ignore Proud::CStartServerParameterBase::m_bottleneckWarningSettings;
%ignore Proud::CStartServerParameterBase::m_timerCallbackContext;


// CStartServerParameter
%ignore Proud::CStartServerParameter::m_bottleneckWarningSettings;