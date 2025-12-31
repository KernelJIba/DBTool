LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := DBTool-v1.sh
LOCAL_SRC_FILES := main.cpp
LOCAL_LDLIBS := -llog -lm
LOCAL_CPPFLAGS := -std=c++11 -finput-charset=utf-8 -fexec-charset=utf-8
LOCAL_MODULE_CLASS := EXECUTABLES

include $(BUILD_EXECUTABLE)
