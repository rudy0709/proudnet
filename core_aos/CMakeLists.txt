cmake_minimum_required (VERSION 3.6.0)
project(ProudNetClient)

set(CMAKE_SUPPRESS_REGENERATION true)

# POSIX Unix( Linux & Darwin ( macOS, iOS )) Makefiles
# 아래의 문장 대신에 CXX_STANDARD 속성에 11을 주는 문장을 쓰지는 말자,
# 그 이유는 CMake의 CXX_STANDARD 페이지에 이 속성을 변경하는 것은 VS의 경우 2015 update3 미만에 대해서는 효과가 없다고 써있는데,
# 행여나 CMAKE_STANDARD를 적용하고 윈도우쪽에도 적용되게 CMake 코드 정리를 해버릴 여지가 있기 때문이다.
# CXX_STANDARD 속성 변경 문장을 보면 윈도우에서도 적용될 것처럼 생겨먹었다.
# set(CMAKE_CXX_STANDARD 11)    <-- 이걸 쓰지 말자.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_CONFIGURATION_TYPES Debug Release)

# To set COMPILE_DEFINITIONS property
# In the future, This should be changed.
cmake_policy(SET CMP0043 OLD)

file(GLOB SRCS_UNITY_BUILD
    ../core_cpp/src/UnityBuild/ClientUnityBuild001.cpp
    ../core_cpp/src/UnityBuild/ClientUnityBuild002.cpp
    ../core_cpp/src/UnityBuild/ClientUnityBuild003.cpp
    ../core_cpp/src/UnityBuild/ClientUnityBuild004.cpp
    ../core_cpp/src/UnityBuild/ClientUnityBuild005.cpp
    ../core_cpp/src/UnityBuild/ClientUnityBuild006.cpp
    ../core_cpp/src/UnityBuild/ClientUnityBuild007.cpp
    ../core_cpp/src/UnityBuild/ClientUnityBuild008.cpp
    ../core_cpp/src/UnityBuild/ClientUnityBuild009.cpp
    ../core_cpp/src/UnityBuild/ClientUnityBuild010.cpp
    ../core_cpp/src/UnityBuild/ClientUnityBuild011.cpp
    ../core_cpp/src/UnityBuild/ClientUnityBuild012.cpp
    ../core_cpp/src/UnityBuild/ClientUnityBuild013.cpp
    ../core_cpp/src/UnityBuild/ClientUnityBuild014.cpp
    ../core_cpp/src/UnityBuild/ClientUnityBuild015.cpp
)

file(GLOB_RECURSE OPEN_SRCS_ICONV
    ../core_cpp/OpenSources/iconv-embed/*.h
    ../core_cpp/OpenSources/iconv-embed/*.c
)

file(GLOB_RECURSE OPEN_SRCS_LIBTOM_CRYPTO
    ../core_cpp/OpenSources/libtom/crypto/*.h
    ../core_cpp/OpenSources/libtom/crypto/*.c
)
list(FILTER OPEN_SRCS_LIBTOM_CRYPTO EXCLUDE REGEX ".*/OpenSources/libtom/crypto/ciphers/aes/aes_tab.c$")
list(FILTER OPEN_SRCS_LIBTOM_CRYPTO EXCLUDE REGEX ".*/OpenSources/libtom/crypto/hashes/sha2/sha224.c$")
list(FILTER OPEN_SRCS_LIBTOM_CRYPTO EXCLUDE REGEX ".*/OpenSources/libtom/crypto/hashes/sha2/sha384.c$")
list(FILTER OPEN_SRCS_LIBTOM_CRYPTO EXCLUDE REGEX ".*/OpenSources/libtom/crypto/hashes/whirl/whirltab.c$")
list(FILTER OPEN_SRCS_LIBTOM_CRYPTO EXCLUDE REGEX ".*/OpenSources/libtom/crypto/prngs/sober128tab.c$")

file(GLOB_RECURSE OPEN_SRCS_LIBTOM_MATH
    ../core_cpp/OpenSources/libtom/math/*.h
    ../core_cpp/OpenSources/libtom/math/*.c
)

#file(GLOB_RECURSE OPEN_SRCS_ZLIB
#    ../core_cpp/OpenSources/zlib/*.h
#    ../core_cpp/OpenSources/zlib/*.c
#)
file(GLOB OPEN_SRCS_ZLIB
    ../core_cpp/OpenSources/zlib/adler32.c
    ../core_cpp/OpenSources/zlib/compress.c
    ../core_cpp/OpenSources/zlib/crc32.c
    ../core_cpp/OpenSources/zlib/deflate.c
    ../core_cpp/OpenSources/zlib/gzclose.c
    ../core_cpp/OpenSources/zlib/gzlib.c
    ../core_cpp/OpenSources/zlib/gzread.c
    ../core_cpp/OpenSources/zlib/gzwrite.c
    ../core_cpp/OpenSources/zlib/infback.c
    ../core_cpp/OpenSources/zlib/inffast.c
    ../core_cpp/OpenSources/zlib/inflate.c
    ../core_cpp/OpenSources/zlib/inftrees.c
    ../core_cpp/OpenSources/zlib/trees.c
    ../core_cpp/OpenSources/zlib/uncompr.c
    ../core_cpp/OpenSources/zlib/zutil.c
    ../core_cpp/OpenSources/zlib/zconf.h
)

file(GLOB SRCS_LINUX
    #../core_cpp/src/stacktrace-unix.h
    #../core_cpp/src/stacktrace-unix.cpp
    ../core_cpp/src/swprintf.cpp
)

#Setting the language property to unity build files for using C++ compile options ( ex - frtti )
set_property(SOURCE ${DEV_SRC_UNITYBUILD_CLIENT} PROPERTY LANGUAGE CXX)
set_property(SOURCE ${DEV_SRC_UNITYBUILD_CLIENT} PROPERTY LINKER_LANGUAGE CXX)

## options
#add_definitions(
#    -DPROUD_STATIC_LIB
#    -D_NO_NTTNTRACE
#    -DPRIVATE
#)
#
#add_compile_options(
#    -fPIC
#    -W -Wall -Wextra
#    -O2
#    #-std=c++11
#)

# link
add_library(ProudNetClient_static STATIC ${OPEN_SRCS_ICONV} ${OPEN_SRCS_LIBTOM_CRYPTO} ${OPEN_SRCS_LIBTOM_MATH} ${OPEN_SRCS_ZLIB} ${SRCS_LINUX} ${SRCS_UNITY_BUILD})

set_property(TARGET ProudNetClient_static PROPERTY OUTPUT_NAME ${PROJECT_NAME})
set_property(TARGET ProudNetClient_static APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:DEBUG>:DEBUG _DEBUG> $<$<CONFIG:RELEASE>:NDEBUG>)
