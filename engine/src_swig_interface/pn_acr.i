
%extend Proud::CRemoteOfflineEventArgs
{
    // C++ ��ü�� C# ��ü�� �����մϴ�.
	inline void CopyFromNative(void* obj)
	{
		assert(self);
		assert(obj);

		Proud::CRemoteOfflineEventArgs* args = static_cast<Proud::CRemoteOfflineEventArgs*>(obj);
		*self = *args;
	}
}

%extend Proud::CRemoteOnlineEventArgs
{
	// C++ ��ü�� C# ��ü�� �����մϴ�.
	inline void CopyFromNative(void* obj)
	{
		assert(self);
		assert(obj);

		Proud::CRemoteOnlineEventArgs* args = static_cast<Proud::CRemoteOnlineEventArgs*>(obj);
		*self = *args;
	}
}
