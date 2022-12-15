
// Swig 인터페이스 컴파일러 대상이 되는 헤더를 포함합니다.
//Includes
%include "../../include/FakeClrBase.h"
%include "../../include/ConnectParam.h"
%include "../../include/Enums.h"
%include "../../include/ErrorType.h"
//%include "../../include/INetCoreEvent.h"
//%include "../../include/INetClientEvent.h"
%include "../../include/ErrorInfo.h"
%include "../../include/NetClientInterface.h"
%include "../../include/acr.h"
%include "../../include/Exception.h"
%include "../../include/IRmiHost.h"
//%include "../../include/IRMIStub.h"
%include "../../include/AddrPort.h"
%include "../../include/HostIDArray.h"
%include "../../include/NetPeerInfo.h"
%include "../../include/RMIContext.h"
%include "../../include/EncryptEnum.h"
%include "../../include/CompressEnum.h"
%include "../../include/pnguid.h"
%include "../../include/ByteArray.h"
%include "../../include/NetCoreStats.h"
%include "../../include/NetClientStats.h"
%include "../../include/NetConfig.h"
%include "../../include/ThreadPool.h"
%include "../../include/ReceivedMessage.h"
%include "../../include/RoundTripLatencyTest.h"

%{
using namespace Proud;
%}
