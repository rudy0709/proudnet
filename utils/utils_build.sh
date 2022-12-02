#!/bin/bash

projects=(all code-virtualizer image-gen pidl-compiler)

func_main() {
	# 사용법을 출력해야 할 지를 검사합니다.
	func_check_comandline_params $1 $2
	if [ $? == 1 ]; then
		# 파일 경로에서 파일명만 뽑아냅니다.
		filename="$(basename $0)"

		# 사용법을 출력합니다.
		echo "Usage:"
		echo "    ${filename} clean <all | code-virtualizer | image-gen | pidl-compiler>"
		echo "    ${filename} build <all | code-virtualizer | image-gen | pidl-compiler>"
		return
	fi

	# 프로젝트 별로 빌드함수를 호출합니다.
	#   (1) 환경변수 체크
	func_check_environment_variable

	#   (2) CodeVirtualizer 프로젝트 빌드
	func_build_project_code_virtualizer $1 $2

	#   (3) ImageGen 프로젝트 빌드
	func_build_project_image_gen $1 $2

	##   (4) Pidl 컴파일러 프로젝트 빌드
	func_build_project_pidl_compiler $1 $2
}

func_check_environment_variable() {
	# 환경변수가 등록되어 있는 지를 체크합니다.
	if [ "${PN_BUILD_GCC_PATH}" == "" ]; then
		# 예시 : /usr/bin/gcc
		echo ">>>> Error : Register the PN_BUILD_GCC_PATH environment variable before executing the batch file."
		return
	fi

	if [ "${PN_BUILD_GPP_PATH}" == "" ]; then
		# 예시 : /usr/bin/g++
		echo ">>>> Error : Register the PN_BUILD_GPP_PATH environment variable before executing the batch file."
		return
	fi

	if [ "${CMAKE_MODULE_PATH}" == "" ]; then
		# 예시 : /usr/local/cmake-3.25.0/bin/cmake
		echo ">>>> Error : Register the CMAKE_MODULE_PATH environment variable before executing the batch file."
		return
	fi

	echo ">>>> Environment-Variable(PN_BUILD_GCC_PATH) = ${PN_BUILD_GCC_PATH}"
	echo ">>>> Environment-Variable(PN_BUILD_GPP_PATH) = ${PN_BUILD_GPP_PATH}"
	echo ">>>> Environment-Variable(CMAKE_MODULE_PATH) = ${CMAKE_MODULE_PATH}"
	echo ">>>>"
}

func_build_project_code_virtualizer() {
	if [ "$2" != "all" ]; then
		if [ "$2" != "code-virtualizer" ]; then
			return
		fi
	fi

	func_compile_command "CodeVirtualizer" $1
}

func_build_project_image_gen() {
	if [ "$2" != "all" ]; then
		if [ "$2" != "image-gen" ]; then
			return
		fi
	fi

	func_compile_command "ImageGen" $1
}

func_build_project_pidl_compiler() {
	if [ "$2" != "all" ]; then
		if [ "$2" != "pidl-compiler" ]; then
			return
		fi
	fi

	#func_compile_command "Pidl" $1
	echo ">>>> 'PIDL 컴파일러' =================================================="
	echo ">>>> 윈도우 환경에서 생성된 .pidl 파일의 결과물을 사용하기 때문에 'PIDL 컴파일러' 프로젝트를 빌드하지 않습니다."
	echo ">>>> 'PIDL 컴파일러' =================================================="
	echo ""
}

func_compile_command() {
	echo ">>>> $1 =================================================="

	if [ "$2" == "clean" ]; then
		# 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		echo ">>>> CommandLine : ${CMAKE_MODULE_PATH} --build $1/build/ --config Release --target clean"
		echo ">>>> CommandLine : rm -rf $1/build/"
		echo ">>>>"
		echo ">>>> --------------------------------------------------"
		${CMAKE_MODULE_PATH} --build $1/build/ --config Release --target clean
		rm -rf $1/build/
	elif [ "$2" == "build" ]; then
		# 프로젝트를 빌드합니다.
		echo ">>>> CommandLine : ${CMAKE_MODULE_PATH} -G \"Unix Makefiles\" -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_C_COMPILER:FILEPATH=${PN_BUILD_GCC_PATH} -DCMAKE_CXX_COMPILER:FILEPATH=${PN_BUILD_GPP_PATH} -S$1 -B$1/build/ $1/"
		echo ">>>> CommandLine : ${CMAKE_MODULE_PATH} --build $1/build/ --config Release --target all"
		echo ">>>>"
		echo ">>>> --------------------------------------------------"
		${CMAKE_MODULE_PATH} -G "Unix Makefiles" -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_C_COMPILER:FILEPATH=${PN_BUILD_GCC_PATH} -DCMAKE_CXX_COMPILER:FILEPATH=${PN_BUILD_GPP_PATH} -S$1 -B$1/build/ $1/
		${CMAKE_MODULE_PATH} --build $1/build/ --config Release --target all
	fi

	echo ">>>> $1 =================================================="
	echo ""
}

func_check_comandline_params() {
	if [ "$1" != "clean" ]; then
		if [ "$1" != "build" ]; then
			return 1
		fi
	fi

	for prj in ${projects[@]}
	do
		if [ "$2" == "$prj" ]; then
			return 0
		fi
	done

	return 1
}

func_main $1 $2
