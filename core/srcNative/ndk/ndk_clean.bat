setlocal

if "%1"=="arm64-v8a" (
rd /q /s ..\..\lib\Plugins\Unity\5.x\Android\libs\arm64-v8a
)

if "%1"=="armeabi-v7a" (
rd /q /s ..\..\lib\Plugins\Unity\5.x\Android\libs\armeabi-v7a
)

if "%1"=="x86" (
rd /q /s ..\..\lib\Plugins\Unity\5.x\Android\libs\x86
)

set PATH=%THIRD_PARTY%\android-ndk-r17;

call ndk-build clean

endlocal