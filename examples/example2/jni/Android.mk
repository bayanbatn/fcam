LOCAL_PATH := $(call my-dir)

# Define the example build instructions
include $(CLEAR_VARS)
TARGET_ARCH_ABI 	:= armeabi-v7a
LOCAL_MODULE 		:= fcam_example2

LOCAL_CFLAGS 		+= -DFCAM_PLATFORM_ANDROID

LOCAL_SRC_FILES 	:= ../example2.cpp
LOCAL_SRC_FILES		+= com_nvidia_fcam_example2_Example.cpp

LOCAL_STATIC_LIBRARIES  += fcamlib libjpeg
LOCAL_SHARED_LIBRARIES  += fcamhal

include $(BUILD_SHARED_LIBRARY)
$(call import-module,fcam)