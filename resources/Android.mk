LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

$(shell mkdir -p $(TARGET_OUT_ETC))
$(shell mkdir -p $(TARGET_OUT))

$(shell cp $(LOCAL_PATH)/blacksiren.json $(TARGET_OUT_ETC))
$(shell cp -r $(LOCAL_PATH)/en $(TARGET_OUT)/workdir_en)
$(shell cp -r $(LOCAL_PATH)/cn $(TARGET_OUT)/workdir_cn)

