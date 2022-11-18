cmake_minimum_required(VERSION 3.0.0)
project(PNLicenseRunHidden)

set(SOURCES
    ./AppExecuter.inl
    ./Binary.inl
    ./LicenseMessage.inl
    ./MemoryHopper.inl
    ./PNLic.inl
    ./PNLicenseManager.cpp
    ./stdafx.cpp
)

set(HEADERS
    ./Defines.h
    ./Edgegap.h
    ./Edgegap.inl
    ./HiddenAppExecuter.inl
    ./KernelObjectNames.h
    ./LicenseMessage.h
    ./LicenseType.h
    ./PNLic.h
    ./PNTypes.h
    ./RunLicenseApp.h
    ./stdafx.h
    ./WarningAppExecuter.inl
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
    -D_NO_NTTNTRACE
    -DPROUD_STATIC_LIB
    -DPNLICENSESDK_STATIC
    -DPROUDSRCCOPY
)

# link
add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})
