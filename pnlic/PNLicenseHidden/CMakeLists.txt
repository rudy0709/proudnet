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
    # ${PROJECT_NAME}_UnProtect 파일을 난독화합니다.
    #   - 리눅스 환경에선 난독화한 실행파일이 비정상 동작하기 때문에 스킵
    #   - Virtualizer.exe 파일은 윈도우 용의 실행파일로 리눅스 환경에서 실행되지 않음
    # COMMAND pushd ./
    # COMMAND cd "$(CMAKE_SOURCE_DIR)../../utils/CodeVirtualizer"
    # COMMAND ms-dos start /w Virtualizer.exe /protect .\ProudNet.cv /inputfile "${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/${PROJECT_NAME}_UnProtect" /outputfile "${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/${PROJECT_NAME}"
    # COMMAND popd

    # 실행파일을 .inl 파일로 변환합니다.
    COMMAND echo "==== --------------------------------------------------"
    COMMAND echo "cp -f ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/${PROJECT_NAME}_UnProtect ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/${PROJECT_NAME}"
    COMMAND echo "${CMAKE_SOURCE_DIR}/../../utils/ImageGen/build/${CMAKE_BUILD_TYPE}/ImageGen --bin2inl ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/../PNLicenseManager/${PROJECT_NAME}Image.inl"
    COMMAND echo "==== --------------------------------------------------"
    COMMAND cp -f ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/${PROJECT_NAME}_UnProtect ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/${PROJECT_NAME}
    COMMAND ${CMAKE_SOURCE_DIR}/../../utils/ImageGen/build/${CMAKE_BUILD_TYPE}/ImageGen --bin2inl ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/../PNLicenseManager/${PROJECT_NAME}Image.inl
)
