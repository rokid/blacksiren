LOCAL_PATH:= $(call my-dir)
CURRENT_PATH := $(LOCAL_PATH)

include $(LOCAL_PATH)/siren_config.mk
include $(LOCAL_PATH)/siren_enable_flags.mk

$(info -->$(ROKID_BLACKSIREN_CONFIG))
TARGET_CONFIG = $(ROKID_BLACKSIREN_CONFIG)
ifeq (,$(TARGET_CONFIG))
TARGET_CONFIG = mini
endif

$(info TARGET_CONFIG=$(TARGET_CONFIG))
$(shell mkdir -p $(TARGET_OUT_ETC))
$(shell mkdir -p $(TARGET_OUT))
$(shell cp $(LOCAL_PATH)/resource/$(TARGET_CONFIG)/blacksiren.json $(TARGET_OUT_ETC))
$(shell cp $(LOCAL_PATH)/resource/$(TARGET_CONFIG)/blacksiren_xmos.json $(TARGET_OUT_ETC))
$(shell cp $(LOCAL_PATH)/resource/$(TARGET_CONFIG)/blacksiren_i2s.json $(TARGET_OUT_ETC))
$(shell cp $(LOCAL_PATH)/resource/$(TARGET_CONFIG)/blacksiren_c.json $(TARGET_OUT_ETC))
$(shell cp -r $(LOCAL_PATH)/resource/$(TARGET_CONFIG)/cn $(TARGET_OUT)/workdir_cn)
$(shell cp -r $(LOCAL_PATH)/resource/$(TARGET_CONFIG)/en $(TARGET_OUT)/workdir_en)


$(info $(TARGET_DEVICE))
include $(wildcard $(LOCAL_PATH)/thirdparty/libjsonc/Android.mk)
LOCAL_PATH := $(CURRENT_PATH)

ifneq (,$(ONE_SHOT_MAKEFILE))
$(info ONE SHOT MAKEFILE!!!!)
include $(wildcard external/curl/Android.mk)
LOCAL_PATH := $(CURRENT_PATH)
endif


include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := \
		libr2audio:thirdparty/support/libs/android/armv7eabi/legacy/libr2audio.so \
		libr2ssp:thirdparty/support/libs/android/armv7eabi/libr2ssp.so \
		libztvad:thirdparty/support/libs/android/armv7eabi/libztvad.so \
		libr2vt:thirdparty/support/libs/android/armv7eabi/legacy/libr2vt.so \
		libopus:thirdparty/support/libs/android/armv7eabi/libopus.a
ifdef CONFIG_BF_MVDR
LOCAL_PREBUILT_LIBS += libr2mvdrbf:thirdparty/support/libs/android/armv7eabi/libr2mvdrbf.so
endif
include $(BUILD_MULTI_PREBUILT)

THIRD_INCLUDES += \
	$(LOCAL_PATH)/thirdparty/support/include \
	$(LOCAL_PATH)/thirdparty/libjsonc/include \
	external/curl/include 

include $(CLEAR_VARS)


SRC := $(call all-named-files-under,*.cpp,src) 

LOCAL_SRC_FILES:= \
	$(SRC) 

LOCAL_C_INCLUDES += \
		$(THIRD_INCLUDES) \
		$(LOCAL_PATH)/include 

ifeq ($(PLATFORM_SDK_VERSION),22)
LOCAL_SHARED_LIBRARIES += libdl
LOCAL_STATIC_LIBRARIES += libc++
LOCAL_C_INCLUDES += external/libcxx/include
endif

LOCAL_CFLAGS:= $(L_CFLAGS) -Wall -Wextra -std=gnu++11
LOCAL_MODULE:= libbsiren
LOCAL_SHARED_LIBRARIES += liblog libr2ssp libztvad libr2vt
LOCAL_STATIC_LIBRARIES += libjsonc_static libopus

ifdef CONFIG_BF_MVDR
$(info use mvdr)
LOCAL_SHARED_LIBRARIES += libr2mvdrbf
endif

ifdef CONFIG_USE_AD1 
$(info use ad1)
LOCAL_SHARED_LIBRARIES += libr2audio
endif

ifdef CONFIG_USE_AD2
$(info use ad2)
LOCAL_SHARED_LIBRARIES += libr2audio
endif


include $(BUILD_SHARED_LIBRARY)

#ifeq (,$(ONE_SHOT_MAKEFILE))
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	test.cpp

LOCAL_C_INCLUDES += \
		$(LOCAL_PATH)/include \
		falcon/platform/hardware/common \
		external/curl/include \
		robot/hardware/include

ifeq ($(PLATFORM_SDK_VERSION),22)
LOCAL_SHARED_LIBRARIES += libdl
LOCAL_STATIC_LIBRARIES += libc++
LOCAL_C_INCLUDES += external/libcxx/include
endif

LOCAL_MODULE := test
LOCAL_SHARED_LIBRARIES += libbsiren libhardware libcurl
LOCAL_STATIC_LIBRARIES += libjsonc_static
include $(BUILD_EXECUTABLE)
#endif
#include $(wildcard $(LOCAL_PATH)/java/jni/Android.mk)

