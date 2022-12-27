cmake_minimum_required(VERSION 3.0.0)
project(PNLicenseAuth)

set(CMAKE_CXX_STANDARD 11)

file(GLOB SOURCES
    PNLicenseAuth.cpp
    ../../version/PNVersion.cpp
    stdafx.cpp
)

file(GLOB HEADERS
    PNLicenseAuth.h
    ../../version/PNVersion.h
    stdafx.h
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
    -O2
    -std=c++11
)

# link
add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    COMPILE_DEFINITIONS  $<$<CONFIG:DEBUG>:DEBUG _DEBUG> $<$<CONFIG:RELEASE>:NDEBUG>
    RUNTIME_OUTPUT_NAME ${PROJECT_NAME}_UnProtect
)

target_link_libraries(${PROJECT_NAME}
    -lpthread
    ${CMAKE_SOURCE_DIR}/../LicAuthLib/PNLicAuthClient/build/${CMAKE_BUILD_TYPE}/libPNLicAuthClient.a
    ${CMAKE_SOURCE_DIR}/../LicAuthLib/PNLicAuthCommon/build/${CMAKE_BUILD_TYPE}/libPNLicAuthCommon.a
    ${CMAKE_SOURCE_DIR}/../PNLicenseSdk/build/${CMAKE_BUILD_TYPE}/libPNLicenseSdk.a
    ${CMAKE_SOURCE_DIR}/../AuthNetLib/ProudNetServer/build/${CMAKE_BUILD_TYPE}/libAuthNetServer.a
    ${CMAKE_SOURCE_DIR}/../AuthNetLib/ProudNetClient/build/${CMAKE_BUILD_TYPE}/libAuthNetClient.a
)

# event
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND echo "윈도우 환경에서 ${PROJECT_NAME}_UnProtect 파일을 난독화해야 합니다."
)
