@echo off

:main
	if "%1" == "clean" (
		call :clean_library
	) else if "%1" == "build" (
		call :build_library %2
	) else (
		@rem 사용법을 출력합니다.
		echo Usage:
		echo     proudnet_build.bat clean
		echo     or
		echo     proudnet_build.bat build [ndk-version=r22b^|r20b^|r17c^|r13b]
	)
exit /b 0

:clean_library
	call gradlew.bat clean

	@rem 빌드 시에 생성되었으나, clean으로 제거되지 않은 디렉토리를 삭제합니다.
	rd /s /q app\.cxx\
	rd /s /q build\
	rd /s /q proudnetlib\build\
	rd /s /q proudnetlib\libs\
	del /s /q proudnetlib/build.gradle
	del /s /q local.properties
exit /b 0

:build_library
	chcp 65001
	setlocal enabledelayedexpansion

	@rem 안드로이드 SDK, NDK 경로를 환경변수에서 가져옵니다.
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

	@rem 2번째 인자가 전달된 경우엔 해당 버전의 NDK만을 남기고 나머지 버전의 NDK는 버립니다.
	set ndk_version_list[0]=r22b
	set ndk_version_list[1]=r20b
	set ndk_version_list[2]=r17c
	set ndk_version_list[3]=r13b

	for /l %%i in (0,1,3) do (
		if not "%~1" == "" (
			if not "%~1" == "!ndk_version_list[%%i]!" (
				set ndk_version_list[%%i]=
			)
		)
	)

	@rem ndk_version_list 배열에서 값이 남아있는 항목만 빌드합니다.
	for /l %%i in (0,1,3) do (
		set ndk_version=!ndk_version_list[%%i]!
		if not "!ndk_version!" == "" (
			call :build_library_by_clang

			call :compress_library "arm64-v8a" "aarch64-linux-android-4.9" "aarch64-linux-android-strip.exe"
			call :compress_library "armeabi-v7a" "arm-linux-androideabi-4.9" "arm-linux-androideabi-strip.exe"
			call :compress_library "x86" "x86-4.9" "i686-linux-android-strip.exe"
		)
	)
exit /b 0

:build_library_by_clang
	setlocal enabledelayedexpansion

	echo %ndk_version% 버전의 NDK로 빌드합니다...

	@rem 1) NDK 버전에 맞게 local.properties 파일을 생성합니다.
	set android_ndk_path=%android_ndk_base_path:\=\\%
	set android_ndk_path=%android_ndk_path::=\:%
	set android_ndk_path=%android_ndk_path%\\android-ndk-%ndk_version%

	del local.properties
	echo ^-^-^> local.properties 파일을 생성합니다.
	echo ndk.dir=%android_ndk_path%>  local.properties
	echo sdk.dir=%android_sdk_path%>> local.properties

	@rem 2) NDK 버전에 맞는 proudnetlib\build.gradle 파일을 복사합니다.
	echo ^-^-^> proudnetlib\build.gradle.%ndk_version% 파일을 proudnetlib\build.gradle로 복사합니다.
	copy /y proudnetlib\build.gradle.%ndk_version% proudnetlib\build.gradle

	@rem 3) 빌드를 시작합니다.
	set GRADLE_EXIT_CONSOLE=1
	call gradlew.bat build -p proudnetlib\
exit /b 0

:compress_library
	setlocal enabledelayedexpansion

	set current_path=%~dp0
	set last_char=%current_path:~-1%
	if "%last_char%" == "\" (
		set current_path=%current_path:~0,-1%
	)

	set org_debug_lib_path=%current_path%\proudnetlib\libs\ndk\%ndk_version%\cmake\clangDebug\%~1
	set org_release_lib_path=%current_path%\proudnetlib\libs\ndk\%ndk_version%\cmake\clangRelease\%~1
	set debug_lib_path=%current_path%\build\libs\%ndk_version%\cmake\cclangDebug\%~1
	set release_lib_path=%current_path%\build\libs\%ndk_version%\cmake\cclangRelease\%~1
	set strip_exe_path=%android_ndk_base_path%\android-ndk-%ndk_version%\toolchains\%~2\prebuilt\windows-x86_64\bin\%~3

	mkdir %debug_lib_path%\
	mkdir %release_lib_path%\stripped\
	copy /y %org_debug_lib_path%\libProudNetClient.a %debug_lib_path%\libProudNetClient.a
	copy /y %org_release_lib_path%\libProudNetClient.a %release_lib_path%\libProudNetClient.a
	copy /y %org_release_lib_path%\libProudNetClient.a %release_lib_path%\stripped\libProudNetClient.a
	%strip_exe_path% %release_lib_path%\stripped\libProudNetClient.a
exit /b 0

call main %1, %2
