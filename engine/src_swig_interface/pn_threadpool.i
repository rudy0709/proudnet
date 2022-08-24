
//%feature("director") Proud::IThreadPoolEvent;

%csmethodmodifiers Proud::CThreadPool::Create "internal"
%csmethodmodifiers Proud::CThreadPool::SetDesiredThreadCount "internal"
%csmethodmodifiers Proud::CThreadPool::Process "internal"

%extend Proud::CThreadPool
{
	// C#에서 호출가능한 CThreadPool::Create 함수를 생성합니다.
	static Proud::CThreadPool* Create(int threadCount) throw (Proud::Exception)
	{
		return Proud::CThreadPool::Create(NULL, threadCount);
	}
}

%ignore Proud::CThreadPool::Create;
%ignore Proud::CThreadPool::RunAsync;


%ignore Proud::CNetServer::IThreadPoolEvent;
