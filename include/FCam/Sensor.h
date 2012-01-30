#ifndef FCAM_SENSOR_H
#define FCAM_SENSOR_H

//! \file 
//! A base class for Sensors

#include "Base.h"
#include <vector>
#include "Device.h"
#include "Frame.h"

namespace FCam {

    class Shot;

    /** A base class for image sensors. Takes shots via \ref Sensor::capture and \ref Sensor::stream, and returns frames via \ref Sensor::getFrame. */
    class Sensor : public Device {
      public:
        Sensor();
        virtual ~Sensor();


        /** Queue up the next shot. Makes a deep copy of the argument. */
        virtual void capture(const Shot &) = 0;
        /** Queue up a burst of shots. Makes a deep copy of the argument. */
        virtual void capture(const std::vector<Shot> &) = 0;

        /** Set a shot to be captured when the sensor isn't busy capturing anything else. Makes a deep copy of the argument. */
        virtual void stream(const Shot &s) = 0;

        /** Set a burst to be captured whenever the sensor isn't busy capturing anything else. Makes a deep copy of the argument. */
        virtual void stream(const std::vector<Shot> &) = 0;

        /** Is there a shot or burst currently streaming? */
        virtual bool streaming() = 0;

        /** Stop the sensor from streaming a shot or burst set with
            stream. The sensor will continue to run. To turn off the
            sensor completely use \ref Sensor::stop */
        virtual void stopStreaming() = 0;

        /** Power up the sensor, image processor, and the FCam daemon.
            This is done automatically when you call capture for the
            first time. **/
        virtual void start() = 0;

        /** Shut down the sensor, image processor, and the FCam
            daemon. Call this to save power and/or processing cycles
            while your program is doing something unrelated to taking
            pictures. */
        virtual void stop() = 0;

        /** Set the maximum number of frames that can exist in the
            frame queue. This a failsafe for memory management, not a
            rate control tool. */
        void setFrameLimit(int);
        
        /** Get the current frame limit (see \ref FCam::Sensor::setFrameLimit "setFrameLimit") */
        int getFrameLimit();

        /** Which frames should be dropped if there are too many frames in the frame Queue. */
        enum DropPolicy {DropNewest = 0, //!< Drop the newest frames
                         DropOldest      //!< Drop the oldest frames
        };

        /** Set which frames should be dropped if the frame limit is exceeded. */
        void setDropPolicy(DropPolicy);

        /** Get which frames will be dropped if the frame limit is exceeded. */
        DropPolicy getDropPolicy();

        /** Get the next frame. We promise that precisely one frame
         * will come back per time capture is called. A
         * reference-counted shared pointer object is returned, so you
         * don't need to worry about deleting it. */
        Frame getFrame() {return getBaseFrame();}

        /** How many frames are in the frame queue (i.e., how many
         * times can you call \ref FCam::Sensor::getFrame "getFrame"
         * before it blocks? */
        virtual int framesPending() const = 0;

        /** How many shots are pending. This includes frames in the
         * frame queue, shots currently in the pipeline, and shots in
         * the capture queue. Stop streaming 
         * (\ref FCam::Sensor::stopStreaming "stopStreaming") and
         * get frames (\ref FCam::Sensor::getFrame "getFrame") until
         * this hits zero to completely drain the system. */
        virtual int shotsPending() const = 0;

        /** Allow a device to tag the frames that come from this
         * sensor. */
        void attach(Device *);
        
        /** The maximum exposure time supported by this sensor. */
        virtual int maxExposure() const = 0;

        /** The minimum supported exposure time */
        virtual int minExposure() const = 0;

        /** The maximum supported frame time. */
        virtual int maxFrameTime() const = 0;

        /** The minimum supported frame time */
        virtual int minFrameTime() const = 0;      

        /** The maximum supported gain */
        virtual float maxGain() const = 0;

        /** The minimum supported gain. Defaults to 1.0. */
        virtual float minGain() const {return 1.0;}
        
        /** The smallest image size */
        virtual Size minImageSize() const = 0; 

        /** The largest image size */
        virtual Size maxImageSize() const {return Size(2592, 1968);}

        /** The maximum supported number of histogram regions */
        virtual int maxHistogramRegions() const {return 4;}

        /** The time difference between the first line exposing and
         * the last line in microseconds, for given shot parameters. */
        virtual int rollingShutterTime(const Shot &) const = 0;

        /** Access to the static platform data about this sensor. */
        virtual const Platform &platform() = 0;
        
        /** The sensor has the option of tagging the frames it
         * produces, just like any other device. By default it doesn't
         * add anything. */
        virtual void tagFrame(Frame) {};

    protected:
        std::vector<Device *> devices;

        // Derived sensors should implement this method to return an
        // FCam::Frame, and also implement the non-virtual getFrame()
        // method and have it return a platform-specific frame type
        // (e.g. an FCam::N900::Frame). In this way, an N900::Sensor
        // can return an N900::Frame with getFrame, while casting it
        // to a base sensor will result in it returning base frames
        // (because getFrame is not virtual, and in the base class
        // just calls getBaseFrame)
        virtual Frame getBaseFrame() = 0;

        // enforce the specified drop policy
        virtual void enforceDropPolicy() = 0;
        DropPolicy dropPolicy;
        size_t frameLimit;
    };

}

#endif
