cmake_minimum_required(VERSION 3.0.0)
project(PnUtils VERSION 0.1.0)

set(SOURCES
    PnUtils.cpp
    ../../../version/PNVersion.cpp
)

set(HEADERS
    ../../../version/PNVersion.h
)

# compile
add_definitions(-D_DEBUG)
add_compile_options(-fPIC -W -Wall -Wextra -Wno-unknown-pragmas -Wno-unused-variable -Wno-switch -Wno-strict-aliasing -Wno-missing-field-initializers -O2 -std=c++11)

# link
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
