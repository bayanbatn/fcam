LOCAL_PATH := $(call my-dir)

# Define the example build instructions
include $(CLEAR_VARS)
TARGET_ARCH_ABI 	:= armeabi-v7a
LOCAL_MODULE 		:= fcam_example6

LOCAL_CFLAGS 		+= -DFCAM_PLATFORM_ANDROID

LOCAL_SRC_FILES 	:= ../example6.cpp
LOCAL_SRC_FILES     += ../SoundPlayer.cpp
LOCAL_SRC_FILES		+= com_nvidia_fcam_example6_Example.cpp

LOCAL_STATIC_LIBRARIES  += fcamlib libjpeg
LOCAL_SHARED_LIBRARIES  += fcamhal

# for native audio
LOCAL_LDLIBS    += -lOpenSLES
# for logging
LOCAL_LDLIBS    += -llog
# for native asset manager
LOCAL_LDLIBS    += -landroid

include $(BUILD_SHARED_LIBRARY)
$(call import-module,fcam)