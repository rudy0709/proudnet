#!/bin/bash

project_forlder=
msbuild_config=Release

# SimpleCommon 프로젝트를 빌드합니다
project_forlder=./Common

echo ">>>> CommandLine : ${CMAKE_MODULE_PATH} --build ${project_forlder}/build/${msbuild_config}/ --config ${msbuild_config} --target clean"
echo ">>>> CommandLine : ${CMAKE_MODULE_PATH} -G \"Unix Makefiles\" -DCMAKE_BUILD_TYPE:STRING=${msbuild_config} -B${project_forlder}/build/${msbuild_config}/ ${project_forlder}/"
echo ">>>> CommandLine : ${CMAKE_MODULE_PATH} --build ${project_forlder}/build/${msbuild_config}/ --config ${msbuild_config} --target all"
echo ">>>> --------------------------------------------------"
${CMAKE_MODULE_PATH} --build ${project_forlder}/build/${msbuild_config}/ --config ${msbuild_config} --target clean
${CMAKE_MODULE_PATH} -G \"Unix Makefiles\" -DCMAKE_BUILD_TYPE:STRING=${msbuild_config} -B${project_forlder}/build/${msbuild_config}/ ${project_forlder}/
${CMAKE_MODULE_PATH} --build ${project_forlder}/build/${msbuild_config}/ --config ${msbuild_config} --target all
echo ">>>> --------------------------------------------------"

# SimpleClient 프로젝트를 빌드합니다
project_forlder=./Client

echo ">>>> CommandLine : ${CMAKE_MODULE_PATH} --build ${project_forlder}/build/${msbuild_config}/ --config ${msbuild_config} --target clean"
echo ">>>> CommandLine : ${CMAKE_MODULE_PATH} -G \"Unix Makefiles\" -DCMAKE_BUILD_TYPE:STRING=${msbuild_config} -B${project_forlder}/build/${msbuild_config}/ ${project_forlder}/"
echo ">>>> CommandLine : ${CMAKE_MODULE_PATH} --build ${project_forlder}/build/${msbuild_config}/ --config ${msbuild_config} --target all"
echo ">>>> --------------------------------------------------"
${CMAKE_MODULE_PATH} --build ${project_forlder}/build/${msbuild_config}/ --config ${msbuild_config} --target clean
${CMAKE_MODULE_PATH} -G \"Unix Makefiles\" -DCMAKE_BUILD_TYPE:STRING=${msbuild_config} -B${project_forlder}/build/${msbuild_config}/ ${project_forlder}/
${CMAKE_MODULE_PATH} --build ${project_forlder}/build/${msbuild_config}/ --config ${msbuild_config} --target all
echo ">>>> --------------------------------------------------"

# SimpleServer 프로젝트를 빌드합니다
project_forlder=./Server

echo ">>>> CommandLine : ${CMAKE_MODULE_PATH} --build ${project_forlder}/build/${msbuild_config}/ --config ${msbuild_config} --target clean"
echo ">>>> CommandLine : ${CMAKE_MODULE_PATH} -G \"Unix Makefiles\" -DCMAKE_BUILD_TYPE:STRING=${msbuild_config} -B${project_forlder}/build/${msbuild_config}/ ${project_forlder}/"
echo ">>>> CommandLine : ${CMAKE_MODULE_PATH} --build ${project_forlder}/build/${msbuild_config}/ --config ${msbuild_config} --target all"
echo ">>>> --------------------------------------------------"
${CMAKE_MODULE_PATH} --build ${project_forlder}/build/${msbuild_config}/ --config ${msbuild_config} --target clean
${CMAKE_MODULE_PATH} -G \"Unix Makefiles\" -DCMAKE_BUILD_TYPE:STRING=${msbuild_config} -B${project_forlder}/build/${msbuild_config}/ ${project_forlder}/
${CMAKE_MODULE_PATH} --build ${project_forlder}/build/${msbuild_config}/ --config ${msbuild_config} --target all
echo ">>>> --------------------------------------------------"
