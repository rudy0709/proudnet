
%csmethodmodifiers Proud::CDisconnectArgs::GetNativeComment "private"

%typemap(cscode) Proud::CDisconnectArgs
%{
	public void SetComment(ByteArray data)
	{
		if (data == null)
		{
			return;
		}

		System.IntPtr nativeData = GetNativeComment();

		ConvertToNative.CopyByteArrayToNative(data, nativeData);	
	}
%}

%extend Proud::CDisconnectArgs
{
	inline void* GetNativeComment()
	{
		assert(self);
		return &(self->m_comment);
	}
}

%ignore Proud::CDisconnectArgs::m_comment;