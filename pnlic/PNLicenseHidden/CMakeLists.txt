cmake_minimum_required(VERSION 3.0.0)
project(PNLicenseHidden)

set(CMAKE_CXX_STANDARD 11)

file(GLOB SOURCES
    PNLicenseHidden.cpp
    PNLicInformer.cpp
    ../LicAuthLib/PNLicAuthCommon/Vars.cpp
    stdafx.cpp
)

file(GLOB HEADERS
    PNLicenseHidden.h
    PNLicInformer.h
    ../LicAuthLib/PNLicAuthCommon/Vars.h
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
    ${CMAKE_SOURCE_DIR}/../PNLicenseSdk/build/${CMAKE_BUILD_TYPE}/libPNLicenseSdk.a
    ${CMAKE_SOURCE_DIR}/../AuthNetLib/ProudNetServer/build/${CMAKE_BUILD_TYPE}/libAuthNetServer.a
    ${CMAKE_SOURCE_DIR}/../AuthNetLib/ProudNetClient/build/${CMAKE_BUILD_TYPE}/libAuthNetClient.a
)

# event
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND rm -f ${CMAKE_SOURCE_DIR}/../PNLicenseManager/${PROJECT_NAME}Image.inl
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND echo ">>>> --------------------------------------------------"
    COMMAND echo ">>>> 다음의 과정을 수작업으로 처리해야만 core_cpp 프로젝트들을 빌드할 수 있습니다."
    COMMAND echo ">>>> 1) 윈도우 환경에서 ${PROJECT_NAME}_UnProtect 파일을 난독화"
    COMMAND echo ">>>> 2) 난독화된 ${PROJECT_NAME} 파일을 .inl 파일로 변환"
    COMMAND echo ">>>>    ${CMAKE_SOURCE_DIR}/../../utils/ImageGen/build/${CMAKE_BUILD_TYPE}/ImageGen --bin2inl ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/../PNLicenseManager/${PROJECT_NAME}Image.inl"
    COMMAND echo ">>>> --------------------------------------------------"
)
