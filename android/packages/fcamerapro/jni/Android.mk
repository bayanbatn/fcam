LOCAL_PATH := $(call my-dir)

# Define the example build instructions
include $(CLEAR_VARS)

LOCAL_MODULE      := fcam_iface
LOCAL_ARM_MODE    := arm
TARGET_ARCH_ABI   := armeabi-v7a

ifeq ($(NDK_DEBUG),1)
  LOCAL_CFLAGS      += -DDEBUG
else
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  LOCAL_ARM_NEON  := true
endif
endif

LOCAL_CFLAGS 		+= -DFCAM_PLATFORM_ANDROID

LOCAL_SRC_FILES     := $(wildcard *.cpp)
LOCAL_SRC_FILES     += $(wildcard *.c)

LOCAL_STATIC_LIBRARIES += fcamlib libjpeg
LOCAL_SHARED_LIBRARIES += fcamhal
LOCAL_LDLIBS           += -llog -lGLESv2 -lEGL

include $(BUILD_SHARED_LIBRARY)

$(call import-module,fcam)
