
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <FCam/Tegra.h>

/** \file */

/* Macros for Android logging
 * Use: adb logcat FCAM_EXAMPLE6:*  *:S
 * to see all messages from this app
 */
#if !defined(LOG_TAG)
#define LOG_TAG "FCAM_EXAMPLE5"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , LOG_TAG, __VA_ARGS__)
#endif
#include <android/log.h>

#include "SoundPlayer.h"

namespace Plat = FCam::Tegra;
const int width  = 2560;
const int height = 1920;
const FCam::ImageFormat format = FCam::YUV420p;
const char output_image[] = "/data/fcam/example6.jpg";

// The native assetManager
extern AAssetManager* nativeAssetManager;


/***********************************************************/
/* Shutter sound                                           */
/*                                                         */
/* This example shows how to declare and attach a device,  */
/* and write the appropriate actions. In this example, the */
/* camera will trigger two actions at the beginning of the */
/* exposure: a flash, and a shutter sound.                 */
/* See SoundPlayer class for more information.             */
/***********************************************************/
int main(int argc, char ** argv) {

    if (nativeAssetManager == NULL)
    {
        LOGE("setAssets() was not called - can't get assetManager()\n");
        return 0;
    }

    // Devices
    Plat::Sensor sensor;
    Plat::Flash flash;
    
    // We defined a custom device to play a sound during the
    // exposure. See SoundPlayer.h/cpp for details.
    SoundPlayer audio(nativeAssetManager);

    sensor.attach(&flash); // Attach the flash to the sensor
    sensor.attach(&audio); // Attach the sound player to the sensor
    
    // Set the shot parameters
    Plat::Shot shot1;
    shot1.exposure = 50000;
    shot1.gain = 1.0f;
    shot1.image = FCam::Image(width, height, format);
    
    // Action (Flash)
    FCam::Flash::FireAction fire(&flash);
    fire.time = 0; 
    fire.duration = flash.minDuration();
    fire.brightness = flash.maxBrightness();
    
    // Action (Sound)
    SoundPlayer::SoundAction click(&audio);
    click.time = 0; // Start at the beginning of the exposure
    click.setAsset("camera_snd.mp3");
    
    // Attach actions
    shot1.addAction(fire);
    shot1.addAction(click);
    
    // Order the sensor to capture a shot.
    // The flash and the shutter sound should happen simultaneously.
    sensor.capture(shot1);
    assert(sensor.shotsPending() == 1); // There should be exactly one shot
    
    // Retrieve the frame from the sensor
    Plat::Frame frame = sensor.getFrame();
    assert(frame.shot().id == shot1.id); // Check the source of the request
    
    // Write out the file
    FCam::saveJPEG(frame, output_image);
    
    // Check that the pipeline is empty
    assert(sensor.framesPending() == 0);
    assert(sensor.shotsPending() == 0);
}
