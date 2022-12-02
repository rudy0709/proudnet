// module(directors="1")의 의미 => C++ 클래스를 C#에서 상속받아서 사용할려고 할 때 사용하는 키워드 
%module(directors="1") ProudNetServerPlugin

%{
#include "NativeType.h"
#include "ProudNetServerPlugin.h"
%}

%include "../PrivateSwigInterface/ProudNetServer.i"
%include "NativeType.h"
%include "ProudNetServerPlugin.h"
