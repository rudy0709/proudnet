@echo off

:main
	if "%1" == "clean" (
		call :clean_library
	) else if "%1" == "build" (
		call :build_library
	) else (
		@rem 사용법을 출력합니다.
		echo Usage:
		echo     pn_build.bat clean
		echo     pn_build.bat build
	)
exit /b 0

:clean_library
	@rem PNLicenseSDK 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
	call :compile_command ".\PNLicenseSDK.vcxproj" "/t:clean" "/p:Configuration=Release_static_CRT;Platform=x64;VisualStudioVersion=17;BuildProjectReferences=false" /m

	rd /s /q .\bin\
	rd /s /q .\obj\
exit /b 0

:build_library
	call :download_virtualizer

	@rem PNLicenseSDK 클라이언트 프로젝트를 빌드합니다.
	call :compile_command ".\PNLicenseSDK.vcxproj" "/t:build" "/p:Configuration=Release_static_CRT;Platform=x64;VisualStudioVersion=17;BuildProjectReferences=false" /m
exit /b 0

:compile_command
	echo ## CommandLine ## : "%PN_BUILD_PATH%" %1 %2 %3 %4
	echo --------------------------------------------------
	"%PN_BUILD_PATH%" %1 %2 %3 %4
	echo --------------------------------------------------
exit /b 0

:download_virtualizer
	if not exist "..\..\utils\CodeVirtualizer\" (
		mkdir ..\..\utils\CodeVirtualizer\
	)

	if not exist "..\..\utils\CodeVirtualizer\CodeVirtualizer-v2.2.2.zip" (
		powershell "(new-Object System.Net.WebClient).DownloadFile('https://proudnet-utils.s3.us-east-1.amazonaws.com/CodeVirtualizer-v2.2.2.zip', '..\..\utils\CodeVirtualizer\CodeVirtualizer-v2.2.2.zip')"
	)

	if not exist "..\..\utils\CodeVirtualizer\Virtualizer.exe" (
		powershell "(Expand-Archive -Path ..\..\utils\CodeVirtualizer\CodeVirtualizer-v2.2.2.zip -DestinationPath ..\..\utils\CodeVirtualizer\)"
	)
exit /b 0

call main %1
