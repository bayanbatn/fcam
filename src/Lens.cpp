#include <stdlib.h>

#include "FCam/Lens.h"
#include "FCam/Frame.h"

#include "Debug.h"

namespace FCam {

    Lens::FocusAction::FocusAction(Lens *l) :
        lens(l) {
        focus = 0.0f; 
        speed = lens->maxFocusSpeed();
        latency = lens->focusLatency();
        time = 0;
    }

    Lens::FocusAction::FocusAction(Lens *l, int t, float f) :
        focus(f), lens(l) {
        speed = lens->maxFocusSpeed();
        latency = lens->focusLatency();
        time = t;
    }

    Lens::FocusAction::FocusAction(Lens *l, int t, float f, float s) :
        focus(f), speed(s), lens(l) {
        latency = lens->focusLatency();
        time = t;
    }

    void Lens::FocusAction::doAction() {
       lens->setFocus(focus, speed);
    }

    Lens::FocusSteppingAction::FocusSteppingAction(Lens *l) :
        lens(l) {
        step  = 0.0f; 
        speed = lens->maxFocusSpeed();
        latency = lens->focusLatency();
        time = 0;
        repeat = 1;
    }

    Lens::FocusSteppingAction::FocusSteppingAction(Lens *l, int r, int t, float f) :
        step(f), repeat(r), lens(l) {
        speed = lens->maxFocusSpeed();
        latency = lens->focusLatency();
        time = t;
    }

    Lens::FocusSteppingAction::FocusSteppingAction(Lens *l, int r, int t, float f, float s) :
        step(f), speed(s), repeat(r), lens(l) {
        latency = lens->focusLatency();
        time = t;
    }

    void Lens::FocusSteppingAction::doAction() {
        if (repeat > 0 && !lens->focusChanging()) {
            float current = lens->getFocus();
            lens->setFocus(step + current, speed);
            repeat--;
        }
    }

    Lens::ZoomAction::ZoomAction(Lens *l) :
        lens(l) {
        zoom = 0.0f; 
        speed = lens->maxZoomSpeed();
        latency = lens->zoomLatency();
        time = 0;
    }

    Lens::ZoomAction::ZoomAction(Lens *l, int t, float f) :
        zoom(f), lens(l) {
        speed = lens->maxZoomSpeed();
        latency = lens->zoomLatency();
        time = t;
    }

    Lens::ZoomAction::ZoomAction(Lens *l, int t, float f, float s) :
        zoom(f), speed(s), lens(l) {
        latency = lens->zoomLatency();
        time = t;
    }

    void Lens::ZoomAction::doAction() {
        lens->setZoom(zoom, speed);
    }


    Lens::ApertureAction::ApertureAction(Lens *l) :
        lens(l) {
        aperture = 0.0f; 
        speed = lens->maxApertureSpeed();
        latency = lens->apertureLatency();
        time = 0;
    }

    Lens::ApertureAction::ApertureAction(Lens *l, int t, float f) :
        aperture(f), lens(l) {
        speed = lens->maxApertureSpeed();
        latency = lens->apertureLatency();
        time = t;
    }

    Lens::ApertureAction::ApertureAction(Lens *l, int t, float f, float s) :
        aperture(f), speed(s), lens(l) {
        latency = lens->apertureLatency();
        time = t;
    }

    void Lens::ApertureAction::doAction() {
        lens->setAperture(aperture, speed);
    }    

    Lens::Tags::Tags(Frame f) {
        initialFocus    = f["lens.initialFocus"];
        finalFocus      = f["lens.finalFocus"];
        focus           = f["lens.focus"];
        focusSpeed      = f["lens.focusSpeed"];
        zoom            = f["lens.zoom"];
        initialZoom     = f["lens.initialZoom"];
        finalZoom       = f["lens.finalZoom"];
        aperture        = f["lens.aperture"];
        initialAperture = f["lens.initialAperture"];
        finalAperture   = f["lens.finalAperture"];
        apertureSpeed   = f["lens.apertureSpeed"];
    }

}
