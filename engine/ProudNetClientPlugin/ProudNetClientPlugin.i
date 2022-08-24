/* Q: 

Custom Build가 project file이 아닌 이 .i 파일에 되어 있습니다. 게다가 
..\..\util\internal\AreFilesModified ..\swig\swig.exe;..\..\include\*.h;.\*.i SwigCompileHistoryClient.yaml
if %ERRORLEVEL% GEQ 1 (
    ..\swig\swig.exe -c++ -csharp -namespace Nettention.Proud ProudNetClientPlugin.i 
) else (
    echo Source files are up-to-date. Compiling is skipped.
)
이런 기괴한 형식이고, 특히, custom build output이,
intentional_never_existing_output_file_for_always_running_this_command.txt
이러한 기괴한 형식입니다. 

왜 이런가요? 그냥 Project-wide하게 Build Action을 지정해도 되잖아요?

A: 예전에는 그랬으나, msbuild 내부에서 모종의 빌드 불필요 판단을 해버리는 경우 Build Action 자체를 안해버리더군요.
그래서 이렇게 하고 있습니다.

*/

// module(directors="1")의 의미 => C++ 클래스를 C#에서 상속받아서 사용할려고 할 때 사용하는 키워드 
%module(directors="1") ProudNetClientPlugin

%{
#include "NativeType.h"
#include "EventWrap.h"
#include "ProudNetClientPlugin.h"
%}

%include "../src_swig_interface/ProudNetClient.i"
%include "NativeType.h"
%include "ProudNetClientPlugin.h"
