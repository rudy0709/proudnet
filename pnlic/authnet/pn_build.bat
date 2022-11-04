@echo off

:main
	if "%1" == "clean" (
		call :clean_library
	) else if "%1" == "build" (
		call :build_library %2
	) else (
		@rem 사용법을 출력합니다.
		echo Usage:
		echo     pn_build.bat clean
		echo     pn_build.bat build ^<all ^| client ^| server^>
	)
exit /b 0

:clean_library
	set pn_build_exe=%PN_BUILD_PATH%

	@rem AuthNet 클라이언트 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
	echo ## CommandLine ## : "%pn_build_exe%" ".\ProudNetClient\ProudNetClient.vcxproj" "/t:clean" "/p:Configuration=Release_static_CRT;Platform=x64;VisualStudioVersion=17;BuildProjectReferences=false" /m
	"%pn_build_exe%" ".\ProudNetClient\ProudNetClient.vcxproj" "/t:clean" "/p:Configuration=Release_static_CRT;Platform=x64;VisualStudioVersion=17;BuildProjectReferences=false" /m

	rd /s /q .\ProudNetClient\bin\
	rd /s /q .\ProudNetClient\obj\

	@rem AuthNet 서버 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
	echo ## CommandLine ## : "%pn_build_exe%" ".\ProudNetServer\ProudNetServer.vcxproj" "/t:clean" "/p:Configuration=Release_static_CRT;Platform=x64;VisualStudioVersion=17;BuildProjectReferences=false" /m
	"%pn_build_exe%" ".\ProudNetServer\ProudNetServer.vcxproj" "/t:clean" "/p:Configuration=Release_static_CRT;Platform=x64;VisualStudioVersion=17;BuildProjectReferences=false" /m

	rd /s /q .\ProudNetServer\bin\
	rd /s /q .\ProudNetServer\obj\
exit /b 0

:build_library
	set client_flag=0
	set server_flag=0
	if "%1" == "all" (
		set client_flag=1
		set server_flag=1
	) else if "%1" == "client" (
		set client_flag=1
	) else if "%1" == "server" (
		set server_flag=1
	) else (
		@rem 사용법을 출력합니다.
		echo Usage:
		echo     pn_build.bat build ^<all ^| client ^| server^>
	)

	set pn_build_exe=%PN_BUILD_PATH%

	if %client_flag% == 1 (
		@rem AuthNet 클라이언트 프로젝트를 빌드합니다.
		echo ## CommandLine ## : "%pn_build_exe%" ".\ProudNetClient\ProudNetClient.vcxproj" "/t:build" "/p:Configuration=Release_static_CRT;Platform=x64;VisualStudioVersion=17;BuildProjectReferences=false" /m
		"%pn_build_exe%" ".\ProudNetClient\ProudNetClient.vcxproj" "/t:build" "/p:Configuration=Release_static_CRT;Platform=x64;VisualStudioVersion=17;BuildProjectReferences=false" /m
	)

	if %server_flag% == 1 (
		@rem AuthNet 서버 프로젝트를 빌드합니다.
		echo ## CommandLine ## : "%pn_build_exe%" ".\ProudNetServer\ProudNetServer.vcxproj" "/t:build" "/p:Configuration=Release_static_CRT;Platform=x64;VisualStudioVersion=17;BuildProjectReferences=false" /m
		"%pn_build_exe%" ".\ProudNetServer\ProudNetServer.vcxproj" "/t:build" "/p:Configuration=Release_static_CRT;Platform=x64;VisualStudioVersion=17;BuildProjectReferences=false" /m
	)
exit /b 0

call main %1 %2
