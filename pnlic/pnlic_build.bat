@echo off

set projects=all tools authnet_lib lic_auth_lib pnlic_lib pnlic_warn pnlic_hidden pnlic_auth watermark

:main
	@rem 사용법을 출력해야 할 지를 검사합니다.
	call :check_comandline_params %1 %2
	if "%errorlevel%" == "1" (
		@rem 사용법을 출력합니다.
		echo Usage:
		echo     pnlic_build.bat clean ^<all ^| tools ^| authnet_lib ^| lic_auth_lib ^| pnlic_lib ^| pnlic_warn ^| pnlic_hidden ^| pnlic_auth ^| watermark^>
		echo     pnlic_build.bat build ^<all ^| tools ^| authnet_lib ^| lic_auth_lib ^| pnlic_lib ^| pnlic_warn ^| pnlic_hidden ^| pnlic_auth ^| watermark^>
		exit /b
	)

	@rem 프로젝트 별로 빌드함수를 호출합니다.
	@rem   (1) 환경변수 체크
	call :check_environment_variable

	@rem   (2) Tool 빌드
	call :process_library_tools %1 %2

	@rem   (3) 공용 Library 빌드 (PNLicAuthServer.exe도 포함)
	call :process_library_authnet_lib %1 %2
	call :process_library_lic_auth_lib %1 %2
	call :process_library_pnlic_lib %1 %2

	@rem   (4) PNLicense 관련 .exe 빌드
	call :process_library_pnlic_warn %1 %2
	call :process_library_pnlic_hidden %1 %2
	call :process_library_pnlic_auth %1 %2

	@rem   (5) Watermark 관련 .lib/.dll 빌드
	call :process_library_watermark %1 %2
exit /b

:check_environment_variable
	@rem 환경변수가 등록되어 있는 지를 체크합니다.
	if "%PN_BUILD_PATH%" == "" (
		@rem 예시 : C:\Program Files\Microsoft Visual Studio\2022\Professional\Msbuild\Current\Bin\MSBuild.exe
		echo ^>^>^>^> Error : Register the PN_BUILD_PATH environment variable before executing the batch file.
		exit /b
	)

	if "%PN_SIGN_TOOL_PATH%" == "" (
		@rem 예시 : C:\Program Files (x86)\Microsoft SDKs\ClickOnce\SignTool\signtool.exe
		echo ^>^>^>^> Error : Register the PN_SIGN_TOOL_PATH environment variable before executing the batch file.
		exit /b
	)

	echo ^>^>^>^> Environment-Variable^(PN_BUILD_PATH^) = "%PN_BUILD_PATH%"
	echo ^>^>^>^> Environment-Variable^(PN_SIGN_TOOL_PATH^) = "%PN_SIGN_TOOL_PATH%"
	echo ^>^>^>^>
exit /b

:process_library_tools
	if not "%2" == "all" (
		if not "%2" == "tools" (
			exit /b
		)
	)

	if "%1" == "clean" (
		@rem ImageGen 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\Tools\ImageGen\ImageGen.vcxproj" clean Release_static_CRT x64
		rd /s /q .\Tools\ImageGen\bin\
		rd /s /q .\Tools\ImageGen\obj\

		@rem PIDL 컴파일러 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\Tools\PIDL\PIDL.csproj" clean Release AnyCPU
		rd /s /q .\Tools\PIDL\bin\
		rd /s /q .\Tools\PIDL\obj\
	) else if "%1" == "build" (
		@rem ImageGen 프로젝트를 빌드합니다.
		call :compile_command ".\Tools\ImageGen\ImageGen.vcxproj" build Release_static_CRT x64

		@rem PIDL 컴파일러 프로젝트를 빌드합니다.
		call :compile_command ".\Tools\PIDL\PIDL.csproj" build Release AnyCPU
	)
exit /b

:process_library_authnet_lib
	if not "%2" == "all" (
		if not "%2" == "authnet_lib" (
			exit /b
		)
	)

	if "%1" == "clean" (
		@rem AuthNet PIDL 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\AuthNetLib\ProudNetPidl\ProudNetPidl.vcxproj" clean Release_static_CRT x64
		rd /s /q .\AuthNetLib\ProudNetPidl\bin\
		rd /s /q .\AuthNetLib\ProudNetPidl\obj\

		@rem AuthNet 클라이언트 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\AuthNetLib\ProudNetClient\ProudNetClient.vcxproj" clean Release_static_CRT x64
		rd /s /q .\AuthNetLib\ProudNetClient\bin\
		rd /s /q .\AuthNetLib\ProudNetClient\obj\

		@rem AuthNet 서버 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\AuthNetLib\ProudNetServer\ProudNetServer.vcxproj" clean Release_static_CRT x64
		rd /s /q .\AuthNetLib\ProudNetServer\bin\
		rd /s /q .\AuthNetLib\ProudNetServer\obj\
	) else if "%1" == "build" (
		@rem AuthNet PIDL 프로젝트를 빌드합니다.
		call :compile_command ".\AuthNetLib\ProudNetPidl\ProudNetPidl.vcxproj" build Release_static_CRT x64

		@rem AuthNet 클라이언트 프로젝트를 빌드합니다.
		call :compile_command ".\AuthNetLib\ProudNetClient\ProudNetClient.vcxproj" build Release_static_CRT x64

		@rem AuthNet 서버 프로젝트를 빌드합니다.
		call :compile_command ".\AuthNetLib\ProudNetServer\ProudNetServer.vcxproj" build Release_static_CRT x64
	)
exit /b

:process_library_lic_auth_lib
	if not "%2" == "all" (
		if not "%2" == "lic_auth_lib" (
			exit /b
		)
	)

	if "%1" == "clean" (
		@rem PNLicAuthCommon 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\LicAuthLib\PNLicAuthCommon\PNLicAuthCommon.vcxproj" clean Release_static_CRT x64
		rd /s /q .\LicAuthLib\PNLicAuthCommon\bin\
		rd /s /q .\LicAuthLib\PNLicAuthCommon\obj\

		@rem PNLicAuthClient 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\LicAuthLib\PNLicAuthClient\PNLicAuthClient.vcxproj" clean Release_static_CRT x64
		rd /s /q .\LicAuthLib\PNLicAuthClient\bin\
		rd /s /q .\LicAuthLib\PNLicAuthClient\obj\

		@rem PNLicAuthServer 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\LicAuthLib\PNLicAuthServer\PNLicAuthServer.vcxproj" clean Release_static_CRT x64
		rd /s /q .\LicAuthLib\PNLicAuthServer\bin\
		rd /s /q .\LicAuthLib\PNLicAuthServer\obj\
	) else if "%1" == "build" (
		@rem PNLicAuthCommon 프로젝트를 빌드합니다.
		call :compile_command ".\LicAuthLib\PNLicAuthCommon\PNLicAuthCommon.vcxproj" build Release_static_CRT x64

		@rem PNLicAuthClient 프로젝트를 빌드합니다.
		call :compile_command ".\LicAuthLib\PNLicAuthClient\PNLicAuthClient.vcxproj" build Release_static_CRT x64

		@rem PNLicAuthServer 프로젝트를 빌드합니다.
		call :compile_command ".\LicAuthLib\PNLicAuthServer\PNLicAuthServer.vcxproj" build Release_static_CRT x64
	)
exit /b

:process_library_pnlic_lib
	if not "%2" == "all" (
		if not "%2" == "pnlic_lib" (
			exit /b
		)
	)

	if "%1" == "clean" (
		@rem PNLicenseManager 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\PNLicenseManager\PNLicenseManager.vcxproj" clean Release_static_CRT x64
		rd /s /q .\PNLicenseManager\bin\
		rd /s /q .\PNLicenseManager\obj\

		@rem PNLicenseSdk 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\PNLicenseSdk\PNLicenseSdk.vcxproj" clean Release_static_CRT x64
		rd /s /q .\PNLicenseSdk\bin\
		rd /s /q .\PNLicenseSdk\obj\

		@rem PNLicense 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\PNLicense\PNLicense.vcxproj" clean Release_static_CRT x64
		rd /s /q .\PNLicense\bin\
		rd /s /q .\PNLicense\obj\
	) else if "%1" == "build" (
		@rem PNLicenseManager 프로젝트를 빌드합니다.
		call :compile_command ".\PNLicenseManager\PNLicenseManager.vcxproj" build Release_static_CRT x64

		@rem PNLicenseSdk 프로젝트를 빌드합니다.
		call :compile_command ".\PNLicenseSdk\PNLicenseSdk.vcxproj" build Release_static_CRT x64

		@rem PNLicense 프로젝트를 빌드합니다.
		call :compile_command ".\PNLicense\PNLicense.vcxproj" build Release_static_CRT x64
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
		del /f ..\PNLicenseManager\PNLicenseWarningImage.inl
	) else if "%1" == "build" (
		@rem PNLicenseWarning 프로젝트를 빌드합니다.
		call :compile_command ".\PNLicenseWarning\PNLicenseWarning.vcxproj" build Release_static_CRT x64
	)
exit /b

:process_library_pnlic_hidden
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

:process_library_pnlic_auth
	if not "%2" == "all" (
		if not "%2" == "pnlic_auth" (
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

:process_library_watermark
	if not "%2" == "all" (
		if not "%2" == "watermark" (
			exit /b
		)
	)

	if "%1" == "clean" (
		@rem WatermarkLib 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\Watermark\WatermarkLib\WatermarkLib.vcxproj" clean Release_static_CRT x64
		rd /s /q .\Watermark\WatermarkLib\bin\
		rd /s /q .\Watermark\WatermarkLib\obj\

		@rem WatermarkDll 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\Watermark\WatermarkDll\WatermarkDll.vcxproj" clean Release_static_CRT x64
		rd /s /q .\Watermark\WatermarkDll\bin\
		rd /s /q .\Watermark\WatermarkDll\obj\
	) else if "%1" == "build" (
		@rem WatermarkLib 프로젝트를 빌드합니다.
		call :compile_command ".\Watermark\WatermarkLib\WatermarkLib.vcxproj" build Release_static_CRT x64

		@rem WatermarkDll 프로젝트를 빌드합니다.
		call :compile_command ".\Watermark\WatermarkDll\WatermarkDll.vcxproj" build Release_static_CRT x64
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

call :main %1 %2
