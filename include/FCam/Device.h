#ifndef FCAM_DEVICE_H
#define FCAM_DEVICE_H

#include "Time.h"
#include "Event.h"

/** \file 
 * Abstract base classes for devices. */

namespace FCam {

    class Frame;

    /** An abstract base class for devices. Devices will typically
        have a bunch of methods to do their thing (e.g. the lens has
        methods to move it around.). Devices also typically define
        nested actions, and tags. */
    class Device : public EventGenerator {
      public:
        /** Your device should implement this method to tag a frame
         * coming back from the sensor. Don't forget to \ref
         * Sensor::attach your device to the sensor so that this gets
         * called. Frames are tagged just before they are returned via
         * \ref Sensor::getFrame, which may be some time after they
         * actually occured, so do not tag the frame with your devices
         * current state. Instead your device should keep a history of
         * recent state, and inspect the \ref Frame::exposureStartTime
         * and \ref Frame::exposureEndTime to add the appropriate
         * tags. */
        virtual void tagFrame(Frame) = 0;

        /** Devices might need to handle events from the sensor they
         *  are attached to.
         */
        virtual void handleEvent(const Event &e) = 0;

        virtual ~Device() {}
    };

}
#endif
