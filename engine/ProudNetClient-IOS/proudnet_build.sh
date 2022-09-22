#!/bin/bash

# ProudNetClient.xcodeproj 폴더가 존재하는 지를 검사합니다.
proj_dir='./ProudNetClient.xcodeproj'
if [ ! -d $proj_dir ]; then
	echo "'$proj_dir' 폴더를 cmake로 생성합니다."

	#sudo xcode-select --reset
	cmake -DCMAKE_C_COMPILER=`xcrun -find cc` -DCMAKE_CXX_COMPILER=`xcrun -find c++` -G Xcode ./
fi

# iOS 용의 라이브러리를 빌드합니다.
xcodebuild -project ProudNetClient.xcodeproj -configuration Release -sdk iphoneos -arch arm64 GCC_PREPROCESSOR_DEFINITIONS='_MULTIBYTE NDEBUG' CLANG_CXX_LIBRARY=libc++ CLANG_CXX_LANGUAGE_STANDARD=c++11
xcodebuild -project ProudNetClient.xcodeproj -configuration Debug -sdk iphoneos -arch arm64 GCC_PREPROCESSOR_DEFINITIONS='_MULTIBYTE' CLANG_CXX_LIBRARY=libc++ CLANG_CXX_LANGUAGE_STANDARD=c++11
xcodebuild -project ProudNetClient.xcodeproj -configuration Release -sdk iphonesimulator -arch x86_64 GCC_PREPROCESSOR_DEFINITIONS='_MULTIBYTE NDEBUG' CLANG_CXX_LIBRARY=libc++ CLANG_CXX_LANGUAGE_STANDARD=c++11
xcodebuild -project ProudNetClient.xcodeproj -configuration Debug -sdk iphonesimulator -arch x86_64 GCC_PREPROCESSOR_DEFINITIONS='_MULTIBYTE' CLANG_CXX_LIBRARY=libc++ CLANG_CXX_LANGUAGE_STANDARD=c++11

# arm64, x86_64 아키텍처 각각의 라이브러를 하나로 합칩니다.
mkdir Release/ Debug/
libtool -static ./Release-iphoneos/libProudNetClient.a ./Release-iphonesimulator/libProudNetClient.a -o ./Release/libProudNetClient.a
libtool -static ./Debug-iphoneos/libProudNetClient.a ./Debug-iphonesimulator/libProudNetClient.a -o ./Debug/libProudNetClient.a
