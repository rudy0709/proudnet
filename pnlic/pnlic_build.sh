#!/bin/bash

projects=(all authnet_lib lic_auth_lib pnlic_lib pnlic_warn pnlic_hidden pnlic_auth)

func_main() {
	# 사용법을 출력해야 할 지를 검사합니다.
	func_check_comandline_params $1 $2
	if [ $? == 1 ]; then
		# 사용법을 출력합니다.
		echo "Usage:"
		echo "    pnlic_build.bat clean <all | authnet_lib | lic_auth_lib | pnlic_lib | pnlic_warn | pnlic_hidden | pnlic_auth>"
		echo "    pnlic_build.bat build <all | authnet_lib | lic_auth_lib | pnlic_lib | pnlic_warn | pnlic_hidden | pnlic_auth>"
		return
	fi

	# 프로젝트 별로 빌드함수를 호출합니다.
	#   (1) 환경변수 체크
	func_check_environment_variable

	#   (2) Tool 빌드
	#func_build_library_pidl $1 $2				# 리눅스 환경에선 빌드하지 않음

	#   (3) 공용 Library 빌드
	func_build_library_authnet_lib $1 $2
	func_build_library_lic_auth_lib $1 $2
	func_build_library_pnlic_lib $1 $2

	#   (4) PNLicense 관련 .exe 빌드
	func_build_library_pnlic_warn $1 $2
	func_build_library_pnlic_hidden $1 $2
	func_build_library_pnlic_auth $1 $2

	#   (5) Watermark 관련 .lib/.dll 빌드
	#func_build_library_watermark $1 $2			# 리눅스 환경에선 빌드하지 않음
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

func_build_library_authnet_lib() {
	if [ "$2" != "all" ]; then
		if [ "$2" != "authnet_lib" ]; then
			return
		fi
	fi

	#func_compile_command "AuthNetLib/ProudNetPidl" $1		# 리눅스 환경에선 빌드하지 않음
	func_compile_command "AuthNetLib/ProudNetClient" $1
	func_compile_command "AuthNetLib/ProudNetServer" $1
}

func_build_library_lic_auth_lib() {
	if [ "$2" != "all" ]; then
		if [ "$2" != "lic_auth_lib" ]; then
			return
		fi
	fi

	func_compile_command "LicAuthLib/PNLicAuthCommon" $1
	func_compile_command "LicAuthLib/PNLicAuthClient" $1
	#func_compile_command "LicAuthLib/PNLicAuthServer" $1	# 리눅스 환경에선 빌드하지 않음
}

func_build_library_pnlic_lib() {
	if [ "$2" != "all" ]; then
		if [ "$2" != "pnlic_lib" ]; then
			return
		fi
	fi

	func_compile_command "PNLicenseManager" $1
	func_compile_command "PNLicenseSDK" $1
	func_compile_command "PNLicense" $1
}

func_build_library_pnlic_warn() {
	if [ "$2" != "all" ]; then
		if [ "$2" != "pnlic_warn" ]; then
			return
		fi
	fi

	if [ "$1" == "clean" ]; then
		rm -f PNLicenseManager/PNLicenseWarningImage.inl
	fi
	func_compile_command "PNLicenseWarning" $1
}

func_build_library_pnlic_hidden() {
	if [ "$2" != "all" ]; then
		if [ "$2" != "pnlic_hidden" ]; then
			return
		fi
	fi

	if [ "$1" == "clean" ]; then
		rm -f PNLicenseManager/PNLicenseHiddenImage.inl
	fi
	func_compile_command "PNLicenseHidden" $1
}

func_build_library_pnlic_auth() {
	if [ "$2" != "all" ]; then
		if [ "$2" != "pnlic_auth" ]; then
			return
		fi
	fi

	func_compile_command "PNLicenseAuth" $1
	#func_compile_command "PNLicenseAuthGui" $1				# 리눅스 환경에선 빌드하지 않음
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
