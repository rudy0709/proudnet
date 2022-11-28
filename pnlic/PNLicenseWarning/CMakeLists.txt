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
#set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_NAME ${PROJECT_NAME}_UnProtect)
target_link_libraries(${PROJECT_NAME}
    -lpthread
    ${CMAKE_SOURCE_DIR}/../PNLicense/build/libPNLicense.a
    ${CMAKE_SOURCE_DIR}/../PNLicenseSDK/build/libPNLicenseSdk.a
    ${CMAKE_SOURCE_DIR}/../PNLicenseManager/build/libPNLicenseManager.a
    ${CMAKE_SOURCE_DIR}/../AuthNetLib/ProudNetServer/build/libAuthNetServer.a
    ${CMAKE_SOURCE_DIR}/../AuthNetLib/ProudNetClient/build/libAuthNetClient.a
)

# event
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/../Tools/ImageGen/build/ImageGen --bin2inl ${CMAKE_SOURCE_DIR}/build/${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/../PNLicenseManager/${PROJECT_NAME}Image.inl
)
