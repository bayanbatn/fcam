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
#include <limits>
#include "FCam/Tegra/Flash.h"
#include "FCam/Tegra/Platform.h"
#include "FCam/Frame.h"

#include "../Debug.h"

namespace FCam { namespace Tegra {
    
    Flash::Flash() :  
        flashHistory(512),
        pHardwareInterface(NULL)
    {}

    Flash::~Flash() {}
    
    void Flash::attach(Hal::ICamera *pHardwareInterface)
    {
        this->pHardwareInterface = pHardwareInterface;
    }
    
    void Flash::setBrightness(float b) {
        if (b < minBrightness()) b = minBrightness();
        if (b > maxBrightness()) b = maxBrightness();

    }

    
    void Flash::setDuration(int d) {
        if (d < minDuration()) d = minDuration();
        if (d > maxDuration()) d = maxDuration();

    }

    void Flash::fire(float brightness, int duration) {

        Time fireTime = Time::now() + fireLatency();

        dprintf(4, "Flash fire time %d %d - brightness %f\n", fireTime.s(), fireTime.us(), brightness);
        brightness = pHardwareInterface->setFlashForStillCapture(brightness, duration);

        setBrightness(brightness);
        setDuration(duration);

        // update flash history
        {
            FlashState f;
            f.time = fireTime;
            f.brightness = brightness;
            flashHistory.push(f);
        }

    }

    void Flash::torch(float brightness) {

        Time fireTime = Time::now() + fireLatency();

        setBrightness(brightness);
        setDuration(0);

        brightness = pHardwareInterface->setFlashTorchMode(brightness);

        // update flash history
        {
            FlashState f;
            f.time = fireTime;
            f.brightness = brightness;
            flashHistory.push(f);
        }
    }

        
    void Flash::tagFrame(FCam::Frame f) {
        Time t1 = f.exposureStartTime();
        Time t2 = f.exposureEndTime();

        // what was the state of the flash initially and finally
        float b1 = 0; 
        float b2 = 0; 

        for (size_t i = 0; i < flashHistory.size(); i++) {
            if (t1 > flashHistory[i].time) {
                b1 = flashHistory[i].brightness;
                break;
            }
        }

        for (size_t i = 0; i < flashHistory.size(); i++) {
            if (t2 > flashHistory[i].time) {
                b2 = flashHistory[i].brightness;
                break;
            }
        }

        // What was the last flash-turning-off event within this
        // exposure, and the first flash-turning-on event.
        int offTime = -1, onTime = -1;
        float brightness;
        for (size_t i = 0; i < flashHistory.size(); i++) {
            if (flashHistory[i].time < t1) break;
            if (flashHistory[i].time > t2) continue;
            if (flashHistory[i].brightness == 0 && offTime == -1) {
                offTime = flashHistory[i].time - t1;
            }                    
            if (flashHistory[i].brightness > 0) {
                brightness = flashHistory[i].brightness;
                onTime = flashHistory[i].time - t1;
            }
        }

        // If these guys are not set, and the initial brightness is zero, the flash didn't fire
        if ((offTime < 0 || onTime < 0) && (b1 == 0)) {
        	f["flash.brightness"] = 0;
			f["flash.duration"] = 0;
            f["flash.start"] = 0;
            f["flash.peak"] = 0;
        } else if (b1 > 0) {

            if (b2 == 0) {
                // it was on initially, turned off and stayed off
                f["flash.brightness"] = b1;
                f["flash.duration"] = offTime;
                f["flash.start"] = 0;
                f["flash.peak"] = offTime/2;
            } else {
                // was on at the start and the end of the frame
                f["flash.brightness"] = (b1+b2)/2;
                f["flash.duration"] = t2-t1;
                f["flash.start"] = 0;
                f["flash.peak"] = (t2-t1)/2;
            }
        } else {
            if (b2 > 0) {
                // off initially, turned on, stayed on
                int duration = (t2-t1) - onTime;
                f["flash.brightness"] = b2;
                f["flash.duration"] = duration;
                f["flash.start"] = onTime;
                f["flash.peak"] = onTime + duration/2;
            } else {
                // either didn't fire or pulsed somewhere in the middle
                if (onTime >= 0) {
                    // pulsed in the middle
                    f["flash.brightness"] = brightness;
                    f["flash.duration"] = offTime - onTime;
                    f["flash.start"] = onTime;
                    f["flash.peak"] = onTime + (offTime - onTime)/2;
                } else {
                    // didn't fire. No tags.
                }
            }
        }

        

    }

    void Flash::handleEvent(const FCam::Event &e)
    {}

    float Flash::getBrightness(Time t) {
        for (size_t i = 0; i < flashHistory.size(); i++) {
            if (t > flashHistory[i].time) {
                return flashHistory[i].brightness;
            }
        }

        // uh oh, we ran out of history!
        error(Event::FlashHistoryError, "Flash brightness at time %d %d is unknown", t.s(), t.us());
        return std::numeric_limits<float>::quiet_NaN(); // unknown        
    }

    Flash::TorchAction::TorchAction(Flash *f) : flash(f) {
        latency = flash->fireLatency();        
        time = 0;
    }

    Flash::TorchAction::TorchAction(Flash *f, int t) :
        flash(f) {
        time = t;
        latency = flash->fireLatency();        
        brightness = flash->maxBrightness();
    }

    Flash::TorchAction::TorchAction(Flash *f, int t, float b) : 
        brightness(b), flash(f) {
        time = t;
        latency = flash->fireLatency();
    }

    void Flash::TorchAction::doAction() {
        flash->torch(brightness);
    }

       
}
}
