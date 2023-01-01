@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

set project_list=all pidl authnet_lib lic_auth_lib pnlic_lib pnlic_auth pnlic_warn pnlic_hidden watermark
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
		echo           project : all ^| pidl ^| authnet_lib ^| lic_auth_lib ^| pnlic_lib ^| pnlic_auth  ^| pnlic_warn ^| pnlic_hidden ^| watermark
		exit /b
	)

	@rem 프로젝트 별로 빌드함수를 호출합니다.
	@rem   (1) 환경변수 체크
	call :check_environment_variable

	@rem   (2) Pidl 프로젝트 빌드
	call :build_project_pidl

	@rem   (3) Library 빌드 (PNLicAuthServer.exe도 포함)
	call :build_project_authnet_lib
	call :build_project_lic_auth_lib
	call :build_project_pnlic_lib

	@rem   (4) PNLicense 실행파일 빌드
	call :build_project_pnlic_auth

	@rem   (5) PNLicense 이미지 빌드 - core/ProudNetServer LIB/DLL 프로젝트에서 활용
	call :build_project_pnlic_warn
	call :build_project_pnlic_hidden

	@rem   (6) Watermark 빌드 - 패키징 시에 활용
	call :build_project_watermark
exit /b

:build_project_pidl
	if not "%msbuild_project%" == "all" (
		if not "%msbuild_project%" == "pidl" (
			exit /b
		)
	)

	@rem NuGet 패키지 매니저를 다운로드 받아 설치합니다. - 현재(2022.11.01) 기준으로 v6.3.1이 최신 버전입니다.
	call :download_nuget

	@rem PIDL 컴파일러 프로젝트를 빌드/클린합니다.
	call :compile_command ".\Pidl" "1-1) Pidl.csproj"
exit /b

:build_project_authnet_lib
	if not "%msbuild_project%" == "all" (
		if not "%msbuild_project%" == "authnet_lib" (
			exit /b
		)
	)

	@rem pnlic 전용의 ProudNet 프로젝트들을 빌드/클린합니다.
	call :compile_command ".\AuthNetLib\ProudNetPidl" "ProudNetPidl.vcxproj"
	call :compile_command ".\AuthNetLib\ProudNetClient" "ProudNetClient.vcxproj"
	call :compile_command ".\AuthNetLib\ProudNetServer" "ProudNetServer.vcxproj"
exit /b

:build_project_lic_auth_lib
	if not "%msbuild_project%" == "all" (
		if not "%msbuild_project%" == "lic_auth_lib" (
			exit /b
		)
	)

	@rem PNLicAuth 프로젝트들을 빌드/클린합니다.
	call :compile_command ".\LicAuthLib\PNLicAuthCommon" "PNLicAuthCommon.vcxproj"
	call :compile_command ".\LicAuthLib\PNLicAuthClient" "PNLicAuthClient.vcxproj"
	call :compile_command ".\LicAuthLib\PNLicAuthServer" "PNLicAuthServer.vcxproj"
exit /b

:build_project_pnlic_lib
	if not "%msbuild_project%" == "all" (
		if not "%msbuild_project%" == "pnlic_lib" (
			exit /b
		)
	)

	@rem PNLicense 프로젝트들을 빌드/클린합니다.
	call :compile_command ".\PNLicenseSdk" "PNLicenseSdk.vcxproj"
	call :compile_command ".\PNLicense" "PNLicense.vcxproj"
exit /b

:build_project_pnlic_auth
	if not "%msbuild_project%" == "all" (
		if not "%msbuild_project%" == "pnlic_auth" (
			exit /b
		)
	)

	@rem PNLicenseAuth, PNLicenseAuthGui 프로젝트를 빌드/클린합니다.
	call :compile_command ".\PNLicenseAuth" "PNLicenseAuth.vcxproj"
	call :compile_command ".\PNLicenseAuthGui" "1-11) PNLicenseAuthGui.csproj"
exit /b

:build_project_pnlic_warn
	if not "%msbuild_project%" == "all" (
		if not "%msbuild_project%" == "pnlic_warn" (
			exit /b
		)
	)

	@rem PNLicenseWarning 프로젝트를 빌드/클린합니다.
	call :compile_command ".\PNLicenseWarning" "PNLicenseWarning.vcxproj"

	if "%msbuild_target%" == "clean" (
		del /f ".\PNLicenseManager\PNLicenseWarningImage.inl"
	)
exit /b

:build_project_pnlic_hidden
	if not "%msbuild_project%" == "all" (
		if not "%msbuild_project%" == "pnlic_hidden" (
			exit /b
		)
	)

	@rem PNLicenseHidden 프로젝트를 빌드/클린합니다.
	call :compile_command ".\PNLicenseHidden" "PNLicenseHidden.vcxproj"

	if "%msbuild_target%" == "clean" (
		del /f ".\PNLicenseManager\PNLicenseHiddenImage.inl"
	)
exit /b

:build_project_watermark
	if not "%msbuild_project%" == "all" (
		if not "%msbuild_project%" == "watermark" (
			exit /b
		)
	)

	@rem Watermark 프로젝트들을 빌드/클린합니다.
	call :compile_command ".\Watermark\WatermarkLib" "WatermarkLib.vcxproj"
	call :compile_command ".\Watermark\WatermarkDll" "WatermarkDll.vcxproj"
exit /b

:download_nuget
	@rem NuGet 패키지 관리 파일을 다운로드 받아 설치합니다.
	set nuget_folder=..\utils\NuGet
	set nuget_filename=nuget.exe
	set s3_url=https://proudnet-utils.s3.us-east-1.amazonaws.com
	set zip_filename=NuGet-v6.4.0.zip

	if not exist "%nuget_folder%" (
		mkdir "%nuget_folder%"
	)

	if not exist "%nuget_folder%\%nuget_filename%" (
		powershell "(new-Object System.Net.WebClient).DownloadFile('%s3_url%/%zip_filename%', '%nuget_folder%\%zip_filename%')"
		powershell "(Expand-Archive -Path %nuget_folder%\%zip_filename% -DestinationPath %nuget_folder%)"
		del /f /q "%nuget_folder%\%zip_filename%"
	)

	@rem NuGet 패키지를 다운로드 받습니다.
	set pkg_flag=F

	if not exist ".\packages\Antlr4.4.6.6\" (
		set pkg_flag=T
	)
	if not exist ".\packages\Antlr4.CodeGenerator.4.6.6\" (
		set pkg_flag=T
	)
	if not exist ".\packages\Antlr4.Runtime.4.6.6\" (
		set pkg_flag=T
	)
	if not exist ".\packages\YamlDotNet.12.0.2\" (
		set pkg_flag=T
	)

	if "%pkg_flag%" == "T" (
		pushd .\
		cd .\Pidl\
		"..\..\utils\NuGet\nuget.exe" restore -PackagesDirectory ..\packages\
		popd
	)
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
