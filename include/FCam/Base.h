#ifndef FCAM_BASE_H
#define FCAM_BASE_H

#include <stdlib.h>

/*! \file
 * Basic types and namespace for FCam API.
 *
 * This file declares some basic FCam types, utility functions, and enums
 */  

namespace FCam {
  
    /** The various image formats supported by FCam */
    typedef enum {
        /** 24 bit per pixel RGB, with 8 bits for each channel */
        RGB24 = 0, 

        /** 16 bit per pixel RGB, broken down into 5 bits for R, 6 for
         * G, and 5 for B  */
        RGB16,     
        
        /** 16 bit per pixel UYVY. The even numbered bytes alternate
         * between U and V (chrominance), and the odd numbered bytes
         * are Y (luminance). This is usually the best format for
         * blitting to overlays, and is also typically the best format
         * if you just want grayscale data for computer vision
         * applications (in which case, just ignore the even-numbered
         * bytes). */
        UYVY,      
        
        /** 24 bit per pixel YUV, with 8 bits for each channels */
        YUV24, 

        /** YUV420p = planar YUV. For each 2x2 block in the image 
         * there are 4 Y samples but all 4 pixels share the same U and V sample
         * Memory storage: Y widthxheigh plane followed by U width/2 x height/2 plane
         * followed by V width/2 x height/2 plane.
         */
        YUV420p, 

        /** 16 bit per pixel raw sensor data. Call \ref
         * Sensor::platform()::bayerPattern or \ref
         * Frame::platform()::bayerPattern for the correct
         * interpretation. Most sensors are not natively 16-bit, so
         * the high 4-6 bits will commonly be zero (for 12 or 10 bit
         * sensors respectively).*/
        RAW,
        
        /** An unknown or invalid format. Also acts as a sentinel, so
         * this must be the last entry in the enum. */
        UNKNOWN} ImageFormat;

    /** The various types of bayer pattern. Listed as the colors of
     *  the top 2x2 block of pixels in scanline order. Non-bayer
     *  sensors should return NotBayer. */
    enum BayerPattern {RGGB = 0, //!< Red in top left corner
                       BGGR,     //!< Blue in top left corner
                       GRBG,     //!< Green in top left corner and first row is green/red
                       GBRG,     //!< Green in top left corner and first row is green/blue
                       NotBayer  //!< Not a bayer-patterened sensor
    };

    /** Various color space definitions. Used to indicate what 
     *  color space the channels of a histogram are in. */
    enum ColorSpace {RGB    = 0,  //!< RGB non-gamma corrected.
                     sRGB,        //!< Gamma corrected RGB
                     YUV,         //!< Linear YUV
                     YpUV         //!< Gamma corrected YUV
    };

    /** How many bytes per pixel are used by a given format */
    int bytesPerPixel(ImageFormat);
    
    /** A class to represent sizes of two dimensional objects like
     * images. */
    struct Size {
        /** Construct a size with width and height of zero. */
        Size() : width(0), height(0) {}

        /** Construct a size of the given width and height. */
        Size(int w, int h) : width(w), height(h) {}

        /** Compare two sizes for equality */
        bool operator==(const Size &other) const {return width == other.width && height == other.height;}

        /** Compare two sizes for equality */
        bool operator!=(const Size &other) const {return width != other.width || height != other.height;}

        int width; //!< The width as an int.
        int height; //!< The height as an int.
    };
    
    /** A class to represent rectangles, like regions of an image. */
    struct Rect {
        /** Construct a size zero rectangle at the origin */
        Rect() : x(0), y(0), width(0), height(0) {}

        /** Construct a rectangle with the given x, y, width, and
         * height */
        Rect(int x_, int y_, int w_, int h_) : x(x_), y(y_), width(w_), height(h_) {}

        /** Compare two rectangles for equality */
        bool operator==(const Rect &other) const {
            return (width == other.width &&
                    height == other.height &&
                    x == other.x &&
                    y == other.y);
        }

        /** Compare two rectangles for equality */
        bool operator!=(const Rect &other) const {
            return (width != other.width ||
                    height != other.height ||
                    x != other.x ||
                    y != other.y);
        }
        
        int x;      //!< The x coord of the top left corner
        int y;      //!< The y coord of the top left corner
        int width;  //!< The width of the rectangle
        int height; //!< The height of the rectangle
    };

    /** For lowest-level irrecoverable problems, the panic message simply dumps the error message
        to the terminal and exits immediately */
#define fcamPanic(fmt, ...) panic(__FILE__, __LINE__, fmt, __VA_ARGS__)
    void panic(const char* fileName, int line, const char *fmt, ...);

}

#endif
