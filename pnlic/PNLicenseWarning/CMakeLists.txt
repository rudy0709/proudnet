cmake_minimum_required(VERSION 3.0.0)
project(PNLicenseWarning)

set(CMAKE_CXX_STANDARD 11)

file(GLOB SOURCES
    PNLicenseWarning.cpp
    stdafx.cpp
)

file(GLOB HEADERS
    PNLicenseWarning.h
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
#        -Wno-unused-parameter -Wno-unknown-pragmas
#        -Wno-comment -Wno-extra -Wno-ignored-qualifiers
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
    ${CMAKE_SOURCE_DIR}/../PNLicense/build/libPNLicense.a
    ${CMAKE_SOURCE_DIR}/../PNLicenseSdk/build/libPNLicenseSdk.a
    ${CMAKE_SOURCE_DIR}/../AuthNetLib/ProudNetServer/build/libAuthNetServer.a
    ${CMAKE_SOURCE_DIR}/../AuthNetLib/ProudNetClient/build/libAuthNetClient.a
)

# event
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND rm -f ${CMAKE_SOURCE_DIR}/../PNLicenseManager/${PROJECT_NAME}Image.inl
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND echo "윈도우 환경에서 ${PROJECT_NAME}_UnProtect 파일을 난독화해야 합니다."
    COMMAND echo "난독화된 ${PROJECT_NAME} 파일을 .inl 파일로 변환합니다."
    COMMAND echo "아래 명령을 실행해 주세요."
    COMMAND echo "  ${CMAKE_SOURCE_DIR}/../../utils/ImageGen/build/ImageGen --bin2inl ${CMAKE_SOURCE_DIR}/build/${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/../PNLicenseManager/${PROJECT_NAME}Image.inl"
)
