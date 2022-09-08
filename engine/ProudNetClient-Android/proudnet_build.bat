@echo off
chcp 65001
setlocal enabledelayedexpansion

set arg2=%2
set arg1=%1
if "%arg1%" == "" (
    echo Usage:
    echo     proudnet_build.bat clean
    echo     or
    echo     aproudnet_build.bat build [r22b^|r20b^|r17c^|r13b]
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
@rem .pidl, ErrorType.yaml 파일을 컴파일합니다.
..\..\utils\Pidl\bin\Release\PIDL_UnProtect.exe ..\ProudNetPidlBuild\*.pidl -outdir ..\src\
..\..\utils\Pidl\bin\Release\PIDL_UnProtect.exe ..\ProudNetPidlBuild\ErrorType.yaml -outdir ..\src\ -incdir ..\src\

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

set ndk_version_list[0]=r22b
set ndk_version_list[1]=r20b
set ndk_version_list[2]=r17c
set ndk_version_list[3]=r13b

@rem 2번째 인자가 전달된 경우엔 해당 버전의 NDK만을 남기고 나머지 버전의 NDK는 버립니다.
for /l %%i in (0,1,3) do (
    set temp=!ndk_version_list[%%i]!
    if not "!arg2!" == "" (
        if not "!arg2!" == "!ndk_version_list[%%i]!" (
            set temp=
        )
    )
    set ndk_version_list[%%i]=!temp!
)

@rem ndk_version_list 배열에서 값이 남아있는 항목만 빌드합니다.
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

        @rem 3) .a 파일 압축하기
        set current_path=%~dp0
        if "%current_path:~-1%" == "\" (
            set current_path=%current_path:0,-1%
        )

        @rem 3-1) arm64-v8a
        set org_lib_path=!current_path!\proudnetlib\libs\ndk\!ndk_version_list[%%i]!\cmake\clangRelease\arm64-v8a
        set strip_exe_path=!android_ndk_base_path!\android-ndk-!ndk_version_list[%%i]!\toolchains\aarch64-linux-android-4.9\prebuilt\windows-x86_64\aarch64-linux-android\bin

        mkdir !org_lib_path!\stripped
        copy /y !org_lib_path!\libProudNetClient.a !org_lib_path!\stripped\libProudNetClient.a
        !strip_exe_path!\strip.exe !org_lib_path!\stripped\libProudNetClient.a

        @rem 3-2) armeabi-v7a
        set org_lib_path=!current_path!\proudnetlib\libs\ndk\!ndk_version_list[%%i]!\cmake\clangRelease\armeabi-v7a
        set strip_exe_path=!android_ndk_base_path!\android-ndk-!ndk_version_list[%%i]!\toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\arm-linux-androideabi\bin

        mkdir !org_lib_path!\stripped
        copy /y !org_lib_path!\libProudNetClient.a !org_lib_path!\stripped\libProudNetClient.a
        !strip_exe_path!\strip.exe !org_lib_path!\stripped\libProudNetClient.a

        @rem 3-3) x86
        set org_lib_path=!current_path!\proudnetlib\libs\ndk\!ndk_version_list[%%i]!\cmake\clangRelease\x86
        set strip_exe_path=!android_ndk_base_path!\android-ndk-!ndk_version_list[%%i]!\toolchains\x86-4.9\prebuilt\windows-x86_64\i686-linux-android\bin

        mkdir !org_lib_path!\stripped
        copy /y !org_lib_path!\libProudNetClient.a !org_lib_path!\stripped\libProudNetClient.a
        !strip_exe_path!\strip.exe !org_lib_path!\stripped\libProudNetClient.a
    )
)

:exit
