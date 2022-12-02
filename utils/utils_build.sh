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
