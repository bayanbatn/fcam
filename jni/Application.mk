#
# FCam library Application.mk
#

#APP_BUILD_SCRIPT
#    By default, the NDK build system will look for a file named Android.mk
#    under $(APP_PROJECT_PATH)/jni, i.e. for the file:
#
#       $(APP_PROJECT_PATH)/jni/Android.mk
#
#    If you want to override this behaviour, you can define APP_BUILD_SCRIPT
#    to point to an alternate build script. A non-absolute path will always
#    be interpreted as relative to the NDK's top-level directory.

APP_BUILD_SCRIPT=$(NDK_PROJECT_PATH)/Android.mk

#APP_ABI
#    By default, the NDK build system will generate machine code for the
#    'armeabi' ABI. This corresponds to an ARMv5TE based CPU with software
#    floating point operations. You can use APP_ABI to select a different
#    ABI.
#
#    For example, to support hardware FPU instructions on ARMv7 based devices,
#    use:
APP_ABI := armeabi-v7a

#APP_STL
#    By default, the NDK build system provides C++ headers for the minimal
#    C++ runtime library (/system/lib/libstdc++.so) provided by the Android
#    system.
#
#    However, the NDK comes with alternative C++ implementations that you can
#    use or link to in your own applications. Define APP_STL to select one of
#    them. Examples are:
#
#       APP_STL := stlport_static    --> static STLport library
#       APP_STL := stlport_shared    --> shared STLport library
#       APP_STL := system            --> default C++ runtime library

APP_STL := gnustl_static

APP_MODULES := fcamlib libjpeg

APP_PLATFORM := android-9
