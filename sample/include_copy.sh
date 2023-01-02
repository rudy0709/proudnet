#!/bin/bash

inc_path="../include/ProudNet"

if [ ! -d ${inc_path} ]; then
    mkdir -p ${inc_path}
fi

if [ ! -e "${inc_path}/ProudNetClient.h" ]; then
    cp -rf ../core_cpp/include/* ${inc_path}
fi
