#!/bin/bash

projects=(all pnutils virtualizer authnet pnlic_mgr pnlic_sdk pnlic pnlic_auth_lib watermark pnlic_warn pnlic_hidden pnlic_auth_exe)

func_main() {
	# 사용법을 출력해야 할 지를 검사합니다.
	func_check_comandline_params $1 $2
	if [ $? == 1 ]; then
		# 사용법을 출력합니다.
		echo "Usage:"
		echo "    pnlic_build.bat clean <all | pnutils | virtualizer | authnet | pnlic_mgr | pnlic_sdk | pnlic | pnlic_auth_lib | watermark | pnlic_warn | pnlic_hidden | pnlic_auth_exe>"
		echo "    pnlic_build.bat build <all | pnutils | virtualizer | authnet | pnlic_mgr | pnlic_sdk | pnlic | pnlic_auth_lib | watermark | pnlic_warn | pnlic_hidden | pnlic_auth_exe>"
		return
	fi

	# 프로젝트 별로 빌드함수를 호출합니다.
	func_process_library_pnutils $1 $2
	#func_process_library_pidl $1 $2		# 리눅스에선 사용하지 않음
	func_process_library_virtualizer $1 $2
}

func_process_library_pnutils() {
	if [ "$2" != "all" ]; then
		if [ "$2" != "pnutils" ]; then
			return
		fi
	fi

	func_compile_command "[Utils]/PnUtils" $1
}

func_process_library_virtualizer() {
	if [ "$2" != "all" ]; then
		if [ "$2" != "virtualizer" ]; then
			return
		fi
	fi

	func_compile_command "CodeVirtualizer" $1
}

func_compile_command() {
	echo ">>>> $1 =================================================="

	if [ "$2" == "clean" ]; then
		# 프로젝트의 빌드 시에 만들어진 폴더 및 파일들을 삭제합니다.
		echo ">>>> CommandLine : make -C $1/build/ clean"
		echo ">>>> CommandLine : rm -rf $1/build/"
		echo ">>>>"
		echo ">>>> --------------------------------------------------"
		make -C $1/build/ clean
		rm -rf $1/build/
	elif [ "$2" == "build" ]; then
		# 프로젝트를 빌드합니다.
		echo ">>>> CommandLine : cmake -DCMAKE_BUILD_TYPE=Release -B $1/build/ ./"
		echo ">>>> CommandLine : make -C $1/build/"
		echo ">>>>"
		echo ">>>> --------------------------------------------------"
		cmake -DCMAKE_BUILD_TYPE=Release -B $1/build/ $1/
		make -C $1/build/
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
