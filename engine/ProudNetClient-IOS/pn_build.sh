#!/bin/bash

clean_library() {
	rm -rf build/
	rm -rf CMakeFiles/
	rm -rf CMakeScripts/
	rm -rf Debug*
	rm -rf EagerLinkingTBDs/
	rm -rf ProudNetClient.build/
	rm -rf Release*
	rm -f cmake_install.cmake
	rm -f CMakeCache.txt
	rm -rf ProudNetClient.xcodeproj
}

build_library() {
	case $1 in
		Release)
			xcodebuild -project ProudNetClient.xcodeproj -configuration Release -sdk $2 -arch $3 GCC_PREPROCESSOR_DEFINITIONS='_MULTIBYTE NDEBUG' CLANG_CXX_LIBRARY=libc++ CLANG_CXX_LANGUAGE_STANDARD=c++11
			;;

		Debug)
			xcodebuild -project ProudNetClient.xcodeproj -configuration Debug -sdk $2 -arch $3 GCC_PREPROCESSOR_DEFINITIONS='_MULTIBYTE' CLANG_CXX_LIBRARY=libc++ CLANG_CXX_LANGUAGE_STANDARD=c++11
			;;
	esac
}

build_library_all() {
	echo "build .................." #222-삭제:

	# ProudNetClient.xcodeproj 폴더가 존재하는 지를 검사합니다.
	proj_dir_path='./ProudNetClient.xcodeproj'
	if [ ! -d $proj_dir_path ]; then
		echo "'$proj_dir_path' 폴더를 cmake로 생성합니다."

		#sudo xcode-select --reset
		cmake -DCMAKE_C_COMPILER=`xcrun -find cc` -DCMAKE_CXX_COMPILER=`xcrun -find c++` -G Xcode ./
	fi

	# arm64, x86_64 아키텍처 용의 라이브러리를 각각 빌드합니다.
	build_library "Release" "iphoneos" "arm64"
	build_library "Debug" "iphoneos" "arm64"
	build_library "Release" "iphonesimulator" "arm64"
	build_library "Debug" "iphonesimulator" "arm64"

	# arm64, x86_64 아키텍처 용의 라이브러를 하나로 합칩니다.
	mkdir Release-universal-lib/ Debug-universal-lib/
	libtool -static ./Release-iphoneos/libProudNetClient.a ./Release-iphonesimulator/libProudNetClient.a -o ./Release-universal-lib/libProudNetClient.a
	libtool -static ./Debug-iphoneos/libProudNetClient.a ./Debug-iphonesimulator/libProudNetClient.a -o ./Debug-universal-lib/libProudNetClient.a
}


# 실행인자가 없을 경우에 사용법을 출력
if [ $# -eq 0 ]; then
	echo "Usage:"
	echo "    ./pn_build.sh clean       // delete temporary folders and files"
	echo "    or"
	echo "    ./pn_build.sh build       // build"
	exit 0
fi

# 첫번째 실행인자에 따라 명령을 실행
case $1 in
	clean)
		clean_library
		;;

	build)
		build_library_all
		;;
esac
