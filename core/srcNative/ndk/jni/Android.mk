LOCAL_SHORT_COMMANDS := true
LOCAL_ABSOLUTE_PATH := $(call my-dir)

# ProudNetClient Prebuilt

include $(CLEAR_VARS)

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

LOCAL_PATH := $(LOCAL_ABSOLUTE_PATH)

LOCAL_MODULE := ProudNetClient

LOCAL_SRC_FILES := ../../../lib/NDK/r17/cmake/clangRelease/ARCHSTRING/libProudNetClient.a

include $(PREBUILT_STATIC_LIBRARY)

# ProudNetClientPlugin Build

include $(CLEAR_VARS)

LOCAL_PATH := $(LOCAL_ABSOLUTE_PATH)

LOCAL_MODULE := ProudNetClientPlugin

LOCAL_SRC_FILES := 	../../ProudNetClientPlugin/ProudNetClientPlugin.cpp \
					../../ProudNetClientPlugin/IncludeSwig.cpp \
					../../ProudNetClientPlugin/EventWrap.cpp

LOCAL_C_INCLUDES += $(THIRD_PARTY)/android-ndk-r17/sources/cxx-stl/llvm-libc++/include \
					$(LOCAL_PATH)/../../../include  \
					$(LOCAL_PATH)/../../ProudNetClientPlugin/ProudNetClientPlugin.h \
					$(LOCAL_PATH)/../../ProudNetClientPlugin/EventWrap.h \
					$(LOCAL_PATH)/../../ProudNetClientPlugin/NativeType.h
					
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog

LOCAL_CFLAGS := -D__STDC_LIMIT_MACROS \
	-DBUILDING_LIBCHARSET \
    -DBUILDING_LIBICONV \
    -DIN_LIBRARY

# https://android.googlesource.com/platform/ndk/+/master/docs/BuildSystemMaintainers.md#Unwinding
# 상기 링크의 페이지에 나와있는 내용에 의거하여 아래 두 줄을 추가합니다. 실질적으로 효력이 없더라도 문제 예방차원에서 넣어둡니다.
LOCAL_LDFLAGS += -Wl,--exclude-libs,libgcc_real.a
LOCAL_LDFLAGS += -Wl,--exclude-libs,libunwind.a

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := ProudNetClient

include $(BUILD_SHARED_LIBRARY)