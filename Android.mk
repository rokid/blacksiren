LOCAL_PATH:= $(call my-dir)

include $(LOCAL_PATH)/android.config
L_CFLAGS := -DANDROID -DANDROID_LOG_TAG=\"BlackSiren\"

ifdef CONFIG_ANDROID_LOG
L_CFLAGS += -DCONFIG_ANDROID_LOG
endif

ifdef CONFIG_LEGACY_SIREN_TEST
L_CFLAGS += -DCONFIG_LEGACY_SIREN_TEST
endif

include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := \
		libr2ssp:thirdparty/support/$(TARGET_ARCH)/libs/libr2ssp.so \
		libztvad:thirdparty/support/$(TARGET_ARCH)/libs/libztvad.so \
		libztcodec2:thirdparty/support/$(TARGET_ARCH)/libs/libztcodec2.so 
include $(BUILD_MULTI_PREBUILT)

THIRD_INCLUDES += \
	$(LOCAL_PATH)/thirdparty/libjsonc/include

include $(CLEAR_VARS)

SRC := $(call all-named-files-under,*.cpp,src)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_ARM_NEON := true
EXTRA_CFLAGS += "-march=armv7-a -mfloat-abi-softfp -mfpu=neon"
EXTRA_LDFLAGS += "-Wl,--fix-cortex-a8"
THIRD_INCLUDES += $(LOCAL_PATH)/thirdparty/support/armeabi-v7a/include
endif

ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
EXTRA_LDFLAGS="-Wl"
THIRD_INCLUDES += $(LOCAL_PATH)/thirdparty/support/arm64-v8a/include
endif

LOCAL_SRC_FILES:= \
	$(SRC) \
	test.cpp

LOCAL_C_INCLUDES += \
		$(THIRD_INCLUDES) \
		$(LOCAL_PATH)/include

LOCAL_CFLAGS:= $(L_CFLAGS) -Wall -Werror -Wextra -std=gnu++11 $(EXTRA_CFLAGS)
LOCAL_MODULE:= cpp_test
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_STATIC_LIBRARIES += libjsonc_static

include $(BUILD_EXECUTABLE)

include $(wildcard $(LOCAL_PATH)/thirdparty/libjsonc/Android.mk)
