#ifndef FCAM_DUMP_H
#define FCAM_DUMP_H

#include "../Frame.h"
#include <string>

/** \file 
 * Loading and saving files as raw data dumps.
 */

namespace FCam {
    
    /** Save a UYVY, RGB24, or RAW dump file. 
     * For saving uncompressed image data in a very basic image format:
     *  Header: 5 4-byte integers representing frames, width, height, channels, and type.
     *  Frames will always be 1. Type is either:
     *
     *  - 2 = 8-bit data, such as UYVY or RGB24 data.
     *  - 4 = 16-bit, such as RAW sensor pixel values, in a Bayer mosaic.
     *
     *  Channels will be 3 for RGB24, 2 for UYVY, 1 for RAW data.
     */

    void saveDump(Frame frame, std::string filename);
    void saveDump(Image frame, std::string filename);

    /** Load a UYVY, RGB24, or RAW dump file. */
    Image loadDump(std::string filename);

}

#endif
