#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

/* Macros for Android logging
 * Use: adb logcat FCAM_EXAMPLE4:*  *:S
 * to see all messages from this app
 */
#if !defined(LOG_TAG)
#define LOG_TAG "FCAM_EXAMPLE4"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , LOG_TAG, __VA_ARGS__)
#endif
#include <android/log.h>

#include <FCam/Tegra.h>
#include <FCam/AutoExposure.h>
#include <FCam/AutoWhiteBalance.h>

namespace Plat = FCam::Tegra;
const int width  = 640;
const int height = 480;
const FCam::ImageFormat format = FCam::YUV420p;
const char output_image[] = "/data/fcam/example4.jpg";

// Print text in the app's textview
void appPrint(char *str);

static void fcam_print(const char *fmt, ...)
{
    char buf[512];

    va_list arglist;
    va_start(arglist, fmt);
    vsnprintf(buf, 512, fmt, arglist);
    va_end(arglist);

    // Print to the app textview and also send to android log
    LOGD("%s", buf);
    appPrint(buf);
}

// namespace Plat = FCam::F2;

/***********************************************************/
/* Autoexposure                                            */
/*                                                         */
/* This example shows how to request streams and deal with */
/* the incoming frames, and also uses the provided         */
/* auto-exposure and auto-white-balance routines.          */
/***********************************************************/
int main(int argc, char ** argv) {

    // Make a sensor
    Plat::Sensor sensor;
    
    // Shot
    Plat::Shot stream1;
    // Set the shot parameters
    stream1.exposure = 33333;
    stream1.gain = 1.0f;

    // We don't bother to set frameTime in this example. It defaults
    // to zero, which the implementation will clamp to the minimum
    // possible value given the exposure time.

    // Request an image size and allocate storage
    stream1.image = FCam::Image(width, height, format);

    // Enable the histogram unit
    stream1.histogram.enabled = true;
    stream1.histogram.region = FCam::Rect(0, 0, width, height);
    
    // We will stream until the exposure stabilizes
    int count = 0;          // # of frames streamed
    int stableCount = 0;    // # of consecutive frames with stable exposure
    float exposure;         // total exposure for the current frame (exposure time * gain)
    float lastExposure = 0; // total exposure for the previous frame 
    
    Plat::Frame frame;

    do {
        // Ask the sensor to stream with the given parameters
        sensor.stream(stream1);
    
        // Retrieve a frame
        frame = sensor.getFrame();
        assert(frame.shot().id == stream1.id); // Check the source of the request

        fcam_print("Exposure time: %d, gain: %f\n", frame.exposure(), frame.gain());

        // Calculate the total exposure used (including gain)
        exposure = frame.exposure() * frame.gain();
    
        // Call the autoexposure algorithm. It will update stream1
        // using this frame's histogram.
        autoExpose(&stream1, frame);
    
        // Call the auto white-balance algorithm. It will similarly
        // update the white balance using the histogram.
        autoWhiteBalance(&stream1, frame);

        // Increment stableCount if the exposure is within 5% of the
        // previous one
        if (fabs(exposure - lastExposure) < 0.05f * lastExposure) {
            stableCount++;
        } else {
            stableCount = 0;
        }

        // Update lastExposure
        lastExposure = exposure;
    
    } while (stableCount < 5); // Terminate when stable for 5 frames

    // Write out the well-exposed frame
    FCam::saveJPEG(frame, output_image);
    
    // Order the sensor to stop streaming
    sensor.stopStreaming();
    printf("Processed %d frames until stable for 5 frames!\n", count);
    
    // There may still be shots in the pipeline. Consume them.
    while (sensor.shotsPending() > 0) frame = sensor.getFrame();

    // Check that the pipeline is empty
    assert(sensor.framesPending() == 0);
    assert(sensor.shotsPending() == 0);
}
