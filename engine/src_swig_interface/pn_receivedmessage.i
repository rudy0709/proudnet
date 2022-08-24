
%extend Proud::CReceivedMessage
{
	inline void* GetMsgBuffer()
	{
		assert(self);
		return (void*)self->m_unsafeMessage.m_msgBuffer.GetData();
	}

	inline int GetMsgBufferLength()
	{
		assert(self);
		return self->m_unsafeMessage.m_msgBuffer.GetCount();
	}

	inline int GetMsgReadOffset()
	{
		assert(self);
		return self->m_unsafeMessage.GetReadOffset();
	}
}

%ignore Proud::CReceivedMessage::m_unsafeMessage;

%ignore Proud::CReceivedMessage::GetWriteOnlyMessage;

%ignore Proud::CReceivedMessage::GetReadOnlyMessage;

%ignore Proud::CReceivedMessage::GetRemoteAddr;

%ignore Proud::CReceivedMessage::GetRemoteAddr;

%ignore Proud::CReceivedMessage::GetRemoteHostID;

%ignore Proud::CReceivedMessage::IsRelayed;

%ignore Proud::CReceivedMessage::GetEncryptMode;

%ignore Proud::CReceivedMessage::GetCompressMode;

%ignore Proud::CReceivedMessageList;