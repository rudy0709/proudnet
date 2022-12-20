@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

set project_list=all pidl client server
set msbuild_target=
set msbuild_config_cpp=
set msbuild_platform_cpp=
set msbuild_config_cs=
set msbuild_platform_cs=
set msbuild_project=

:main
	@rem 사용법을 출력해야 할 지를 검사합니다.
	call :check_comandline_params %1 %2 %3 %4
	if "%errorlevel%" == "1" (
		@rem 사용법을 출력합니다.
		echo Usage:
		echo     pnlic_build.bat clean ^<configuration^> ^<platform^> ^<project^>
		echo     pnlic_build.bat build ^<configuration^> ^<platform^> ^<project^>
		echo Options:
		echo     configuration : Debug ^| Debug_static_CRT ^| Release ^| Release_static_CRT
		echo          platform : Win32 ^| x64
		echo           project : all ^| pidl ^| client ^| server
		exit /b
	)

	@rem 프로젝트 별로 빌드함수를 호출합니다.
	@rem   (1) 환경변수 체크
	call :check_environment_variable

	@rem   (2) ProudNetPidl 프로젝트 빌드
	call :build_library_pidl

	@rem   (3) ProudNetClient 프로젝트 빌드
	call :build_library_client

	@rem   (4 ProudNetServer 프로젝트 빌드
	call :build_library_server
exit /b

:build_library_pidl
	if not "%msbuild_project%" == "all" (
		if not "%msbuild_project%" == "pidl" (
			exit /b
		)
	)

	@rem ProudNetPidl 프로젝트를 빌드/클린합니다.
	call :compile_command ".\ProudNetPidl" "ProudNetPidl.vcxproj"

	if "%msbuild_target%" == "clean" (
		del /f /s /q ".\include\ErrorType*.h"
		del /f /s /a ".src\ErrorType.inl"
		del /f /s /q ".\src\AgentC2S_*.*"
		del /f /s /q ".\src\AgentS2C_*.*"
		del /f /s /q ".\src\DB_*.*"
		del /f /s /q ".\src\DumpC2S_*.*"
		del /f /s /q ".\src\DumpS2C_*.*"
		del /f /s /q ".\src\EmergencyC2S_*.*"
		del /f /s /q ".\src\NetC2C_*.*"
		del /f /s /q ".\src\NetC2S_*.*"
		del /f /s /q ".\src\NetS2C_*.*"
		del /f /s /q ".\src\Viz_*.*"
	)
exit /b

:build_library_client
	if not "%msbuild_project%" == "all" (
		if not "%msbuild_project%" == "client" (
			exit /b
		)
	)

	@rem ProudNetClient 프로젝트들을 빌드/클린합니다.
	call :compile_command ".\ProudNetClientLib" "ProudNetClientLib.vcxproj"
	call :compile_command ".\ProudNetClientDll" "ProudNetClientDll.vcxproj"
exit /b

:build_library_server
	if not "%msbuild_project%" == "all" (
		if not "%msbuild_project%" == "server" (
			exit /b
		)
	)

	@rem ProudNetServer 프로젝트들을 빌드/클린합니다.
	call :compile_command ".\ProudNetServerLib" "ProudNetServerLib.vcxproj"
	call :compile_command ".\ProudNetServerDll" "ProudNetServerDll.vcxproj"
exit /b

@rem 공통 로직...
:check_comandline_params
	@rem 첫번째 인자값의 유효성을 검사합니다.
	set target_list=clean build
	for %%i in (%target_list%) do (
		if "%1" == "%%i" (
			set msbuild_target=%%i
		)
	)

	if "%msbuild_target%" == "" (
		echo ^>^>^>^> Error : 1st^(Build Target^) argument is invalid.
		echo ^>^>^>^>
		exit /b 1
	)

	@rem 두번째 인자값의 유효성을 검사합니다.
	set config_list=Debug Debug_static_CRT Release Release_static_CRT
	for %%i in (%config_list%) do (
		if "%2" == "%%i" (
			set msbuild_config_cpp=%%i
		)
	)

	if "%msbuild_config_cpp%" == "" (
		echo ^>^>^>^> Error : 2nd^(Build Configuration^) argument is invalid.
		echo ^>^>^>^>
		exit /b 1
	)

	@rem 세번째 인자값의 유효성을 검사합니다.
	set platform_list=Win32 x64
	for %%i in (%platform_list%) do (
		if "%3" == "%%i" (
			set msbuild_platform_cpp=%%i
		)
	)

	if "%msbuild_platform_cpp%" == "" (
		echo ^>^>^>^> Error : 3rd^(Build Platform^) argument is invalid.
		echo ^>^>^>^>
		exit /b 1
	)

	@rem 세번째 인자값의 유효성을 검사합니다.
	for %%i in (%project_list%) do (
		if "%4" == "%%i" (
			set msbuild_project=%%i
		)
	)

	if "%msbuild_project%" == "" (
		echo ^>^>^>^> Error : 4th^(Build Project^) argument is invalid.
		echo ^>^>^>^>
		exit /b 1
	)

	if not "%msbuild_config_cpp:Release=%" == "%msbuild_config_cpp%" (
		set msbuild_config_cs=Release
	) else (
		set msbuild_config_cs=Debug
	)
	set msbuild_platform_cs=AnyCPU
exit /b 0

:check_environment_variable
	@rem 환경변수가 등록되어 있는 지를 체크합니다.
	if "%PN_BUILD_PATH%" == "" (
		@rem 예시 : C:\Program Files\Microsoft Visual Studio\2022\Professional\Msbuild\Current\Bin\MSBuild.exe
		echo ^>^>^>^> Error : Register the PN_BUILD_PATH environment variable before executing the batch file.
		echo ^>^>^>^>
		exit /b
	)

	if "%PN_OBFUSCATION_TOOL_PATH%" == "" (
		@rem 예시 : C:\Program Files (x86)\Gapotchenko\Eazfuscator.NET\Eazfuscator.NET.exe
		echo ^>^>^>^> Error : Register the PN_OBFUSCATION_TOOL_PATH environment variable before executing the batch file.
		echo ^>^>^>^>
		exit /b
	)

	if "%PN_SIGN_TOOL_PATH%" == "" (
		@rem 예시 : C:\Program Files (x86)\Microsoft SDKs\ClickOnce\SignTool\signtool.exe
		echo ^>^>^>^> Error : Register the PN_SIGN_TOOL_PATH environment variable before executing the batch file.
		echo ^>^>^>^>
		exit /b
	)

	echo ^>^>^>^> Env-Var = ^{
	echo ^>^>^>^>              "PN_BUILD_PATH" : "%PN_BUILD_PATH%",
	echo ^>^>^>^>   "PN_OBFUSCATION_TOOL_PATH" : "%PN_OBFUSCATION_TOOL_PATH%",
	echo ^>^>^>^>           "PN_SIGN_TOOL_PATH": "%PN_SIGN_TOOL_PATH%"
	echo ^>^>^>^> ^}
	echo ^>^>^>^>
exit /b

:compile_command
	set param1=%~1
	set param2=%~2
	set vs_version=17

	set msbuild_config=
	set msbuild_platform=
	if not "%param2:vcxproj=%" == "%param2%" (
		set msbuild_config=!msbuild_config_cpp!
		set msbuild_platform=!msbuild_platform_cpp!
	) else if not "%param2:csproj=%" == "%param2%" (
		set msbuild_config=!msbuild_config_cs!
		set msbuild_platform=!msbuild_platform_cs!
	)

	echo ^>^>^>^> CommandLine : "%PN_BUILD_PATH%" "%param1%\%param2%" "/t:%msbuild_target%" "/p:Configuration=%msbuild_config%;Platform=%msbuild_platform%;VisualStudioVersion=%vs_version%;BuildProjectReferences=false" /m
	echo ^>^>^>^>
	echo ^>^>^>^> --------------------------------------------------
	"%PN_BUILD_PATH%" "%param1%\%param2%" "/t:%msbuild_target%" "/p:Configuration=%msbuild_config%;Platform=%msbuild_platform%;VisualStudioVersion=%vs_version%;BuildProjectReferences=false" /m
	echo ^>^>^>^> --------------------------------------------------

	if "%msbuild_target%" == "clean" (
		rd /s /q "%param1%\bin\"
		rd /s /q "%param1%\obj\"
	)
exit /b

call :main
