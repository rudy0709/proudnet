
// Swig 인터페이스 컴파일러 대상이 되는 헤더를 포함합니다.
//Includes
%include "../../core_cpp/include/FakeClrBase.h"
%include "../../core_cpp/include/ConnectParam.h"
%include "../../core_cpp/include/Enums.h"
%include "../../core_cpp/include/ErrorType.h"
//%include "../../core_cpp/include/INetCoreEvent.h"
//%include "../../core_cpp/include/INetClientEvent.h"
%include "../../core_cpp/include/ErrorInfo.h"
%include "../../core_cpp/include/NetClientInterface.h"
%include "../../core_cpp/include/acr.h"
%include "../../core_cpp/include/Exception.h"
%include "../../core_cpp/include/IRmiHost.h"
//%include "../../core_cpp/include/IRMIStub.h"
%include "../../core_cpp/include/AddrPort.h"
%include "../../core_cpp/include/HostIDArray.h"
%include "../../core_cpp/include/NetPeerInfo.h"
%include "../../core_cpp/include/RMIContext.h"
%include "../../core_cpp/include/EncryptEnum.h"
%include "../../core_cpp/include/CompressEnum.h"
%include "../../core_cpp/include/pnguid.h"
%include "../../core_cpp/include/ByteArray.h"
%include "../../core_cpp/include/NetCoreStats.h"
%include "../../core_cpp/include/NetClientStats.h"
%include "../../core_cpp/include/NetConfig.h"
%include "../../core_cpp/include/ThreadPool.h"
%include "../../core_cpp/include/ReceivedMessage.h"
%include "../../core_cpp/include/RoundTripLatencyTest.h"

%{
using namespace Proud;
%}
