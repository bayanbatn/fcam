#ifndef FCAM_DEBUG
#define FCAM_DEBUG
/** \file 
 * Basic debugging utility macros. These cannot be placed in
 * the FCam namespace, being macros, so this header is not included by
 * default by any part of the FCam public interface. Feel free to
 * include it if you wish to use it, but be aware of the possibility
 * of namespace pollution. To enable debugging, define DEBUG before including this header.
 */

#include <stdio.h>
#include <stdarg.h>

#if defined(FCAM_PLATFORM_ANDROID)
#if !defined(LOG_TAG) 
#define LOG_TAG "FCAM"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__) 
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , LOG_TAG, __VA_ARGS__) 
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , LOG_TAG, __VA_ARGS__) 
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , LOG_TAG, __VA_ARGS__) 
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , LOG_TAG, __VA_ARGS__) 
#endif
#include <android/log.h>
#endif


#if defined(DEBUG) && !defined(FCAM_DEBUG_LEVEL)
/** Debugging verbosity level. Larger numbers mean more debugging output. 
 * Our convention is:
 * 0: Error conditions also get printed.
 * 1: Warnings and unusual conditions get printed.
 * 2: Major normal events like mode switches get printed.
 * 3: Minor normal events get printed. 
 * 4+: Varying levels of trace */
#define FCAM_DEBUG_LEVEL 0
#endif

/** Encoding of the error levels described above. Use these for the
 * dprintf level argument, and then set FCAM_DEBUG_LEVEL to the
 * desired runtime verbosity. */
enum debugLevel { DBG_ERROR=0,
                  DBG_WARN=1,
                  DBG_MAJOR=2,
                  DBG_MINOR=3 };

/** Stringify number-valued preprocessor defines like __LINE__ */
#define STRX(x) #x
#define STR(x) STRX(x)

#if defined(DEBUG)
/** Raw printf-like debugging fuction. It has a verbosity
 * level(lower=more verbose), controlled by FCAM_DEBUG_LEVEL, and a
 * source string as arguments, beyond the usual printf-style
 * fields. */
inline void _dprintf(int level, const char *src, const char *fmt, ...) {
    if (level <= FCAM_DEBUG_LEVEL) {

        char buf[512];

        va_list arglist;
        va_start(arglist, fmt);
        vsnprintf(buf, 512, fmt, arglist);
        va_end(arglist);

/** On Android platforms, send debug msg to console and log */
#if defined(FCAM_PLATFORM_ANDROID)
        LOGD("%*c(%d) %s: %s",level*2+2,' ',level,src, buf);
#endif
        fprintf(stderr, "%*c(%d) %s: %s",level*2+2,' ',level,src, buf);
    }
}

#else
/** Raw printf-like debugging fuction. It has a verbosity
 * level(lower=more verbose), controlled by FCAM_DEBUG_LEVEL, and a
 * source string as arguments, beyond the usual printf-style
 * fields. An empty function if DEBUG is not defined. */
inline void _dprintf(int, const char *, const char *, ...) {}
#endif

/** Printf-like debugging macro, which fills the source argument of _dprintf using the file name and line number. */
#define dprintf(level, ...) _dprintf(level, __FILE__ ":" STR(__LINE__), __VA_ARGS__) 

#endif
