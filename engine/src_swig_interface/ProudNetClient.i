
#if defined(_WIN32)
#define PROUD_API __declspec(dllimport)
#define PROUDSRV_API __declspec(dllimport)
#else
#define PROUD_API
#define PROUDSRV_API
#endif

//  Syntax error 방지
#define PN_ALIGN(1)
#define NO_COPYABLE(CReceivedMessage)

%{
#if defined(_WIN32)
#include <windows.h>
#include <rpc.h>
#endif
%}

%include <arrays_csharp.i>
%include <windows.i>
%include <std_string.i>
%include <stdint.i>
%include <cpointer.i>

%include <typemaps.i>
%include <carrays.i>

// 맨 처음에 include
%include "ProudCatchAllException.i"
%include "ProudReName.i"

%include "pn_enum.i"
%include "pn_int.i"
%include "pn_string.i"
%include "pn_guid.i"
%include "pn_fastarray.i"
%include "pn_bytearray.i"
%include "pn_netconnectionparam.i"
//%include "pn_netcoreevent.i"
//%include "pn_netclientevent.i"
%include "pn_netclient.i"
%include "pn_errorinfo.i"
%include "pn_exception.i"
%include "pn_rmistub.i"
%include "pn_irmihost.i"
%include "pn_addrport.i"
%include "pn_hostidarray.i"
%include "pn_hostidset.i"
%include "pn_socketinfo.i"
%include "pn_rmicontext.i"
%include "pn_acr.i"
%include "pn_functiontype.i"
%include "pn_disconnectargs.i"
%include "pn_netpeerinfo.i"
%include "pn_netconfig.i"
%include "pn_threadpool.i"
%include "pn_receivedmessage.i"

%include "ProudNetTypeMaps.i"

%include "ProudNetClientCPlusIncludes.i"
using namespace Proud;
