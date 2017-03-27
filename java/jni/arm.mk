LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	com_rokid_server_rkspeech_RKSirenNative.cpp 

LOCAL_SHARED_LIBRARIES := \
    libandroid_runtime \
	libbinder \
	libnativehelper \
    libcutils \
    libutils \
    liblog \
    libhardware \
	libsiren \
	libeasyr2

LOCAL_MULTILIB := 32

LOCAL_C_INCLUDES += \
		robot/hardware/modules/siren/libsiren \
		robot/native/services/siren/libsiren \
		robot/easyr2/include \
		robot/easyrobot/include \
		robot/external/easyrobot/include
#LOCAL_CFLAGS += -O0 -g

LOCAL_MODULE := libspeech_siren
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
