LOCAL_PATH:= $(call my-dir)
CURRENT_PATH := $(LOCAL_PATH)

include $(LOCAL_PATH)/android.config
L_CFLAGS := -DANDROID -DANDROID_LOG_TAG=\"BlackSiren\"

ifdef CONFIG_ANDROID_LOG
L_CFLAGS += -DCONFIG_ANDROID_LOG
endif

ifdef CONFIG_LEGACY_SIREN_TEST
L_CFLAGS += -DCONFIG_LEGACY_SIREN_TEST
endif

ifdef CONFIG_USE_FIFO
L_CFLAGS += -DCONFIG_USE_FIFO
endif

ifdef CONFIG_DEBUG_CHANNEL
L_CFLAGS += -DCONFIG_DEBUG_CHANNEL
endif

ifdef CONFIG_RECORDING_PATH
L_CFLAGS += -DCONFIG_RECORDING_PATH=\"$(CONFIG_RECORDING_PATH)\"
endif

ifdef CONFIG_RECORDING_MIC_ARRAY
L_CFLAGS += -DCONFIG_RECORDING_MIC_ARRAY 
endif

ifdef CONFIG_RECORDING_PROCESSED_DATA
L_CFLAGS += -DCONFIG_RECORDING_PROCESSED_DATA
endif

ifdef CONFIG_BACKUP_FILE_PATH
L_CFLAGS += -DCONFIG_BACKUP_FILE_PATH=\"$(CONFIG_BACKUP_FILE_PATH)\"
endif

ifdef CONFIG_STORE_FILE_PATH
L_CFLAGS += -DCONFIG_STORE_FILE_PATH=\"$(CONFIG_STORE_FILE_PATH)\"
endif

ifdef CONFIG_REMOTE_CONFIG_FILE_URL
L_CFLAGS += -DCONFIG_REMOTE_CONFIG_FILE_URL=\"$(CONFIG_REMOTE_CONFIG_FILE_URL)\"
endif

ifdef CONFIG_MONITOR_UDP_PORT
L_CFLAGS += -DCONFIG_MONITOR_UDP_PORT=$(CONFIG_MONITOR_UDP_PORT)
endif

$(shell mkdir -p $(TARGET_OUT_ETC))
$(shell mkdir -p $(TARGET_OUT))
$(shell cp $(LOCAL_PATH)/resource/blacksiren.json $(TARGET_OUT_ETC))
$(shell cp -r $(LOCAL_PATH)/resource/en $(TARGET_OUT)/workdir_en)
$(shell cp -r $(LOCAL_PATH)/resource/cn $(TARGET_OUT)/workdir_cn)

$(info $(TARGET_DEVICE))
include $(wildcard $(LOCAL_PATH)/thirdparty/libjsonc/Android.mk)
LOCAL_PATH := $(CURRENT_PATH)

ifneq (,$(ONE_SHOT_MAKEFILE))
$(info ONE SHOT MAKEFILE!!!!)
include $(wildcard external/curl/Android.mk)
LOCAL_PATH := $(CURRENT_PATH)
endif

#include $(CLEAR_VARS)
#LOCAL_PREBUILT_LIBS := \
#		libr2ssp:thirdparty/support/$(TARGET_ARCH)/libs/libr2ssp.so \
#		libztvad:thirdparty/support/$(TARGET_ARCH)/libs/libztvad.so \
#		libztcodec2:thirdparty/support/$(TARGET_ARCH)/libs/libztcodec2.so \
#		libr2audio:thirdparty/support/$(TARGET_ARCH)/libs/legacy/libr2audio.so \
#		libr2vt:thirdparty/support/$(TARGET_ARCH)/libs/legacy/libr2vt.so
#include $(BUILD_MULTI_PREBUILT)

THIRD_INCLUDES += \
	$(LOCAL_PATH)/thirdparty/libjsonc/include \
	external/curl/include 

include $(CLEAR_VARS)


SRC := $(call all-named-files-under,*.cpp,src) 

LOCAL_SRC_FILES:= \
	$(SRC) 

LOCAL_C_INCLUDES += \
		$(THIRD_INCLUDES) \
		$(LOCAL_PATH)/include 

LOCAL_CFLAGS:= $(L_CFLAGS) -Wall -Wextra -Werror -std=gnu++11
LOCAL_MODULE:= libbsiren
LOCAL_SHARED_LIBRARIES := liblog libr2ssp libztvad libztcodec2 libr2vt libr2audio libcurl 
LOCAL_STATIC_LIBRARIES += libjsonc_static

include $(BUILD_SHARED_LIBRARY)

#ifeq (,$(ONE_SHOT_MAKEFILE))
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	test.cpp

LOCAL_C_INCLUDES += \
		$(LOCAL_PATH)/include \
		$(LOCAL_PATH)/../../hardware/include \
		external/curl/include

LOCAL_MODULE := test
LOCAL_SHARED_LIBRARIES := libbsiren libhardware libcurl
LOCAL_STATIC_LIBRARIES := libjsonc_static
include $(BUILD_EXECUTABLE)
#endif
include $(wildcard $(LOCAL_PATH)/java/jni/Android.mk)
