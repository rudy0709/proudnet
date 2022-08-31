
%ignore Proud::Exception::m_pVoidSource;
%ignore Proud::Exception::m_pStdSource;
%ignore Proud::Exception::m_comSource;
%ignore Proud::Exception::m_delegateObject;
%ignore Proud::Exception::m_errorInfoSource;
%ignore Proud::Exception::chMsg;

%ignore Proud::Exception::Exception(const wchar_t* text);
%ignore Proud::Exception::Exception(const char* pFormat, ...);
%ignore Proud::Exception::Exception(const wchar_t* pFormat, ...);
%ignore Proud::Exception::Exception(std::exception& src);
%ignore Proud::Exception::Exception(void* src);

%ignore Proud::m_delegThrowInvalidArgumentExceptionateObject;
%ignore Proud::ThrowArrayOutOfBoundException;
%ignore Proud::XXXHeapChkImpl;
