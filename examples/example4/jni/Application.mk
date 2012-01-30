#
# FCam example Application.mk
#


#APP_MODULES
#    This variable is optional. If not defined, the NDK will build by
#    default _all_ the modules declared by your Android.mk, and any
#    sub-makefile it may include.
#
#    If APP_MODULES is defined, it must be a space-separated list of module
#    names as they appear in the LOCAL_MODULE definitions of Android.mk
#    files. Note that the NDK will compute module dependencies automatically.
#
#    NOTE: This variable's behaviour changed in NDK r4. Before that:
#
#      - the variable was mandatory in your Application.mk
#      - all required modules had to be listed explicitly.


# Required for FCam applications:
#    1. fcamhal
#    2. The shared lib module name that contains the 
#    native FCam application code (fcam_example5 in this case)

APP_MODULES := fcam_example4 fcamhal

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

APP_PLATFORM := android-9
