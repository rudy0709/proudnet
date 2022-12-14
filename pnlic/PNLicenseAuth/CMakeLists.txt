cmake_minimum_required(VERSION 3.0.0)
project(PNLicenseAuth)

set(CMAKE_CXX_STANDARD 11)

file(GLOB SOURCES
    ./PNLicenseAuth.cpp
    ../../version/PNVersion.cpp
    ./stdafx.cpp

)

file(GLOB HEADERS
    ./PNLicenseAuth.h
    ../../version/PNVersion.h
    ./stdafx.h
)

# options
include_directories(
    ../../utils/CodeVirtualizer/Include/C/
)

add_definitions(
    -D__x86_64__
    -D__linux__
    -D_LINUX
    -D_CONSOLE
    -D_NO_NTTNTRACE
    -DPROUDSRCCOPY
)

add_compile_options(
    -fPIC
    -W -Wall -Wextra
#    -O2
    -std=c++11
)

# link
add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT_NAME}
    -lpthread
    ${CMAKE_SOURCE_DIR}/../LicAuthLib/PNLicAuthClient/build/libPNLicAuthClient.a
    ${CMAKE_SOURCE_DIR}/../LicAuthLib/PNLicAuthCommon/build/libPNLicAuthCommon.a
    ${CMAKE_SOURCE_DIR}/../PNLicenseSDK/build/libPNLicenseSdk.a
    ${CMAKE_SOURCE_DIR}/../AuthNetLib/ProudNetServer/build/libAuthNetServer.a
    ${CMAKE_SOURCE_DIR}/../AuthNetLib/ProudNetClient/build/libAuthNetClient.a
)
