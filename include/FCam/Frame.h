#ifndef FCAM_FRAME_H
#define FCAM_FRAME_H

/** \file 
 * A frame is the data returned by the sensor as a result of a \ref FCam::Shot. */

#include <tr1/unordered_map>
#include <tr1/memory>
using std::tr1::shared_ptr;

#include "Base.h"
#include "Device.h"
#include "Time.h"
#include "Image.h"
#include "TagValue.h"
#include "Shot.h"
#include "Event.h"
#include "Platform.h"

namespace FCam {

    class Action;
    class Lens;

    /** A TagMap is a dictionary mapping strings to \ref TagValue
     * "TagValues" */
    typedef std::tr1::unordered_map<std::string, TagValue> TagMap;

    /** A struct containing the data that makes up a \ref Frame.  You
     * should not instantiate a _Frame, unless you're making dummy
     * frames for testing purposes. */
    struct _Frame : public EventGenerator {
        _Frame();
        virtual ~_Frame();

        Image image;
        Time exposureStartTime;
        Time exposureEndTime;    
        Time processingDoneTime; 
        int exposure; 
        int frameTime; 
        float gain;    
        int whiteBalance;
        Histogram histogram;    
        SharpnessMap sharpness;  
        TagMap tags;

        // Derived frames should implement these to return a
        // platform-specific shot with \ref Shot(), and a basic Shot
        // using \ref baseShot().
        const Shot &shot() const { return baseShot(); }
        virtual const Shot &baseShot() const = 0;

        // Derived frames should implement these to return static
        // platform data necessary for interpreting this frame.
        virtual const Platform &platform() const = 0;

        /** A Frame debugging dump function. Prints out all Frame
         * fields and details of the included Image */
        virtual void debug(const char *name="") const;
    };

    /** Data returned by the sensor as a result of a shot. May contain
     * image data, a histogram, sharpness map, and assorted tags placed
     * there by devices attached to the sensor from whence this frame
     * came. It may also contain none of these, so check each component
     * is valid before using it. This class is a reference counted
     * pointer type to the real data, so pass it by copy. */
    class Frame {
    protected:    
        shared_ptr<_Frame> ptr;

    public:

        /** Frames are normally acquired by sensor::getFrame(). The
         * Frame constructor can be used to construct dummy frames for
         * testing purposes. The Frame takes ownership of the _Frame
         * passed in. */
        Frame(_Frame *f=NULL) : ptr(f) {}
        Frame(shared_ptr<_Frame> f) : ptr(f) {}

        /** Virtual destructor to allow derived frames to delete
         * themselves properly when accessed as base frames */
        virtual ~Frame();

        /** Does this Frame refer to a valid frame? */
        bool valid() const {return (bool)ptr;}

        /** Equality operator, to see if two frames point to the same underlying data **/
        bool operator==(const Frame &other) const { return ptr == other.ptr; }

        /** The actual image data. Check image().valid() before using
         * it, image data can be dropped in a variety of cases. */
        Image image() const {return ptr->image;}
        
        /** The time the earliest pixel in the image started exposing. */
        Time exposureStartTime() const {return ptr->exposureStartTime;}
        
        /** The time the latest pixel in the rolling shutter finished
            exposing. This is exposureStart + exposure + rolling shutter time. */
        Time exposureEndTime() const {return ptr->exposureEndTime;}    

        /** The time the image appeared out of the imaging pipe. */
        Time processingDoneTime() const {return ptr->processingDoneTime;}

        /** The actual exposure time for this frame in microseconds. Note that 
         * \ref exposureEndTime - \ref exposureStartTime may be more than
         * this for rolling shutter sensors. */
        int exposure() const {return ptr->exposure;}

        /** The actual number of microseconds between the start of
         * this frame and the start of the next one. Note that 
         * \ref exposureEndTime - \ref exposureStartTime may be more than
         * this for rolling shutter sensors. The frame time will be at
         * least the exposure time, typically plus a small overhead of
         * several hundred microseconds. */
        int frameTime() const {return ptr->frameTime;}
        
        /** The actual gain used to produce this frame. This may be a
         * combination of analog and digital gain. Analog gain is
         * preferred, and low gain settings should use only analog
         * gain. */
        float gain() const {return ptr->gain;}
               
        /** The actual white balance setting used to produce this
         * frame. */
        int whiteBalance() const {return ptr->whiteBalance;}

        /** A histogram produced by the imaging pipe. Check
         * histogram.valid before using it. 
         */
        const Histogram &histogram() const {return ptr->histogram;}

        /** A sharpness map produced by the imaging pipe. Check
         * sharpness.valid before using it. 
         */
        const SharpnessMap &sharpness() const {return ptr->sharpness;}

        /** A const reference to the shot that generated this
         * frame. If you have a fancy sensor that takes more
         * parameters, and a corresponding fancy shot that inherits
         * from the base shot, this method should be overridden to
         * return a const reference to your derived shot type
         * instead. It is not a virtual method, so if your fancy frame
         * is cast to a base frame, this method will return a base
         * shot. */
        const Shot &shot() const {
            return ptr->shot();
        }

        /** A const reference to the tags that have been placed on
         * this frame by any devices. In general you use
         * frame["tagName"] to get and set tags, rather than directory
         * accessing this map. If you wish to iterate over tags,
         * however, you can use this TagMap, which is an
         * std::unordered_map*/
        const TagMap &tags() const {
            return ptr->tags;
        }

        /** Retrieve a reference to a tag placed on this frame by
         * name. This can be used to lookup tags like so:
         * double x = frame["focus"];
         * Or to attach new tags that will survive being saved to a
         * file and loaded again like so:
         * frame["mySpecialTag"] = 42;
         */
        TagValue &operator[](const std::string &name) const {
            return ptr->tags[name];
        }

        /** Access to the static platform data about the sensor that
         * produced this frame. */
        virtual const Platform &platform() const {
            return ptr->platform();
        }

        /** Treat a frame as an EventGenerator pointer. This allows us
         * to associate events (such as failing to load a DNG) with a
         * frame and look for them later in the event queue using
         * getNextEvent(). */
        operator EventGenerator*() {
            return static_cast<EventGenerator *>(ptr.get());
        }

        /** A Frame debugging dump function. Prints out all Frame
         * fields and details of the included Image */
        void debug(const char *name="") const {
            ptr->debug(name);
        }

    };

}

/** A debug helper macro that outputs the variable name of the image
 * object at the macro call site along with the image debug
 * information */

#define FCAM_FRAME_DEBUG(i) (i).debug(#i)

#endif
