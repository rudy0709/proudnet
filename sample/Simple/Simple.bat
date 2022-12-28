@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

call :compile_command ".\Common" "SimpleCommon.vcxproj"
call :compile_command ".\Client" "SimpleClient.vcxproj"
call :compile_command ".\Server" "SimpleServer.vcxproj"

set project_name=
set msbuild_config=Release
set msbuild_platform=x64
set vs_version=17

@rem SimpleCommon 프로젝트를 빌드합니다
set project_name=.\Common\SimpleCommon.vcxproj

echo ^>^>^>^> CommandLine : "%PN_BUILD_PATH%" "%project_name%" "/t:clean" "/p:Configuration=%msbuild_config%;Platform=%msbuild_platform%;VisualStudioVersion=%vs_version%;BuildProjectReferences=false" /m
echo ^>^>^>^> CommandLine : "%PN_BUILD_PATH%" "%project_name%" "/t:build" "/p:Configuration=%msbuild_config%;Platform=%msbuild_platform%;VisualStudioVersion=%vs_version%;BuildProjectReferences=false" /m
echo ^>^>^>^> --------------------------------------------------
"%PN_BUILD_PATH%" "%project_name%" "/t:clean" "/p:Configuration=%msbuild_config%;Platform=%msbuild_platform%;VisualStudioVersion=%vs_version%;BuildProjectReferences=false" /m
"%PN_BUILD_PATH%" "%project_name%" "/t:build" "/p:Configuration=%msbuild_config%;Platform=%msbuild_platform%;VisualStudioVersion=%vs_version%;BuildProjectReferences=false" /m
echo ^>^>^>^> --------------------------------------------------

@rem SimpleClient 프로젝트를 빌드합니다
set project_name=.\Client\SimpleClient.vcxproj

echo ^>^>^>^> CommandLine : "%PN_BUILD_PATH%" "%project_name%" "/t:clean" "/p:Configuration=%msbuild_config%;Platform=%msbuild_platform%;VisualStudioVersion=%vs_version%;BuildProjectReferences=false" /m
echo ^>^>^>^> CommandLine : "%PN_BUILD_PATH%" "%project_name%" "/t:build" "/p:Configuration=%msbuild_config%;Platform=%msbuild_platform%;VisualStudioVersion=%vs_version%;BuildProjectReferences=false" /m
echo ^>^>^>^> --------------------------------------------------
"%PN_BUILD_PATH%" "%project_name%" "/t:clean" "/p:Configuration=%msbuild_config%;Platform=%msbuild_platform%;VisualStudioVersion=%vs_version%;BuildProjectReferences=false" /m
"%PN_BUILD_PATH%" "%project_name%" "/t:build" "/p:Configuration=%msbuild_config%;Platform=%msbuild_platform%;VisualStudioVersion=%vs_version%;BuildProjectReferences=false" /m
echo ^>^>^>^> --------------------------------------------------

@rem SimpleServer 프로젝트를 빌드합니다
set project_name=.\Server\SimpleServer.vcxproj

echo ^>^>^>^> CommandLine : "%PN_BUILD_PATH%" "%project_name%" "/t:clean" "/p:Configuration=%msbuild_config%;Platform=%msbuild_platform%;VisualStudioVersion=%vs_version%;BuildProjectReferences=false" /m
echo ^>^>^>^> CommandLine : "%PN_BUILD_PATH%" "%project_name%" "/t:build" "/p:Configuration=%msbuild_config%;Platform=%msbuild_platform%;VisualStudioVersion=%vs_version%;BuildProjectReferences=false" /m
echo ^>^>^>^> --------------------------------------------------
"%PN_BUILD_PATH%" "%project_name%" "/t:clean" "/p:Configuration=%msbuild_config%;Platform=%msbuild_platform%;VisualStudioVersion=%vs_version%;BuildProjectReferences=false" /m
"%PN_BUILD_PATH%" "%project_name%" "/t:build" "/p:Configuration=%msbuild_config%;Platform=%msbuild_platform%;VisualStudioVersion=%vs_version%;BuildProjectReferences=false" /m
echo ^>^>^>^> --------------------------------------------------
