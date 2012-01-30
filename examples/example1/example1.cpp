#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/* Macros for Android logging
 * Use: adb logcat FCAM_EXAMPLE1:*  *:S
 * to see all messages from this app
 */
#if !defined(LOG_TAG)
#define LOG_TAG "FCAM_EXAMPLE1"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , LOG_TAG, __VA_ARGS__)
#endif
#include <android/log.h>

#include <FCam/Tegra.h>
namespace Plat = FCam::Tegra;
const int width  = 2560;
const int height = 1920;
const FCam::ImageFormat format = FCam::YUV420p;
const char output_image[] = "/data/fcam/example1.jpg";

/** \file */

/***********************************************************/
/* A program that takes a single shot                      */
/*                                                         */
/* This example is a simple demonstration of the usage of  */
/* the FCam API.                                           */
/***********************************************************/

void errorCheck();

int main(int argc, char ** argv) {

    // Make the image sensor
    Plat::Sensor sensor;
    
    // Make a new shot
    Plat::Shot shot1;
    shot1.exposure = 50000; // 50 ms exposure
    shot1.gain = 1.0f;      // minimum ISO

    // Specify the output resolution and format, and allocate storage for the resulting image
    shot1.image = FCam::Image(width, height, format);
    
    // Order the sensor to capture a shot
    sensor.capture(shot1);

    // Check for errors 
    errorCheck();

    assert(sensor.shotsPending() == 1); // There should be exactly one shot
    
    // Retrieve the frame from the sensor
    Plat::Frame frame = sensor.getFrame();

    // This frame should be the result of the shot we made
    assert(frame.shot().id == shot1.id);

    // This frame should be valid too
    assert(frame.valid());
    assert(frame.image().valid());
    
    // Save the frame
    FCam::saveJPEG(frame, output_image);
    
    // Check that the pipeline is empty
    assert(sensor.framesPending() == 0);
    assert(sensor.shotsPending() == 0);

    return 0;
}

void errorCheck() {
    // Make sure FCam is running properly by looking for DriverError
    FCam::Event e;
    while (FCam::getNextEvent(&e, FCam::Event::Error) ) {
        printf("Error: %s\n", e.description.c_str());
        if (e.data == FCam::Event::DriverMissingError) {
            printf("example1: FCam can't find its driver. Did you install "
                   "fcam-drivers on your platform, and reboot the device "
                   "after installation?\n");
            exit(1);
        }
        if (e.data == FCam::Event::DriverLockedError) {
            printf("example1: Another FCam program appears to be running "
                   "already. Only one can run at a time.\n");
            exit(1);
        }        
    }
}
