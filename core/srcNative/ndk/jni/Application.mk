APP_MODULES := ProudNetClientPlugin
APP_CPPFLAGS += -fexceptions -fpermissive -frtti -Wwrite-strings -std=c++11
APP_STL := c++_static
APP_BUILD_SCRIPT := $(call my-dir)/Android.mk
APP_PLATFORM := android-14
APP_ABI := ARCHSTRING
