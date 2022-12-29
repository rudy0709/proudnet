cmake_minimum_required(VERSION 3.0.0)
project(PNLicenseSdk)

set(CMAKE_CXX_STANDARD 11)

file(GLOB SOURCES
    ./PNErrorInfo.cpp
    ./PNFileSecureAttribute.cpp
    ./PNFileSystem.cpp
    ./PNLicenseInfo.cpp
    ./PNLicenseSdk.cpp
    ./PNRegistry.cpp
    ./PNSecure.cpp
    ./PNTime.cpp
    #./WinTime.cpp
    ./stdafx.cpp
)

file(GLOB HEADERS
    ./PNErrorInfo.h
    ./PNExport.h
    ./PNFileSecureAttribute.h
    ./PNFileSystem.h
    ./PNLicenseInfo.h
    ./PNLicenseSdk.h
    ./PNRegistry.h
    ./PNRegistryInfo.h
    ./PNSecure.h
    ./PNTime.h
    ./WinTime.h
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
    -DPNLICENSESDK_STATIC
    -D_NO_NTTNTRACE
    -DPROUDSRCCOPY
)

add_compile_options(
    -fPIC
    -W -Wall -Wextra
#        -Wno-unused-parameter
#        -Wno-comment -Wno-extra -Wno-ignored-qualifiers
    -O2
    -std=c++11
)

# link
add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    COMPILE_DEFINITIONS  $<$<CONFIG:DEBUG>:DEBUG _DEBUG> $<$<CONFIG:RELEASE>:NDEBUG>
)

target_link_libraries(${PROJECT_NAME}
    -lpthread
)
