#ifndef FCAM_AUTO_WHITE_BALANCE_H
#define FCAM_AUTO_WHITE_BALANCE_H

#include "Frame.h"

/** \file
 * Utility algorithms for white balance
 */

namespace FCam {

    class Shot;   

    /** Given a shot pointer and a frame, modify the shot parameters
     * to make it better white-balanced, given the histogram data
     * present in that frame. Other parameters are the minimum and
     * maximum allowable white balance, and how smooth changes should
     * be. 0 makes instant changes. 1 never changes at all. 0.5 is
     * recommended.
     *
     */
    void autoWhiteBalance(Shot *s, const Frame &f, 
                          int minWB = 3200,
                          int maxWB = 7000,
                          float smoothness = 0.5);
}

#endif
