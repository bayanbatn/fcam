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
#ifndef FCAM_TEGRA_FLASH_H
#define FCAM_TEGRA_FLASH_H

/** \file
 * The LED flash on the Tegra */

#include "../Flash.h"
#include "../CircularBuffer.h"
#include "../Time.h"
#include "hal/CameraHal.h"
#include <vector>

namespace FCam { 
    namespace Tegra {

        /** The LED flash on the Tegra camera driver
         * 
         * The flash is synchronized with the frame exposure and there
         * is no control over its duration. However, torch mode (see below)
         * can be used to provide cool effects. Attach a FlashFire action
         * to a shot to add flash. The duration and time of the action
         * are ignored.
         *
         * Torch: The flash + driver support torch mode (see Flash::torch).
         * Torch settings are asynchronous with respect to a capture. 
         * Setting torch on/off through actions can enable interesting effects.
         * A new TorchAction is provided for this purpose.
         */
  
         class Flash : public FCam::Flash {
          public:
            Flash();
            ~Flash();

            /** The minimum duration. No control over the duration in the current release. */
            int minDuration() {return 0;}

            /** The maximum duration. No control over the duration in the current release. */
            int maxDuration() {return 0;}
        
            /** The flash brightness is a fractional value 
             * in the range 0 to 1.0.
             * 
             * The camera driver rounds the flash intensity upwards
             * depending on the actual flash unit in use. If the unit
             * doesn't support variable intensite, setting the flash
             * brightness to non-zero values will rount it to 1.
             * */
            float minBrightness() {return 0.0f;}

            /** The flash on the Tegra has a maximum brightness
             * settings of 1.0f */
            float maxBrightness() {return 1.0f;}

            /** fire the flash for a given brightness and duration,
             *  flash is synchronized with the capture, duration
             *  is ignored.  */
            void fire(float brightness, int duration);

            /** Turn on the torch at the given brightness. Use a brightness 
             * of 0.0f to turn the torch off. */
            void torch(float brightness);

            /** How long after I call \ref FCam::Flash::fire "fire" does
              * the flash actually fire.
              *
              * \todo Calibrate the fireLatency.
              *  Set to 1ms for now. */
            int fireLatency() {return 1000;}

            /** Return the brightness of the flash at a specific
             * time. Tegra's flash has very little ramp-up and
             * ramp-down time, so this will either return the
             * requested brightness or zero. */
            float getBrightness(Time);

            /** Tag a frame with the state of the flash during that
             * frame. */
            void tagFrame(FCam::Frame);

            void handleEvent(const FCam::Event &e);

            void attach(Hal::ICamera *pHardwareInterface);

            /** An action to set the flash in torch mode with the given
             *  brightness. A brightness greater than 0 puts the flash
             *  in torch mode. A brightness of 0 turns to the torch
             *  off.
             **/
            class TorchAction : public CopyableAction<TorchAction> {
              public:

            	virtual int type() const { return Action::FlashTorch; }

                /** Construct a TorchAction to fire the given flash. */
                TorchAction(Flash *f);

                /** Construct a TorchAction to set the torch,
                 * starting at the given time into the exposure (in
                 * microseconds). */
                TorchAction(Flash *f, int time);

                /** Construct a TorchAction to set the torch,
                 *  starting at the given time into the exposure (in
                 *  microseconds), at the given brightness, for the given
                 *  duration (in microseconds). */
                TorchAction(Flash *f, int time, float brightness);

                /** The brightness setting for this torch event. Units are
                 * platform specific. Peak lumens are suggested. */
                float brightness;

                void doAction();

                /** Get the flash associated with this fire event. */
                Flash *getFlash() {return flash;}
              private:

                Flash *flash;
            };


          private:

            void setDuration(int);
            void setBrightness(float);

            struct FlashState {
                // Assume the flash state is piecewise constant. This
                // struct indicates a period of a constant brightness
                // starting at the given time.
                Time time;
                float brightness;
            };
            
            CircularBuffer<FlashState> flashHistory; 
            Hal::ICamera *pHardwareInterface;
        };

    }
}


#endif
