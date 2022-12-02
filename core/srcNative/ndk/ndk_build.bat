setlocal

rem lib ������ ���丮�� �����Ѵ�.
if "%1"=="arm64-v8a" (
mkdir ..\..\lib\Plugins\Unity\Android\libs\arm64-v8a
mkdir ..\..\lib\Plugins\Unity\Android\libs\arm64-v8a\stripped
)

if "%1"=="armeabi-v7a" (
mkdir ..\..\lib\Plugins\Unity\Android\libs\armeabi-v7a
mkdir ..\..\lib\Plugins\Unity\Android\libs\armeabi-v7a\stripped
)

if "%1"=="x86" (
mkdir ..\..\lib\Plugins\Unity\Android\libs\x86
mkdir ..\..\lib\Plugins\Unity\Android\libs\x86\stripped
)

rem cross compiler�� ���� ��θ� �����Ѵ�.
set PATH=%THIRD_PARTY%\android-ndk-r17;

rem ndk-build�� �����Ѵ�.
call ndk-build --jobs=4 -e NDK_TOOLCHAIN_VERSION=4.9 NDK_DEBUG=0

endlocal

rem library ���丮�� �����Ѵ�.
if "%1"=="arm64-v8a" (
copy obj\local\arm64-v8a\libProudNetClientPlugin.so ..\..\lib\Plugins\Unity\Android\libs\arm64-v8a
copy libs\arm64-v8a\libProudNetClientPlugin.so ..\..\lib\Plugins\Unity\Android\libs\arm64-v8a\stripped
)

if "%1"=="armeabi-v7a" (
copy obj\local\armeabi-v7a\libProudNetClientPlugin.so ..\..\lib\Plugins\Unity\Android\libs\armeabi-v7a
copy libs\armeabi-v7a\libProudNetClientPlugin.so ..\..\lib\Plugins\Unity\Android\libs\armeabi-v7a\stripped
)

if "%1"=="x86" (
copy obj\local\x86\libProudNetClientPlugin.so ..\..\lib\Plugins\Unity\Android\libs\x86
copy libs\x86\libProudNetClientPlugin.so ..\..\lib\Plugins\Unity\Android\libs\x86\stripped
)
