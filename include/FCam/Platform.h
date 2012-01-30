#ifndef FCAM_PLATFORM
#define FCAM_PLATFORM

#include <string>
#include "FCam/Base.h"

/** \file 
 * The abstract base class for static platform data.
 */

namespace FCam {

    /** The abstract base class for static platform data. */
    class Platform {
    public:
        /** Get the bayer pattern of this sensor when in raw mode. */
        virtual BayerPattern bayerPattern() const = 0;

        /** Produce a 3x4 affine matrix that maps from sensor RGB to
         * linear-luminance sRGB at the given white balance. Given in
         * row-major order. */
        virtual void rawToRGBColorMatrix(int kelvin, float *matrix) const = 0;

        /** The camera's manufacturer. (e.g. Canon). */
        virtual const std::string &manufacturer() const = 0;

        /** The camera's model. Should also include manufacturer (e.g. Canon 400D). */
        virtual const std::string &model() const = 0;

        /** The smallest value to expect when in raw mode. */
        virtual unsigned short minRawValue() const = 0;

        /** The largest value to expect when in raw mode. */
        virtual unsigned short maxRawValue() const = 0; 

        virtual ~Platform(){}
    };
}

#endif
