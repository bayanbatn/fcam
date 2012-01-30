#include <math.h>

#include "FCam/AutoFocus.h"
#include "FCam/Lens.h"
#include "FCam/Frame.h"

#include "Debug.h"

namespace FCam {

    AutoFocus::AutoFocus(Lens *l, Rect r) : lens(l), state(IDLE), rect(r) {}
    
    void AutoFocus::startSweep() {
      // Left empty intentionally.
    }

    
    void AutoFocus::update(const Frame &f, FCam::Shot *shots) {
      // Left empty intentinoally.
    }

}
