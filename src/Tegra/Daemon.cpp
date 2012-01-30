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
#include <math.h>
#include <errno.h>

#include "FCam/Time.h"
#include "FCam/Frame.h"
#include "FCam/Action.h"
#include "FCam/Tegra/YUV420.h"

#include "../Debug.h"
#include "Daemon.h"
#include "Statistics.h"

namespace FCam { namespace Tegra {

    void *daemon_setter_thread_(void *arg) {
        Daemon *d = (Daemon *)arg;
        d->runSetter();    
        d->setterRunning = false;    
        close(d->daemon_fd);
        pthread_exit(NULL);
        return NULL;
    } 

    void *daemon_action_thread_(void *arg) {
        Daemon *d = (Daemon *)arg;
        d->runAction();
        d->actionRunning = false;
        pthread_exit(NULL);
        return NULL;
    }

    Daemon::Daemon(Sensor *sensor) :
        sensor(sensor),
        m_pCameraInterface(sensor->getHardwareInterface()),
        stop(false), 
        frameLimit(128),
        dropPolicy(Sensor::DropNewest),
        setterRunning(false),
        exposureLatency(0),
        gainLatency(0),
        actionRunning(false),
        threadsLaunched(false) {

        // make the mutexes for the producer-consumer queues
        if (errno = -(pthread_mutex_init(&actionQueueMutex, NULL))) {
            error(Event::InternalError, sensor, "Error creating mutexes: %d", errno);
        }
    
        // make the semaphore
        sem_init(&actionQueueSemaphore, 0, 0);
        sem_init(&readySemaphore, 0, 0);

    }

    void Daemon::launchThreads() {    
        if (threadsLaunched) return;
        threadsLaunched = true;

        // launch the threads
        pthread_attr_t attr;
        struct sched_param param;


        // I should now have CAP_SYS_NICE

        // make the setter thread
        param.sched_priority = sched_get_priority_min(SCHED_FIFO)+1;

        pthread_attr_init(&attr);

        if ((errno =
             -(pthread_attr_setschedparam(&attr, &param) ||
               pthread_attr_setschedpolicy(&attr, SCHED_FIFO) ||
#ifndef FCAM_PLATFORM_ANDROID
               pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) ||
#endif
               pthread_create(&setterThread, NULL, daemon_setter_thread_, this)))) {
            error(Event::InternalError, sensor, "Error creating daemon setter thread: %d", errno);
            return;
        } else {
            setterRunning = true;
        }

        // make the actions thread
        param.sched_priority = sched_get_priority_max(SCHED_FIFO);

        if ((errno =
             -(pthread_attr_setschedparam(&attr, &param) ||
               pthread_attr_setschedpolicy(&attr, SCHED_FIFO) ||
#ifndef FCAM_PLATFORM_ANDROID
               pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) ||
#endif
               pthread_create(&actionThread, NULL, daemon_action_thread_, this)))) {
            error(Event::InternalError, sensor, "Error creating daemon action thread: %d", errno);
            return;
        } else {
            actionRunning = true;
        }

        pthread_attr_destroy(&attr);
    }

    Daemon::~Daemon() {
        stop = true;

        // post a wakeup call to the action thread
        sem_post(&actionQueueSemaphore);

        // post a wakeup call to the setter thread if needed
        sem_post(&readySemaphore);

        if (setterRunning) 
            pthread_join(setterThread, NULL);
    
        if (actionRunning)
            pthread_join(actionThread, NULL);

        pthread_mutex_destroy(&actionQueueMutex);

        sem_destroy(&actionQueueSemaphore);
        sem_destroy(&readySemaphore);

        // Clean up all the internal queues
        while (inFlightQueue.size()) delete inFlightQueue.pull();        
        while (requestQueue.size()) delete requestQueue.pull();
        while (frameQueue.size()) delete frameQueue.pull();
        while (actionQueue.size()) {
            delete actionQueue.top().action;
            actionQueue.pop();
        }

    }


    void Daemon::setDropPolicy(Sensor::DropPolicy p, int f) {
        dropPolicy = p;
        frameLimit = f;
        enforceDropPolicy();
    }

    void Daemon::enforceDropPolicy() {
        if (frameQueue.size() > frameLimit) {
            warning(Event::FrameLimitHit, sensor,
                    "WARNING: frame limit hit (%d), silently dropping %d frames.\n"
                   "You're not draining the frame queue quickly enough. Use longer \n"
                   "frame times or drain the frame queue until empty every time you \n"
                   "call getFrame()\n", frameLimit, frameQueue.size() - frameLimit);
            if (dropPolicy == Sensor::DropOldest) {
                while (frameQueue.size() >= frameLimit) {
                    sensor->decShotsPending();
                    delete frameQueue.pull();
                }
            } else if (dropPolicy == Sensor::DropNewest) {
                while (frameQueue.size() >= frameLimit) {
                    sensor->decShotsPending();
                    delete frameQueue.pullBack();
                }
            } else {
                error(Event::InternalError, sensor, 
                      "Unknown drop policy! Not dropping frames.\n");
            }
        }
    }

    void Daemon::runSetter() {
        dprintf(2, "Running setter...\n"); fflush(stdout);

        while (!stop) 
        {
            sem_wait(&readySemaphore);
            if (stop) break;
            tickSetter(Time::now());
        }

        // TODO: Add any actions required to shutdown.
    }

    void Daemon::tickSetter(Time hs_vs) {
        static _Frame *req = NULL;

        dprintf(3, "Current time  %d %d\n", hs_vs.s(), hs_vs.us());

        /*********************
         * Limitations:
         * 1. We don't have a good way of enforcing frame time - this is currently 
         * handled automatically by setting the exposure time. For long exposures
         * the nvodm_imager changes the frame length.
         * 2. Exposure and gain values are flushed to the sensor by the camera 
         * driver after a capture is done - need to send new gain/exposure ahead.
         * 3. Exposure and gain values don't always take effect on the next frame,
         * the sensorLatchTime will tell how many frames to discard.
         */

        // Check if the last shot had a flash enabling action and disable the flash
        // for the next capture if it did.
        for (std::set<FCam::Action*>::const_iterator i = current.shot().actions().begin();
				 i != current.shot().actions().end();
				 i++) {

				 if ((*i)->type() == FCam::Action::FlashFire)
				 {
					 // Check if the actions turns the flash on..
					 FCam::Flash::FireAction *fire = (FCam::Flash::FireAction *)(*i);
					 if (fire->brightness > 0)
					 {
						 // Set flash off.
						 fire->getFlash()->fire(0,0);
					 }
				 }
        }

        // Finished inspecting actions - clear them for the next shot.
        current._shot.clearAllActions();

        // grab a new request and set an appropriate exposure and
        // frame time for it make sure there's a request ready for us
        if (!requestQueue.size()) {
            sensor->generateRequest();
        }

        // Peek ahead into the request queue to see what request we're
        // going to be handling next
        if (requestQueue.size()) {

            // There's a real request for us to handle
            req = requestQueue.front();
            dprintf(4, "Setter: grabbing next request\n");

        } else {

            // We might have been waken due to a stop request
            if (stop) return;

            // Insert a bubble
            req  = insertBubble(&current);
            dprintf(4, "Inserting bubble - empty queue: 0x%x\n", req);
        }

        // Check if the next request requires a mode switch
        if (req->shot().image.size() != current._shot.image.size() ||
            req->shot().image.type() != current._shot.image.type()) {

            dprintf(3, "Setter: Starting up camera in new mode\n");

            // set all the params for the new frame
            Hal::CameraMode m;
            m.width  = req->shot().image.width();
            m.height = req->shot().image.height();
            // enforce YUV420p or RAW as capture mode - we will do
            // any conversions when handling the output.
            m.type   = req->shot().image.type() == FCam::RAW ? Hal::RAW : Hal::YUV420p;

            // do the mode switch - the camera hal layer will flush
        	// the pipeline before doing the mode switch.
        	// setCaptureMode is a blocking call and will return after
            // all buffers are flushed or a timeout occurs.
            if (!m_pCameraInterface->setCaptureMode(m)) {
            	error(Event::DriverError, sensor,
            		"Failed to set mode %d x %d\n", m.width, m.height);

            	// If this was a user requested shot - clear the request
            	// and push the frame to avoid getFrame() blocking.
            	if (req->_shot.wanted) {
            	    requestQueue.pop();
            	    frameQueue.push(req);
            	}

            	return;
            }
            
            // Notify the sensor of the new mode.
            Event e;
            e.creator = NULL;
            e.type = FCam::Event::TegraModeChange;
            e.time = Time::now();
            e.data = 0;
            sensor->handleEvent(e);

     
            // Set destination image to new mode settings
            req->image = Image(m.width, m.height, req->shot().image.type(), Image::Discard);
        
            current._shot.image = Image(req->shot().image.size(), req->shot().image.type(), Image::Discard);
            current._shot.histogram  = req->shot().histogram;
            current._shot.sharpness  = req->shot().sharpness;
            
            // make sure we set everything else for the next frame,
            // because a setMode will change gain, frametime and exposure
            current._shot.frameTime = -1;
            current._shot.exposure = -1;
            current._shot.gain = -1.0;
            current._shot.whiteBalance = -1;
            current.image = Image(m.width, m.height, req->shot().image.type(), Image::Discard);

        } else {
            // no mode switch required            
        }

        // If current exposure != req exposure
        // replace the current request with a bubble. The exposure
        // setting is applied after the capture is done. The bubble
        // will ensure the correct order of operations.
        // Same for wb - wb settings are changed after the capture.
        if (req->shot().exposure != current.shot().exposure || 
            fabs(req->shot().gain - current.shot().gain) > 0.01 ||
            req->shot().whiteBalance != current.shot().whiteBalance)
        {
            // Insert a dummy request
            req = insertBubble(req);
            dprintf(DBG_MAJOR, "Inserting bubble 0x%x to satisfy exposure change\n", req);
        }


        // If the request queue is not empty, pop the request
        // (req could be a bubble if the request queue was empty).
        if (requestQueue.size()) {
            requestQueue.pop();
        }

        // Save the img informaton with the request.
        req->image = current.image;

        // Look ahead to the next frame, does it require a 
        // different exposure/gain?
        if (!requestQueue.size()) {
            sensor->generateRequest();
        }

        // Tag this frame with the expected gain and exposure values
        // before changing current.
        req->exposure = current._shot.exposure;
        req->gain     = current._shot.gain;
        req->whiteBalance = current._shot.whiteBalance;

        if (requestQueue.size() > 0)
        {
            _Frame *nextreq = requestQueue.front();

            // Setup settings for next frame
            // The camera driver sets settings after the capture is done.
            // So we look ahead and send now the settings for the next
            // frame.
            // Also track the gain and exposure latencies in case
            // the request is fastMode - so we can accurately
            // set the confidence field.
            if (nextreq->shot().exposure != current.shot().exposure ||
                fabs(nextreq->shot().gain - current.shot().gain) > 0.01)
            {

                if (current._shot.exposure != nextreq->shot().exposure) {
                    m_pCameraInterface->setSensorExposure(nextreq->shot().exposure);
                    current._shot.exposure = nextreq->shot().exposure;
                    exposureLatency = sensor->exposureLatency();
                }
        
                if (current._shot.gain != nextreq->shot().gain) {
                    m_pCameraInterface->setSensorEffectiveISO(nextreq->shot().gain * 100);
                    current._shot.gain = nextreq->shot().gain;
                    gainLatency = sensor->gainLatency();
                }
            }
            else
            {
                // No gain or exposure changes, then decrement the latencies
                if (gainLatency > 0) gainLatency--;
                if (exposureLatency > 0) exposureLatency--;
            }

            // Same for whitebalance, set it ahead of the next frame.
            if (current._shot.whiteBalance != nextreq->shot().whiteBalance) {
                m_pCameraInterface->setISPWhiteBalance(nextreq->shot().whiteBalance);
                current._shot.whiteBalance = nextreq->shot().whiteBalance;
            }

            nextreq->fastMode   = nextreq->_shot.fastMode;
            nextreq->confidence = Frame::MATCH_REQUEST;

            // Not a fastMode request? - insert the necessary bubles.
            if (!nextreq->fastMode) {
                int latency = gainLatency > exposureLatency ? gainLatency : exposureLatency;

                while (latency > 0)
                {
                    insertBubble(nextreq);
                    dprintf(DBG_MAJOR, "Inserting an exposure/gain latency bubble 0x%x\n", req);
                    latency--;
                }

                // We don't need to insert the bubbles again unless a new change comes in
                // reset gain and exposure latency to 0
                gainLatency = 0;
                exposureLatency = 0;

            } else {

                if (gainLatency > 0) nextreq->confidence |= Frame::UNCERTAIN_GAIN;
                if (exposureLatency > 0) nextreq->confidence |= Frame::UNCERTAIN_EXPOSURETIME;
            }

        }


        // Update the frameTime
        current._shot.frameTime = req->shot().frameTime;
        if (req->shot().frameTime > req->shot().exposure) {
            current.frameTime = req->shot().frameTime;
        } else {
            current.frameTime = req->shot().exposure;
        }
        
        dprintf(4, "Setter: setting frame parameters\n");
       
        // Set the current request parameters
        if (current._shot.histogram != req->shot().histogram) {
            current._shot.histogram = req->shot().histogram;
        }

        if (current._shot.sharpness != req->shot().sharpness) {
            current._shot.sharpness = req->shot().sharpness;
        }

        // now queue up this request's actions
        pthread_mutex_lock(&actionQueueMutex);
        int queuedActions = 0;
        for (std::set<FCam::Action*>::const_iterator i = req->shot().actions().begin();
             i != req->shot().actions().end();
             i++) {

             // If this is an action we can defer, queue it.
             // Otherwise do the action now because it will be pipelined by the NvMM Camera driver and executed with the shot.
             // Actions with time == 0 we don't defer. It is not possible at this point to accurately support actions with
             // time > 0.
             if ((*i)->time > 0) 
             {
            	 Action a;
            	 a.action = (*i)->copy();
                 a.time = hs_vs + (*i)->time - (*i)->latency;
                 actionQueue.push(a);
                 queuedActions++;
             }
             else
             {
                 (*i)->doAction();
             }

             // Copy the action to the current shot
             current._shot.addAction(*(*i));
        }
        pthread_mutex_unlock(&actionQueueMutex);
        for (int i = 0; i < queuedActions; i++) {
             sem_post(&actionQueueSemaphore);
        }

        // Trigger capture
        if (!m_pCameraInterface->capture())
        {
            error(Event::DriverError, sensor,
                "Failed to trigger the capture\n");

            if (req->_shot.wanted) {
                frameQueue.push(req);
            }

            return;
        }

        // The setter is done with this frame. Push it into the
        // in-flight queue for the handler to deal with.
        dprintf(4, "Setter: pushing request 0x%x\n", req);
        inFlightQueue.push(req);

        dprintf(4, "Setter: Done with this HS_VS, waiting for the next one\n");
    }

    _Frame* Daemon::insertBubble(_Frame *params) {

        _Frame *req = new _Frame;

        if (params == NULL) 
        {
            // bubbles should just run at whatever resolution is going. If
            // fast mode switches were possible, it might be nice to run
            // at the minimum resolution to go even faster, but they're
            // not.
            req->_shot.image = Image(current._shot.image.size(), current._shot.image.type(), Image::Discard);

            // generate histograms and sharpness maps if they're going,
            // but drop the data.
            req->_shot.histogram = current._shot.histogram;
            req->_shot.sharpness = current._shot.sharpness;
        }
        else
        {
            req->_shot = params->_shot;
            req->_shot.image = Image(params->_shot.image.size(), params->_shot.image.type(), Image::Discard);
        }
        req->_shot.clearAllActions();
        req->_shot.wanted = false;

        // push the bubble into the pipe
        requestQueue.pushFront(req);
        dprintf(4, "Inserted bubble 0x%x\n", req);
        return req;
    }


    void Daemon::onFrame(Hal::CameraFrame* f)
    {
        _Frame *req = NULL;
        if (inFlightQueue.size()) {
            req = inFlightQueue.pull();
            dprintf(4, "Handler: popping a frame request 0x%x\n", req);
        } else {
            // there's no request for this frame - probably coming up
            // from a mode switch or starting up
            dprintf(3, "Handler: Got a frame without an outstanding request,"
                    " dropping it.\n");
            return;
        }

        // Update the parameters
        req->gain = (float) f->params.iso/100.0f;
        req->exposure = f->params.exposure;
        req->frameTime = f->params.frameTime;

        // Based on the end time for this frame, predict the start time.
        req->processingDoneTime = f->params.processingDoneTime;
        req->exposureEndTime    = f->params.captureDoneTime; // This could actually include ISP time, need to verify.
        req->exposureStartTime  = req->exposureEndTime - f->params.frameTime;

        dprintf(4, "exposureStartTime: %d %d, exposureEndTime %d %d\n",
        		req->exposureStartTime.s(), req->exposureStartTime.us(),
        		req->exposureEndTime.s(), req->exposureEndTime.us());


        // Since we can't reliable predict when the frame is going to be done
        // don't bother assesing late frames. Assume for now that frames arrive in
        // the order requested.
        if (false) { // more than 25 ms late
          dprintf(3, "Handler: Expected a frame at %d %d, but one didn't arrive until %d %d\n",
                    req->processingDoneTime.s(), req->processingDoneTime.us(),
                    f->params.processingDoneTime.s(), f->params.processingDoneTime.us());

          error(Event::ImageDroppedError, sensor,
                  "Expected image data not returned.");
            req->image = Image(req->image.size(), req->image.type(), Image::Discard);
            if (!req->shot().wanted) {
                delete req;
            } else {
                // the histogram and sharpness map may still have appeared
                // req->histogram = m_pCameraInterface->getHistogram(req->exposureEndTime, req->shot().histogram);
                // req->sharpness = m_pCameraInterface->getSharpnessMap(req->exposureEndTime, req->shot().sharpness);
                frameQueue.push(req);
                enforceDropPolicy();
            }
            req = NULL;
        } else if (true) {
            Image im(f->width, f->height, 
                    f->format == Hal::YUV420p ? FCam::YUV420p : FCam::RAW, (unsigned char*)f->pBuffer);

            // Is this frame wanted or a bubble?
            if (!req->shot().wanted) {
                // it's a bubble - drop it
                dprintf(4, "Handler: discarding a bubble 0x%x\n", req);
                delete req;
            } else {

                // CPU computed sharpness/statistics
                if (req->_shot.sharpness.enabled) {
                    req->sharpness = Statistics::evaluateSharpness(req->_shot.sharpness, im);
                }
                if (req->_shot.histogram.enabled) {
                    req->histogram = Statistics::evaluateHistogram(req->_shot.histogram, im);
                }

                if (req->shot().image.autoAllocate()) {
                    if (req->image.type() == req->shot().image.type() && im.weak()) {
                        req->image = im.copy();
                    } else if(req->image.type() == req->shot().image.type() && !im.weak()) {
                        req->image = im;
                    } else if (req->image.type() == YUV420p && req->shot().image.type() == RGB24) {
                        req->image = Image(req->image.size(), req->shot().image.type());
                        convertYUV420ToRGB24(req->image, im);
                    } else {
                        error(Event::FormatMismatch, sensor,
                              "Requested unsupported image format %d "
                              "for an already allocated image.",
                              req->shot().image.type());                              
                    }
                } else if (req->shot().image.discard()) {
                    req->image = Image(req->image.size(), req->image.type(), Image::Discard);
                } else {
                    if (req->image.size() != req->shot().image.size()) {
                        error(Event::ResolutionMismatch, sensor, 
                              "Requested image size (%d x %d) "
                              "on an already allocated image does not "
                              "match actual image size (%d x %d). Dropping image data.",
                              req->shot().image.width(), req->shot().image.height(),
                              req->image.width(), req->image.height());
                        req->image = Image(req->image.size(), req->image.type(), Image::Discard);
                        // TODO: crop instead?
                    } else if (req->image.type() != req->shot().image.type() && 
                               req->shot().image.type() != RGB24 &&
                               req->image.type() != YUV420p) {
                        error(Event::FormatMismatch, sensor, 
                              "Requested unsupported image format %d "
                              "for an already allocated image.",
                              req->shot().image.type());
                        req->image = Image(req->image.size(), req->image.type(), Image::Discard);
                    } else { // the size matches
                        // Cache the output type in case we need color space conversions
                        ImageFormat srcType = req->image.type();
                        req->image = req->shot().image;
                        // figure out how long I can afford to wait
                        // For now, 10000 us should be safe
                        Time lockStart = Time::now();
                        if (req->image.lock(10000)) {
                            if (req->shot().image.type() == RGB24 && srcType == YUV420p) {
                                convertYUV420ToRGB24( req->image, im);
                            } else if (im.weak()) {
                                req->image.copyFrom(im);
                            } else {
                                req->image = im;
                            }
                            req->image.unlock();
                        } else {
                            warning(Event::ImageTargetLocked, sensor,
                                    "Daemon discarding image data (target is still locked, "
                                    "waited for %d us)\n", Time::now() - lockStart);
                            req->image = Image(req->image.size(), req->image.type(), Image::Discard);
                        }
                    }
                }

                frameQueue.push(req);
                enforceDropPolicy();

            }

            req = NULL;

        } else { // more than 10ms early. Perhaps there was a mode switch.
          dprintf(3, "Handler: Received an early mystery frame (%d %d) vs (%d %d), dropping it.\n",
                    f->params.processingDoneTime.s(), f->params.processingDoneTime.us(),
                    req->processingDoneTime.s(), req->processingDoneTime.us());
        }

    }
    
    void Daemon::readyToCapture()
    {
        // Wake up the setter thread
        sem_post(&readySemaphore);
    }

    void Daemon::runAction() {
        dprintf(2, "Action thread running...\n");
        while (1) {       
            sem_wait(&actionQueueSemaphore);
            if (stop) break;
            // priority inversion :(
            pthread_mutex_lock(&actionQueueMutex);
            Action a = actionQueue.top();
            actionQueue.pop();
            pthread_mutex_unlock(&actionQueueMutex);

            Time t = Time::now();
            int delay = (a.time - t) - 500;
            if (delay > 0) usleep(delay);
            Time before = Time::now();
            // busy wait until go time
            while (a.time > before) before = Time::now();
            a.action->doAction();
            //Time after = Time::now();
            dprintf(3, "Action thread: Initiated action %d us after scheduled time\n", before - a.time);
            delete a.action;
        }
    }

}}
 
