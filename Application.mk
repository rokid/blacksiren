NDK_TOOLCHAIN_VERSION := 4.9

APP_STL := system
#APP_STL := gnustl_static
#APP_STL := c++_static

#APP_ABI := armeabi-v7a arm64-v8a
#APP_ABI := arm64-v8a
APP_ABI := armeabi-v7a

#APP_OPTIM := debug
APP_PLATFORM := android-23
APP_BUILD_SCRIPT := ./Android.mk
#APP_CFLAGS += -Wno-error=format-security
