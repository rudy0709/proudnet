@echo off

:main
	if "%1" == "clean" (
		call :clean_library
	) else if "%1" == "build" (
		call :build_library
	) else (
		@rem 사용법을 출력합니다.
		echo Usage:
		echo     pn_build.bat ^<clean ^| build^>
	)
exit /b 0

:clean_library
	@rem PIDL 컴파일러 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
	set pn_build_exe=%PN_BUILD_PATH%
	call :compile_command "%pn_build_exe%" ".\PIDL.csproj" "/t:clean" "/p:Configuration=Release;Platform=AnyCPU;VisualStudioVersion=17;BuildProjectReferences=false" /m

	rd /s /q .\packages\
	rd /s /q .\bin\
	rd /s /q .\obj\
exit /b 0

:build_library
	set pn_build_exe=%PN_BUILD_PATH%

	@rem NuGet 패키지 매니저를 다운로드 받아 설치합니다.
	@rem 현재(2022.11.01) 기준으로 v6.3.1이 최신 버전입니다.
	call :download_nuget

	@rem PIDL 컴파일러 프로젝트에서 사용할 NuGet 패키지를 다운로드 받습니다.
	call :download_nuget_packages

	@rem PIDL 컴파일러 프로젝트를 빌드합니다.
	call :compile_command "%pn_build_exe%" ".\PIDL.csproj" "/t:build" "/p:Configuration=Release;Platform=AnyCPU;VisualStudioVersion=17;BuildProjectReferences=false" /m
exit /b 0

:download_nuget
	if not exist "..\..\utils\" (
		mkdir ..\..\utils\
	)

	if not exist "..\..\utils\nuget.exe" (
		powershell "(new-Object System.Net.WebClient).DownloadFile('https://dist.nuget.org/win-x86-commandline/latest/nuget.exe', '..\..\utils\nuget.exe')"
	)
exit /b 0

:download_nuget_packages
	set packages_flag=0
 	if not exist ".\packages\Antlr4.4.6.6\" (
		set packages_flag=1
	)
	if not exist ".\packages\Antlr4.CodeGenerator.4.6.6\" (
		set packages_flag=1
	)
	if not exist ".\packages\Antlr4.Runtime.4.6.6\" (
		set packages_flag=1
	)
	if not exist ".\packages\YamlDotNet.12.0.2\" (
		set packages_flag=1
	)

	if %packages_flag% == 1 (
		..\..\utils\nuget.exe restore -PackagesDirectory .\packages
	)
exit /b 0

:compile_command
	echo ## CommandLine ## : %1 %2 %3 %4 %5
	%1 %2 %3 %4 %5
exit /b 0

call main %1
