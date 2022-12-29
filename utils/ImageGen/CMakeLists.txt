cmake_minimum_required(VERSION 3.0.0)
project(ImageGen)

set(CMAKE_CXX_STANDARD 11)

file(GLOB SOURCES
    ImageGen.cpp
    ../../version/PNVersion.cpp
)

file(GLOB HEADERS
    ../../version/PNVersion.h
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
    -lpthread
)
