
#ifndef FCAM_AUTOFOCUS_H
#define FCAM_AUTOFOCUS_H

#include <vector>
#include "Frame.h"

//! \file
//! Convenience functions for performing autofocus

namespace FCam {

    class Lens;

    /** This class does autofocus, by sweeping the sensor back and
        forth until a nice thing to focus on is found. You call \ref
        startSweep, then feed it frames using \ref update, until \ref
        focused returns true. */
    class AutoFocus {
    public:
        /** Construct an AutoFocus helper object that uses the
         * specified lens, and attempts to bring the target rectangle
         * into focus. By default, the target rectangle is the entire
         * region covered by the sharpness map of the frames that come
         * in. */
        AutoFocus(FCam::Lens *l, FCam::Rect r = FCam::Rect());

        virtual ~AutoFocus(){};

        /** Start the autofocus routine. Returns immediately. */
        virtual void startSweep();

        /** Feed the autofocus routine a new frame. It will have its
         * sharpness map examined, and the autofocus will react
         * appropriately. This function will ignore frames that aren't
         * tagged by the lens this object was constructed with, and
         * will also ignore frames with no sharpnes map.
         * When the lens minFocusSpeed doesn't allow for a focus
         * in one second, the autofocus algorithm will step through
         * the focus positions. Pass a shot to add the focus action. */
        virtual void update(const FCam::Frame &f, FCam::Shot *s = NULL);

        /** Is the lens currently focused? */
        bool focused() {return state == FOCUSED;}

        /** Is the lens currently focused, or ready to begin focusing? */
        bool idle() {return state == FOCUSED || state == IDLE;}

        /** Set the target on the frame to look at in pixels. Don't
         * change this in the middle of a sweep. */
        void setTarget(Rect r) {rect = r;}

    protected:
        Lens *lens;

        struct Stats {
            float position;
            int sharpness;
        };
        std::vector<Stats> stats;
        enum {IDLE = 0, HOMING, SWEEPING, STEPPING, SETTING, FOCUSED} state;

        Rect rect;
    };

}

#endif
