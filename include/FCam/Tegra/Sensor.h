/* Copyright (c) 1995-2010, Stanford University
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Stanford University nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ''AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/* Copyright (c) 2011, NVIDIA CORPORATION. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*  * Neither the name of NVIDIA CORPORATION nor the names of its
*    contributors may be used to endorse or promote products derived 
*    from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef FCAM_TEGRA_SENSOR_H
#define FCAM_TEGRA_SENSOR_H

/** \file 
 * The Tegra Sensor class 
 */

#include "../Sensor.h"
#include <vector>
#include <pthread.h>
#include "Shot.h"
#include "Frame.h"
#include "Platform.h"
#include "Lens.h"
#include "Flash.h"
#include "FCam/Tegra/hal/CameraHal.h"

namespace FCam { namespace Tegra {

    class Daemon;

    /** The Tegra Sensor class. It takes vanilla shots and
     * returns vanilla frames. See the base class documentation
     * for the semantics of its methods. 
     * 
     * The actual image sensor will change with the product.
     * 
     */
    class Sensor : public FCam::Sensor, public Hal::ICameraObserver {
    public:
        
        /** Sensor constructor. The index specifies which camera
          * the sensor corresponds to. Default and only supported
          * value for now is 0 (the backside camera). */           
        Sensor(int index = 0);
        ~Sensor();
        
        /** Generic device attach method, */
        void attach(Device *device);

        /** Attach a Lens. The Lens is configured after attach */
        void attach(Lens* lens);

        /** Attach the flash. */
        void attach(Flash *flash);
        
        /* Specialized functions for Tegra::Shots */
        void capture(const Shot &);
        void capture(const std::vector<Shot> &);
        void stream(const Shot &s);
        void stream(const std::vector<Shot> &);

        /* These ones take vanilla shots - these are
         * required to satisfy the abstract class definition.
         */
        void capture(const FCam::Shot &);
        void capture(const std::vector<FCam::Shot> &);
        void stream(const FCam::Shot &s);
        void stream(const std::vector<FCam::Shot> &);

        bool streaming();
        void stopStreaming();
        void start();
        void stop();

        int maxExposure(Size s) const;
        int minExposure(Size s) const;

        virtual int maxExposure() const { return maxExposure(Size(0,0)); }
        virtual int minExposure() const { return minExposure(Size(0,0)); }

        int maxFrameTime(Size s) const;
        int minFrameTime(Size s) const;

        virtual int maxFrameTime() const { return maxFrameTime(Size(0,0)); }
        virtual int minFrameTime() const { return minFrameTime(Size(0,0)); }

        virtual float maxGain() const;
        virtual float minGain() const;

        virtual Size minImageSize() const;
        virtual Size maxImageSize() const;

        virtual int maxHistogramRegions() const {return 0;}

        int rollingShutterTime(const Shot &) const;
        int rollingShutterTime(const FCam::Shot &) const;
            
        int framesPending() const;
        int shotsPending() const;

        /* How many frames to discard after an exposure change */
        int exposureLatency() const;

        /* How many frames to discard after a gain change */
        int gainLatency() const;

        virtual Platform &platform() {return Tegra::Platform::instance();}
        virtual const Platform &platform() const {return Tegra::Platform::instance();}

        FCam::Tegra::Frame getFrame();

        Hal::ICamera *getHardwareInterface() { return pHardwareInterface; }

        // IObserver interface...
        void onFrame(Hal::CameraFrame* frame);
        void readyToCapture();


    protected:
        
        FCam::Frame getBaseFrame() {return getFrame();}

    private:
        // The currently streaming shot            
        std::vector<Shot> streamingShot;            

        // The daemon that manages the Tegra's sensor
        friend class Daemon;
        Daemon *daemon;
            
        // the Daemon calls this when it's time for new frames to be queued up
        void generateRequest();

        // the Daemon calls this function to notify of any event that needs the
        // Sensor's attention.
        void handleEvent(const FCam::Event &e);

        pthread_mutex_t requestMutex;
          
        // enforce the specified drop policy
        void enforceDropPolicy();

        // The number of outstanding shots
        int shotsPending_;  

        // This is so the daemon can inform the sensor that a frame
        // was dropped due to the frame limit being hit in a
        // thread-safe way
        void decShotsPending();

        // Returns the index to the sensor mode that will 
        // be applied for the given image size.
        int getBestModeIndex(Size s) const;

        // The sensor index
        int sensorIndex;

        // The sensor configuration
        Hal::SensorConfig sensorConfig;

        // Hardware interfaces
        Hal::IProduct * pProduct;
        Hal::ICamera  * pHardwareInterface;
};
        
}
}


#endif
