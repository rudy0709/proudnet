@echo off

set projects=all pnutils pidl virtualizer authnet pnlic_sdk pnlic pnlic_auth_lib watermark pnlic_warn pnlic_hidden pnlic_auth_exe

:main
	@rem 도움말을 출력해야 할 지를 검사합니다.
	call :check_comandline_params %1 %2
	if "%errorlevel%" == "1" (
		@rem 사용법을 출력합니다.
		echo Usage:
		echo     pnlic_build.bat clean ^<all ^| pnutils ^| pidl ^| virtualizer ^| authnet ^| pnlic_sdk ^| pnlic ^| pnlic_auth_lib ^| watermark ^| pnlic_warn ^| pnlic_hidden ^| pnlic_auth_exe^>
		echo     pnlic_build.bat build ^<all ^| pnutils ^| pidl ^| virtualizer ^| authnet ^| pnlic_sdk ^| pnlic ^| pnlic_auth_lib ^| watermark ^| pnlic_warn ^| pnlic_hidden ^| pnlic_auth_exe^>
		exit /b
	)

	@rem 프로젝트 별로 빌드함수를 호출합니다.
@rem	if "%1" == "build" (
@rem		call :download_utils_and_pkgs
@rem	)

	call :check_environment_variable
	call :process_library_pnutils %1 %2
	call :process_library_pidl %1 %2
	call :process_library_virtualizer %1 %2
	call :process_library_authnet %1 %2
	call :process_library_pnlic_sdk %1 %2
	call :process_library_pnlic %1 %2
	call :process_library_pnlic_auth_lib %1 %2
	call :process_library_watermark %1 %2
	call :process_library_pnlic_warn %1 %2
	call :process_library_pnlic_pnlic_hidden %1 %2
	call :process_library_pnlic_auth_exe %1 %2
exit /b

:check_environment_variable
	@rem 환경변수가 등록되어 있는 지를 체크합니다.
	if "%PN_BUILD_PATH%" == "" (
		echo ^>^>^>^> Error : Register the PN_BUILD_PATH environment variable before executing the batch file.
		exit /b
	)

	if "%PN_SIGN_TOOL_PATH%" == "" (
		echo ^>^>^>^> Error : Register the PN_SIGN_TOOL_PATH environment variable before executing the batch file.
		exit /b
	)

	echo ^>^>^>^> Environment-Variable^(PN_BUILD_PATH^) = "%PN_BUILD_PATH%"
	echo ^>^>^>^> Environment-Variable^(PN_SIGN_TOOL_PATH^) = "%PN_SIGN_TOOL_PATH%"
	echo ^>^>^>^>
exit /b

@rem //222-삭제: :download_utils_and_pkgs
@rem //222-삭제: 	call :download_nuget
@rem //222-삭제:
@rem //222-삭제: 	@rem PIDL 컴파일러 프로젝트에서 사용할 NuGet 패키지를 다운로드 받습니다.
@rem //222-삭제: 	set packages_flag=F
@rem //222-삭제: 	if not exist ".\packages\Antlr4.4.6.6\" (
@rem //222-삭제: 		set packages_flag=T
@rem //222-삭제: 	)
@rem //222-삭제: 	if not exist ".\packages\Antlr4.CodeGenerator.4.6.6\" (
@rem //222-삭제: 		set packages_flag=T
@rem //222-삭제: 	)
@rem //222-삭제: 	if not exist ".\packages\Antlr4.Runtime.4.6.6\" (
@rem //222-삭제: 		set packages_flag=T
@rem //222-삭제: 	)
@rem //222-삭제: 	if not exist ".\packages\YamlDotNet.12.0.2\" (
@rem //222-삭제: 		set packages_flag=T
@rem //222-삭제: 	)
@rem //222-삭제:
@rem //222-삭제: 	if "%packages_flag%" == "T" (
@rem //222-삭제: 		"..\utils\nuget.exe" restore -PackagesDirectory .\packages
@rem //222-삭제: 	)
@rem //222-삭제:
@rem //222-삭제: 	@rem CodeVirtualizer 코드 난독화 도구를 다운로드 받아 설치합니다.
@rem //222-삭제: 	call :download_virtualizer
@rem //222-삭제: exit /b

:process_library_pnutils
	if not "%2" == "all" (
		if not "%2" == "pnutils" (
			exit /b
		)
	)

	if "%1" == "clean" (
		@rem PnUtils 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\[Utils]\PnUtils\PnUtils.vcxproj" clean Release_static_CRT x64
		rd /s /q .\[Utils]\PnUtils\bin\
		rd /s /q .\[Utils]\PnUtils\obj\
	) else if "%1" == "build" (
		@rem PnUtils 프로젝트를 빌드합니다.
		call :compile_command ".\[Utils]\PnUtils\PnUtils.vcxproj" build Release_static_CRT x64
	)
exit /b

:process_library_pidl
	if not "%2" == "all" (
		if not "%2" == "pidl" (
			exit /b
		)
	)

	if "%1" == "clean" (
		@rem PIDL 컴파일러 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\[Utils]\PIDL\PIDL.csproj" clean Release AnyCPU
		rd /s /q .\[Utils]\PIDL\bin\
		rd /s /q .\[Utils]\PIDL\obj\
	) else if "%1" == "build" (
		@rem PIDL 컴파일러 프로젝트를 빌드합니다.
		call :compile_command ".\[Utils]\PIDL\PIDL.csproj" build Release AnyCPU
	)
exit /b

:process_library_virtualizer
	if not "%2" == "all" (
		if not "%2" == "virtualizer" (
			exit /b
		)
	)
	if "%1" == "clean" (
		@rem CodeVirtualizer 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\CodeVirtualizer\CodeVirtualizer.vcxproj" clean Release_static_CRT x64
		rd /s /q .\CodeVirtualizer\bin\
		rd /s /q .\CodeVirtualizer\obj\
	) else if "%1" == "build" (
		@rem CodeVirtualizer 프로젝트를 빌드합니다.
		call :compile_command ".\CodeVirtualizer\CodeVirtualizer.vcxproj" build Release_static_CRT x64
	)
exit /b

:process_library_authnet
	if not "%2" == "all" (
		if not "%2" == "authnet" (
			exit /b
		)
	)

	if "%1" == "clean" (
		@rem AuthNet PIDL 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\[AuthNetLib]\ProudNetPidl\ProudNetPidl.vcxproj" clean Release_static_CRT x64
		rd /s /q .\[AuthNetLib]\ProudNetPidl\bin\
		rd /s /q .\[AuthNetLib]\ProudNetPidl\obj\

		@rem AuthNet 클라이언트 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\[AuthNetLib]\ProudNetClient\ProudNetClient.vcxproj" clean Release_static_CRT x64
		rd /s /q .\[AuthNetLib]\ProudNetClient\bin\
		rd /s /q .\[AuthNetLib]\ProudNetClient\obj\

		@rem AuthNet 서버 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\[AuthNetLib]\ProudNetServer\ProudNetServer.vcxproj" clean Release_static_CRT x64
		rd /s /q .\[AuthNetLib]\ProudNetServer\bin\
		rd /s /q .\[AuthNetLib]\ProudNetServer\obj\
	) else if "%1" == "build" (
		@rem AuthNet PIDL 프로젝트를 빌드합니다.
		call :compile_command ".\[AuthNetLib]\ProudNetPidl\ProudNetPidl.vcxproj" build Release_static_CRT x64

		@rem AuthNet 클라이언트 프로젝트를 빌드합니다.
		call :compile_command ".\[AuthNetLib]\ProudNetClient\ProudNetClient.vcxproj" build Release_static_CRT x64

		@rem AuthNet 서버 프로젝트를 빌드합니다.
		call :compile_command ".\[AuthNetLib]\ProudNetServer\ProudNetServer.vcxproj" build Release_static_CRT x64
	)
exit /b

:process_library_pnlic_sdk
	if not "%2" == "all" (
		if not "%2" == "pnlic_sdk" (
			exit /b
		)
	)

	if "%1" == "clean" (
		@rem PNLicenseSdk 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\PNLicenseSdk\PNLicenseSdk.vcxproj" clean Release_static_CRT x64
		rd /s /q .\PNLicenseSdk\bin\
		rd /s /q .\PNLicenseSdk\obj\
	) else if "%1" == "build" (
		@rem PNLicenseSdk 프로젝트를 빌드합니다. (CodeVirtualizer 도구의 라이브러리 활용)
		call :compile_command ".\PNLicenseSdk\PNLicenseSdk.vcxproj" build Release_static_CRT x64
	)
exit /b

:process_library_pnlic
	if not "%2" == "all" (
		if not "%2" == "pnlic" (
			exit /b
		)
	)

	if "%1" == "clean" (
		@rem PNLicense 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\PNLicense\PNLicense.vcxproj" clean Release_static_CRT x64
		rd /s /q .\PNLicense\bin\
		rd /s /q .\PNLicense\obj\
	) else if "%1" == "build" (
		@rem PNLicense 프로젝트를 빌드합니다.
		call :compile_command ".\PNLicense\PNLicense.vcxproj" build Release_static_CRT x64
	)
exit /b

:process_library_pnlic_auth_lib
	if not "%2" == "all" (
		if not "%2" == "pnlic_auth_lib" (
			exit /b
		)
	)

	if "%1" == "clean" (
		@rem PNLicAuthCommon 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\[PNLicAuthLib]\PNLicAuthCommon\PNLicAuthCommon.vcxproj" clean Release_static_CRT x64
		rd /s /q .\[PNLicAuthLib]\PNLicAuthCommon\bin\
		rd /s /q .\[PNLicAuthLib]\PNLicAuthCommon\obj\

		@rem PNLicAuthClient 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\[PNLicAuthLib]\PNLicAuthClient\PNLicAuthClient.vcxproj" clean Release_static_CRT x64
		rd /s /q .\[PNLicAuthLib]\PNLicAuthClient\bin\
		rd /s /q .\[PNLicAuthLib]\PNLicAuthClient\obj\

		@rem PNLicAuthServer 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\[PNLicAuthLib]\PNLicAuthServer\PNLicAuthServer.vcxproj" clean Release_static_CRT x64
		rd /s /q .\[PNLicAuthLib]\PNLicAuthServer\bin\
		rd /s /q .\[PNLicAuthLib]\PNLicAuthServer\obj\
	) else if "%1" == "build" (
		@rem PNLicAuthCommon 프로젝트를 빌드합니다.
		call :compile_command ".\[PNLicAuthLib]\PNLicAuthCommon\PNLicAuthCommon.vcxproj" build Release_static_CRT x64

		@rem PNLicAuthClient 프로젝트를 빌드합니다.
		call :compile_command ".\[PNLicAuthLib]\PNLicAuthClient\PNLicAuthClient.vcxproj" build Release_static_CRT x64

		@rem PNLicAuthServer 프로젝트를 빌드합니다.
		call :compile_command ".\[PNLicAuthLib]\PNLicAuthServer\PNLicAuthServer.vcxproj" build Release_static_CRT x64
	)
exit /b

:process_library_watermark
	if not "%2" == "all" (
		if not "%2" == "watermark" (
			exit /b
		)
	)

	if "%1" == "clean" (
		@rem WatermarkLib 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\[Watermark]\WatermarkLib\WatermarkLib.vcxproj" clean Release_static_CRT x64
		rd /s /q .\[Watermark]\WatermarkLib\bin\
		rd /s /q .\[Watermark]\WatermarkLib\obj\

		@rem WatermarkDll 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\[Watermark]\WatermarkDll\WatermarkDll.vcxproj" clean Release_static_CRT x64
		rd /s /q .\[Watermark]\WatermarkDll\bin\
		rd /s /q .\[Watermark]\WatermarkDll\obj\
	) else if "%1" == "build" (
		@rem WatermarkLib 프로젝트를 빌드합니다.
		call :compile_command ".\[Watermark]\WatermarkLib\WatermarkLib.vcxproj" build Release_static_CRT x64

		@rem WatermarkDll 프로젝트를 빌드합니다.
		call :compile_command ".\[Watermark]\WatermarkDll\WatermarkDll.vcxproj" build Release_static_CRT x64
	)
exit /b

:process_library_pnlic_warn
	if not "%2" == "all" (
		if not "%2" == "pnlic_warn" (
			exit /b
		)
	)

	if "%1" == "clean" (
		@rem PNLicenseWarning 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\PNLicenseWarning\PNLicenseWarning.vcxproj" clean Release_static_CRT x64
		rd /s /q .\PNLicenseWarning\bin\
		rd /s /q .\PNLicenseWarning\obj\
	) else if "%1" == "build" (
		@rem PNLicenseWarning 프로젝트를 빌드합니다.
		call :compile_command ".\PNLicenseWarning\PNLicenseWarning.vcxproj" build Release_static_CRT x64
	)
exit /b

:process_library_pnlic_pnlic_hidden
	if not "%2" == "all" (
		if not "%2" == "pnlic_hidden" (
			exit /b
		)
	)

	if "%1" == "clean" (
		@rem PNLicenseHidden 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\PNLicenseHidden\PNLicenseHidden.vcxproj" clean Release_static_CRT x64
		rd /s /q .\PNLicenseHidden\bin\
		rd /s /q .\PNLicenseHidden\obj\
	) else if "%1" == "build" (
		@rem PNLicenseHidden 프로젝트를 빌드합니다.
		call :compile_command ".\PNLicenseHidden\PNLicenseHidden.vcxproj" build Release_static_CRT x64
	)
exit /b

:process_library_pnlic_auth_exe
	if not "%2" == "all" (
		if not "%2" == "pnlic_auth_exe" (
			exit /b
		)
	)

	if "%1" == "clean" (
		@rem PNLicenseAuth 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\PNLicenseAuth\PNLicenseAuth.vcxproj" clean Release_static_CRT x64
		rd /s /q .\PNLicenseAuth\bin\
		rd /s /q .\PNLicenseAuth\obj\

		@rem PNLicenseAuthGui 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\PNLicenseAuthGui\PNLicenseAuthGui.csproj" clean Release AnyCPU
		rd /s /q .\PNLicenseAuthGui\bin\
		rd /s /q .\PNLicenseAuthGui\obj\
	) else if "%1" == "build" (
		@rem PNLicenseAuth 프로젝트를 빌드합니다.
		call :compile_command ".\PNLicenseAuth\PNLicenseAuth.vcxproj" build Release_static_CRT x64

		@rem PNLicenseAuthGui 프로젝트를 빌드합니다.
		call :compile_command ".\PNLicenseAuthGui\PNLicenseAuthGui.csproj" build Release AnyCPU
	)
exit /b

:compile_command
	set vs_version=17

	echo ^>^>^>^> CommandLine : "%PN_BUILD_PATH%" %1 "/t:%2" "/p:Configuration=%3;Platform=%4;VisualStudioVersion=%vs_version%;BuildProjectReferences=false" /m
	echo ^>^>^>^>
	echo ^>^>^>^> --------------------------------------------------
	"%PN_BUILD_PATH%" %1 "/t:%2" "/p:Configuration=%3;Platform=%4;VisualStudioVersion=%vs_version%;BuildProjectReferences=false" /m
	echo ^>^>^>^> --------------------------------------------------
exit /b

:check_comandline_params
	@rem 도움말을 출력해야 할 지를 검사합니다.
	if not "%1" == "clean" (
		if not "%1" == "build" (
			exit /b 1
		)
	)

	for %%p in (%projects%) do (
		if "%2" == "%%p" (
			exit /b 0
		)
	)
exit /b 1

:download_nuget
	@rem NuGet 패키지 매니저를 다운로드 받아 설치합니다.
	@rem 현재(2022.11.01) 기준으로 v6.3.1이 최신 버전입니다.
	if not exist "..\utils\" (
		mkdir ..\utils\
	)

	if not exist "..\utils\nuget.exe" (
		powershell "(new-Object System.Net.WebClient).DownloadFile('https://dist.nuget.org/win-x86-commandline/latest/nuget.exe', '..\utils\nuget.exe')"
	)
exit /b

:download_virtualizer
	@rem CodeVirtualizer 코드 난독화 도구를 다운로드 받아 설치합니다.
	set util_path=..\utils\CodeVirtualizer

	if not exist "%util_path%" (
		mkdir %util_path%
	)

	if not exist "%util_path%\Virtualizer.exe" (
		powershell "(new-Object System.Net.WebClient).DownloadFile('https://proudnet-utils.s3.us-east-1.amazonaws.com/CodeVirtualizer-v2.2.2.zip', '%util_path%\CodeVirtualizer-v2.2.2.zip')"
		powershell "(Expand-Archive -Path %util_path%\CodeVirtualizer-v2.2.2.zip -DestinationPath %util_path%)"
		del /f /q "%util_path%\CodeVirtualizer-v2.2.2.zip"
	)
exit /b

call :main %1 %2
