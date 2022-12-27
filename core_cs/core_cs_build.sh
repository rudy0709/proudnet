#!/bin/bash

project_list=(all swig plugin dotnet sample)
msbuild_target=
msbuild_config_cpp=
msbuild_platform_cpp=
msbuild_config_cs=
msbuild_platform_cs=
msbuild_project=

func_main() {
	# 사용법을 출력해야 할 지를 검사합니다.
	func_check_comandline_params $1 $2 $3 $4
	if [ $? == 1 ]; then
		# 파일 경로에서 파일명만 뽑아냅니다.
		filename="$(basename $0)"

		# 사용법을 출력합니다.
		echo "Usage:"
		echo "    ${filename} clean <configuration> <platform> <project>"
		echo "    ${filename} build <configuration> <platform> <project>"
		echo "Options:"
		echo "    configuration : Debug | Release"
		echo "         platform : x64"
		echo "          project : all | swig | plugin | dotnet | sample"
		return
	fi

	# 프로젝트 별로 빌드함수를 호출합니다.
	#   (1) 환경변수 체크
	func_check_environment_variable

	#   (2) SWIG 프로젝트 빌드
	func_build_project_swig

	#   (3) 클라이언트/서버/WebGL 용의 Plugin 프로젝트 빌드
	func_build_project_plugin

	#   (4) 닷넷 용의 클라이언트/서버 프로젝트 빌드
	func_build_project_dotnet
	
	#   (5) 샘플 프로젝트 빌드
	func_build_sample_app
}

func_build_project_swig() {
	if [ ${msbuild_project} != "all" ]; then
		if [ ${msbuild_project} != "swig" ]; then
			return
		fi
	fi

	echo ">>>> 'SWIG 프로젝트' =================================================="
	echo ">>>> 'ProudNetClientPluginSwig', 'ProudNetServerPluginSwig' C# 프로젝트를 빌드하지 않습니다."
	echo ">>>> 'SWIG 프로젝트' =================================================="
	echo ""
}

func_build_project_plugin() {
	if [ ${msbuild_project} != "all" ]; then
		if [ ${msbuild_project} != "plugin" ]; then
			return
		fi
	fi

	# ProudNetClientPlugin, ProudNetServerPlugin, ProudNetClientPlugin-webgl 프로젝트를 빌드/클린합니다.
	func_compile_command ".\ProudNetClientPlugin"
	func_compile_command ".\ProudNetServerPlugin"
	#func_compile_command ".\ProudNetClientPlugin-webgl"	# 리눅스 환경에선 빌드하지 않음
}

func_build_project_dotnet() {
	if [ ${msbuild_project} != "all" ]; then
		if [ ${msbuild_project} != "dotnet" ]; then
			return
		fi
	fi

	echo ">>>> '.NET 프로젝트' =================================================="
	echo ">>>> 'ProudDotNetClient', 'ProudDotNetServer' C# 프로젝트를 빌드하지 않습니다."
	echo ">>>> '.NET 프로젝트' =================================================="
	echo ""
}

func_build_sample_app() {
	if [ ${msbuild_project} != "all" ]; then
		if [ ${msbuild_project} != "sample" ]; then
			return
		fi
	fi

	echo ">>>> '샘플 프로젝트' =================================================="
	echo ">>>> 'SimpleClient', 'SimpleServer' C# 프로젝트를 빌드하지 않습니다."
	echo ">>>> '샘플 프로젝트' =================================================="
	echo ""
}

# 공통 로직...
func_check_comandline_params() {
	# 첫번째 인자값의 유효성을 검사합니다.
	target_list=(clean build)
	for i in ${target_list[@]}
	do
		if [ "$1" == "$i" ]; then
			msbuild_target="$i"
		fi
	done

	if [ "${msbuild_target}" == "" ]; then
		echo ">>>> Error : 1st(Build Target) argument is invalid."
		echo ">>>>"
		return 1
	fi

	# 두번째 인자값의 유효성을 검사합니다.
	config_list=(Debug Release)
	for i in ${config_list[@]}
	do
		if [ "$2" == "$i" ]; then
			msbuild_config_cpp="$i"
		fi
	done

	if [ "${msbuild_config_cpp}" == "" ]; then
		echo ">>>> Error : 2nd(Build Configuration) argument is invalid."
		echo ">>>>"
		return 1
	fi

	# 세번째 인자값의 유효성을 검사합니다.
	platform_list=(x64)
	for i in ${platform_list[@]}
	do
		if [ "$3" == "$i" ]; then
			msbuild_platform_cpp="$i"
		fi
	done

	if [ "${msbuild_platform_cpp}" == "" ]; then
		echo ">>>> Error : 3rd(Build Platform) argument is invalid."
		echo ">>>>"
		return 1
	fi

	# 네번째 인자값의 유효성을 검사합니다.
	for i in ${project_list[@]}
	do
		if [ "$4" == "$i" ]; then
			msbuild_project="$i"
		fi
	done

	if [ "${msbuild_project}" == "" ]; then
		echo ">>>> Error : 4th(Build Project) argument is invalid."
		echo ">>>>"
		return 1
	fi

	if [[ $msbuild_config_cpp =~ "Release" ]]; then
		msbuild_config_cs=Release
	else
		msbuild_config_cs=Debug
	fi
	msbuild_platform_cs=AnyCPU

	return 0
}

func_check_environment_variable() {
	# 환경변수가 등록되어 있는 지를 체크합니다.
	if [ "${PN_BUILD_GCC_PATH}" == "" ]; then
		# 예시 : /usr/bin/gcc
		echo ">>>> Error : Register the PN_BUILD_GCC_PATH environment variable before executing the batch file."
		echo ">>>>"
		return
	fi

	if [ "${PN_BUILD_GPP_PATH}" == "" ]; then
		# 예시 : /usr/bin/g++
		echo ">>>> Error : Register the PN_BUILD_GPP_PATH environment variable before executing the batch file."
		echo ">>>>"
		return
	fi

	if [ "${CMAKE_MODULE_PATH}" == "" ]; then
		# 예시 : /usr/local/cmake-3.25.0/bin/cmake
		echo ">>>> Error : Register the CMAKE_MODULE_PATH environment variable before executing the batch file."
		echo ">>>>"
		return
	fi

	echo ">>>> Env-Var = {"
	echo ">>>>   'PN_BUILD_GCC_PATH' : '${PN_BUILD_GCC_PATH}',"
	echo ">>>>   'PN_BUILD_GPP_PATH' : '${PN_BUILD_GPP_PATH}',"
	echo ">>>>   'CMAKE_MODULE_PATH' : '${CMAKE_MODULE_PATH}'"
	echo ">>>> }"
	echo ">>>>"
}

func_compile_command() {
	param1=$1

	msbuild_config=
	msbuild_platform=
	if [[ $msbuild_config_cpp =~ "vcxproj" ]]; then
		msbuild_config=${msbuild_config_cpp}
		msbuild_platform=${msbuild_platform_cpp}
	elif [[ $msbuild_config_cpp =~ "csproj" ]]; then
		msbuild_config=${msbuild_config_cs}
		msbuild_platform=${msbuild_platform_cs}
	fi

	echo ">>>> ${param1} =================================================="

	if [ ${msbuild_target} == "clean" ]; then
		echo ">>>> CommandLine : ${CMAKE_MODULE_PATH} --build ${param1}/build/${msbuild_config_cpp}/ --config ${msbuild_config_cpp} --target clean"
		echo ">>>> CommandLine : rm -rf ${param1}/build/${msbuild_config_cpp}/"
		echo ">>>>"
		echo ">>>> --------------------------------------------------"

		${CMAKE_MODULE_PATH} --build ${param1}/build/${msbuild_config_cpp}/ --config ${msbuild_config_cpp} --target clean
		rm -rf ${param1}/build/${msbuild_config_cpp}/
	elif [ ${msbuild_target} == "build" ]; then
		echo ">>>> CommandLine : ${CMAKE_MODULE_PATH} -G \"Unix Makefiles\" -DCMAKE_BUILD_TYPE:STRING=${msbuild_config_cpp} -DCMAKE_C_COMPILER:FILEPATH=${PN_BUILD_GCC_PATH} -DCMAKE_CXX_COMPILER:FILEPATH=${PN_BUILD_GPP_PATH} -S${param1} -B${param1}/build/${msbuild_config_cpp}/ ${param1}/"
		echo ">>>> CommandLine : ${CMAKE_MODULE_PATH} --build ${param1}/build/${msbuild_config_cpp}/ --config ${msbuild_config_cpp} --target all"
		echo ">>>>"
		echo ">>>> --------------------------------------------------"

		${CMAKE_MODULE_PATH} -G "Unix Makefiles" -DCMAKE_BUILD_TYPE:STRING=${msbuild_config_cpp} -DCMAKE_C_COMPILER:FILEPATH=${PN_BUILD_GCC_PATH} -DCMAKE_CXX_COMPILER:FILEPATH=${PN_BUILD_GPP_PATH} -S${param1} -B${param1}/build/${msbuild_config_cpp}/ ${param1}/
		${CMAKE_MODULE_PATH} --build ${param1}/build/${msbuild_config_cpp}/ --config ${msbuild_config_cpp} --target all
	fi

	echo ">>>> ${param1} =================================================="
	echo ""
}

func_main $1 $2 $3 $4
