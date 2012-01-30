#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <FCam/Tegra.h>

/* Macros for Android logging
 * Use: adb logcat FCAM_EXAMPLE2:*  *:S
 * to see all messages from this app
 */
#if !defined(LOG_TAG)
#define LOG_TAG "FCAM_EXAMPLE2"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , LOG_TAG, __VA_ARGS__)
#endif
#include <android/log.h>

namespace Plat = FCam::Tegra;
const int width  = 2560;
const int height = 1920;
const FCam::ImageFormat format = FCam::YUV420p;
const char output_image_flash[] = "/data/fcam/example2_flash.jpg";
const char output_image_noflash[] = "/data/fcam/example2_noflash.jpg";

/***********************************************************/
/* Flash / No-flash                                        */
/*                                                         */
/* This example demonstrates capturing multiple shots with */
/* possibly different settings.                            */
/***********************************************************/

int main(int argc, char ** argv) {

	// Devices
    Plat::Sensor sensor;
    Plat::Flash flash;
    sensor.attach(&flash); // Attach the flash to the sensor

    // Make two shots
    std::vector<Plat::Shot> shot(2);
    
    // Set the first shot parameters (to be done with flash)
    shot[0].exposure = 10000;
    shot[0].gain = 1.0f;
    shot[0].image = FCam::Image(width, height, format);
    
    // Set the second shot parameters (to be done without flash)
    shot[1].exposure = 10000;
    shot[1].gain = 1.0f;
    shot[1].image = FCam::Image(width, height, format);
    
    // Make an action to fire the flash
    Plat::Flash::FireAction fire(&flash);

    // Flash on must be triggered at time 0 - duration is ignored.
    fire.duration = flash.minDuration();
    fire.time = 0; 								// at the start of the exposure
    fire.brightness = flash.maxBrightness();    // at full power

    // Attach the action to the first shot
    shot[0].addAction(fire);

    // Order the sensor to capture the two shots
    sensor.capture(shot);
    assert(sensor.shotsPending() == 2);    // There should be exactly two shots
    
    // Retrieve the first frame
    Plat::Frame frame = sensor.getFrame();
    assert(sensor.shotsPending() == 1);    // There should be one shot pending
    assert(frame.shot().id == shot[0].id); // Check the source of the request
    
    FCam::Flash::Tags tags(frame);
    LOGD("First frame flash settings:\n");
    LOGD(" brightness %f, start %d, duration %d, peak %d\n",
    		tags.brightness, tags.start, tags.duration, tags.peak);

        // Write out file if needed
    FCam::saveJPEG(frame, output_image_flash);
    
    // Retrieve the second frame
    frame = sensor.getFrame();
    assert(frame.shot().id == shot[1].id); // Check the source of the request
    
    tags = FCam::Flash::Tags(frame);
    LOGD("Second frame flash settings:\n");
    LOGD(" brightness %f, start %d, duration %d, peak %d\n",
    		tags.brightness, tags.start, tags.duration, tags.peak);

    // Write out file
    FCam::saveJPEG(frame, output_image_noflash);
    
    // Check the pipeline is empty
    assert(sensor.framesPending() == 0);
    assert(sensor.shotsPending() == 0);
}
