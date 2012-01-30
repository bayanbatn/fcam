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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <linux/videodev2.h>
#include <limits>

#include "FCam/Tegra/Lens.h"
#include "FCam/Frame.h"

#include "../Debug.h"

namespace FCam { namespace Tegra {
    
    Lens::Lens() : 
        lensHistory(512),
        pHardwareInterface(NULL)
    {}

    void Lens::attach(Hal::ICamera *pHardwareInterface) {

        // Initialize
        this->pHardwareInterface = pHardwareInterface;
        lensConfig = pHardwareInterface->getLensConfig();

        // focus at infinity
        setFocus(0, -1);
    }

    Lens::~Lens() {
    }
    
    float Lens::farFocus() const 
    {
        return lensConfig.fFarFocusDiopter;
    }

    float Lens::nearFocus() const 
    {
        return lensConfig.fNearFocusDiopter;
    }
    

    void Lens::setFocus(float f, float speed) {

        // set the focus speed
        // If there is no specified focus speed assume focus speed is instantaneous after latency.
        if (minFocusSpeed() == 0 && maxFocusSpeed() == 0)
        {
            speed = 0;
        }
        else
        {
            if (speed < 0) speed = maxFocusSpeed();
            if (speed < minFocusSpeed()) speed = minFocusSpeed();
            if (speed > maxFocusSpeed()) speed = maxFocusSpeed();
        }
        
        // TODO: None of our camera stack components allows us to set the focus speed.

        // set the focus
        if (f < farFocus()) f = farFocus();
        if (f > nearFocus()) f = nearFocus();

        int ticks = dioptersToTicks(f);
        int oldTicks = 0;
        float oldDiopters = 0.0f;

        if (pHardwareInterface->getFocuserPosition(&oldTicks))
        {
            oldDiopters = ticksToDiopters(oldTicks);
        }

        if(pHardwareInterface->setFocuserPosition(ticks))
        {
            Time start, end;
            start = Time::now() + focusLatency();              
            end = start + (int)(abs(f - oldDiopters)*speed) + focusSettleTime();
            LensState s;
            s.time = start;
            s.position = oldDiopters;
            lensHistory.push(s);
            s.time = end;
            s.position = f;
            // dprintf(4, "set focus: start %d %d %f - end %d %d %f\n", start.s(), start.us(), oldDiopters, end.s(), end.us(), f); 
            lensHistory.push(s);
        }
    }

    float Lens::getFocus() const {
        return getFocus(Time::now());
    }
    
    float Lens::getFocus(Time t) const {
        if (lensHistory.size() && t > lensHistory[0].time) {
            return lensHistory[0].position;
        } else if (lensHistory.size() == 0) {
            return 0.0f;
        }

        for (size_t i = 0; i < lensHistory.size()-1; i++) {
            if (t < lensHistory[i+1].time) continue;
            if (t < lensHistory[i].time) {
                // linearly interpolate
                float alpha = float(t - lensHistory[i+1].time)/(lensHistory[i].time - lensHistory[i+1].time);
                return alpha * lensHistory[i].position + (1-alpha) * lensHistory[i+1].position;
            }
        }

        // uh oh, we ran out of history!
        error(Event::LensHistoryError, "Lens position at time %d %d is unknown", t.s(), t.us());
        return std::numeric_limits<float>::quiet_NaN(); // unknown
    }

    bool Lens::focusChanging() const {
        Time t = Time::now();
        return (lensHistory.size()) && (lensHistory[0].time > t);
    }

    float Lens::minFocusSpeed() const 
    {
        return tickRateToDiopterRate((int)lensConfig.fMinFocusSpeed);
    }
    
    float Lens::maxFocusSpeed() const {
        return tickRateToDiopterRate((int)lensConfig.fMaxFocusSpeed);
    }

    int  Lens::focusLatency() const {
        return lensConfig.iFocusLatency;
    }

    int  Lens::focusSettleTime() const {
        return lensConfig.iFocusSettleTime;
    }

    /* No zoom support for now - may be we could consider digital zoom */
    void  Lens::setZoom(float, float) {
    }

    /* Fixed focal length */
    float Lens::getZoom() const {
        return lensConfig.fMinZoomFocalLength;
    }

    /* Fixed focal length */
    float Lens::minZoom() const {
        return lensConfig.fMinZoomFocalLength;
    }

    /* Fixed focal length */
    float Lens::maxZoom() const {
        return lensConfig.fMaxZoomFocalLength;
    }

    /* Fixed focal length */
    bool  Lens::zoomChanging() const {
        return false;
    }


    float Lens::minZoomSpeed() const {
        return lensConfig.fMinZoomSpeed;
    }


    float Lens::maxZoomSpeed() const {
        return lensConfig.fMaxZoomSpeed;
    }


    int   Lens::zoomLatency() const {
        return lensConfig.iZoomLatency;
    }

    /* Fixed aperture*/
    void    Lens::setAperture(float aperture, float speed) {
        return;
    }
    
    float   Lens::getAperture() const {
        return lensConfig.fWideAperture;
    }
    
    float   Lens::wideAperture(float) const {
        return lensConfig.fWideAperture;
    }
    
    float   Lens::narrowAperture(float) const {
        return lensConfig.fNarrowAperture;
    }
    
    bool    Lens::apertureChanging() const {
        return false;
    }
    
    int     Lens::apertureLatency()  const {
        return lensConfig.iApertureLatency;
    }
    
    float   Lens::minApertureSpeed() const {
        return lensConfig.fMinApertureSpeed;
    }
    
    float   Lens::maxApertureSpeed() const {
        return lensConfig.fMaxApertureSpeed;
    }

    float Lens::ticksToDiopters(int ticks) const {
        float rate = lensConfig.fFocusDioptersPerTick;
        float d = rate*(ticks - lensConfig.iFocusMinPosition);

        // the lens hits internal blockers and doesn't actually move
        // beyond a certain range
        if (d < farFocus())  d = farFocus();
        if (d > nearFocus()) d = nearFocus();
        return d;
    }

    int Lens::dioptersToTicks(float diopter) const {
        return (int)(diopter/lensConfig.fFocusDioptersPerTick + 
                     lensConfig.iFocusMinPosition);
    }
    
    
    float Lens::tickRateToDiopterRate(int ticks) const {
        return ticks * lensConfig.fFocusDioptersPerTick;
    }

    int Lens::diopterRateToTickRate(float diopter) const {
        return (int)(diopter/lensConfig.fFocusDioptersPerTick);
    }

    void Lens::handleEvent(const FCam::Event &e)
    {
    	switch(e.type)
    	{
    		// Reset the focus position
			case FCam::Event::TegraModeChange:
			{
				// Reset the focus.
				setFocus(getFocus(e.time));
    		}
    		default:
    			break;
    	}
    }

  
    void Lens::tagFrame(FCam::Frame f) {
        float initialFocus = getFocus(f.exposureStartTime());
        float finalFocus = getFocus(f.exposureEndTime());
        
        // dprintf(4, "initial focus: %d %d %f final focus %d %d %f\n", f.exposureStartTime().s(), f.exposureStartTime().us(), initialFocus, f.exposureEndTime().s(), f.exposureEndTime().us(), finalFocus);
        f["lens.initialFocus"] = initialFocus;
        f["lens.finalFocus"] = finalFocus;
        f["lens.focus"] = finalFocus; //(initialFocus + finalFocus)/2;
        f["lens.focusSpeed"] = (1000000.0f * (finalFocus - initialFocus)/
                                (f.exposureEndTime() - f.exposureStartTime()));

        float zoom = getZoom();
        f["lens.zoom"] = zoom;
        f["lens.initialZoom"] = zoom;
        f["lens.finalZoom"] = zoom;
        f["lens.zoomSpeed"] = 0.0f;

        float aperture = getAperture();
        f["lens.aperture"] = aperture;
        f["lens.initialAperture"] = aperture;
        f["lens.finalAperture"] = aperture;
        f["lens.apertureSpeed"] = 0.0f;

        // static properties of the Tegra's lens. In the future, we may
        // just add "lens.*" with a pointer to this lens to the tags,
        // and have the tag map lazily evaluate anything starting with
        // devicename.* by asking the appropriate device for more
        // details. For now there are only four more fields, so we
        // don't mind.
        f["lens.minZoom"] = minZoom();
        f["lens.maxZoom"] = maxZoom();
        f["lens.wideApertureMin"] = wideAperture(minZoom());
        f["lens.wideApertureMax"] = wideAperture(maxZoom());
    }

}}
