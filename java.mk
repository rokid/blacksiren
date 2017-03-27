LOCAL_PATH := $(call my-dir)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	$(call all-java-files-under, java)

LOCAL_MODULE := blacksiren
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_NO_STANDARD_LIBRARIES := true

LOCAL_DX_FLAGS := --core_library --multi-dex
LOCAL_JACK_FLAGS := --multi-dex native

LOCAL_JAR_PACKAGES := java\com\*

include $(BUILD_JAVA_LIBRARY)
include $(wildcard $(LOCAL_PATH)/java/jni/Android.mk)

