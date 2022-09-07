@echo off
chcp 65001
setlocal enabledelayedexpansion

set arg2=%2
set arg1=%1
if "%arg1%" == "" (
    echo Usage:
    echo     proudnet_build.bat clean
    echo     or
    echo     aproudnet_build.bat build [r13b^|r17c^|r20b^|r25b]
    goto exit
) else (
    goto %arg1%
)

:clean
call gradlew.bat clean
rd /s /q app\.cxx\
rd /s /q build\
rd /s /q proudnetlib\build\
rd /s /q proudnetlib\libs\
goto exit

:build
@rem 안드로이드 SDK, NDK 경로
set android_sdk_path=%ANDROID_SDK_HOME%
if "%android_sdk_path:~-1%" == "\" (
    set android_sdk_path=%android_sdk_path:0,-1%
)
set android_sdk_path=%android_sdk_path:\=\\%
set android_sdk_path=%android_sdk_path::=\:%

set android_ndk_base_path=%ANDROID_NDK_HOME%
if "%android_ndk_base_path:~-1%" == "\" (
    set android_ndk_base_path=%android_ndk_base_path:0,-1%
)

set ndk_version_list[0]=r13b
set ndk_version_list[1]=r17c
set ndk_version_list[2]=r20b
set ndk_version_list[3]=r25b

@rem 2번째 인자가 전달된 경우엔 해당 버전의 NDK만을 이용하여 빌드합니다.
for /l %%i in (0,1,3) do (
    set temp=!ndk_version_list[%%i]!
    if not "!arg2!" == "" (
        if not "!arg2!" == "!ndk_version_list[%%i]!" (
            set temp=
        )
    )
    set ndk_version_list[%%i]=!temp!
)

for /l %%i in (0,1,3) do (
    if not "!ndk_version_list[%%i]!" == "" (
        echo !ndk_version_list[%%i]! 버전을 빌드합니다...

        @rem 1) NDK 버전에 맞게 local.properties 파일 생성하기
        set android_ndk_path=!android_ndk_base_path:\=\\!
        set android_ndk_path=!android_ndk_path::=\:!
        set android_ndk_path=!android_ndk_path!\\android-ndk-!ndk_version_list[%%i]!

        del local.properties
        echo.ndk.dir=!android_ndk_path!>> local.properties
        echo.sdk.dir=!android_sdk_path!>> local.properties

        @rem 2) 빌드하기
        copy /y proudnetlib\build.gradle.!ndk_version_list[%%i]! proudnetlib\build.gradle
        call gradlew.bat build -p proudnetlib\
    )
)

:exit
