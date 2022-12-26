@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

set project_list=all swig plugin dotnet sample
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
		echo           project : all ^| swig ^| plugin ^| dotnet ^| sample
		exit /b
	)

	@rem 프로젝트 별로 빌드함수를 호출합니다.
	@rem   (1) 환경변수 체크
	call :check_environment_variable

	@rem   (2) SWIG 프로젝트 빌드
	call :build_project_swig

	@rem   (3) 클라이언트/서버/WebGL 용의 Plugin 프로젝트 빌드
	call :build_project_plugin

	@rem   (4) 닷넷 용의 클라이언트/서버 프로젝트 빌드
	call :build_project_dotnet
	
	@rem   (5) 샘플 프로젝트 빌드
	call :build_sample_app
exit /b

:build_project_swig
	if not "%msbuild_project%" == "all" (
		if not "%msbuild_project%" == "swig" (
			exit /b
		)
	)

	@rem ProudNetClientPluginSwig, ProudNetServerPluginSwig 프로젝트를 빌드/클린합니다.
	call :compile_command ".\ProudNetClientPlugin" "ProudNetClientPluginSwig.vcxproj"
	call :compile_command ".\ProudNetServerPlugin" "ProudNetServerPluginSwig.vcxproj"

	if "%msbuild_target%" == "clean" (
		del /f /s /q ".\src\NetClient\PInvoke\*"
		del /f /s /q ".\src\NetServer\PInvoke\*"
	)
exit /b

:build_project_plugin
	if not "%msbuild_project%" == "all" (
		if not "%msbuild_project%" == "plugin" (
			exit /b
		)
	)

	@rem ProudNetClientPlugin, ProudNetServerPlugin, ProudNetClientPlugin-webgl 프로젝트를 빌드/클린합니다.
	call :compile_command ".\ProudNetClientPlugin" "ProudNetClientPlugin.vcxproj"
	call :compile_command ".\ProudNetServerPlugin" "ProudNetServerPlugin.vcxproj"
	call :compile_command ".\ProudNetClientPlugin-webgl" "5) ProudNetClientPlugin-webgl.csproj"
exit /b

:build_project_dotnet
	if not "%msbuild_project%" == "all" (
		if not "%msbuild_project%" == "dotnet" (
			exit /b
		)
	)

	@rem ProudDotNetClient, ProudDotNetServer 프로젝트를 빌드/클린합니다.
	call :compile_command ".\ProudDotNetClient" "6) ProudDotNetClient.csproj"
	call :compile_command ".\ProudDotNetServer" "7) ProudDotNetServer.csproj"
	call :compile_command ".\ProudDotNetClient" "8) ProudDotNetClientUnity.csproj"
exit /b

:build_sample_app
	if not "%msbuild_project%" == "all" (
		if not "%msbuild_project%" == "sample" (
			exit /b
		)
	)

	@rem SimpleClient, SimpleServer 프로젝트를 빌드/클린합니다.
	call :compile_command ".\SimpleClient" "SimpleClient.csproj"
	call :compile_command ".\SimpleServer" "SimpleServer.csproj"
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

	@rem 네번째 인자값의 유효성을 검사합니다.
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
