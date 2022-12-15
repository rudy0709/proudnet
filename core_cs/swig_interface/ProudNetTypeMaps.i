
//Allow array of strings
CSHARP_ARRAYS(char *,string)

//====================================================================================================================

//INOUT char to byte array
%apply unsigned char INOUT[] {unsigned char* inputByteArray};
%apply unsigned char INOUT[] {unsigned char* inByteArray};
%apply unsigned char INOUT[] {unsigned char* inByteArray2};
%apply unsigned char INOUT[] {unsigned char* inOutByteArray};
%apply unsigned char INOUT[] {unsigned char* _data};
%apply unsigned char INOUT[] {unsigned char* inOutData};

%typemap(ctype)   unsigned char *inByteArrayArray "unsigned char*"
%typemap(cstype)  unsigned char *inByteArrayArray  "byte[,]"
%typemap(imtype, inattributes="[In, MarshalAs(UnmanagedType.LPArray)]") unsigned char *inByteArrayArray  "byte[,]"
%typemap(csin)    unsigned char *inByteArrayArray  "$csinput"

%typemap(in)     unsigned char *inByteArrayArray  "$1 = $input;"
%typemap(freearg) unsigned char *inByteArrayArray  ""
%typemap(argout)  unsigned char *inByteArrayArray  ""

//====================================================================================================================

// void* -> global::System.IntPtr
%typemap(ctype)  void * "void *"
%typemap(imtype) void * "global::System.IntPtr"
%typemap(cstype) void * "global::System.IntPtr"
%typemap(csin)   void * "$csinput"
%typemap(in)     void * %{ $1 = $input; %}
%typemap(out)    void * %{ $result = $1; %}
%typemap(csout)  void * { return $imcall; }
%typemap(csdirectorin) void* "$iminput"

//====================================================================================================================

// uint8_t* -> global::System.IntPtr
%typemap(ctype)  uint8_t * "void *"
%typemap(imtype) uint8_t * "global::System.IntPtr"
%typemap(cstype) uint8_t * "global::System.IntPtr"
%typemap(csin)   uint8_t * "$csinput"
%typemap(in)     uint8_t * %{ $1 = $input; %}
%typemap(out)    uint8_t * %{ $result = $1; %}
%typemap(csout)  uint8_t * { return $imcall; }
%typemap(csdirectorin) uint8_t * "$iminput"

%typemap(in, canthrow=1) uint8_t*
%{
   $1 = (uint8_t*)($input);
%}

//====================================================================================================================

// Proud::CFastArray<Proud::AddrPort>* -> AddrPortArray
%typemap(ctype)  Proud::CFastArray<Proud::AddrPort> * "void *"
//%typemap(imtype) Proud::CFastArray<Proud::AddrPort> "AddrPortArray"
%typemap(cstype) Proud::CFastArray<Proud::AddrPort> "AddrPortArray"
%typemap(csin)   Proud::CFastArray<Proud::AddrPort> "$csinput"
%typemap(csout)  Proud::CFastArray<Proud::AddrPort> {
 AddrPortArray ret = new AddrPortArray($imcall, true);
 return ret;
}
//%typemap(in)     Proud::CFastArray<Proud::AddrPort> %{ $1 = $input; %}
//%typemap(out)    Proud::CFastArray<Proud::AddrPort>  %{ $result = $1; %}
//%typemap(csdirectorin) Proud::CFastArray<Proud::AddrPort> * "$iminput"

//%typemap(in, canthrow=1) Proud::CFastArray<Proud::AddrPort>*
//%{
//   $1 = (Proud::CFastArray<Proud::AddrPort>)($input);
//%}

//====================================================================================================================

// etc ignore
%ignore Proud::_RmiID;
%ignore Proud::SocketErrorCode;
%ignore Proud::ShutdownFlag;
%ignore Proud::CClientWorkerInfo::m_workerThreadID;
%ignore Proud::CNetPeerInfo::m_hostTag;
%ignore Proud::AppendTextOut;


%ignore Proud::CNetCoreStats::ToString;

%ignore Proud::CP2PGroups;

// ikpil.choi 2017-01-10 : MessageSummary.cs 에 직접 구현해 사용하므로, Native는 없어도 됨
%ignore Proud::MessageSummary;
%ignore Proud::AfterRmiSummary;
%ignore Proud::BeforeRmiSummary;

%ignore CPNElementTraits<Proud::ByteArray>;
%ignore CPNElementTraits<Proud::AddrPort>;
%ignore CPNElementTraits<Proud::Guid>;
%ignore CPNElementTraits<Proud::StringW>;
%ignore CPNElementTraits<Proud::StringA>;
%ignore CPNElementTraits<Proud::RefCount<T>>;
