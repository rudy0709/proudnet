cmake_minimum_required(VERSION 3.0.0)
project(PNLicenseSdk)

set(SOURCES
    ./PNErrorInfo.cpp
    ./PNFileSecureAttribute.cpp
    ./PNFileSystem.cpp
    ./PNLicenseInfo.cpp
    ./PNLicenseSDK.cpp
    ./PNRegistry.cpp
    ./PNSecure.cpp
    ./PNTime.cpp
    ./WinTime.cpp
    ./stdafx.cpp
)

set(HEADERS
    ./PNErrorInfo.h
    ./PNExport.h
    ./PNFileSecureAttribute.h
    ./PNFileSystem.h
    ./PNLicenseInfo.h
    ./PNLicenseSDK.h
    ./PNRegistry.h
    ./PNRegistryInfo.h
    ./PNSecure.h
    ./PNTime.h
    ./WinTime.h
    ./stdafx.h
)

# compile
add_compile_options(
    -fPIC
    -W -Wall -Wextra
    -O2
    -std=c++11
)
add_definitions(
    -D_LINUX
    -D_LIB
    -D_CRT_SECURE_NO_WARNINGS
    -D_NO_NTTNTRACE
    -DPROUD_STATIC_LIB
    -DPNLICENSESDK_STATIC
    -DPROUDSRCCOPY
    -D__x86_64__
    -D__linux__
)

# link
add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT_NAME}
    -L../AuthNet/ProudNetServer/build/ -lAuthNetServer
    pthread
)
#222-체크: -L../../utils/CodeVirtualizer/Lib/COFF/VirtualizerSDK$(PlatformArchitecture).lib
