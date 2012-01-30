#ifndef FCAM_JPEG_H
#define FCAM_JPEG_H

/** \file
 * Loading and Saving JPEGs */

#include "../Frame.h"

namespace FCam {

    /** Save a JPEG file. The frame must have an image in RAW, UYVY,
     * YUV420p or RGB format.  The frame and frame.sensor are queried for all
     * relevant data to put in the EXIF tags. You can also pass in a
     * jpeg quality level between 0 and 100.
     */
    void saveJPEG(Frame, std::string filename, int quality=80);       

    /** Save an image as a JPEG file. The image must be in UYVY or RGB
     * format. No EXIF tags are saved. You can also pass in a jpeg
     * quality level between 0 and 100. */
    void saveJPEG(Image, std::string filename, int quality=80); 
}

#endif
