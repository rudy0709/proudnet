
%csmethodmodifiers Proud::ErrorInfo::GetNativeReceivedMessage "private"
%csmethodmodifiers Proud::ErrorInfo::GetNativeReceivedMessageLength "private"
%csmethodmodifiers Proud::ErrorInfo::CopyFromNative "internal"


#ifdef __NETSERVER__
%typemap(cscode) Proud::ErrorInfo
%{
	public ByteArray GetReceivedMessage()
	{
		System.IntPtr nativeData = GetNativeReceivedMessage();
		int length = GetNativeReceivedMessageLength();

		if ((length == 0) || (nativeData == System.IntPtr.Zero))
		{
			return null;
		}

		return ConvertToManaged.NativeDataToManagedByteArray(nativeData, length);
	}
%}
#else
%typemap(cscode) Proud::ErrorInfo
%{
	public ByteArray GetReceivedMessage()
	{
		System.IntPtr nativeData = GetNativeReceivedMessage();
		return ConvertToCSharp.NativeByteArrayToCSharp(nativeData);
	}
%}
#endif // __NETSERVER__

%typemap(cscode) Proud::ErrorInfo %{
	public override string ToString()
	{
		return GetString();
	}
%}

%extend Proud::ErrorInfo
{
	inline void CopyFromNative(void* obj) throw (Proud::Exception)
	{
		assert(self);
		assert(obj);

		Proud::ErrorInfo* errorInfo = static_cast<Proud::ErrorInfo*>(obj);
		*self = *errorInfo;
	}

	inline void* GetNativeReceivedMessage() throw (Proud::Exception)
	{
		assert(self);
		return &(self->m_lastReceivedMessage);
	}

	inline int GetNativeReceivedMessageLength() throw (Proud::Exception)
	{
		assert(self);
		return self->m_lastReceivedMessage.GetCount();
	}
}



%ignore Proud::ErrorInfo::m_socketError;

%ignore Proud::ErrorInfo::m_remoteAddr;

%ignore Proud::ErrorInfo::FromSocketError;

%ignore Proud::ErrorInfo::From;

%ignore Proud::ErrorInfo::m_lastReceivedMessage;
