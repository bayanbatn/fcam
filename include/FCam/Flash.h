#ifndef FCAM_FLASH_H
#define FCAM_FLASH_H

#include "Action.h"
#include "Device.h"

/** \file 
 * The FCam interface to the camera flash */

namespace FCam {
    
    class Action;

    /** An abstract base class for camera flashes. */
    class Flash : public Device {
      public:
        ~Flash();

        /** Minimum flash duration in microseconds */
        virtual int minDuration() = 0;

        /** Maximum flash duration in microseconds */
        virtual int maxDuration() = 0;
        
        /** The minimum brightness setting of the flash. The units are platform specific. */
        virtual float minBrightness() = 0;

        /** The maximum brightness setting the flash. The units are platform specific. */
        virtual float maxBrightness() = 0;

        /** Fire the flash with a given brightness for a given number
            of microseconds. */
        virtual void fire(float brightness, int duration) = 0;       

        /** How long after I call \ref FCam::Flash::fire "fire" does
            the flash actually fire. */
        virtual int fireLatency() = 0;

        /** What was the brightness of the flash at some time in the
         * recent past. */
        virtual float getBrightness(Time t) = 0;
        
        /** Attach tags describing the state of the flash during a frame to a 
         * \ref Frame. You should never need to call this function - it's
         * done for you by the \ref Sensor. */
        virtual void tagFrame(Frame) = 0;

        /** An action to fire the flash during an exposure */
        class FireAction : public CopyableAction<FireAction> {
          public:
        	virtual int type() const { return Action::FlashFire; }

            /** Construct a FireAction to fire the given flash. */
            FireAction(Flash *f);

            /** Construct a FireAction to fire the given flash,
             * starting at the given time into the exposure (in
             * microseconds). */
            FireAction(Flash *f, int time);
            
            /** Construct a FireAction to fire the given flash,
             * starting at the given time into the exposure (in
             * microseconds), at the given brightness, for the given
             * duration (in microseconds). */
            FireAction(Flash *f, int time, float brightness, int duration);

            /** The brightness setting for this flash fire. Units are
             * platform specific. Peak lumens are suggested. */
            float brightness;

            /** The length of time the flash should fire for. */
            int duration;

            void doAction();

            /** Get the flash associated with this fire event. */
            Flash *getFlash() {return flash;}
          private:

            Flash *flash;
        };

        /** A flash adds the tags "flash.brightness", "flash.start",
         * "flash.duration", and "flash.peak", as described below. */
        class Tags {
        public:
            /** Extract the tags placed on a frame by a flash */
            Tags(Frame);
            
            /** The brightness setting used for this firing */
            float brightness;
            
            /** The time the flash started firing, measured as
             * microseconds into the exposure. If the flash was firing
             * before the exposure began, this should be zero.*/
            int start;

            /** For how many microseconds of the frame was the flash
             * emitting light. */
            int duration;

            /** The time of peak firing activation (us after start of
             * exposure). For flashes which emit a fairly constant
             * level of light over some period of time this should be
             * the center time of that plateau. */
            int peak;
        };
    };

}

#endif
