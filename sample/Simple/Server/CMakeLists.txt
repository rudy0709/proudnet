cmake_minimum_required(VERSION 3.0.0)
project(SimpleServer)

set(CMAKE_CXX_STANDARD 11)

file(GLOB SOURCES
    Server.cpp
    stdafx.cpp
)

file(GLOB HEADERS
    stdafx.h
)

include_directories(
    ${CMAKE_SOURCE_DIR}/../../../core_cpp/include
)

# options
add_definitions(
    -D__x86_64__
    -D__linux__
    -D_LINUX
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
)

target_link_libraries(${PROJECT_NAME}
    ${CMAKE_SOURCE_DIR}/../../../core_cpp/ProudNetClientLib/build/${CMAKE_BUILD_TYPE}/libProudNetClient.a
    ${CMAKE_SOURCE_DIR}/../../../core_cpp/ProudNetServerLib/build/${CMAKE_BUILD_TYPE}/libProudNetServer.a
    ${CMAKE_SOURCE_DIR}/../Common/build/${CMAKE_BUILD_TYPE}/libSimpleCommon.a
    -L/usr/local/ssl/lib -lcrypto -lssl
    -lcurl
    -lodbc
    -lpthread
)
