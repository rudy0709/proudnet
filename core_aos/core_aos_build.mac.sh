#!/bin/bash

project_list=(all r13b r17 r20)
build_target=
build_project=

#222-개발:
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
		echo "          project : all | client | server"
		return
	fi

	# 프로젝트 별로 빌드함수를 호출합니다.
	#   (1) 환경변수 체크
	func_check_environment_variable

	#   (2) ProudNetPidl 프로젝트 빌드 - 리눅스 환경에선 빌드하지 않음
	#func_build_project_pidl

	#   (3) ProudNetClient 프로젝트 빌드
	func_build_project_client

	#   (4) ProudNetServer 프로젝트 빌드
	func_build_project_server
}

func_build_project_client() {
	if [ ${msbuild_project} != "all" ]; then
		if [ ${msbuild_project} != "client" ]; then
			return
		fi
	fi

	# ProudNetClient 프로젝트들을 빌드/클린합니다.
	func_compile_command "./ProudNetClientLib"
	func_compile_command "./ProudNetClientDll"
}

func_build_project_server() {
	if [ ${msbuild_project} != "all" ]; then
		if [ ${msbuild_project} != "server" ]; then
			return
		fi
	fi

	# ProudNetClient 프로젝트들을 빌드/클린합니다.
	func_compile_command "./ProudNetServerLib"
	func_compile_command "./ProudNetServerDll"
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
