#ifndef FCAM_TEGRA_STATISTICS_H
#define FCAM_TEGRA_STATISTICS_H

#include "FCam/FCam.h"

namespace FCam { namespace Tegra { 


    namespace Statistics {


        /* Returns the sharpness value over a given region using a subsample
           interval */
        int evaluateVariance(Image im, Rect region, int subsample);

        /* Returns the SharpnessMap over a given region using a subsample
           interval */
        SharpnessMap evaluateSharpness(SharpnessMapConfig mapCfg, Image im);

        /* Returns the YUV histogram for an image*/
        Histogram evaluateHistogram(const HistogramConfig& histoCfg, const Image& im);

        enum { 
            // The max number of samples to avoid overflow is 65535
            // (8bit ^ 2 -> 16bit) -> sum up to 64k of 16bit values without
            // overflow.
            MAX_VARIANCE_SAMPLES  = 32768,

            // Just an deliberate selection.
            MAX_HISTOGRAM_SAMPLES = 32768,
         };


    } // namespace Statistics

}}

#endif

