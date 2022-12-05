@echo off

set projects=all code-virtualizer image-gen pidl-compiler

:main
	@rem 사용법을 출력해야 할 지를 검사합니다.
	call :check_comandline_params %1 %2
	if "%errorlevel%" == "1" (
		@rem 파일 경로에서 파일명만 뽑아냅니다.
		set filename=""
		for %%i in ("%0") do (
			set filename=%%~ni%%~xi
		)

		@rem 사용법을 출력합니다.
		echo Usage:
		echo     %filename% clean ^<all ^| code-virtualizer ^| image-gen ^| pidl-compiler^>
		echo     %filename% build ^<all ^| code-virtualizer ^| image-gen ^| pidl-compiler^>
		exit /b
	)

	@rem 프로젝트 별로 빌드함수를 호출합니다.
	@rem   (1) 환경변수 체크
	call :check_environment_variable

	@rem   (2) CodeVirtualizer 프로젝트 빌드
	call :build_project_code_virtualizer %1 %2

	@rem   (3) ImageGen 프로젝트 빌드
	call :build_project_image_gen %1 %2

	@rem   (4) Pidl 컴파일러 프로젝트 빌드
	call :build_project_pidl_compiler %1 %2
exit /b

:check_environment_variable
	@rem 환경변수가 등록되어 있는 지를 체크합니다.
	if "%PN_BUILD_PATH%" == "" (
		@rem 예시 : C:\Program Files\Microsoft Visual Studio\2022\Professional\Msbuild\Current\Bin\MSBuild.exe
		echo ^>^>^>^> Error : Register the PN_BUILD_PATH environment variable before executing the batch file.
		exit /b
	)

	if "%PN_OBFUSCATION_TOOL_PATH%" == "" (
		@rem 예시 : C:\Program Files (x86)\Gapotchenko\Eazfuscator.NET\Eazfuscator.NET.exe
		echo ^>^>^>^> Error : Register the PN_OBFUSCATION_TOOL_PATH environment variable before executing the batch file.
		exit /b
	)

	if "%PN_SIGN_TOOL_PATH%" == "" (
		@rem 예시 : C:\Program Files (x86)\Microsoft SDKs\ClickOnce\SignTool\signtool.exe
		echo ^>^>^>^> Error : Register the PN_SIGN_TOOL_PATH environment variable before executing the batch file.
		exit /b
	)

	echo ^>^>^>^> Environment-Variable^(PN_BUILD_PATH^) = "%PN_BUILD_PATH%"
	echo ^>^>^>^> Environment-Variable^(PN_OBFUSCATION_TOOL_PATH^) = "%PN_OBFUSCATION_TOOL_PATH%"
	echo ^>^>^>^> Environment-Variable^(PN_SIGN_TOOL_PATH^) = "%PN_SIGN_TOOL_PATH%"
	echo ^>^>^>^>
exit /b

:build_project_code_virtualizer
	if not "%2" == "all" (
		if not "%2" == "code-virtualizer" (
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

:build_project_image_gen
	if not "%2" == "all" (
		if not "%2" == "image-gen" (
			exit /b
		)
	)

	if "%1" == "clean" (
		@rem ImageGen 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\ImageGen\ImageGen.vcxproj" clean Release_static_CRT x64
		rd /s /q .\ImageGen\bin\
		rd /s /q .\ImageGen\obj\
	) else if "%1" == "build" (
		@rem ImageGen 프로젝트를 빌드합니다.
		call :compile_command ".\ImageGen\ImageGen.vcxproj" build Release_static_CRT x64
	)
exit /b

:build_project_pidl_compiler
	if not "%2" == "all" (
		if not "%2" == "pidl-compiler" (
			exit /b
		)
	)

	if "%1" == "clean" (
		@rem PIDL 컴파일러 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		call :compile_command ".\Pidl\Pidl.csproj" clean Release AnyCPU
		rd /s /q .\Pidl\bin\
		rd /s /q .\Pidl\obj\
	) else if "%1" == "build" (
		@rem PIDL 컴파일러 프로젝트를 빌드합니다.
		call :compile_command ".\Pidl\Pidl.csproj" build Release AnyCPU
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
