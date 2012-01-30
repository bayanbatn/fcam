#ifndef FCAM_TEGRA_H
#define FCAM_TEGRA_H

/** \file
 * Including this file includes all the necessary components for the Tegra implementation */

#include "FCam.h"

#include "Tegra/Sensor.h"
#include "Tegra/Flash.h"
#include "Tegra/Lens.h"
#include "Tegra/Frame.h"
#include "Tegra/Platform.h"

namespace FCam {
   /** The namespace for the NVIDIA Tegra platform.
     *
     * Summary of salient characteristics
     * - The Sensor class adds an index in the constructor to indicate
     *   which camera sensor we want to open. Currently 0 is the only
     *   supported value, and it will open the back-side camera.
     *   In the future, other values will enable  more cameras,
     *   such as the display-side camera and the stereo camera set.
     * - Added a Viewfinder device. The device is linked to an Android 
     *   framework SurfaceHolder and will enable hw accelerated display of
     *   frames on an Android app.
     * - The Flash on the current Tegra platform is synchronized with
     *   the shutter. Flash::fire will cause the next capture to
     *   trigger the flash for a hardcoded duration. The flash will
     *   fire on every capture until Flash::fire is called again 
     *   with a brightness of 0.
     * - The AutoFocus class has been adapted to allow for focus 
     *   stepping. AutoFocus::update  adds a Shot parameter to enable 
     *   the AutoFocus framework to add/clear Actions on the shot.
     * - RAW images can be saved as DNG. However, please ignore any
     *   matrices that are embedded in the DNG file.
     *   
     */

    namespace Tegra {
    }
}

#endif
