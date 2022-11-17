cmake_minimum_required(VERSION 3.0.0)
project(CodeVirtualizer)

set(CUSTOM_VMS
    ./custom_vms/fish32_black.vm
    ./custom_vms/fish32_red.vm
    ./custom_vms/fish32_white.vm
    ./custom_vms/fish64_black.vm
    ./custom_vms/fish64_red.vm
    ./custom_vms/fish64_white.vm
    ./custom_vms/puma32_black.vm
    ./custom_vms/puma32_red.vm
    ./custom_vms/puma32_white.vm
    ./custom_vms/puma64_black.vm
    ./custom_vms/puma64_red.vm
    ./custom_vms/puma64_white.vm
    ./custom_vms/shark32_black.vm
    ./custom_vms/shark32_red.vm
    ./custom_vms/shark32_white.vm
    ./custom_vms/shark64_black.vm
    ./custom_vms/shark64_red.vm
    ./custom_vms/shark64_white.vm
    ./custom_vms/tiger32_black.vm
    ./custom_vms/tiger32_red.vm
    ./custom_vms/tiger32_white.vm
    ./custom_vms/tiger64_black.vm
    ./custom_vms/tiger64_red.vm
    ./custom_vms/tiger64_white.vm
)

set(HEADERS
    ./Include/C/VirtualizerSDK.h
    ./Include/C/VirtualizerSDK_BorlandC_inline.h
    ./Include/C/VirtualizerSDK_CustomVMs.h
    ./Include/C/VirtualizerSDK_CustomVMs_BorlandC_inline.h
    ./Include/C/VirtualizerSDK_CustomVMs_GNU_inline.h
    ./Include/C/VirtualizerSDK_CustomVMs_ICL_inline.h
    ./Include/C/VirtualizerSDK_CustomVMs_LCC_inline.h
    ./Include/C/VirtualizerSDK_CustomVMs_VC_inline.h
    ./Include/C/VirtualizerSDK_GNU_inline.h
    ./Include/C/VirtualizerSDK_ICL_inline.h
    ./Include/C/VirtualizerSDK_LCC_inline.h
    ./Include/C/VirtualizerSDK_VC_inline.h
    ./StealthMode/C/StealthCodeArea.h
    ./StealthMode/C/StealthCodeArea_Max.h
    ./StealthMode/C/StealthExample.c
)

include_directories(
	./Include/C
)

# compile
add_compile_options(-fPIC -W -Wall -Wextra -Wno-unknown-pragmas -Wno-unused-variable -Wno-switch -Wno-strict-aliasing -Wno-missing-field-initializers -O2 -std=c++11)

# link
add_library(${PROJECT_NAME} ${CUSTOM_VMS} ${HEADERS})
