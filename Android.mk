LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := daemon_c
LOCAL_SRC_FILES := daemon.c

#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_EXECUTABLE)    # Use this to build an executable.