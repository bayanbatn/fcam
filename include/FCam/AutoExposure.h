#ifndef FCAM_AUTO_EXPOSURE_H
#define FCAM_AUTO_EXPOSURE_H

#include "Frame.h"

/** \file
 * Utility algorithms for metering
 */

namespace FCam {

    class Shot;

    /** Given a shot pointer and a frame, modify the shot parameters
     * to make it better exposed, given the histogram data present in
     * that frame. Other parameters are the maximum allowable gain,
     * the maximum allowable exposure, and how smooth changes should
     * be. 0 makes instant changes (and could even oscillate out of
     * control. 1 never changes at all. 0.5 is recommended.
     *
     * If the algorithm needs to increase brightness, it does so in
     * this order.
     *
     * \li Increase the exposure time up to just less than the frame time of the given shot
     * \li Increase the gain up to the maximum gain
     * \li Increase the exposure time up to the maximum exposure time
     *
     * If the algorithm needs to decrease brightness, it does so in
     * the reverse order.
     */
    void autoExpose(Shot *s, const Frame &f,
                    float maxGain = 32.0f,     // ISO 3200
                    int maxExposure = 125000, // 8 fps
                    int minExposure = 500,    // 1/2000 s
                    float smoothness = 0.5);
}

#endif

