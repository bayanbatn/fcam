#
# FCam lbrary Android.mk
#
LOCAL_PATH := $(call my-dir)



ifeq ($(NDK_DEBUG),1)
  PREBUILT_DIR := debug
else
  PREBUILT_DIR := release
endif

# First - define the prebuilt hal library module
include $(CLEAR_VARS)
LOCAL_MODULE := fcamhal
LOCAL_SRC_FILES := prebuilt/$(PREBUILT_DIR)/lib/armeabi-v7a/libFCamTegraHal.so
include $(PREBUILT_SHARED_LIBRARY)

# The FCam module
include $(CLEAR_VARS)
LOCAL_MODULE := fcamlib
LOCAL_MODULE_FILENAME := libFCam

# Export variables
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_EXPORT_LDLIBS := -llog
# For some reason, the NDK ignores these completly, so
# you need to add these libraries in the module that
# includes fcam.
LOCAL_EXPORT_SHARED_LIBRARIES := fcamhal
LOCAL_EXPORT_STATIC_LIBRARIES := libjpeg

# A define that indicates we are building on Android.
LOCAL_CFLAGS += -DFCAM_PLATFORM_ANDROID
LOCAL_ARM_MODE := arm

#Set FCAM debug level 
# * Our convention is:
# * 0: Error conditions also get printed.
# * 1: Warnings and unusual conditions get printed.
# * 2: Major normal events like mode switches get printed.
# * 3: Minor normal events get printed. 
# * 4+: Varying levels of trace */

ifeq ($(NDK_DEBUG),1)
  LOCAL_CFLAGS += -DFCAM_DEBUG_LEVEL=4
  LOCAL_CFLAGS += -DDEBUG
else
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  LOCAL_ARM_NEON  := true
endif
endif

BUILD_FROM_SRC := $(strip $(PREBUILD))
BUILD_FCAM_FROM_SRC := $(BUILD_FROM_SRC)
BUILD_JPEG_FROM_SRC := $(BUILD_FROM_SRC)

ifneq ($(BUILD_FROM_SRC),1)
  ifeq (,$(strip $(wildcard $(LOCAL_PATH)/prebuilt/$(PREBUILT_DIR)/lib/armeabi-v7a/libFCam.a)))
    $(call __ndk_info,WARNING: Rebuilding FCam library from sources!)
    $(call __ndk_info,You might want to run $(LOCAL_PATH)/prebuild-$(PREBUILT_DIR).sh)
    $(call __ndk_info,in order to build a prebuilt version to speed up your builds!)
    BUILD_FCAM_FROM_SRC := 1
  endif
  ifeq (,$(strip $(wildcard $(LOCAL_PATH)/prebuilt/$(PREBUILT_DIR)/lib/armeabi-v7a/libjpeg.a)))
    $(call __ndk_info,WARNING: Rebuilding jpeg library from sources!)
    $(call __ndk_info,You might want to run $(LOCAL_PATH)/prebuild-$(PREBUILT_DIR).sh)
    $(call __ndk_info,in order to build a prebuilt version to speed up your builds!)
    BUILD_JPEG_FROM_SRC := 1
  endif
endif

ifneq ($(BUILD_FCAM_FROM_SRC),1)

$(call __ndk_info,Using prebuilt FCam library)

LOCAL_MODULE := fcamlib
LOCAL_MODULE_FILENAME := libFCam
LOCAL_SRC_FILES := prebuilt/$(PREBUILT_DIR)/lib/armeabi-v7a/libFCam.a
include $(PREBUILT_STATIC_LIBRARY)

else

$(call __ndk_info,Rebuilding FCam libraries from sources)

LOCAL_SRC_FILES :=
LOCAL_SRC_FILES += src/Action.cpp src/AutoExposure.cpp src/AutoFocus.cpp src/AutoWhiteBalance.cpp src/AsyncFile.cpp 
LOCAL_SRC_FILES += src/Base.cpp src/Device.cpp src/Event.cpp src/Flash.cpp src/Frame.cpp src/Image.cpp 
LOCAL_SRC_FILES += src/Lens.cpp src/Shot.cpp src/Sensor.cpp src/Time.cpp src/TagValue.cpp 
LOCAL_SRC_FILES += src/processing/DNG.cpp src/processing/TIFF.cpp src/processing/TIFFTags.cpp
LOCAL_SRC_FILES += src/processing/Dump.cpp src/processing/JPEG.cpp src/processing/Demosaic.cpp src/processing/Color.cpp

# FCam Tegra files
LOCAL_SRC_FILES += src/Tegra/AutoFocus.cpp src/Tegra/Shot.cpp
LOCAL_SRC_FILES += src/Tegra/Platform.cpp src/Tegra/Sensor.cpp src/Tegra/Frame.cpp
LOCAL_SRC_FILES += src/Tegra/Statistics.cpp
LOCAL_SRC_FILES += src/Tegra/Lens.cpp src/Tegra/Flash.cpp
LOCAL_SRC_FILES += src/Tegra/Daemon.cpp src/Tegra/YUV420.cpp

LOCAL_C_INCLUDES += 
LOCAL_C_INCLUDES += $(LOCAL_PATH)/external/libjpeg
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include

include $(BUILD_STATIC_LIBRARY)

endif

## JPEG module
ifneq ($(BUILD_JPEG_FROM_SRC),1)

$(call __ndk_info,Using prebuilt jpeg library)

include $(CLEAR_VARS)
LOCAL_MODULE := libjpeg
LOCAL_MODULE_FILENAME := libjpeg
LOCAL_SRC_FILES := prebuilt/$(PREBUILT_DIR)/lib/armeabi-v7a/libjpeg.a
include $(PREBUILT_STATIC_LIBRARY)

else
$(call import-module, fcam/external/libjpeg)
endif