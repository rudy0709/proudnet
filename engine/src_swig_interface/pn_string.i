
%{
#include "PNString.h"

Proud::CStringEncoder* GetUTF16toUTF8()
{
	// ikpil.choi 2017.01.10 : Unity 4.x 쪽이 아직 C++98을 사용하므로, C++11 코드 제거
	//static Proud::CHeldPtr<Proud::CStringEncoder> encoder(Proud::CStringEncoder::Create("UTF-16LE", "UTF-8"));
	//return encoder;

	// junghoon.lee 2018.08.23 상기의 주석처리되어 있는 인코더를 사용하면 한글 문자열이 깨지는 문제가 발생하여 아래와 같이 디폴트 인코더로 CP949를 사용하게 바꾸었습니다.
	// 3개국어 (영어반드시 포함) 이상 한 문자열에 쓰이게 되면 깨지는 문제가 아직 있습니다.
	// 현재 임시 미봉책입니다.
	return NULL;
}

Proud::CStringEncoder* GetUTF8toUTF16()
{
	// ikpil.choi 2017.01.10 : Unity 4.x 쪽이 아직 C++98을 사용하므로, C++11 코드 제거
	//static Proud::CHeldPtr<Proud::CStringEncoder> encoder(Proud::CStringEncoder::Create("UTF-8", "UTF-16LE"));
	//return encoder;

	// junghoon.lee 2018.08.23 상기의 주석처리되어 있는 인코더를 사용하면 한글 문자열이 깨지는 문제가 발생하여 아래와 같이 디폴트 인코더로 CP949를 사용하게 바꾸었습니다.
	// 3개국어 (영어반드시 포함) 이상 한 문자열에 쓰이게 되면 깨지는 문제가 아직 있습니다.
	// 현재 임시 미봉책입니다.
	return NULL;
}

%}

namespace Proud
{

%naturalvar String;

class String;

// String
%typemap(ctype) String "char *"
%typemap(imtype) String "string"
%typemap(cstype) String "string"

%typemap(csdirectorin) String "$iminput"
%typemap(csdirectorout) String "$cscall"

%typemap(in, canthrow=1) String
%{
	if (!$input) {
		SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
		return $null;
	}
	$1 = StringA2T($input, GetUTF8toUTF16());
%}

%typemap(out) String %{ $result = SWIG_csharp_string_callback(StringT2A($1, GetUTF16toUTF8()).GetString()); %}
%typemap(out) const PNTCHAR* %{  $result = SWIG_csharp_string_callback(StringT2A($1, GetUTF16toUTF8()).GetString()); %}

%typemap(directorout, canthrow=1) String
%{ if (!$input) {
		SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
		return $null;
	}
	$result = StringA2T($input, GetUTF8toUTF16());
%}

%typemap(in) Proud::String %{
	// ikpil.choi 2017-01-09 : swig의 input이 Proud::String일 경우, A2T 실행하여 할당
	$1 = StringA2T($input, GetUTF8toUTF16());
%}

%typemap(in) Proud::PNTCHAR* %{
	// ikpil.choi 2017-01-09 : swig 의 input 이 PNTCHAR* 일 경우, StringA2T로 생성한 객체를 유지하고, PNTCHAR* 를 리턴하게 처리한다.(N3728)
	Proud::String $input_pnstring = StringA2T($input, GetUTF8toUTF16());
	$1 = (PNTCHAR*)$input_pnstring.GetString();
%}

%typemap(csin) String "$csinput"

%typemap(csout, excode=SWIGEXCODE) String {
	string ret = $imcall;$excode
	return ret;
}

%typemap(typecheck) String = char *;

%typemap(throws, canthrow=1) String
%{
	// 주의 : 어쩌면 ProudCatchAllException.i에 넣은 %exception으로 인해서 이 throws typemap이 가려질 수도 있을 거 같다.
	SWIG_CSharpSetPendingException(SWIG_CSharpApplicationException, StringT2A($1, GetUTF16toUTF8()).GetString());
	return $null;
%}

// const String &
%typemap(ctype) const String & "char *"
%typemap(imtype) const String & "string"
%typemap(cstype) const String & "string"

%typemap(csdirectorin) const String & "$iminput"
%typemap(csdirectorout) const String & "$cscall"

%typemap(in, canthrow=1) const String &
%{ if (!$input) {
		SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
		return $null;
	}
	$*1_ltype $1_str(StringA2T($input, GetUTF8toUTF16()));
	$1 = &$1_str;
%}

%typemap(out) const String & %{ $result = SWIG_csharp_string_callback(StringT2A(*$1, GetUTF16toUTF8()).GetString()); %}


%typemap(csvarin, excode=SWIGEXCODE2) const String & %{
	set {
		$imcall;$excode
	}
%}
%typemap(csvarout, excode=SWIGEXCODE2) const String & %{
	// 주의 : 어쩌면 ProudCatchAllException.i에 넣은 %exception으로 인해서 이 throws typemap이 가려질 수도 있을 거 같다.
	get {
		string ret = $imcall;$excode
		return ret;
	}
%}

%typemap(csin) const String & "$csinput"

%typemap(throws, canthrow=1) const String &
%{
	SWIG_CSharpSetPendingException(SWIG_CSharpApplicationException, StringT2A(*$1, GetUTF16toUTF8()).GetString());
	return $null;
%}

}
