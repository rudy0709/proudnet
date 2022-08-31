
// managed netClient delegte event
// C++ => C# �ݹ� �Լ�����, �Լ� ���� ������ ���� ������ �������ش�.

%typemap(csin) void (*)(void*, void*, void*) "$csinput";
%typemap(cstype) void (*)(void*, void*, void*) "Nettention.Proud.ProudDelegate.Delegate_8";
%typemap(imtype) void (*)(void*, void*, void*) "Nettention.Proud.ProudDelegate.Delegate_8";

%typemap(csin) void (*)(void*) "$csinput";
%typemap(cstype) void (*)(void*) "Nettention.Proud.ProudDelegate.Delegate_5";
%typemap(imtype) void (*)(void*) "Nettention.Proud.ProudDelegate.Delegate_5";

%typemap(csin) void (*)(void*, int, int, int, void*) "$csinput";
%typemap(cstype) void (*)(void*, int, int, int, void*) "Nettention.Proud.ProudDelegate.Delegate_9";
%typemap(imtype) void (*)(void*, int, int, int, void*) "Nettention.Proud.ProudDelegate.Delegate_9";

%typemap(csin) void (*)(void*, int, int, int) "$csinput";
%typemap(cstype) void (*)(void*, int, int, int) "Nettention.Proud.ProudDelegate.Delegate_10";
%typemap(imtype) void (*)(void*, int, int, int) "Nettention.Proud.ProudDelegate.Delegate_10";

%typemap(csin) void (*)(void*, int, int) "$csinput";
%typemap(cstype) void (*)(void*, int, int) "Nettention.Proud.ProudDelegate.Delegate_11";
%typemap(imtype) void (*)(void*, int, int) "Nettention.Proud.ProudDelegate.Delegate_11";

%typemap(csin) void (*)(void*, int) "$csinput";
%typemap(cstype) void (*)(void*, int) "Nettention.Proud.ProudDelegate.Delegate_12";
%typemap(imtype) void (*)(void*, int) "Nettention.Proud.ProudDelegate.Delegate_12";

%typemap(csin) void (*)(void*, int, void*, void*, int) "$csinput";
%typemap(cstype) void (*)(void*, int, void*, void*, int) "Nettention.Proud.ProudDelegate.Delegate_7";
%typemap(imtype) void (*)(void*, int, void*, void*, int) "Nettention.Proud.ProudDelegate.Delegate_7";

%typemap(csin) void* (*)(void*) "$csinput";
%typemap(cstype) void* (*)(void*) "Nettention.Proud.ProudDelegate.Delegate_6";
%typemap(imtype) void* (*)(void*) "Nettention.Proud.ProudDelegate.Delegate_6";

%typemap(csin) int (*)(void*) "$csinput";
%typemap(cstype) int (*)(void*) "Nettention.Proud.ProudDelegate.Delegate_1";
%typemap(imtype) int (*)(void*) "Nettention.Proud.ProudDelegate.Delegate_1";

%typemap(csin) bool (*)(void*, void*, int64_t) "$csinput";
%typemap(cstype) bool (*)(void*, void*, int64_t) "Nettention.Proud.ProudDelegate.Delegate_4";
%typemap(imtype) bool (*)(void*, void*, int64_t) "Nettention.Proud.ProudDelegate.Delegate_4";

%typemap(csin) void (*)(void*, int, void*) "$csinput";
%typemap(cstype) void (*)(void*, int, void*) "Nettention.Proud.ProudDelegate.Delegate_3";
%typemap(imtype) void (*)(void*, int, void*) "Nettention.Proud.ProudDelegate.Delegate_3";

%typemap(csin) void (*)(int, void*) "$csinput";
%typemap(cstype) void (*)(int, void*) "Nettention.Proud.ProudDelegate.Delegate_2";
%typemap(imtype) void (*)(int, void*) "Nettention.Proud.ProudDelegate.Delegate_2";

%typemap(csin) void* (*)(void*) "$csinput";
%typemap(cstype) void* (*)(void*) "Nettention.Proud.ProudDelegate.Delegate_6";
%typemap(imtype) void* (*)(void*) "Nettention.Proud.ProudDelegate.Delegate_6";

%typemap(csin) void (*)(void*, void*) "$csinput";
%typemap(cstype) void (*)(void*, void*) "Nettention.Proud.ProudDelegate.Delegate_0";
%typemap(imtype) void (*)(void*, void*) "Nettention.Proud.ProudDelegate.Delegate_0";
