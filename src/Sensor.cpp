#include "FCam/Sensor.h"
#include "FCam/Lens.h"
#include "FCam/Shot.h"

#include "Debug.h"

namespace FCam {

    Sensor::Sensor() {
        // A sensor affects the frames it returns, so it is attached
        // to itself. This is pointless for the base sensor, but is
        // useful for fancier derived sensors so that they get a
        // chance to tag the frames they return.
        attach(this);
        dropPolicy = Sensor::DropOldest;
        frameLimit = 128;
    }

    Sensor::~Sensor() {}

    void Sensor::attach(Device *d) {
        devices.push_back(d);
    }

    void Sensor::setFrameLimit(int l) {
        frameLimit = l;
        enforceDropPolicy();
    }
    
    int Sensor::getFrameLimit() {
        return frameLimit;
    }

    void Sensor::setDropPolicy(Sensor::DropPolicy d) {
        dropPolicy = d;
    }

    Sensor::DropPolicy Sensor::getDropPolicy() {
        return dropPolicy;
    }

}
