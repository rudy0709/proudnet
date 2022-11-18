cmake_minimum_required(VERSION 3.0.0)
project(PNLicAuthCommon)

set(SOURCES
    ./Vars.cpp
    ./stdafx.cpp
)

set(HEADERS
    ./Vars.h
    ./stdafx.h
)

set(PIDLS
    ./pidl/C2S.PIDL
    ./pidl/S2C.PIDL
)

# compile
add_compile_options(
    -fPIC
    -W -Wall -Wextra
    -O2
    -std=c++11
)
add_definitions(
    -D_LINUX
    -D_LIB
    -D_CRT_SECURE_NO_WARNINGS
    -D_NO_NTTNTRACE
    -DPROUD_STATIC_LIB
    -DPNLICENSESDK_STATIC
    -DPROUDSRCCOPY
    -D__x86_64__
    -D__linux__
)

# link
add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT_NAME}
    pthread
)
