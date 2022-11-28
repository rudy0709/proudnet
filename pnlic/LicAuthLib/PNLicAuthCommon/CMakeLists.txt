cmake_minimum_required(VERSION 3.0.0)
project(PNLicAuthCommon)

set(CMAKE_CXX_STANDARD 11)

file(GLOB SOURCES
    ./Vars.cpp
    ./stdafx.cpp
)

file(GLOB HEADERS
    ./Vars.h
    ./stdafx.h
)

file(GLOB PIDLS
    ./pidl/C2S.PIDL
    ./pidl/S2C.PIDL
)

# options
add_definitions(
    -D__x86_64__
    -D__linux__
    -D_LINUX
    -D_LIB
    -D_NO_NTTNTRACE
    -DPNLICENSESDK_STATIC
    -DPROUDSRCCOPY
)

add_compile_options(
    -fPIC
    -W -Wall -Wextra
#        -Wno-unused-but-set-variable -Wno-unused-parameter
#        -Wno-comment -Wno-extra -Wno-ignored-qualifiers
    -O2
    -std=c++11
)

# link
add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT_NAME}
    -lpthread
)
