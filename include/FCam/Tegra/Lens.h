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
#ifndef FCAM_TEGRA_LENS_H
#define FCAM_TEGRA_LENS_H

/** \file
 * The Tegra Lens. */

#include "../Lens.h"
#include "../CircularBuffer.h"
#include "../Time.h"
#include "FCam/Tegra/Platform.h"
#include "FCam/Tegra/hal/CameraHal.h"
#include <vector>

namespace FCam { namespace Tegra {
    
    /** The Tegra platform Lens. Each product can have a different focuser/lens 
     *  combination, and there could be more than one focuser on a system. For
     *  this reason, the Lens parameters are only valid after the lens has been
     *  attached to a Sensor. */
    class Lens : public FCam::Lens {
             
    public:
        Lens();
        ~Lens();

        /** Focus at the diopters number at the given speed 
          * (defaults to -1, which indicates max speed) */
        void    setFocus(float diopters, float speed = -1);

        /** Returns the current focus distance in diopters. */
        float   getFocus() const;

        /** Returns the farthest focus distance in diopters. */
        float   farFocus() const;

        /** Returns the nearest focus distance in diopters. */
        float   nearFocus() const;

        /** Returns true if the lens is currently moving. */
        bool    focusChanging() const; 

        /** Returns the maximum focus speed in diopters/sec. */
        float   minFocusSpeed() const;

        /** Returns the mininum focus speed in diopters/sec. */
        float   maxFocusSpeed() const;

        /** Returns how long it takes for the lens to start moving (in us) after
         *  the setFocus function is called. */
        int     focusLatency() const;

        /** Returns how long it takes for the lens to settle steadly after it has
         *  reached its position. */
        int     focusSettleTime() const;
        
        /** Initiate a move to the desired focal length. 
          * Speed is in mm/sec. 
          * Note: The focal length is currently fixed. */
        void    setZoom(float focallength, float speed);

        /** Get the current focal length. */
        float   getZoom() const;

        /** The minimum focal length for this lens. 
          * Note: The focal length is currently fixed. */
        float   minZoom() const;

        /** The maximum focal length.
          * Note: The focal length is currently fixed. */
        float   maxZoom() const;

        /** The focal length is currently fixed. */
        bool    zoomChanging() const;

        /** The focal length is currently fixed. */
        float   minZoomSpeed() const;

        /** The focal length is currently fixed. */
        float   maxZoomSpeed() const;

        /** The focal length is currently fixed. */
        int     zoomLatency() const;
    
        /** Initiates an aperture change to the given
          * fnumber at the requested speed. 
          * Aperture is currently fixed and can't be changed. */
        void    setAperture(float fnumber, float speed);

        /** Returns the value of the fixed aperture. */
        float   getAperture() const;

        /** Returns the value of the fixed aperture. */
        float   wideAperture(float) const;

        /** Returns the value of the fixed aperture. */
        float   narrowAperture(float) const;

        /** Aperture is fixed, returns false. */
        bool    apertureChanging() const;

        /** Aperture if fixed. */
        int     apertureLatency()  const;

        /** Aperture is fixed. */
        float   minApertureSpeed() const;

        /** Aperture is fixed. */
        float   maxApertureSpeed() const;

        /** Tag a frame with the state of the Lens during that
         * frame. See \ref Lens::Tags */
        void tagFrame(FCam::Frame);

        /** What was the focus at some time in the past? Uses linear
         * interpolation from known lens positions. */
        float getFocus(Time t) const;

        /** The Tegra lens needs to handle mode change */
        void handleEvent(const FCam::Event &e);

        virtual const Platform &platform() const {return Tegra::Platform::instance();}
        virtual Platform &platform() {return Tegra::Platform::instance();}

    private:

        float ticksToDiopters(int) const;
        int dioptersToTicks(float) const;
        float tickRateToDiopterRate(int) const;
        int diopterRateToTickRate(float) const;
        
        struct LensState {
            Time time;
            float position;
        };
        CircularBuffer<LensState> lensHistory;

        friend class Sensor;
        void    attach(Hal::ICamera *pHardwareInterface);

        Hal::LensConfig    lensConfig;
        Hal::ICamera      *pHardwareInterface;

    };

}}

#endif
