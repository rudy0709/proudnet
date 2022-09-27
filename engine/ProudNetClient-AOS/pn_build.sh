#!/bin/bash

main() {
	case $1 in
		clean)
			clean_library
			;;
		build)
			build_library $2
			;;
		*)
			# 사용법을 출력합니다.
			echo "Usage:"
			echo "    ./pn_build.sh clean"
			echo "    or"
			echo "    ./pn_build.sh build [ndk-version=r22b|r20b|r17c|r13b]"
			exit 0
			;;
	esac
}

clean_library() {
	./gradlew clean

	# 빌드 시에 생성되었으나, clean으로 제거되지 않은 디렉토리를 삭제합니다.
	rm -rf app/.cxx/
	rm -rf build/
	rm -rf proudnetlib/build/
	rm -rf proudnetlib/libs/
	rm -f proudnetlib/build.gradle
	rm -f local.properties
}

build_library() {
	# 안드로이드 SDK, NDK 경로를 환경변수에서 가져옵니다.
	local android_sdk_path=$ANDROID_SDK_HOME
	if [ ${android_sdk_path:(-1)} == "/" ]; then
		android_sdk_path=${android_sdk_path::${#android_sdk_path}-1}
	fi

	local android_ndk_base_path=$ANDROID_NDK_HOME
	if [ ${android_ndk_base_path:(-1)} == "/" ]; then
		android_ndk_base_path=${android_ndk_base_path::${#android_ndk_base_path}-1}
	fi

	# 2번째 인자가 전달된 경우엔 해당 버전의 NDK만을 남기고 나머지 버전의 NDK는 버립니다.
	local ndk_version_list=("r22b" "r20b" "r17c" "r13b")

	for i in ${!ndk_version_list[*]}; do
		if [ "$1" != "" ] && [ "$1" != "${ndk_version_list[$i]}" ]; then
			ndk_version_list[$i]=""
		fi
	done

	# ndk_version_list 배열에서 값이 남아있는 항목만 빌드합니다.
	for ndk_version in ${ndk_version_list[@]}; do
		if [ $ndk_version != "" ]; then
			build_library_by_clang

			compress_library "arm64-v8a" "aarch64-linux-android-4.9" "aarch64-linux-android-strip"
			compress_library "armeabi-v7a" "arm-linux-androideabi-4.9" "arm-linux-androideabi-strip"
			compress_library "x86" "x86-4.9" "i686-linux-android-strip"
		fi
	done
}

build_library_by_clang() {
	echo "$ndk_version 버전의 NDK로 빌드합니다..."

	# 1) NDK 버전에 맞게 local.properties 파일을 생성합니다.
	local android_ndk_path="$android_ndk_base_path/android-ndk-$ndk_version"

	rm -f local.properties
	echo "--> local.properties 파일을 생성합니다."
	echo "ndk.dir=$android_ndk_path" > local.properties
	echo "sdk.dir=$android_sdk_path" >> local.properties

	# 2) NDK 버전에 맞는 proudnetlib/build.gradle 파일을 복사합니다.
	echo "--> proudnetlib/build.gradle.$ndk_version 파일을 proudnetlib/build.gradle로 복사합니다."
	cp -f proudnetlib/build.gradle.$ndk_version proudnetlib/build.gradle

	# 3) 빌드를 시작합니다.
	export GRADLE_EXIT_CONSOLE=1
	./gradlew build -p proudnetlib/
}

compress_library() {
	local script_file_path=$(dirname "$0")
	local current_path=$(cd "$script_file_path" && pwd)
	if [ ${current_path:(-1)} == "/" ]; then
		current_path=${current_path::${#current_path}-1}
	fi

	local org_debug_lib_path=$current_path/proudnetlib/libs/ndk/$ndk_version/cmake/clangDebug/$1
	local org_release_lib_path=$current_path/proudnetlib/libs/ndk/$ndk_version/cmake/clangRelease/$1
	local debug_lib_path=$current_path/build/libs/$ndk_version/cmake/clangDebug/$1
	local release_lib_path=$current_path/build/libs/$ndk_version/cmake/clangRelease/$1
	local strip_exe_path=$android_ndk_base_path/android-ndk-$ndk_version/toolchains/$2/prebuilt/darwin-x86_64/bin/$3

	mkdir -p $debug_lib_path/
	mkdir -p $release_lib_path/stripped/
	cp -f $org_debug_lib_path/libProudNetClient.a $debug_lib_path/libProudNetClient.a
	cp -f $org_release_lib_path/libProudNetClient.a $release_lib_path/libProudNetClient.a
	cp -f $org_release_lib_path/libProudNetClient.a $release_lib_path/stripped/libProudNetClient.a
	$strip_exe_path $release_lib_path/stripped/libProudNetClient.a
}

main $1 $2
