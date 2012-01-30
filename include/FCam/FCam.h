#ifndef FCAM_H
#define FCAM_H

/** \file
 * The main include file for FCam */

#include "Action.h"
#include "AsyncFile.h"
#include "AutoExposure.h"
#include "AutoFocus.h"
#include "AutoWhiteBalance.h"
#include "Device.h"
#include "Event.h"
#include "Flash.h"
#include "Frame.h"
#include "Image.h"
#include "Lens.h"
#include "Platform.h"
#include "Sensor.h"
#include "Shot.h"
#include "Time.h"

#include "processing/DNG.h"
#include "processing/Demosaic.h"
#include "processing/Dump.h"
#include "processing/JPEG.h"

namespace FCam {

/*! \mainpage FCam: An API for controlling computational cameras.
 * 
 * The FCam API provides mechanisms to control various components of a
 * camera to facilitate complex photographic applications.
 *
 * \image html fcam.png
 *
 * To use the FCam, you pass \ref Shot "Shots" to a \ref Sensor which
 * asynchronously returns \ref Frame "Frames". A Shot completely
 * specifies the capture and post-processing parameters of a single
 * photograph, and a \ref Frame contains the resulting image, along
 * with supplemental hardware-generated statistics like a \ref
 * Histogram and \ref SharpnessMap. You can tell \ref Device "Devices"
 * (like \ref Lens "Lenses" or \ref Flash "Flashes") to schedule \ref
 * Action "Actions" (like \ref Flash::FireAction "firing the flash")
 * to occur at some number of microseconds into a Shot. If timing is
 * unimportant, you can also just tell \ref Device "Devices" to do
 * their thing directly from your code. In either case, \ref Device
 * "Devices" add tags to returned \ref Frame "Frames" (like the
 * position of the \ref Lens for that \ref Shot). Tags are key-value
 * pairs, where the key is a string like "focus" and the value is a
 * \ref TagValue, which can represent one of a number of types.
 *
 * <HR><H1> Recent changes</H1> 
 * \par
 *
 * A list of the most recent changes can be found under the \ref Changes page.
 *
 * <HR><H1> The four basic classes </H1>
 *
 * <H2> Shot </H2>
 *
 * A Shot is a bundle of parameters that completely describes the
 * capture and post-processing of a single output Frame. A Shot
 * specifies Sensor parameters such as 
 *  - gain,
 *  - exposure time (in microseconds),
 *  - total time (clamped to be just over the exposure time, used to set frame rate),
 *  - output resolution,
 *  - format (raw or demosaicked),
 *  - white balance (only relevant if format is demosaicked),
 *  - memory location into which to place the Image data, and
 *  - unique id (auto-generated on construction).
 * 
 * It also specifies the configuration of the fixed-function
 * statistics generators by specifying
 *  - over which region a \ref Histogram should be computed, and
 *  - over which region and at what resolution a \ref SharpnessMap "Sharpness Map"
 *    should be generated.
 *
 *
 * <H2> Sensor </H2>
 *
 * After creation, a shot can be passed to a Sensor by calling
 *  - \ref Sensor::capture "capture" (which results in a single Frame)
 *  - \ref Sensor::stream "stream" (which results in a continuous stream of \ref Frame "Frames")
 *
 * The sensor manages an imaging pipeline in a separate thread. A Shot
 * is issued into the pipeline when it reaches the head of the queue
 * of pending shots, and the Sensor is ready to begin configuring
 * itself for the next Frame. Capturing a Shot just sticks it on the
 * end of the queue of pending shots. Streaming a shot causes the
 * sensor to capture a copy of that shot whenever the queue becomes
 * empty. You can therefore stream one shot while capturing others -
 * the captured shots will have higher priority.
 *
 * If you wish to change the parameters of a streaming Shot, just
 * alter it and call stream again with the updated Shot. This incurs
 * very little overhead and will not slow down streaming (provided you
 * don't change output resolution).
 *
 * \ref Sensor "Sensors" may also capture or stream STL vectors of
 * \ref Shot "Shots", or bursts.  Capturing a burst enqueues those
 * \ref Shot "shots" in the order given, and is useful, for example,
 * to capture a full high-dynamic range stack in the minimal amount of
 * time, or to rotate through a variety of sensor settings at video
 * rate. 
 *
 *
 * <H2> Frame </H2>
 *
 * On the output side, the Sensor produces \ref Frame "Frames",
 * retrieved via the \ref Sensor::getFrame "getFrame" method. This
 * method is the only blocking call in the core API.
 *
 * A frame contains 
 *  - Image data, 
 *  - the output of the statistics generators,
 *  - the precise time the exposure began and ended, 
 *  - the actual parameters used in its capture (Shot), and
 *  - the requested parameters in the form of a copy of the Shot 
 *    used to generate it. 
 *  - Tags placed on it by devices, in its \ref Frame::tags() "tags" dictionary.
 *
 * If the Sensor was unable to achieve the requested parameters
 * (for example if the requested frame time was shorter than the
 * requested exposure time), then the actual parameters will reflect
 * the modification made by the system. 
 *
 * \ref Frame "Frames" can be identified by the \em id field of their
 * Shot. 
 *
 * The API guarantees that one Frame comes out per Shot
 * requested. \ref Frame "Frames" are never duplicated or dropped
 * entirely. If Image data is lost or corrupted due to hardware error,
 * a Frame is still returned (possibly with statistics intact), with
 * its Image marked as invalid.
 *
 *
 * <H2> Device </H2>
 *
 * Cameras are much more than an image sensor. They also include a
 * Lens, a Flash, and other assorted \ref Device "Devices". In FCam
 * each Device is represented by an object with methods for performing
 * its various functions. Each device may additionally define a set of
 * \ref Action "Actions" which are used to synchronize these functions
 * to exposure, and a set of tags representing the metadata attached
 * to returned frames. While the exact list of \ref Device "Devices"
 * is platform-specific, FCam includes abstract base classes that
 * specify the interfaces to the Lens and the Flash.
 *
 * <H3> Lens </H3> 
 * \par
 * The lens is a Device subclass that represents the main lens. It can
 * be asked to initiate a change to any of its three parameters:
 * 
 *  - focus (measured in diopters), 
 *  - focal length (zooming factor), and
 *  - aperture.
 *
 * \par
 * Each of these calls returns immediately, and
 * the lens starts moving in the background. Each call has an optional
 * second argument that specifies the speed with which the action
 * should occur. Additionally, each parameter can be queried to see if
 * it is currently changing, what its bounds are, and its current
 * value. 
 *
 * <H3> Flash </H3>
 * \par  
 * The Flash is another Device subclass that has a single method that
 * tells it to fire with a specified brightness and duration. It also
 * has methods to query bounds on brightness and duration.
 *
 * <H3> Action </H3>
 * \par 
 * In many applications, the timing of device actions must be
 * precisely coordinated with the image sensor to create a successful
 * photograph. For example, the timing of flash firing in a
 * conventional second-curtain sync flash photograph must be accurate
 * to within a millisecond.
 *
 * \par
 * To facilitate this, each device may define a set of \ref Action
 * "Actions" as nested classes. They include
 *  - a start time relative to the beginning of image exposure, 
 *  - a method that will be called to initiate the action, 
 *  - and a latency field that indicates how long the delay is between 
 *    the method call and the action actually beginning. 
 *
 * \par
 * Any Device with predictable latency can thus have its actions
 * precisely scheduled. The Lens defines three actions; one to
 * initiate a change in each lens parameter. The Flash defines a
 * single action to fire it. Each Shot contains a set of actions (a
 * standard STL set) scheduled to occur during the corresponding
 * exposure.
 *
 * \htmlonly <H3> \endhtmlonly
 * \ref Frame::tags "Tags"
 * \htmlonly </H3> \endhtmlonly
 * \par
 * \ref Device "Devices" may also gather useful information or
 * metadata during an exposure, which they may attach to returned \ref
 * Frame "Frames" by including them in the \ref Frame::tags "tags"
 * dictionary. \ref Frame "Frames" are tagged after they leave the
 * pipeline, so this typically requires Devices to keep a short
 * history of their recent state, and correlate the timestamps on the
 * Frame they are tagging with that history.
 *
 * \par 
 * The Lens and Flash tag each Frame with the state they were in
 * during the exposure. This makes writing an autofocus algorithm
 * straightforward; the focus position of the Lens is known for each
 * returned Frame. Other appropriate uses of tags include sensor
 * fusion and inclusion of fixed-function imaging blocks.
 *
 * <HR><H1> FCam Platforms </H1> 
 *
 * \par 
 * There are currently two FCam platforms: The Nokia N900, and the F2.
 * 
 * Each platform defines its own versions of Sensor, Lens, and Flash,
 * and optionally may define its own Shot and Frame. These live in a
 * namespace named after the platform. You can thus refer to them as
 * \c FCam::MyPlatform::Shot.
 * 
 * <H2> The F2 </H2>
 * 
 * \image html f2.jpg
 * 
 * <H2> The N900 </H2>
 * 
 * \image html n900.jpg
 *
 * <H2> NVIDIA Tegra 3 </H2>
 *
 * \ref FCam::Tegra
 *
 * <HR><H1> Other Useful Features </H1>
 *
 * <H2> Events </H2>
 * 
 * \par
 * Some devices generate asynchronous \ref Event "Events". These
 * include user input \ref Event "Events" like the feedback from a
 * manual zoom Lens or phidgets device, as well as asynchronous
 * error conditions like Lens removal. We maintain an Event queue
 * for the application to retrieve and handle these \ref Event
 * "Events". To retrieve an Event from the queue, use \ref
 * getNextEvent.
 *
 * <H2> Autofocus, metering, and white-balance </H2>
 * 
 * \par
 * FCam is sufficiently low level for you to write your own
 * high-performance metering and autofocus functions. For convenience,
 * we include an \ref AutoFocus helper object, and \ref autoExpose 
 * and \ref autoWhiteBalance helper functions.
 * 
 * <H2> File I/O and Post-Processing </H2>
 *
 * \par
 * To synchronously save files to disk, see \ref saveDNG, 
 * \ref saveDump, and \ref saveJPEG. The latter will \ref demosaic, white
 * balance, and gamma correct your image if necessary. To
 * asynchronously save files to disk (in a background thread), see
 * \ref AsyncFileWriter.
 * 
 * \par 
 * You can also load files you have saved with \ref loadDNG and \ref
 * loadDump (loadJpeg is on the todo list). Loading or saving in
 * either format will preserve all tags devices have placed on the
 * frame.
 * 
 * \par
 * We encourage you to use your favorite image processing library for
 * more complicated operations. FCam \ref Image objects can act as
 * weak references to the memory of another library's image object, to
 * support this interopability.
 * 
 * <HR><H1> Examples </H1> 
 * \par
 *
 * Basic examples can be found on the \ref Examples page. For more
 * example code, including Makefiles and Qt project files, refer to
 * the examples subdirectory in the FCam source package. It includes
 * the programs on the \ref Examples page, and also a minimal Qt app
 * that acts as a basic camera with viewfinder (example7).
 */
 

 /** \page Examples 
 * 
 * <BR><BR><H1>Minimal Example FCam Programs</H1>
 * 
 * These examples demonstrate basic API usage. To build them, see the
 * examples subdirectory in the FCam source package.
 * 
 * <HR><H2>Example 1</H2>
 * \li Capture and save a single photograph. Demonstrates Shot,
 * Sensor, and Frame.
 * 
 * \include example1/example1.cpp

 * <HR><H2>Example 2</H2>
 * \li Capture and save a flash/no-flash pair. Demonstrates use of a
 * Device, and a Flash Action.
 * 
 * \include example2/example2.cpp

 * <HR><H2>Example 3</H2>
 * \li Capture a photograph during which the focus ramps from near to
 * far. Demonstrates use of a Device, a Lens Action, and a Tag.
 * 
 * \include example3/example3.cpp

 * <HR><H2>Example 4</H2>
 * \li Stream frames and auto-expose.
 * 
 * \include example4/example4.cpp

 * <HR><H2>Example 5</H2>
 * \li Stream frames, auto-focus, and auto-white-balance. 
 * 
 * \include example5/example5.cpp

 * <HR><H2>Example 6</H2>
 * \li Make a noise synchronized to the start of the
 * exposure. Demonstrates making a custom Device, and synchronizing
 * that device's Action with exposure.
 * 
 * \include example6/example6.cpp

 */

 /** \page Changes
  * <H2>July 2011</H2>
  * - Added support for NVIDIA Tegra. See the Tegra documentation for more details.
  * - Added Lens::FocusSteppingAction to do focus stepping while streaming.
  * - Added Viewfinder device to act as a viewfinder.
  * - Added ColorSpace member to the Histogram class to indicate which colorspace the channels represent.
  */
}


/** Main namespace for the API.
 *
 * FCam is the main namespace for the API.
 * There are sub-namespaces for each platform, currently four: N900, F2, Tegra, and Dummy
 */
namespace FCam {
}

#endif
