LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libjsonc_static
LOCAL_SRC_FILES := $(call all-c-files-under,src)
$(info --->$(LOCAL_SRC_FILES))
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/
LOCAL_CFLAGS := -Werror -Wall -Wno-error=deprecated-declarations -Wextra -Wwrite-strings -Wno-unused-parameter -std=gnu99 -D_GNU_SOURCE -D_REENTRANT
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libjsonc
LOCAL_WHOLE_STATIC_LIBRARIES := libjsonc_static
include $(BUILD_SHARED_LIBRARY)

