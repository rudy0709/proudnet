cmake_minimum_required(VERSION 3.0.0)
project(ImageGen)

set(SOURCES
    ImageGen.cpp
    ../../../version/PNVersion.cpp
)

set(HEADERS
    ../../../version/PNVersion.h
)

# compile
add_compile_options(
    -fPIC
    -W -Wall -Wextra
    -O2
    -std=c++11
)
add_definitions(
    -D__x86_64__
    -D__linux__
    -D_LINUX
    -D_CONSOLE
)

# link
add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT_NAME}
    pthread
)
