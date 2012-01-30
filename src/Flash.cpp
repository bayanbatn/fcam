#include "FCam/Flash.h"
#include "FCam/Action.h"
#include "FCam/Frame.h"

#include "Debug.h"


namespace FCam {
    Flash::~Flash() {}

    Flash::FireAction::FireAction(Flash *f) : flash(f) {
        latency = flash->fireLatency();        
        time = 0;
    }

    Flash::FireAction::FireAction(Flash *f, int t) :
        flash(f) {
        time = t;
        latency = flash->fireLatency();        
        brightness = flash->maxBrightness();
        duration = flash->minDuration();
    }

    Flash::FireAction::FireAction(Flash *f, int t, float b, int d) : 
        brightness(b), duration(d), flash(f) {
        time = t;
        latency = flash->fireLatency();
    }

    void Flash::FireAction::doAction() {
        flash->fire(brightness, duration);
    }

    /** Extract the tags placed on a frame by a flash */
    Flash::Tags::Tags(Frame f) {
        start      = f["flash.start"];
        duration   = f["flash.duration"];
        peak       = f["flash.peak"];
        brightness = f["flash.brightness"];
    }

}
