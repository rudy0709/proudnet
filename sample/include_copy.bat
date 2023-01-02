@echo off

set inc_path=..\include\ProudNet

if not exist "%inc_path%" (
    mkdir "%inc_path%"
)

if not exist "%inc_path%\ProudNetClient.h" (
    xcopy /s /q ..\core_cpp\include\* %inc_path%
)
