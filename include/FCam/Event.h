#ifndef FCAM_EVENT_H
#define FCAM_EVENT_H

//! \file 
//! Events representing change of device state or error conditions

#include <string>

#include "Time.h"
#include "TSQueue.h"


namespace FCam {

    /** A base class for things that generate events
     * 
     */
    class EventGenerator {
    };
    
    /** An Event marks a change in device state or an error
        condition. Retrieve events from the system using \ref getNextEvent. */
    class Event {
      public:
        EventGenerator *creator; //!< creator of this Event
        int type;                //!< type of this Event
        int data;                //!< data associated with this Event
        Time time;               //!< Time this Event occured
        std::string description; //!<  A human-readable description of the event

        /** The list of builtin event types
         * 
         * Builtin FCam events use positive integers. They are
         * guaranteed not to have collisions and should all be listed
         * here. Custom events should use negative integers, and may
         * collide with other custom events (so you should check the
         * creator and the type).
         */
        enum {Error = 0,               //!< An error occurred
              Warning,                 //!< A warning occurred
              FocusPressed,            //!< The focus button was pressed (typically the shutter button was half-depressed)
              FocusReleased,           //!< The focus button was released
              ShutterPressed,          //!< The shutter button was pressed
              ShutterReleased,         //!< The shutter button was released
              N900LensClosed = 10000,  //!< The lens cover on the N900 was closed
              N900LensOpened,          //!< The lens cover on the N900 was opened
              N900SlideOpened,         //!< The keyboard on the N900 was slid open
              N900SlideClosed,               //!< The keyboard on the N900 was slid closed
              F2LensRemoved = 20000,   //!< The lens on the F2 was removed
              F2LensInstalled,         //!< A new lens was inserted in the F2
              F2ZoomChanged,           //!< The lens on the F2 was manually zoomed
              F2FocusChanged,          //!< The lens on the F2 was manually focused
              TegraModeChange = 30000, //!< A mode change has occurred.
        };

        /** The list of builtin error and warning codes
         *
         * Builtin FCam warning and error codes use positive
         * integers. Custom events should use negative integers. */
        enum {Unknown = 0,        //!< Should not be used. Indicates the error event is uninitialized.
              InternalError,      //!< Indicates a probable bug inside FCam. Inform the developers.
              DriverLockedError,  //!< Another FCam program is running
              DriverMissingError, //!< The FCam drivers don't appear to be installed
              DriverError,        //!< Something unexpected happened while communicating with the FCam drivers
              ImageTargetLocked,  //!< FCam was requested to place image data in an image that was locked
              ResolutionMismatch, //!< FCam was requested to place image data in a target of the wrong size
              FormatMismatch,     //!< FCam was requested to place image data in a target of the wrong format
              OutOfMemory,        //!< A memory allocation failed inside FCam
              FrameLimitHit,      //!< Too many frames are waiting on the frame queue. Call getFrame more frequently.
              LensHistoryError,   //!< The state of the lens was requested for a time too far in the past or the future
              FlashHistoryError,  //!< The state of the flash was requested for a time too far in the past or the future
              DemosaicError,      //!< Could not demosaic this image
              ImageLockError,     //!< Incorrect usage of Image::lock or Image::unlock
              BadCast,            //!< A TagValue was cast to a mismatched type
              FileLoadError,       //!< A file could not be read in succesfully
              FileLoadWarning,     //!< A file isn't quite as expected
              FileSaveError,       //!< A file couldn't be written
              FileSaveWarning,     //!< A file couldn't be saved quite as expected
              SensorStoppedError, //!< getFrame called before calling capture or stream
              ParseError,         //!< Could not read a TagValue from a string
              FrameDataError,     //!< Expected frame data (tags, image data) was not available
              ImageDroppedError,  //!< FCam was unable to retrieve the image data for a frame
              OutOfRange,         //!< An argument was outside the valid or accurate range of inputs
        };
    };
        
    /** Copies the next pending event into the pointer given. Returns
     * false if there are no outstanding events. You should
     * periodically process all pending events like so: 
     \code
FCam::Event event;
while (FCam::getNextEvent(&event)) {
    switch(event.type) {
    case FCam::Event::Error:
        ...
        break;
    case FCam::Event::Warning:
        ...
        break;
    case FCam::Event::ShutterPressed:
        ...
        break;
    ...
    default:
        cerr << "Unknown event: " << event.description();
    }
}
     \endcode

*/
    bool getNextEvent(Event *);
    /** Get the next event of a given type. Returns false and does not
     * alter the Event argument if no events of that type are
     * currently in the event queue. */
    bool getNextEvent(Event *, int type);
    /** Get the next event of a given type and with a specific data
     * field. Returns false and does not alter the Event argument if
     * no matching events are currently in the event queue. */
    bool getNextEvent(Event *, int type, int data);
    /** Get the next event of a given type, created by the specified
     * EventGenerator. Returns false and does not alter the Event
     * argument if no matching events are currently in the event queue. */
    bool getNextEvent(Event *, int type, EventGenerator *creator);
    /** Get the next event of a given type, with a specific data
     * field, and created by the specified EventGenerator. Returns
     * false and does not alter the Event argument if no matching
     * events are currently in the event queue. */
    bool getNextEvent(Event *, int type, int data, EventGenerator *creator);
    /** Get the next event created by the specified
     * EventGenerator. Returns false and does not alter the Event
     * argument if no matching events are currently in the event
     * queue. */
    bool getNextEvent(Event *, EventGenerator *creator);

    /** Add an event to the event queue. */
    void postEvent(Event);

    /** A simplified event posting interface that includes a type,
     * integer code, message, and optional creator. */
    void postEvent(int type, int code, const std::string &msg, EventGenerator *creator = NULL);

    /** Post an error event, using printf-style arguments. */
    void error(int code, EventGenerator *creator, const char *fmt, ...);

    /** Post a warning event, using printf-style arguments. */
    void warning(int code, EventGenerator *creator, const char *fmt, ...);    

    /** Post an error event with no creator, using printf-style arguments. */
    void error(int code, const char *fmt, ...);

    /** Post a warning event with no creator, using printf-style arguments. */
    void warning(int code, const char *fmt, ...);    

    /** The global event queue */
    extern TSQueue<Event> _eventQueue;
}

#endif
