#ifndef _COMMON_H
#define _COMMON_H

#include <android/log.h>

#define MODULE "fcam_iface"
// Use LOG and ERROR like printf.
#define LOG(x,...) __android_log_print(ANDROID_LOG_DEBUG,MODULE,x,##__VA_ARGS__)
#define ERROR(x,...) __android_log_print(ANDROID_LOG_ERROR,MODULE,x,##__VA_ARGS__)
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

#endif

