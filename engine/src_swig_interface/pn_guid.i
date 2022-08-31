
%csmethodmodifiers Proud::Guid::CopyManagedByteArrayToNativeGuid "internal"

%ignore Proud::IClonable;
%ignore Proud::PNGUID;

%typemap(csimports) Proud::Guid
%{
%}

%typemap(cscode) Proud::Guid
%{
	public Guid(string str) : this()
	{
		global::System.Guid managedGuid = new global::System.Guid(str);
		Nettention.Proud.Guid.MangedGuidToNativeGuid(this, managedGuid);
	}

	public Guid(global::System.Guid managedGuid) : this()
	{
		Nettention.Proud.Guid.MangedGuidToNativeGuid(this, managedGuid);
	}

	public void Set(global::System.Guid managedGuid)
	{
		Nettention.Proud.Guid.MangedGuidToNativeGuid(this, managedGuid);
	}

	internal static unsafe void MangedGuidToNativeGuid(Nettention.Proud.Guid proudGuid, global::System.Guid managedGuid)
	{
		byte[] inByteArray = managedGuid.ToByteArray();

		fixed (byte* ret = inByteArray)
		{
			proudGuid.CopyManagedByteArrayToNativeGuid(new global::System.IntPtr((void*)ret), inByteArray.Length);
		}
	}

%}

%extend Proud::Guid
{
	inline void CopyManagedByteArrayToNativeGuid(void* inByteArray, int length)
	{
		assert(self);

		if (length <= 0)
		{
			return;
		}

		assert(sizeof(Proud::PNGUID) == length);

		memcpy((void*)self, inByteArray, length);
	}

	inline Proud::String GetString()
	{
		assert(self);
		return self->ToString();
	}
}


%ignore Proud::Guid::Guid(PNGUID src);

%ignore Proud::Guid::RandomGuid();
%ignore Proud::Guid::From(GUID src);
%ignore Proud::Guid::NewGuid();
%ignore Proud::Guid::Create();
%ignore Proud::Guid::ToString() const;
%ignore Proud::Guid::ToBracketString() const;
%ignore Proud::Guid::ConvertUUIDToString(const UUID &uuid, String &uuidStr);
%ignore Proud::Guid::ConvertUUIDToBracketString(const UUID &uuid, String &uuidStr);
%ignore Proud::Guid::ConvertStringToUUID(String uuidStr, UUID &uuid);
%ignore Proud::Guid::GetString(const UUID &uuid);
%ignore Proud::Guid::GetBracketString(const UUID &uuid);
%ignore Proud::Guid::GetFromString(const PNTCHAR* uuidStr);
%ignore Proud::Guid::From(const PNGUID& uuid);

%ignore Proud::Guid::ConvertUUIDToString;
%ignore Proud::Guid::ConvertUUIDToBracketString;

%ignore Proud::Retained::CompareEntityID;
