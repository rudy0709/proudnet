cmake_minimum_required(VERSION 3.0.0)
project(ProudNetClientPlugin)

set(CMAKE_CXX_STANDARD 11)

file(GLOB SOURCES
    ./IncludeNetServerSwig.cpp
    ./ProudNetServerPlugin.cpp
)

file(GLOB HEADERS
    ./NativeType.h
    ./ProudNetServerPlugin.h
)

include_directories(
    ${CMAKE_SOURCE_DIR}/../../core_cpp/include
)

# options
add_definitions(
    -D__x86_64__
    -D__linux__
    -D_LINUX
    -D_NO_NTTNTRACE
    -DPRIVATE
    -DUSE_OPENSSL
)

add_compile_options(
    -fPIC
    -W -Wall -Wextra
#        -Wno-unused-but-set-variable -Wno-unused-function -Wno-unused-label -Wno-unused-parameter -Wno-unused-variable
#        -Wno-comment -Wno-extra -Wno-ignored-qualifiers -Wno-missing-field-initializers -Wno-parentheses -Wno-reorder
#        -Wno-sign-compare -Wno-strict-aliasing -Wno-type-limits -Wno-unknown-pragmas
    -O2
    -std=c++11
)

# link
add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE 1
    COMPILE_DEFINITIONS  $<$<CONFIG:DEBUG>:DEBUG _DEBUG> $<$<CONFIG:RELEASE>:NDEBUG>
)

target_link_libraries(${PROJECT_NAME}
#222-    ${CMAKE_SOURCE_DIR}/../../core_cpp/ProudNetClientDll/build/libProudNetClient.so
    ${CMAKE_SOURCE_DIR}/../../core_cpp/ProudNetServerDll/build/libProudNetServer.so
    -lssl
    -lcrypto
    -lodbc
    -lpthread
)
