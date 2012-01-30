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
#include <pthread.h>

#include "FCam/Action.h"
#include "FCam/Tegra/Sensor.h"

#include "FCam/Tegra/Platform.h"
#include "Daemon.h"
#include "../Debug.h"

namespace FCam { namespace Tegra {

    Sensor::Sensor(int index) :
            FCam::Sensor(),
            daemon(NULL),
            shotsPending_(0),
            sensorIndex(index),
            pProduct(NULL),
            pHardwareInterface(NULL)
    {
        pthread_mutex_init(&requestMutex, NULL);
        pProduct = Hal::System::openProduct();
        pHardwareInterface = pProduct->getCameraHal(this, index);
        sensorConfig = pHardwareInterface->getSensorConfig();

    }
    
    Sensor::~Sensor() {
        stop();
        pthread_mutex_destroy(&requestMutex);
        pProduct->releaseCameraHal(pHardwareInterface);
        Hal::System::closeProduct(pProduct);
    }

    void Sensor::attach(Device *pDevice) 
    {
        // Call teh base class attach
        FCam::Sensor::attach(pDevice);
    }

    void Sensor::attach(Lens* lens)
    {
       	// Start the sensor to get pHardwareInterface->open called.
    	// Otherwise, any operation on the lens before a capture will
    	// cause the app to stop responding.
        start();
        lens->attach(pHardwareInterface);

        // Call the base class attach.
        FCam::Sensor::attach(lens);
    }

    void Sensor::attach(Flash *flash) 
    {
       	// Start the sensor to get pHardwareInterface->open called.
    	// Otherwise, an operation on the flash before a capture could
    	// cause the app to stop responding.
        start();
        flash->attach(pHardwareInterface);
        FCam::Sensor::attach(flash);
    }
    
    int Sensor::getBestModeIndex(Size s) const
    {
        int bestModeIdx = sensorConfig.numberOfModes - 1;

        // Find the smallest mode that will fit the given size;
        // Iterate from largest to smallest.
        for (int modeIdx = bestModeIdx - 1; modeIdx > 0; modeIdx--)
        {
            if (sensorConfig.modes[modeIdx].width  < s.width ||
                sensorConfig.modes[modeIdx].height < s.height)
            {
                break;
            }

            bestModeIdx = modeIdx;
        }

        return bestModeIdx;
    }

    int Sensor::maxExposure(Size s) const {

        int modeIdx = getBestModeIndex(s);
        int exposure = (int)(1000000.0f * sensorConfig.modes[modeIdx].fMaxExposure);

        // Round to the nearest ms - that is what OMX allows us to set
        return exposure > 1000 ? exposure : 1000;
    }

    int Sensor::minExposure(Size s) const {

        int modeIdx = getBestModeIndex(s);
        int exposure = (int)(1000000.0f * sensorConfig.modes[modeIdx].fMinExposure);

        // Round to the nearest ms - that is what OMX allows us to set
        return exposure > 1000 ? exposure : 1000;
    }

    int Sensor::maxFrameTime(Size s) const {
        int modeIdx = getBestModeIndex(s);
        return (int)(1000000.0f * sensorConfig.modes[modeIdx].fMaxFrameRate);
    }

    int Sensor::minFrameTime(Size s) const {
        int modeIdx = getBestModeIndex(s);
        return (int)(1000000.0f * sensorConfig.modes[modeIdx].fMinFrameRate);
    }

    float Sensor::maxGain() const {
        return sensorConfig.fMaxGain;
    }

    float Sensor::minGain() const {
        return sensorConfig.fMinGain;
    }

    Size Sensor::minImageSize() const {
        // Modes are sorted by resolution - pick the first one
        return Size(sensorConfig.modes[0].width,
            sensorConfig.modes[0].height);
    }

    Size Sensor::maxImageSize() const {
        // Modes are sorted by resolution - pick the last one
        return Size(sensorConfig.modes[sensorConfig.numberOfModes - 1].width,
            sensorConfig.modes[sensorConfig.numberOfModes - 1].height);
    }

    int Sensor::exposureLatency() const {
        return sensorConfig.exposureLatency;
    }

    int Sensor::gainLatency() const {
        return sensorConfig.gainLatency;
    }

    void Sensor::start() {        
        if (daemon) return;

        daemon = new Daemon(this);
        pHardwareInterface->open();
        if (streamingShot.size()) daemon->launchThreads();
    }

    void Sensor::stop() {
        dprintf(3, "Entering Sensor::stop\n"); 
        stopStreaming();

        if (!daemon) return;

        dprintf(3, "Cancelling outstanding requests\n");

        // Cancel as many requests as possible
        pthread_mutex_lock(&requestMutex);
        _Frame *req;
        while (daemon->requestQueue.tryPullBack(&req)) {
            delete req;
            shotsPending_--;
        }
        pthread_mutex_unlock(&requestMutex);

        dprintf(3, "Discarding remaining frames\n");

        // Wait for the outstanding ones to complete
        while (shotsPending_) {
            delete daemon->frameQueue.pull();
            decShotsPending();
        }

        // Close camera interface before deleting daemon
        if (pHardwareInterface) {
            pHardwareInterface->close();
        }

        dprintf(3, "Deleting daemon\n"); 

        // Delete the daemon
        if (!daemon) return;
        delete daemon;
        daemon = NULL;

        dprintf(3, "Sensor stopped\n"); 
    }
    
    void Sensor::capture(const FCam::Shot& shot)
    {
        capture(Shot(shot));
    }

    void Sensor::capture(const Shot &shot) {
        start();
        
        _Frame *f = new _Frame;
        
        // make a deep copy of the shot to attach to the request
        f->_shot = shot;        
        // clone the shot ID
        f->_shot.id = shot.id;

        // push the frame to the daemon
        pthread_mutex_lock(&requestMutex);
        shotsPending_++;
        daemon->requestQueue.push(f);
        pthread_mutex_unlock(&requestMutex);

        daemon->launchThreads();
    }
    
    void Sensor::capture(const std::vector<FCam::Shot> &burst) {

        std::vector<Shot> tburst;
        for (size_t i = 0; i < burst.size(); i++) {
            tburst.push_back(Shot(burst[i]));
        }

        capture(tburst);
    }

    void Sensor::capture(const std::vector<Shot> &burst) {
        start();              

        std::vector<_Frame *> frames;
        
        for (size_t i = 0; i < burst.size(); i++) {
            _Frame *f = new _Frame;
            f->_shot = burst[i];
            
            // clone the shot ID
            f->_shot.id = burst[i].id;

            frames.push_back(f); 
        }

        pthread_mutex_lock(&requestMutex);
        for (size_t i = 0; i < frames.size(); i++) {
            shotsPending_++;
            daemon->requestQueue.push(frames[i]);
        }
        pthread_mutex_unlock(&requestMutex);

        daemon->launchThreads();
    }
    
    void Sensor::stream(const FCam::Shot &shot)
    {
        stream(Shot(shot));
    }

    void Sensor::stream(const Shot &shot) {
    	pthread_mutex_lock(&requestMutex);
        streamingShot.clear();
        // this makes a deep copy of the shot
        streamingShot.push_back(shot);
        streamingShot[0].id = shot.id;
        pthread_mutex_unlock(&requestMutex);

        start();
        if (daemon->requestQueue.size() == 0) capture(streamingShot);
    }
    
    void Sensor::stream(const std::vector<FCam::Shot> &burst) {

        std::vector<Shot> tburst;
        for (size_t i = 0; i < burst.size(); i++) {
            tburst.push_back(Shot(burst[i]));
        }

        stream(tburst);
    }

    void Sensor::stream(const std::vector<Shot> &burst) {
        pthread_mutex_lock(&requestMutex);
        
        // do a deep copy of the burst
        streamingShot = burst;
        
        // clone the ids 
        for (size_t i = 0; i < burst.size(); i++) {
            streamingShot[i].id = burst[i].id;
        }
        pthread_mutex_unlock(&requestMutex);

        start();
        if (daemon->requestQueue.size() == 0) capture(streamingShot);
    }
    
    bool Sensor::streaming() {
        return streamingShot.size() > 0;
    }
    
    void Sensor::stopStreaming() {
        pthread_mutex_lock(&requestMutex);
        streamingShot.clear();
        pthread_mutex_unlock(&requestMutex);
    }

    void Sensor::onFrame(Hal::CameraFrame* frame)
    {
        daemon->onFrame(frame);
    }

    void Sensor::readyToCapture() 
    {
    	daemon->readyToCapture();
    }
        
    Frame Sensor::getFrame() {
        if (!daemon) {
            Frame invalid;
            error(Event::SensorStoppedError, "Can't request a frame before calling capture or stream\n");
            return invalid;
        }        
        Frame frame(daemon->frameQueue.pull());
        FCam::Sensor::tagFrame(frame); // Use the base class tagFrame
        for (size_t i = 0; i < devices.size(); i++) {
            devices[i]->tagFrame(frame);
        }
        decShotsPending();
        return frame;
    }
    
    int Sensor::rollingShutterTime(const FCam::Shot &s) const {
        return rollingShutterTime(Shot(s));
    }

    int Sensor::rollingShutterTime(const Shot &s) const {
        // TODO: put better numbers here
        if (s.image.height() > 960) return 77000;
        else return 33000;
    }
    
    // the Daemon calls this when it's time for new frames to be queued up
    void Sensor::generateRequest() {
        pthread_mutex_lock(&requestMutex);
        if (streamingShot.size()) {
            for (size_t i = 0; i < streamingShot.size(); i++) {
                _Frame *f = new _Frame;
                f->_shot = streamingShot[i];                
                f->_shot.id = streamingShot[i].id;
                shotsPending_++;
                daemon->requestQueue.push(f);
            }
        }
        pthread_mutex_unlock(&requestMutex);

    }
    
    void Sensor::enforceDropPolicy() {
        if (!daemon) return;
        daemon->setDropPolicy(dropPolicy, frameLimit);
    }
    
    int Sensor::framesPending() const {
        if (!daemon) return 0;
        return daemon->frameQueue.size();
    }
    
    int Sensor::shotsPending() const {
        return shotsPending_;
    }

    void Sensor::decShotsPending() {
        pthread_mutex_lock(&requestMutex);
        shotsPending_--;
        pthread_mutex_unlock(&requestMutex);        

    }

    void Sensor::handleEvent(const Event &e)
    {
    	// Some events need to be sent to the devices.
    	switch(e.type)
    	{
    		// Notify devices of mode changes - the current
    		// implementation of a mode change causes a camera
    		// driver restart that change the lens position.
			case FCam::Event::TegraModeChange:
			{
				for (size_t i = 0; i < devices.size(); i++) {
					// The Sensor is an attached device, too.
					// Avoid recursion..
					if (devices[i] != this)
					{
						devices[i]->handleEvent(e);
					}
				}
    			break;
    		}
    		default:
    			break;
    	}
    }
        
}}
