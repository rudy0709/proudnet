@echo off

set projects=all pidl client server

:main
	@rem 사용법을 출력해야 할 지를 검사합니다.
	call :check_comandline_params %1 %2
	if "%errorlevel%" == "1" (
		@rem 사용법을 출력합니다.
		echo Usage:
		echo     core_build.bat clean ^<all ^| pidl ^| client ^| server^>
		echo     core_build.bat build ^<all ^| pidl ^| client ^| server^>
		exit /b
	)

	@rem 프로젝트 별로 빌드함수를 호출합니다.
	@rem   (1) 환경변수 체크
	call :check_environment_variable

	@rem   (2) ProudNetPidl 프로젝트 빌드
	call :build_library_pidl %1 %2

	@rem   (3) ProudNetClient 프로젝트 빌드
	call :build_library_client %1 %2

	@rem   (3) ProudNetServer 프로젝트 빌드
	call :build_library_server %1 %2
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

:build_library_pidl
	if not "%2" == "all" (
		if not "%2" == "pidl" (
			exit /b
		)
	)

	if "%1" == "clean" (
		@rem ProudNetPidl 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\ProudNetPidl\ProudNetPidl.vcxproj" clean Release_static_CRT x64
		rd /s /q .\ProudNetPidl\bin\
		rd /s /q .\ProudNetPidl\obj\
	) else if "%1" == "build" (
		@rem ProudNetPidl 프로젝트를 빌드합니다.
		call :compile_command ".\ProudNetPidl\ProudNetPidl.vcxproj" build Release_static_CRT x64
	)
exit /b

:build_library_client %1 %2
	if not "%2" == "all" (
		if not "%2" == "client" (
			exit /b
		)
	)

	if "%1" == "clean" (
		@rem ProudNetClientLib 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\ProudNetClientLib\ProudNetClientLib.vcxproj" clean Release_static_CRT x64
		rd /s /q .\ProudNetClientLib\bin\
		rd /s /q .\ProudNetClientLib\obj\

		@rem ProudNetClientDll 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\ProudNetClientDll\ProudNetClientDll.vcxproj" clean Release_static_CRT x64
		rd /s /q .\ProudNetClientDll\bin\
		rd /s /q .\ProudNetClientDll\obj\
	) else if "%1" == "build" (
		@rem ProudNetClientLib 프로젝트를 빌드합니다.
		call :compile_command ".\ProudNetClientLib\ProudNetClientLib.vcxproj" build Release_static_CRT x64

		@rem ProudNetClientDll 프로젝트를 빌드합니다.
		call :compile_command ".\ProudNetClientDll\ProudNetClientDll.vcxproj" build Release_static_CRT x64
	)
exit /b

:build_library_server %1 %2
	if not "%2" == "all" (
		if not "%2" == "server" (
			exit /b
		)
	)

	if "%1" == "clean" (
		@rem ProudNetServerLib 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\ProudNetServerLib\ProudNetServerLib.vcxproj" clean Release_static_CRT x64
		rd /s /q .\ProudNetServerLib\bin\
		rd /s /q .\ProudNetServerLib\obj\

		@rem ProudNetServerDll 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\ProudNetServerDll\ProudNetServerDll.vcxproj" clean Release_static_CRT x64
		rd /s /q .\ProudNetServerDll\bin\
		rd /s /q .\ProudNetServerDll\obj\
	) else if "%1" == "build" (
		@rem ProudNetServerLib 프로젝트를 빌드합니다.
		call :compile_command ".\ProudNetServerLib\ProudNetServerLib.vcxproj" build Release_static_CRT x64

		@rem ProudNetServerDll 프로젝트를 빌드합니다.
		call :compile_command ".\ProudNetServerDll\ProudNetServerDll.vcxproj" build Release_static_CRT x64
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
