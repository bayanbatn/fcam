#ifndef FCAM_TIFFPROCESSING_H
#define FCAM_TIFFPROCESING_H

/** \file
 * Loading and Saving TIFFs */

#include "../Frame.h"

namespace FCam {

    /** Save a TIFF file. The frame must have an image in RGB24 format.
     */
    void saveTIFF(Frame, std::string filename);

    /** Save an image as a TIFF file. */
    void saveTIFF(Image, std::string filename); 
}

#endif
