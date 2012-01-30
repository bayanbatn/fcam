#ifndef FCAM_DEMOSAIC_ARM_H
#define FCAM_DEMOSAIC_ARM_H
#ifdef FCAM_ARCH_ARM

#include <FCam/Base.h>
#include <FCam/Image.h>
#include <FCam/Frame.h>

// Arm-specific optimized post-processing routines

namespace FCam {
    // Assume the input is 5MP and makes a 640x480 output
    Image makeThumbnailRAW_ARM(Frame src, float contrast, int blackLevel, float gamma);    
    
    Image demosaic_ARM(Frame src, float contrast, bool denoise, int blackLevel, float gamma);
}

#endif
#endif
