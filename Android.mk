LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libjsonc_static
LOCAL_SRC_FILES :=	\
	src/arraylist.c \
	src/debug.c \
	src/json_c_version.c \
	src/json_object.c \
	src/json_object_iterator.c \
	src/json_tokener.c \
	src/json_util.c \
	src/libjson.c \
	src/linkhash.c \
	src/printbuf.c \
	src/random_seed.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/
LOCAL_CFLAGS := -Werror -Wall -Wno-error=deprecated-declarations -Wextra -Wwrite-strings -Wno-unused-parameter -std=gnu99 -D_GNU_SOURCE -D_REENTRANT
include $(BUILD_STATIC_LIBRARY)

