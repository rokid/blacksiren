LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	com_rokid_openVoice_BlackSiren.cpp 

LOCAL_SHARED_LIBRARIES := \
    libandroid_runtime \
	libbinder \
	libnativehelper \
    libcutils \
    libutils \
    liblog \
    libhardware \
	libbsiren 

LOCAL_C_INCLUDES += \
		$(LOCAL_PATH)/../../include \


LOCAL_MODULE := libbsiren_jni
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
