#The target_link_options command requires above 3.13.0
cmake_minimum_required (VERSION 3.13.0 FATAL_ERROR)

set(CMAKE_SUPPRESS_REGENERATION true)

#To set COMPILE_DEFINITIONS property
#In the future, This should be changed.
cmake_policy(SET CMP0043 OLD)

#The name of the .sln file
project(ProudNetNativePlugin)

#include path
include_directories("../include")

#Globbing the files
file(GLOB CLIENT_HEADER_FILES
		ProudNetClientPlugin/EventWrap.h
		ProudNetClientPlugin/NativeType.h
		ProudNetClientPlugin/ProudNetClientPlugin.h
	)
	
file(GLOB CLIENT_SOURCE_FILES
		ProudNetClientPlugin/EventWrap.cpp
		ProudNetClientPlugin/IncludeSwig.cpp
		ProudNetClientPlugin/ProudNetClientPlugin.cpp
	)
	
file(GLOB SERVER_HEADER_FILES
		ProudNetServerPlugin/NativeType.h
		ProudNetServerPlugin/ProudNetServerPlugin.h
	)
	
file(GLOB SERVER_SOURCE_FILES
		ProudNetServerPlugin/IncludeNetServerSwig.cpp
		ProudNetServerPlugin/ProudNetServerPlugin.cpp
	)

#Setting the language property to unity build files for using C++ compile options ( ex - frtti )
set_property(SOURCE ${SERVER_HEADER_FILES} PROPERTY LANGUAGE CXX)
set_property(SOURCE ${SERVER_SOURCE_FILES} PROPERTY LANGUAGE CXX)

set_property(SOURCE ${CLIENT_HEADER_FILES} PROPERTY LANGUAGE CXX)
set_property(SOURCE ${CLIENT_SOURCE_FILES} PROPERTY LANGUAGE CXX)

set(CMAKE_CONFIGURATION_TYPES Debug Release)

set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS "Debug")

#Creating targets
add_library(ProudNetServerPlugin SHARED ${SERVER_SOURCE_FILES})
add_library(ProudNetClientPlugin SHARED ${CLIENT_SOURCE_FILES})

set_property(TARGET ProudNetServerPlugin PROPERTY CXX_STANDARD 11)
set_property(TARGET ProudNetServerPlugin PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET ProudNetClientPlugin PROPERTY CXX_STANDARD 11)
set_property(TARGET ProudNetClientPlugin PROPERTY LINKER_LANGUAGE CXX)

#Setting the properties of targets depends on its architecture
if(WIN32)
	target_link_libraries(ProudNetServerPlugin ../lib/${CMAKE_VS_PLATFORM_NAME}/dll/${CMAKE_CFG_INTDIR}/ProudNetClient)
	target_link_libraries(ProudNetServerPlugin ../lib/${CMAKE_VS_PLATFORM_NAME}/dll/${CMAKE_CFG_INTDIR}/ProudNetServer)
	target_link_libraries(ProudNetClientPlugin ../lib/${CMAKE_VS_PLATFORM_NAME}/dll/${CMAKE_CFG_INTDIR}/ProudNetClient)

	#Setting the targets
	set_property(TARGET ProudNetServerPlugin PROPERTY ARCHIVE_OUTPUT_DIRECTORY ../lib/DotNet/${CMAKE_VS_PLATFORM_NAME})
	set_property(TARGET ProudNetServerPlugin PROPERTY ARCHIVE_OUTPUT_NAME ProudNetServerPlugin)
	set_property(TARGET ProudNetClientPlugin PROPERTY ARCHIVE_OUTPUT_DIRECTORY ../lib/DotNet/${CMAKE_VS_PLATFORM_NAME})
	set_property(TARGET ProudNetClientPlugin PROPERTY ARCHIVE_OUTPUT_NAME ProudNetClientPlugin)
	
	set_target_properties(ProudNetServerPlugin PROPERTIES LINK_FLAGS "/ignore:4221")
	set_target_properties(ProudNetClientPlugin PROPERTIES LINK_FLAGS "/ignore:4221")
	
	#Setting some preprocesors on specific configurations.
	#Debug
	set_property(TARGET ProudNetServerPlugin PROPERTY COMPILE_DEFINITIONS_DEBUG WIN32 _WINDOWS _USRDLL WINDOWSPLUGIN_EXPORTS USE_PROUDNET_DLL USE_OPENSSL _UNICODE UNICODE DEBUG _DEBUG ${COMPILE_DEFINITIONS_DEBUG})
	set_property(TARGET ProudNetClientPlugin PROPERTY COMPILE_DEFINITIONS_DEBUG WIN32 _WINDOWS _USRDLL WINDOWSPLUGIN_EXPORTS USE_PROUDNET_DLL USE_OPENSSL _UNICODE UNICODE DEBUG _DEBUG ${COMPILE_DEFINITIONS_DEBUG})
	
	#Release
	set_property(TARGET ProudNetServerPlugin PROPERTY COMPILE_DEFINITIONS_RELEASE WIN32 _WINDOWS _USRDLL WINDOWSPLUGIN_EXPORTS USE_PROUDNET_DLL USE_OPENSSL _UNICODE UNICODE NDEBUG ${COMPILE_DEFINITIONS_RELEASE})
	set_property(TARGET ProudNetClientPlugin PROPERTY COMPILE_DEFINITIONS_RELEASE WIN32 _WINDOWS _USRDLL WINDOWSPLUGIN_EXPORTS USE_PROUDNET_DLL USE_OPENSSL _UNICODE UNICODE NDEBUG ${COMPILE_DEFINITIONS_RELEASE})

	#Setting compile options for optimizing
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Oy /Ot /Oi")
	set(CMAKE_C_FLAGS_RELEASE_STATIC_CRT "${CMAKE_C_FLAGS_RELEASE} /Oy /Ot /Oi")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Oy /Ot /Oi")
	set(CMAKE_CXX_FLAGS_RELEASE_STATIC_CRT "${CMAKE_CXX_FLAGS_RELEASE} /Oy /Ot /Oi")
else(WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

	target_link_libraries(ProudNetServerPlugin
		${CMAKE_CURRENT_SOURCE_DIR}/../src/libProudNetClient.so
		${CMAKE_CURRENT_SOURCE_DIR}/../src/libProudNetServer.so
		ssl
		crypto
		odbc
		)
	
	target_link_libraries(ProudNetClientPlugin ${CMAKE_CURRENT_SOURCE_DIR}/../src/libProudNetClient.so)
		
	#Setting some preprocesors on specific configurations.
	#Debug
	set_property(TARGET ProudNetServerPlugin PROPERTY COMPILE_DEFINITIONS_DEBUG USE_OPENSSL _UNICODE UNICODE DEBUG _DEBUG ${COMPILE_DEFINITIONS_DEBUG})
	set_property(TARGET ProudNetClientPlugin PROPERTY COMPILE_DEFINITIONS_DEBUG USE_OPENSSL _UNICODE UNICODE DEBUG _DEBUG ${COMPILE_DEFINITIONS_DEBUG})
	
	#Release
	set_property(TARGET ProudNetServerPlugin PROPERTY COMPILE_DEFINITIONS_RELEASE USE_OPENSSL _UNICODE UNICODE NDEBUG ${COMPILE_DEFINITIONS_RELEASE})
	set_property(TARGET ProudNetClientPlugin PROPERTY COMPILE_DEFINITIONS_RELEASE USE_OPENSSL _UNICODE UNICODE NDEBUG ${COMPILE_DEFINITIONS_RELEASE})
	
	#-znow는 gcc의 compile option으로서 so파일이 loading되자마자 모든 전역 데이터가 초기화되도록 만든다.
	#이 옵션을 지정안하면, 리눅스에서 닷넷 코어로 사용시에 전역 데이터를 사용하는 시점에 개별로 초기화가 되는 듯하다.
	target_link_options(ProudNetServerPlugin PUBLIC LINKER:-znow)
	target_link_options(ProudNetClientPlugin PUBLIC LINKER:-znow)
	
	#Setting Environment Variables
	set_property(TARGET ProudNetServerPlugin PROPERTY ARCHIVE_OUTPUT_DIRECTORY ./)
	set_property(TARGET ProudNetServerPlugin PROPERTY ARCHIVE_OUTPUT_NAME ProudNetServer)
	set_property(TARGET ProudNetClientPlugin PROPERTY ARCHIVE_OUTPUT_DIRECTORY ./)
	set_property(TARGET ProudNetClientPlugin PROPERTY ARCHIVE_OUTPUT_NAME ProudNetClient)

	if(XCODE)
		#TO_DO
	else(XCODE)
		#TO_DO
	endif(XCODE)
endif(WIN32)
