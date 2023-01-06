@echo off
setlocal enabledelayedexpansion

set project_list=all r13b r17 r20
set build_target=
set build_project=

:main
	pushd "%~dp0"

	@rem 사용법을 출력해야 할 지를 검사합니다.
	call :check_comandline_params %1 %2
	if "%errorlevel%" == "1" (
		@rem 사용법을 출력합니다.
		echo Usage:
		echo     core_aos_build.bat clean ^<project^>
		echo     core_aos_build.bat build ^<project^>
		echo Options:
		echo     project : all ^| r13b ^| r17 ^| r20
		exit /b
	)

	@rem 프로젝트 별로 빌드함수를 호출합니다.
	@rem   (1) 환경변수 체크
	call :check_environment_variable

	@rem   (2) 안드로이드 NDK 몇몇 버전을 설치합니다.
	call :download_ndk_packages

	@rem   (3) r13b 버전의 NDK로 프로젝트 빌드
	call :build_project_by_ndk "r13b"

	@rem   (4) r17 버전의 NDK로 프로젝트 빌드
	call :build_project_by_ndk "r17"

	@rem   (5) r20 버전의 NDK로 프로젝트 빌드
	call :build_project_by_ndk "r20"

	popd
exit /b

@rem 공통 로직...
:check_comandline_params
	@rem 첫번째 인자값의 유효성을 검사합니다.
	set target_list=clean build
	for %%i in (%target_list%) do (
		if "%~1" == "%%i" (
			set build_target=%%i
		)
	)

	if "%build_target%" == "" (
		echo ^>^>^>^> Error : 1st^(Build Target^) argument is invalid.
		echo ^>^>^>^>
		exit /b 1
	)

	@rem 두번째 인자값의 유효성을 검사합니다.
	for %%i in (%project_list%) do (
		if "%~2" == "%%i" (
			set build_project=%%i
		)
	)

	if "%build_project%" == "" (
		echo ^>^>^>^> Error : 2nd^(Build Project^) argument is invalid.
		echo ^>^>^>^>
		exit /b 1
	)
exit /b 0

:check_environment_variable
	@rem 환경변수가 등록되어 있는 지를 체크합니다.
	if "%ANDROID_SDK_HOME%" == "" (
		@rem 예시 : C:\Users\supertree\AppData\Local\Android\Sdk
		echo ^>^>^>^> Error : Register the ANDROID_SDK_HOME environment variable before executing the batch file.
		echo ^>^>^>^>
		exit /b
	)

	if "%JAVA_HOME%" == "" (
		@rem 예시 : C:\Program Files\java\jdk-15.0.2
		echo ^>^>^>^> Error : Register the JAVA_HOME environment variable before executing the batch file.
		echo ^>^>^>^>
		exit /b
	)

	if "%CLASSPATH%" == "" (
		@rem 예시 : %JAVA_HOME%\lib
		echo ^>^>^>^> Error : Register the CLASSPATH environment variable before executing the batch file.
		echo ^>^>^>^>
		exit /b
	)

	for /f "tokens=3 delims= " %%v in ('java -version 2^>^&1 ^| find "version"') do (
		set java_version=%%v
	)

	echo ^>^>^>^> Java version : %java_version%
	echo ^>^>^>^> Env-Var = ^{
	echo ^>^>^>^>   "ANDROID_SDK_HOME" : "%ANDROID_SDK_HOME%",
	echo ^>^>^>^>          "CLASSPATH" : "%CLASSPATH%",
	echo ^>^>^>^>          "JAVA_HOME" : "%JAVA_HOME%"
	echo ^>^>^>^> ^}
	echo ^>^>^>^>
exit /b

:download_ndk_packages
	@rem 프라우드넷에서 지원한 NDK 버전을 안드로이드 개발자 사이트에서 다운로드 받을 수 없어 AWS S3에서 따로 관리합니다.
	set s3_url=https://proudnet-utils.s3.us-east-1.amazonaws.com

	for %%i in (%project_list%) do (
		if not "%%i" == "all" (
			set ndk_folder_path=..\third_party\AndroidNDK\android-ndk-%%i
			if not exist "!ndk_folder_path!" (
				mkdir "!ndk_folder_path!"
			)

			if not exist "!ndk_folder_path!\ndk-build.cmd" (
				set zip_filename=android-ndk-%%i.zip
				powershell "(new-Object System.Net.WebClient).DownloadFile('!s3_url!/!zip_filename!', '!ndk_folder_path!\!zip_filename!')"
				powershell "(Expand-Archive -Path '!ndk_folder_path!\!zip_filename!' -DestinationPath '!ndk_folder_path!')"
				del /f /q "!ndk_folder_path!\!zip_filename!"
			)
		)
	)
exit /b

:build_project_by_ndk <ndk_version>
	set ndk_version=%~1

	if not "%build_project%" == "all" (
		if not "%build_project%" == "%ndk_version%" (
			exit /b
		)
	)

	@rem 특정 버전의 NDK로 프로젝트 빌드/클린합니다.
	if "%build_target%" == "clean" (
		call :clean_command "%ndk_version%"
	) else if "%build_target%" == "build" (
		call :build_command "%ndk_version%"
	)
exit /b


:clean_command <ndk_version>
	set ndk_version=%~1

	call .\gradlew.bat clean

	rd /s /q .\app\.cxx\
	rd /s /q .\build\
	rd /s /q .\libs\ndk\%ndk_version%
	rd /s /q .\gen-libs\build\
	del /f /q .\gen-libs\build.gradle
	del /f /q .\local.properties

	@rem 빌드 산출물이 생기는 임시 폴더를 삭제합니다.
	set rd_flag=true
	for %%i in (%project_list%) do (
		if exist ".\libs\ndk\%%i\" (
			set rd_flag=false
		)
	)

	if "%rd_flag%" == "true" (
		rd /s /q .\libs\
	)
exit /b

:build_command <ndk_version>
	set ndk_version=%~1

	@rem 1) NDK 버전에 맞게 local.properties 파일을 생성합니다.
	echo ^>^>^>^> Create a new local.properties file...

	set android_ndk_base_path=%~dp0..\third_party\AndroidNDK
	set android_ndk_path=%android_ndk_path%\android-ndk-%ndk_version%
	set android_ndk_path=%android_ndk_base_path:\=\\%
	@rem set android_ndk_path=%android_ndk_path::=\:%

	set android_sdk_home=%ANDROID_SDK_HOME%
	if "%android_sdk_home:~-1%" == "\" (
		set android_sdk_home=%android_sdk_home:~0,-1%
	)
	set android_sdk_home=%android_sdk_home:\=\\%
	@rem set android_sdk_home=%android_sdk_home::=\:%

	if exist ".\local.properties" (
		del /f /q ".\local.properties"
	)
	echo ndk.dir=%android_ndk_path%\\android-ndk-%ndk_version%>  local.properties
	echo sdk.dir=%android_sdk_home%>> local.properties

	@rem 2) NDK 버전에 맞게 gen-libs\build.gradle 파일을 생성합니다.
	echo ^>^>^>^> Create a new gen-libs\build.gradle file...

	set filename=.\gen-libs\build.gradle
	if exist "%filename%" (
		del /f /q %filename%
	)

	for /f "delims=/" %%i in (%filename%.template.%ndk_version%) do (
		set line_str=%%i
		set echo_flag=true

		@rem if not "!line_str:ANDROID_NDK_PATH=!" == "!line_str!" (
		@rem 	echo     ndkPath "!android_ndk_path!\\android-ndk-!ndk_version!">> !filename!
		@rem 	set echo_flag=false
		@rem )

		if not "!line_str:CMAKE_PATH=!" == "!line_str!" (
			echo             path "../CMakeLists.txt">> !filename!
			set echo_flag=false
		)

		if not "!line_str:CMAKE_STAGING_PATH=!" == "!line_str!" (
			echo             buildStagingDirectory "../build/libs/ndk/!ndk_version!">> !filename!
			set echo_flag=false
		)

		if "!echo_flag!" == "true" (
			echo !line_str!>> !filename!
		)
	)

	@rem 3) 빌드를 시작합니다.
	echo ^>^>^>^> Build with the NDK %ndk_version% version...
	@rem set GRADLE_EXIT_CONSOLE=1
	call .\gradlew.bat -p gen-libs\ --parallel build
exit /b

call :main
