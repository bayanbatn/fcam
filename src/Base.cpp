#include <stdarg.h>
#include <stdio.h>

#include "FCam/Base.h"

namespace FCam {
    int bytesPerPixel(ImageFormat f) {
        switch(f) {
        case RGB24: case YUV24:
            return 3;
        case RGB16: case UYVY: case RAW: 
            return 2;
        // For planar formats, return 1 byte per pixel
        case YUV420p:   
            return 1;   
        default:
            return 0;
        }
    }

    void panic(const char* fileName, int line, const char *fmt, ...)  {
        char buf[256];
        va_list arglist;
        va_start(arglist, fmt);
        vsnprintf(buf, 256, fmt, arglist);
        va_end(arglist);
        fprintf(stderr, "  (Panic!) %s: %d: %s\n", fileName, line, buf);
        exit(1);
    }

}
