LOCAL_PATH:= $(call my-dir)

include $(LOCAL_PATH)/android.config
L_CFLAGS := -DANDROID -DANDROID_LOG_TAG=\"BlackSiren\"

ifdef CONFIG_ANDROID_LOG
L_CFLAGS += -DCONFIG_ANDROID_LOG
endif

THIRD_INCLUDES := \
	$(LOCAL_PATH)/thirdparty/libjsonc/include

include $(CLEAR_VARS)

SRC := $(call all-named-files-under,*.cpp,src)

LOCAL_SRC_FILES:= \
	$(SRC) \
	test.cpp

LOCAL_C_INCLUDES += \
		$(THIRD_INCLUDES) \
		$(LOCAL_PATH)/include

LOCAL_CFLAGS:= $(L_CFLAGS) -Wall -Werror -Wextra -std=gnu++11
LOCAL_MODULE:= cpp_test
LOCAL_SHARED_LIBRARIES := liblog libjsonc

include $(BUILD_EXECUTABLE)

include $(wildcard $(LOCAL_PATH)/thirdparty/libjsonc/Android.mk)
